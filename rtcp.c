/* rtcp.c

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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <poll.h>
#include <pthread.h>


#include "rtsp.h"
#include "rtcp.h"
#include "tune.h"
#include "utils.h"
#include "applog.h"

/*
 *
 */
static uint8_t *get_app_packet(uint32_t ssrc, const Client_t *client, size_t *len) {
	uint8_t app[500];

	// Application Defined packet  (APP Packet)
	app[0]  = 0x80;                // version: 2, padding: 0, subtype: 0
	app[1]  = 204;                 // payload type: 204 (0xcc) (APP)
	app[2]  = 0;                   // length (total in 32-bit words minus one)
	app[3]  = 0;                   // length (total in 32-bit words minus one)
	app[4]  = (ssrc >> 24) & 0xff; // synchronization source
	app[5]  = (ssrc >> 16) & 0xff; // synchronization source
	app[6]  = (ssrc >>  8) & 0xff; // synchronization source
	app[7]  = (ssrc >>  0) & 0xff; // synchronization source
	app[8]  = 'S';                 // name
	app[9]  = 'E';                 // name
	app[10] = 'S';                 // name
	app[11] = '1';                 // name
	app[12] = 0;                   // identifier (0000)
	app[13] = 0;                   // identifier
	app[14] = 0;                   // string length
	app[15] = 0;                   // string length
	                               // Now the App defined data is added

	// lock - frontend pointer - frontend data - monitor data
	pthread_mutex_lock(&((Client_t *)client)->fe_ptr_mutex);
	if (client->fe) {
		pthread_mutex_lock(&((Client_t *)client)->fe->mutex);
		pthread_mutex_lock(&((Client_t *)client)->fe->monitor.mutex);

		// make PID csv
		char *pidPtr = NULL;
		addString(&pidPtr, "");
		size_t i = 0;
		if (client->fe->pid.data[i].used) {
			addString(&pidPtr, "%d", i);
		}
		for (i = 1; i < MAX_PIDS; ++i) {
			if (client->fe->pid.data[i].used) {
				addString(&pidPtr, ",%d", i);
			}
		}

		// add APP payload
		//"ver=1.0;src=<srcID>;tuner=<feID>,<level>,<lock>,<quality>,<frequency>,<polarisation>,"
		//"<system>,<type>,<pilots>,<roll_off>,<symbol_rate>,<fec_inner>;pids=<pid0>,…,<pidn>",
		*len = snprintf((char*)&app[16], sizeof(app) - 16,
				"ver=1.0;src=%d;tuner=%d,%d,%d,%d,%d,%d,%s,%s,%d,%s,%d,%s;pids=%s",
				client->fe->diseqc.src,
				client->fe->index+1,
				client->fe->monitor.strength,
				(client->fe->monitor.status & FE_HAS_LOCK) ? 1 : 0,
				client->fe->monitor.snr,
				client->fe->channel.freq,
				client->fe->diseqc.pol,
				delsys_to_string(client->fe->channel.delsys),
				modtype_to_sting(client->fe->channel.modtype),
				client->fe->channel.pilot,
				rolloff_to_sting(client->fe->channel.rolloff),
				client->fe->channel.srate,
				fec_to_string(client->fe->channel.fec),
				pidPtr);

		// free
		FREE_PTR(pidPtr);

		// unlock - frontend pointer - frontend data - monitor data
		pthread_mutex_unlock(&((Client_t *)client)->fe->mutex);
		pthread_mutex_unlock(&((Client_t *)client)->fe->monitor.mutex);
	}
	pthread_mutex_unlock(&((Client_t *)client)->fe_ptr_mutex);

	// total length and align on 32 bits
	*len = 16 + strlen((char*)&app[16]);
	if ((*len % 4) != 0) {
		*len += 4 - (*len % 4);
	}
	// Alloc and copy data and adjust length
	uint8_t *appPtr = malloc(*len);
	if (appPtr != NULL) {
		memcpy(appPtr, app, *len);
		int ws = (*len / 4) - 1;
		appPtr[2] = (ws >> 8) & 0xff;
		appPtr[3] = (ws >> 0) & 0xff;
		int ss = *len - 16;
		appPtr[14] = (ss >> 8) & 0xff;
		appPtr[15] = (ss >> 0) & 0xff;
	}
	return appPtr;
}

/*
 *
 */
