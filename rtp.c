/* rtp.c

   Copyright (C) 2014 Marc Postema

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   Or, point your browser to http://www.gnu.org/copyleft/gpl.html

*/
#define _GNU_SOURCE
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <poll.h>
#include <time.h>

#include "rtsp.h"
#include "rtp.h"
#include "tune.h"
#include "applog.h"
#include "utils.h"

/*
 *
 */
static void *thread_work_rtp(void *arg) {
	struct pollfd pfd[1];

	Client_t *client = (Client_t *)arg;

	client->rtp.timestamp = getmsec() * 90;
	client->rtp.cseq = 0x0000;

	client->rtp.buffer[0]  = 0x80;                                 // version: 2, padding: 0, extension: 0, CSRC: 0
	client->rtp.buffer[1]  = 33;                                   // marker: 0, payload type: 33 (MP2T)
	client->rtp.buffer[2]  = (client->rtp.cseq >> 8) & 0xff;       // sequence number
	client->rtp.buffer[3]  = (client->rtp.cseq >> 0) & 0xff;       // sequence number
	client->rtp.buffer[4]  = (client->rtp.timestamp >> 24) & 0xff; // timestamp
	client->rtp.buffer[5]  = (client->rtp.timestamp >> 16) & 0xff; // timestamp
	client->rtp.buffer[6]  = (client->rtp.timestamp >>  8) & 0xff; // timestamp
	client->rtp.buffer[7]  = (client->rtp.timestamp >>  0) & 0xff; // timestamp
	client->rtp.buffer[8]  = (client->ssrc >> 24) & 0xff;          // synchronization source
	client->rtp.buffer[9]  = (client->ssrc >> 16) & 0xff;          // synchronization source
	client->rtp.buffer[10] = (client->ssrc >>  8) & 0xff;          // synchronization source
	client->rtp.buffer[11] = (client->ssrc >>  0) & 0xff;          // synchronization source

	if ((client->rtp.client.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		PERROR("socket RTP ");
		return NULL;
	}

	const int val = 1;
	if (setsockopt(client->rtp.client.fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		PERROR("RTP setsockopt SO_REUSEADDR");
		return NULL;
	}

	// init
	client->rtp.bufPtr = client->rtp.buffer + RTP_HEADER_LEN;
	pfd[0].fd = -1;
	pfd[0].events = POLLIN | POLLPRI;
	pfd[0].revents = 0;

	for (;;) {
		pthread_mutex_lock(&client->mutex);
		pthread_mutex_lock(&client->fe_ptr_mutex);
		const Session_t state = client->rtp.state;
		pfd[0].fd = client->fe ? client->fe->fd_dvr : -1;
		pthread_mutex_unlock(&client->fe_ptr_mutex);
		pthread_mutex_unlock(&client->mutex);

		switch (state) {
			case Starting:
				pthread_mutex_lock(&client->fe_ptr_mutex);
				if (client->fe) {
					SI_LOG_INFO("Frontend: %d, Start streaming to %s (%d - %d)", client->fe->index, client->ip_addr,
									ntohs(client->rtp.client.addr.sin_port), ntohs(client->rtcp.client.addr.sin_port));
				} else {
					SI_LOG_ERROR("RTP - No Frontend for client %s (%d - %d) ??", client->ip_addr,
									ntohs(client->rtp.client.addr.sin_port), ntohs(client->rtcp.client.addr.sin_port));
				}
				pthread_mutex_unlock(&client->fe_ptr_mutex);

				pthread_mutex_lock(&client->mutex);
				client->rtp.state = Started;
				pthread_mutex_unlock(&client->mutex);
				break;
			case Started:
				{
					// call poll with a timeout of 100 ms
					const int pollRet = poll(pfd, 1, 100);
					if (pollRet > 0) {
						pthread_mutex_lock(&client->mutex);
						pthread_mutex_lock(&client->fe_ptr_mutex);
						if (client->fe) {
							// try read TS_PACKET_SIZE from DVR
							const int bytes_read = read(client->fe->fd_dvr, client->rtp.bufPtr, TS_PACKET_SIZE);
							pthread_mutex_unlock(&client->fe_ptr_mutex);
							pthread_mutex_unlock(&client->mutex);
							if (bytes_read > 0) {
								// sync byte then check cc
								if (client->rtp.bufPtr[0] == 0x47 && bytes_read > 3) {
									// get PID from TS
									const uint16_t pid = ((client->rtp.bufPtr[1] & 0x1f) << 8) | client->rtp.bufPtr[2];
									// get CC from TS
									const uint8_t cc  = client->rtp.bufPtr[3] & 0x0f;

									pthread_mutex_lock(&client->fe_ptr_mutex);
									if (client->fe) {
										pthread_mutex_lock(&client->fe->mutex);
										++client->fe->pid.data[pid].count;
										if (client->fe->pid.data[pid].cc == 0x80) {
											client->fe->pid.data[pid].cc = cc;
										} else {
											++client->fe->pid.data[pid].cc;
											client->fe->pid.data[pid].cc %= 0x10;
											if (client->fe->pid.data[pid].cc != cc) {
												int diff = cc - client->fe->pid.data[pid].cc;
												if (diff < 0) {
													diff += 0x10;
												}
												client->fe->pid.data[pid].cc = cc;
												client->fe->pid.data[pid].cc_error += diff;
											}
										}
										pthread_mutex_unlock(&client->fe->mutex);
									}
									pthread_mutex_unlock(&client->fe_ptr_mutex);
								}
								// inc buffer pointer
								client->rtp.bufPtr += bytes_read;
								const int len = client->rtp.bufPtr - client->rtp.buffer;

								// rtp buffer full
								const long time_ms = getmsec();
								if ((len + TS_PACKET_SIZE) > MTU || client->rtp.send_interval < time_ms) {
									// reset the time interval
									client->rtp.send_interval = time_ms + 100;
									
									// update sequence number
									++client->rtp.cseq;
									client->rtp.buffer[2] = ((client->rtp.cseq >> 8) & 0xFF); // sequence number
									client->rtp.buffer[3] =  (client->rtp.cseq & 0xFF);       // sequence number

									// update timestamp
									client->rtp.timestamp = time_ms * 90;
									client->rtp.buffer[4] = (client->rtp.timestamp >> 24) & 0xFF; // timestamp
									client->rtp.buffer[5] = (client->rtp.timestamp >> 16) & 0xFF; // timestamp
									client->rtp.buffer[6] = (client->rtp.timestamp >>  8) & 0xFF; // timestamp
									client->rtp.buffer[7] = (client->rtp.timestamp >>  0) & 0xFF; // timestamp

									// inc RTP packet counter
									++client->spc;

									// RTP packet octet count (Bytes)
									const uint32_t byte = (len - RTP_HEADER_LEN);
									client->soc += byte;
									// RTP payload in Bytes
									client->rtp_payload += byte;

									// send the RTP packet
									if (sendto(client->rtp.client.fd, client->rtp.buffer, len, MSG_DONTWAIT,
												(struct sockaddr *)&client->rtp.client.addr,
												sizeof(client->rtp.client.addr)) == -1) {
										PERROR("send RTP");
									}
									// set bufPtr to begin of RTP data (after Header)
									client->rtp.bufPtr = client->rtp.buffer + RTP_HEADER_LEN;
								}
							}
						} else {
							pthread_mutex_unlock(&client->fe_ptr_mutex);
							pthread_mutex_unlock(&client->mutex);
							SI_LOG_ERROR("RTP - No Frontend for client %s (%d - %d) ???", client->ip_addr,
											ntohs(client->rtp.client.addr.sin_port), ntohs(client->rtcp.client.addr.sin_port));
						}
					}
				}
				break;
			case Stopping:
				pthread_mutex_lock(&client->mutex);
				pthread_mutex_lock(&client->fe_ptr_mutex);
				if (client->fe) {
					client->fe->pid.all = 0;
					SI_LOG_INFO("Frontend: %d, Stop streaming to %s (%d - %d) (Streamed %.3f MBytes)", client->fe->index, client->ip_addr,
									ntohs(client->rtp.client.addr.sin_port), ntohs(client->rtcp.client.addr.sin_port),
									(client->rtp_payload / (1024.0 * 1024.0)));
				}
				pthread_mutex_unlock(&client->fe_ptr_mutex);
				client->rtp_payload = 0.0;
				client->rtp.state = Stopped; // first stop, teardown checks this
				if (client->teardown_session) {
					client->teardown_session(client);
				}
				pthread_mutex_unlock(&client->mutex);
				break;
			case Stopped:
				usleep(500000);
				break;
			case Not_Initialized:
				// fall-through
			default:
				SI_LOG_ERROR("Wrong rtp state %d", state);
				break;
		}
	}
	return NULL;
}

/*
 *
 */
void init_rtp(RtpSession_t *rtpsession) {
	pthread_mutexattr_t attr;
	size_t  i, j;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	// start time of programm
	pthread_mutex_init(&rtpsession->mutex, &attr);
	rtpsession->appStartTime           = time(NULL);
	rtpsession->rtp_payload            = 0;
	rtpsession->ssdp_announce_time_sec = 60; // 60sec default interval
	rtpsession->bootId                 = 0;

	// frontend properties
	for (j = 0; j < rtpsession->fe.max_fe; ++j) {
		// frontend properties
		pthread_mutex_init(&rtpsession->fe.array[j]->mutex, &attr);
		rtpsession->fe.array[j]->attached        = 0;
		rtpsession->fe.array[j]->tuned           = 0;
		rtpsession->fe.array[j]->fd_dvr          = -1;
		rtpsession->fe.array[j]->fd_fe           = -1;

		// monitor properties
		rtpsession->fe.array[j]->monitor.fd_fe    = -1;
		rtpsession->fe.array[j]->monitor.status   = 0;
		rtpsession->fe.array[j]->monitor.strength = 0;
		rtpsession->fe.array[j]->monitor.snr      = 0;
		rtpsession->fe.array[j]->monitor.ber      = 0;
		rtpsession->fe.array[j]->monitor.ublocks  = 0;
		pthread_mutex_init(&rtpsession->fe.array[j]->monitor.mutex, &attr);

		// Channel properties
		rtpsession->fe.array[j]->channel.freq    = 0;
		rtpsession->fe.array[j]->channel.srate   = 0;
		rtpsession->fe.array[j]->channel.delsys  = SYS_UNDEFINED;
		rtpsession->fe.array[j]->channel.modtype = QAM_64;
		rtpsession->fe.array[j]->channel.fec     = FEC_1_2;
		rtpsession->fe.array[j]->channel.pilot   = PILOT_AUTO;
		rtpsession->fe.array[j]->channel.rolloff = ROLLOFF_35;
		
		rtpsession->fe.array[j]->channel.transmission = TRANSMISSION_MODE_AUTO;
		rtpsession->fe.array[j]->channel.guard        = GUARD_INTERVAL_1_4;
		rtpsession->fe.array[j]->channel.hierarchy    = HIERARCHY_AUTO;
		rtpsession->fe.array[j]->channel.bandwidth    = BANDWIDTH_AUTO;
		rtpsession->fe.array[j]->channel.plp_id       = 0;


		// DiSEqc properties
		rtpsession->fe.array[j]->diseqc.src      = 0;
		rtpsession->fe.array[j]->diseqc.pol_v    = POL_V;
		for (i = 0; i < MAX_LNB; ++i) {
			rtpsession->fe.array[j]->lnb_array[i].type        = LNB_UNIVERSAL;
			rtpsession->fe.array[j]->lnb_array[i].lofStandard = DEFAULT_LOF_STANDARD;
			rtpsession->fe.array[j]->lnb_array[i].switchlof   = DEFAULT_SLOF;
			rtpsession->fe.array[j]->lnb_array[i].lofLow      = DEFAULT_LOF_LOW_UNIVERSAL;
			rtpsession->fe.array[j]->lnb_array[i].lofHigh     = DEFAULT_LOF_HIGH_UNIVERSAL;
		}

		// PID properties
		rtpsession->fe.array[j]->pid.changed              = 0;
		rtpsession->fe.array[j]->pid.all                  = 0;
		for (i = 0; i < MAX_PIDS; ++i) {
			rtpsession->fe.array[j]->pid.data[i].used     = 0;
			rtpsession->fe.array[j]->pid.data[i].cc       = 0x80;
			rtpsession->fe.array[j]->pid.data[i].cc_error = 0;
			rtpsession->fe.array[j]->pid.data[i].count    = 0;
			rtpsession->fe.array[j]->pid.data[i].fd_dmx   = -1;
		}
	}
	////////////////////

	// client properties
	for (i = 0; i < MAX_CLIENTS; ++i) {
		// client mutex
		pthread_mutex_init(&rtpsession->client[i].mutex, &attr);
		pthread_mutex_init(&rtpsession->client[i].fe_ptr_mutex, &attr);
		
		// RTSP properties
		rtpsession->client[i].rtsp.socket.fd       = -1;
		rtpsession->client[i].rtsp.watchdog        = 0;
		rtpsession->client[i].rtsp.streamID        = -1;
		rtpsession->client[i].rtsp.cseq            = -1;
		rtpsession->client[i].rtsp.shall_close     = 0;
		rtpsession->client[i].rtsp.session_timeout = 60;
		sprintf(rtpsession->client[i].rtsp.sessionID, "-1");
		
		// RTP properties
		rtpsession->client[i].rtp.state     = Not_Initialized;
		rtpsession->client[i].rtp.client.fd = -1;
		rtpsession->client[i].rtp.server.fd = -1;
		
		// RTCP properties
		rtpsession->client[i].rtcp.state     = Not_Initialized;
		rtpsession->client[i].rtcp.client.fd = -1;
		rtpsession->client[i].rtcp.server.fd = -1;

		// client properties
		static unsigned int seedp = 0xFEED;
		rtpsession->client[i].ssrc = (uint32_t)(rand_r(&seedp) % 0xffff);
		rtpsession->client[i].rtp_payload = 0.0;
		rtpsession->client[i].spc = 0;
		rtpsession->client[i].soc = 0;
		rtpsession->client[i].fe = NULL;
		rtpsession->client[i].teardown_session = NULL;
		rtpsession->client[i].teardown_graceful = 1;
		sprintf(rtpsession->client[i].ip_addr, "0.0.0.0");
	}
}

/*
 *
 */
void start_rtp(RtpSession_t *rtpsession) {
	size_t i = 0;
	SI_LOG_INFO("Setting up %d RTP servers", MAX_CLIENTS);
	for (i = 0; i < MAX_CLIENTS; ++i) {
		Client_t *client = &rtpsession->client[i];
		client->rtp.state = Stopped;
		if (pthread_create(&client->rtp.threadID, NULL, &thread_work_rtp, client) != 0) {
			SI_LOG_ERROR("thread_work_rtp");
		}
		char np[16];
		snprintf(np, sizeof(np), "thread_rtp%zu", i);
		pthread_setname_np(client->rtp.threadID, np);
	}
}

/*
 *
 */
int stop_rtp(RtpSession_t *rtpsession) {
	size_t i, j;
	for (j = 0; j < rtpsession->fe.max_fe; ++j) {
		Frontend_t *fe = rtpsession->fe.array[j];
		
		CLOSE_FD(fe->fd_dvr);
		CLOSE_FD(fe->fd_fe);
		CLOSE_FD(fe->monitor.fd_fe);
		for (i = 0; i < MAX_PIDS; ++i) {
			CLOSE_FD(fe->pid.data[i].fd_dmx);
		}
		FREE_PTR(fe);
	}
	FREE_PTR(rtpsession->fe.array);
	CLOSE_FD(rtpsession->rtsp_server.fd);

	// client properties
	for (i = 0; i < MAX_CLIENTS; ++i) {
		Client_t *client = &rtpsession->client[i];
		
		pthread_cancel(client->rtp.threadID);
		pthread_cancel(client->rtcp.threadID);
		pthread_join(client->rtp.threadID, NULL);
		pthread_join(client->rtcp.threadID, NULL);

		CLOSE_FD(client->rtp.client.fd);
		CLOSE_FD(client->rtp.server.fd);
		
		CLOSE_FD(client->rtcp.client.fd);
		CLOSE_FD(client->rtcp.server.fd);

		CLOSE_FD(client->rtsp.socket.fd);
	}
	return 1;
}