static uint8_t *get_sr_packet(uint32_t ssrc, const Client_t *client, size_t *len) {
	uint8_t sr[28];
	uint8_t srb[24];

	// Sender Report (SR Packet)
	sr[0]  = 0x81;                         // version: 2, padding: 0, sr blocks: 1
	sr[1]  = 200;                          // payload type: 200 (0xc8) (SR)
	sr[2]  = 0;                            // length (total in 32-bit words minus one)
	sr[3]  = 0;                            // length (total in 32-bit words minus one)
	sr[4]  = (ssrc >> 24) & 0xff;          // synchronization source
	sr[5]  = (ssrc >> 16) & 0xff;          // synchronization source
	sr[6]  = (ssrc >>  8) & 0xff;          // synchronization source
	sr[7]  = (ssrc >>  0) & 0xff;          // synchronization source
	sr[8]  = 0;                            // NTP most sign word
	sr[9]  = 0;                            // NTP most sign word
	sr[10] = 0;                            // NTP most sign word
	sr[11] = 0;                            // NTP most sign word
	sr[12] = 0;                            // NTP least sign word
	sr[13] = 0;                            // NTP least sign word
	sr[14] = 0;                            // NTP least sign word
	sr[15] = 0;                            // NTP least sign word
	sr[16] = 0;                            // RTP timestamp RTS
	sr[17] = 0;                            // RTP timestamp RTS
	sr[18] = 0;                            // RTP timestamp RTS
	sr[19] = 0;                            // RTP timestamp RTS

	pthread_mutex_lock(&((Client_t *)client)->mutex);
	sr[20] = (client->spc >> 24) & 0xff;   // sender's packet count SPC
	sr[21] = (client->spc >> 16) & 0xff;   // sender's packet count SPC
	sr[22] = (client->spc >>  8) & 0xff;   // sender's packet count SPC
	sr[23] = (client->spc >>  0) & 0xff;   // sender's packet count SPC
	sr[24] = (client->soc >> 24) & 0xff;   // sender's octet count SOC
	sr[25] = (client->soc >> 16) & 0xff;   // sender's octet count SOC
	sr[26] = (client->soc >>  8) & 0xff;   // sender's octet count SOC
	sr[27] = (client->soc >>  0) & 0xff;   // sender's octet count SOC
	pthread_mutex_unlock(&((Client_t *)client)->mutex);

	// Sender Report Block n (SR Packet)
	srb[0]  = (ssrc >> 24) & 0xff;         // synchronization source
	srb[2]  = (ssrc >> 16) & 0xff;         // synchronization source
	srb[2]  = (ssrc >>  8) & 0xff;         // synchronization source
	srb[3]  = (ssrc >>  0) & 0xff;         // synchronization source
	srb[4]  = 0;                           // fraction lost
	srb[5]  = 0;                           // cumulative number of packets lost
	srb[6]  = 0;                           // cumulative number of packets lost
	srb[7]  = 0;                           // cumulative number of packets lost
	srb[8]  = 0;                           // extended highest sequence number received
	srb[9]  = 0;                           // extended highest sequence number received
	srb[10] = 0;                           // extended highest sequence number received
	srb[11] = 0;                           // extended highest sequence number received
	srb[12] = 0;                           // inter arrival jitter
	srb[13] = 0;                           // inter arrival jitter
	srb[14] = 0;                           // inter arrival jitter
	srb[15] = 0;                           // inter arrival jitter
	srb[16] = 0;                           // last SR (LSR)
	srb[17] = 0;                           // last SR (LSR)
	srb[18] = 0;                           // last SR (LSR)
	srb[19] = 0;                           // last SR (LSR) delay since last SR (DLSR)
	srb[20] = 0;                           // delay since last SR (DLSR)
	srb[21] = 0;                           // delay since last SR (DLSR)
	srb[22] = 0;                           // delay since last SR (DLSR)
	srb[23] = 0;                           // delay since last SR (DLSR)

	*len = sizeof(sr) + sizeof(srb);

	// Alloc and copy data and adjust length
	uint8_t *srPtr = malloc(*len);
	if (srPtr != NULL) {
		memcpy(srPtr, sr, sizeof(sr));
		memcpy(srPtr + sizeof(sr), srb, sizeof(srb));
		int ws = (*len / 4) - 1;
		srPtr[2] = (ws >> 8) & 0xff;
		srPtr[3] = (ws >> 0) & 0xff;
	}
	return srPtr;
}

/*
 *
 */
static uint8_t *get_sdes_packet(uint32_t ssrc, size_t *len) {
	int8_t sdes[24];

	// Source Description (SDES Packet)
	sdes[0]  = 0x82;                           // version: 2, padding: 0, sc blocks: 2
	sdes[1]  = 202;                            // payload type: 202 (0xca) (SDES)
	sdes[2]  = 0;                              // length (total in 32-bit words minus one)
	sdes[3]  = 3;                              // length (total in 32-bit words minus one)
	sdes[4]  = (ssrc >> 24) & 0xff;            // synchronization source
	sdes[5]  = (ssrc >> 16) & 0xff;            // synchronization source
	sdes[6]  = (ssrc >>  8) & 0xff;            // synchronization source
	sdes[7]  = (ssrc >>  0) & 0xff;            // synchronization source
	sdes[8]  = 1;                              // CNAME: 1
	sdes[9]  = 6;                              // length: 6
	sdes[10] = 'S';                            // data
	sdes[11] = 'A';                            // data
	sdes[12] = 'T';                            // data
	sdes[13] = '>';                            // data
	sdes[14] = 'I';                            // data
	sdes[15] = 'P';                            // data
	sdes[16] = 2;                              // NAME: 2
	sdes[17] = 6;                              // length: 6
	sdes[18] = 'S';                            // data
	sdes[19] = 'A';                            // data
	sdes[20] = 'T';                            // data
	sdes[21] = '>';                            // data
	sdes[22] = 'I';                            // data
	sdes[23] = 'P';                            // data

	*len = sizeof(sdes);

	// Alloc and copy data and adjust length
	uint8_t *sdesPtr = malloc(*len);
	if (sdesPtr != NULL) {
		memcpy(sdesPtr, sdes, sizeof(sdes));
		int ws = (*len / 4) - 1;
		sdesPtr[2] = (ws >> 8) & 0xff;
		sdesPtr[3] = (ws >> 0) & 0xff;
	}
	return sdesPtr;
}

/*
 *
 */
static void *thread_work_rtcp(void *arg) {
	size_t mon_update = 0;

	SI_LOG_INFO("Setting up RTCP server");

	Client_t *client = (Client_t *)arg;

	// see: http://www4.cs.fau.de/Projects/JRTP/pmt/node82.html
	// https://www.ietf.org/rfc/rfc3550.txt

	if ((client->rtcp.client.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		PERROR("socket RTCP");
		return NULL;
	}
	int val = 1;
	if (setsockopt(client->rtcp.client.fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		PERROR("RTCP setsockopt SO_REUSEADDR");
		return NULL;
	}
	for (;;) {
		pthread_mutex_lock(&client->mutex);
		Session_t state = client->rtcp.state;
		pthread_mutex_unlock(&client->mutex);

		switch (state) {
			case Starting:
				pthread_mutex_lock(&client->fe_ptr_mutex);
				if (client->fe) {
					SI_LOG_INFO("Frontend: %d, Start RTCP stream to %s (%d - %d)", client->fe->index, client->ip_addr,
									ntohs(client->rtp.client.addr.sin_port), ntohs(client->rtcp.client.addr.sin_port));
				}
				pthread_mutex_unlock(&client->fe_ptr_mutex);

				pthread_mutex_lock(&client->mutex);
				client->rtcp.state = Started;
				pthread_mutex_unlock(&client->mutex);
//				break;
			case Started:
				pthread_mutex_lock(&client->fe_ptr_mutex);
				if (client->fe) {
					// Check if have already opened the monitor FE (in read only mode)
					if (client->fe->monitor.fd_fe == -1) {
						SI_LOG_INFO("Frontend: %d, Open Frontend Monitor for %s (%d - %d)", client->fe->index, client->ip_addr,
										ntohs(client->rtp.client.addr.sin_port), ntohs(client->rtcp.client.addr.sin_port));
						client->fe->monitor.fd_fe = open_fe(client->fe->path_to_fe, 1);
					}

					if (mon_update == 0) {
						monitor_frontend(&client->fe->monitor, 0);
						mon_update = 1;
					} else {
						--mon_update;
					}
				}
				pthread_mutex_unlock(&client->fe_ptr_mutex);

				// RTCP compound packets must start with a SR, SDES then APP
				size_t srlen   = 0;
				size_t sdeslen = 0;
				size_t applen  = 0;
				uint8_t *sdes  = get_sdes_packet(client->ssrc, &sdeslen);
				uint8_t *sr    = get_sr_packet(client->ssrc, client, &srlen);
				uint8_t *app   = get_app_packet(client->ssrc, client, &applen);

				if (sr && sdes && app) {
					const size_t rtcplen = srlen + sdeslen + applen;

					uint8_t *rtcp = malloc(rtcplen);
					if (rtcp) {
						memcpy(rtcp, sr, srlen);
						memcpy(rtcp + srlen, sdes, sdeslen);
						memcpy(rtcp + srlen + sdeslen, app, applen);

						if (sendto(client->rtcp.client.fd, rtcp, rtcplen, 0,
									(struct sockaddr *)&client->rtcp.client,
									sizeof(client->rtcp.client)) == -1) {
							PERROR("send RTCP");
						}
					}
					FREE_PTR(rtcp);
				}
				FREE_PTR(sr);
				FREE_PTR(sdes);
				FREE_PTR(app);

				// update 5 Hz
				usleep(200000);
				break;
			case Stopping:
				pthread_mutex_lock(&client->mutex);
				client->rtcp.state = Stopped; // first stop, teardown checks this
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
				SI_LOG_ERROR("Wrong rtcp state %d", state);
				break;
		}
	}

	return NULL;
}

/*
 *
 */
void start_rtcp(RtpSession_t *rtpsession) {
	size_t i = 0;
	for (i = 0; i < MAX_CLIENTS; ++i) {
		Client_t *client = &rtpsession->client[i];
		client->rtcp.state = Stopped;
		if (pthread_create(&client->rtcp.threadID, NULL, &thread_work_rtcp, client) != 0) {
			SI_LOG_ERROR("thread_work_rtcp");
		}
		pthread_setname_np(client->rtcp.threadID, "thread_rtcp");
	}
}
