/* ssdp.c

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
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "ssdp.h"
#include "connection.h"
#include "rtp.h"
#include "utils.h"
#include "applog.h"

#define UTIME_DEL    200000

static SocketAttr_t udp_multi_send;
static SocketAttr_t udp_multi_listen;
static pthread_t threadID;

/*
 *
 */
static unsigned int read_bootID_from_file(const char *file) {
#define BOOTID_STR "bootID=%d"

	// Get BOOTID from file, increment and save it again
	unsigned int bootId = 0;
	int fd_bootId = -1;
	char content[50];
	if ((fd_bootId = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH)) > 0) {
		if (read(fd_bootId, content, sizeof(content)) >= 0) {
			if (strlen(content) != 0) {
				sscanf(content, BOOTID_STR, &bootId);
				lseek(fd_bootId, 0, SEEK_SET);
			}
		} else {
			PERROR("Unable to read file: bootID");
		}
		++bootId;
		sprintf(content, BOOTID_STR, bootId);
		if (write(fd_bootId, content, strlen(content)) == -1) {
			PERROR("Unable to write file: bootID");
		}
		CLOSE_FD(fd_bootId);
	} else {
		PERROR("Unable to open file: bootID");
	}
	return bootId;
}

/*
 *
 */
static int send_byebye(unsigned int bootId, const char *uuid) {
#define UPNP_ROOTDEVICE_BB "NOTIFY * HTTP/1.1\r\n" \
                           "HOST: 239.255.255.250:1900\r\n" \
                           "NT: upnp:rootdevice\r\n" \
                           "NTS: ssdp:byebye\r\n" \
                           "USN: uuid:%s::upnp:rootdevice\r\n" \
                           "BOOTID.UPNP.ORG: %d\r\n" \
                           "CONFIGID.UPNP.ORG: 0\r\n" \
                           "\r\n"

#define UPNP_BYEBYE "NOTIFY * HTTP/1.1\r\n" \
                    "HOST: 239.255.255.250:1900\r\n" \
                    "NT: uuid:%s\r\n" \
                    "NTS: ssdp:byebye\r\n" \
                    "USN: uuid:%s\r\n" \
                    "BOOTID.UPNP.ORG: %d\r\n" \
                    "CONFIGID.UPNP.ORG: 0\r\n" \
                    "\r\n"

#define UPNP_DEVICE_BB "NOTIFY * HTTP/1.1\r\n" \
                       "HOST: 239.255.255.250:1900\r\n" \
                       "NT: urn:ses-com:device:SatIPServer:1\r\n" \
                       "NTS: ssdp:byebye\r\n" \
                       "USN: uuid:%s::urn:ses-com:device:SatIPServer:1\r\n" \
                       "BOOTID.UPNP.ORG: %d\r\n" \
                       "CONFIGID.UPNP.ORG: 0\r\n" \
                       "\r\n"
	char msg[500];

	// broadcast message
	snprintf(msg, sizeof(msg), UPNP_ROOTDEVICE_BB, uuid, bootId);
	if (sendto(udp_multi_send.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_multi_send.addr, sizeof(udp_multi_send.addr)) == -1) {
		PERROR("send");
		return -1;
	}
	usleep(UTIME_DEL);

	// broadcast message
	snprintf(msg, sizeof(msg), UPNP_BYEBYE, uuid, uuid, bootId);
	if (sendto(udp_multi_send.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_multi_send.addr, sizeof(udp_multi_send.addr)) == -1) {
		PERROR("send");
		return -1;
	}
	usleep(UTIME_DEL);

	// broadcast message
	snprintf(msg, sizeof(msg), UPNP_DEVICE_BB, uuid, bootId);
	if (sendto(udp_multi_send.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_multi_send.addr, sizeof(udp_multi_send.addr)) == -1) {
		PERROR("send");
		return -1;
	}

	return 1;
}

/*
 *
 */
static void * thread_work_ssdp(void *arg) {
#define UPNP_ROOTDEVICE	"NOTIFY * HTTP/1.1\r\n" \
						"HOST: 239.255.255.250:1900\r\n" \
						"CACHE-CONTROL: max-age=1800\r\n" \
						"LOCATION: http://%s:%d/desc.xml\r\n" \
						"NT: upnp:rootdevice\r\n" \
						"NTS: ssdp:alive\r\n" \
						"SERVER: Linux/1.0 UPnP/1.1 SatPI/%s\r\n" \
						"USN: uuid:%s::upnp:rootdevice\r\n" \
						"BOOTID.UPNP.ORG: %d\r\n" \
						"CONFIGID.UPNP.ORG: 0\r\n" \
						"DEVICEID.SES.COM: %d\r\n" \
						"\r\n"

#define UPNP_ALIVE	"NOTIFY * HTTP/1.1\r\n" \
					"HOST: 239.255.255.250:1900\r\n" \
					"CACHE-CONTROL: max-age=1800\r\n" \
					"LOCATION: http://%s:%d/desc.xml\r\n" \
					"NT: uuid:%s\r\n" \
					"NTS: ssdp:alive\r\n" \
					"SERVER: Linux/1.0 UPnP/1.1 SatPI/%s\r\n" \
					"USN: uuid:%s\r\n" \
					"BOOTID.UPNP.ORG: %d\r\n" \
					"CONFIGID.UPNP.ORG: 0\r\n" \
					"DEVICEID.SES.COM: %d\r\n" \
					"\r\n"

#define UPNP_DEVICE "NOTIFY * HTTP/1.1\r\n" \
					"HOST: 239.255.255.250:1900\r\n" \
					"CACHE-CONTROL: max-age=1800\r\n" \
					"LOCATION: http://%s:%d/desc.xml\r\n" \
					"NT: urn:ses-com:device:SatIPServer:1\r\n" \
					"NTS: ssdp:alive\r\n" \
					"SERVER: Linux/1.0 UPnP/1.1 SatPI/%s\r\n" \
					"USN: uuid:%s::urn:ses-com:device:SatIPServer:1\r\n" \
					"BOOTID.UPNP.ORG: %d\r\n" \
					"CONFIGID.UPNP.ORG: 0\r\n" \
					"DEVICEID.SES.COM: %d\r\n" \
					"\r\n"

#define UPNP_M_SEARCH "M-SEARCH * HTTP/1.1\r\n" \
                      "HOST: %s:%d\r\n" \
                      "MAN: \"ssdp:discover\"\r\n" \
                      "ST: urn:ses-com:device:SatIPServer:1\r\n" \
                      "USER-AGENT: Linux/1.0 UPnP/1.1 SatPI/%s\r\n" \
                      "DEVICEID.SES.COM: %d\r\n" \
                      "\r\n"

#define UPNP_M_SEARCH_OK "HTTP/1.1 200 OK\r\n" \
                         "CACHE-CONTROL: max-age=1800\r\n" \
                         "EXT:\r\n" \
                         "LOCATION: http://%s:%d/desc.xml\r\n" \
                         "SERVER: Linux/1.0 UPnP/1.1 SatPI/%s\r\n" \
                         "ST: urn:ses-com:device:SatIPServer:1\r\n" \
                         "USN: uuid:%s::urn:ses-com:device:SatIPServer:1\r\n" \
                         "BOOTID.UPNP.ORG: %d\r\n" \
                         "CONFIGID.UPNP.ORG: 0\r\n" \
                         "DEVICEID.SES.COM: %d\r\n" \
                         "\r\n"

	RtpSession_t *rtpsession = (RtpSession_t *)arg;
	unsigned int deviceId = 0x01; // start with 1
	time_t repeat_time = 0;

	rtpsession->bootId = read_bootID_from_file("bootID");

	SI_LOG_INFO("Setting up SSDP server with BOOTID: %d", rtpsession->bootId);

	init_udp_socket(&udp_multi_send, SSDP_PORT, inet_addr("239.255.255.250"));
	init_mutlicast_udp_socket(&udp_multi_listen, SSDP_PORT, rtpsession->interface.ip_addr);

	struct pollfd pfd[1];
	pfd[0].fd = udp_multi_listen.fd;
	pfd[0].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
	pfd[0].revents = 0;

	for (;;) {
		// call poll with a timeout of 500 ms
		const int pollRet = poll(pfd, 1, 500);
		if (pollRet > 0) {
			if (pfd[0].revents != 0) {
				char *recv_msg = NULL;
				struct sockaddr_in si_other;
				socklen_t addrlen = sizeof(si_other);
				const ssize_t size = recvfrom_httpc_message(pfd[0].fd, &recv_msg, MSG_DONTWAIT, &si_other, &addrlen);
				if (size > 0) {
					recv_msg[size] = 0;
					char ip_addr[25];
					// save client ip address
					strcpy(ip_addr, (char *)inet_ntoa(si_other.sin_addr));
					// @TODO we should probably listen to only one message
					// check do we hear our echo
					if (strcmp(rtpsession->interface.ip_addr, ip_addr) != 0 && strcmp("127.0.0.1", ip_addr) != 0) {
						// get method from message
						char method[15];
						get_method_from(recv_msg, method);
						if (strncmp(method, "NOTIFY", 6) == 0) {
							char *param = get_header_field_parameter_from(recv_msg, "DEVICEID.SES.COM");
							if (param) {
								// get device id of other
								unsigned int other_deviceId = atoi(param);
								FREE_PTR(param);

								// check server found with clashing DEVICEID
								if (deviceId == other_deviceId) {
									SI_LOG_INFO("Found SAT>IP Server %s: with clashing DEVICEID %d", ip_addr, other_deviceId);
									SocketAttr_t udp_send;
									init_udp_socket(&udp_send, SSDP_PORT, inet_addr(ip_addr));
									char msg[500];
									// send message back
									snprintf(msg, sizeof(msg), UPNP_M_SEARCH, rtpsession->interface.ip_addr, SSDP_PORT, rtpsession->satpi_version, deviceId);
									if (sendto(udp_send.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_send.addr, sizeof(udp_send.addr)) == -1) {
										PERROR("send");
									}
									CLOSE_FD(udp_send.fd);
								} else {
									SI_LOG_INFO("Found SAT>IP Server %s: with DEVICEID %d", ip_addr, other_deviceId);
								}
							}
						} else if (strncmp(method, "M-SEARCH", 8) == 0) {
							char *param = get_header_field_parameter_from(recv_msg, "DEVICEID.SES.COM");
							if (param) {
								FREE_PTR(param);

								// setup reply socket
								SocketAttr_t udp_send;
								init_udp_socket(&udp_send, SSDP_PORT, inet_addr(ip_addr));

								// send directly to us, so this should mean we have the same DEVICEID
								SI_LOG_INFO("SAT>IP Server %s: contacted us because of clashing DEVICEID %d", ip_addr, deviceId);

								// send message back
								char msg[500];
								snprintf(msg, sizeof(msg), UPNP_M_SEARCH_OK, rtpsession->interface.ip_addr, HTTP_PORT, rtpsession->satpi_version, rtpsession->uuid, rtpsession->bootId, deviceId);
								if (sendto(udp_send.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_send.addr, sizeof(udp_send.addr)) == -1) {
									PERROR("send");
								}
								// we should increment DEVICEID and send bye bye
								++deviceId;
								send_byebye(rtpsession->bootId, rtpsession->uuid);

								// now increment bootID
								rtpsession->bootId = read_bootID_from_file("bootID");
								SI_LOG_INFO("Changing BOOTID to: %d", rtpsession->bootId);

								// reset repeat time to annouce new DEVICEID
								repeat_time =  time(NULL) + 5;
								CLOSE_FD(udp_send.fd);
							}
							char *st_param = get_header_field_parameter_from(recv_msg, "ST");
							if (st_param) {
								if (strncmp("urn:ses-com:device:SatIPServer:1", st_param, 32) == 0) {
									// setup reply socket
									SocketAttr_t udp_send;
									init_udp_socket(&udp_send, SSDP_PORT, inet_addr(ip_addr));

									// client is sending a discover
									SI_LOG_INFO("SAT>IP Client %s : tries to discover the network, sending an reply fd: %d", ip_addr, udp_send.fd);

									// send message back
									char msg[500];
									snprintf(msg, sizeof(msg), UPNP_M_SEARCH_OK, rtpsession->interface.ip_addr, HTTP_PORT, rtpsession->satpi_version, rtpsession->uuid, rtpsession->bootId, deviceId);
									if (sendto(udp_send.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_send.addr, sizeof(udp_send.addr)) == -1) {
										PERROR("send");
									}
									CLOSE_FD(udp_send.fd);
								}
								FREE_PTR(st_param);
							}
						}
					}
				}
				FREE_PTR(recv_msg);
			}
		}
		// Notify/announce ourself
		const time_t curr_time = time(NULL);
		if (repeat_time < curr_time) {
			char msg[500];
			// set next announce time
			repeat_time = rtpsession->ssdp_announce_time_sec + curr_time;

			// broadcast message
			snprintf(msg, sizeof(msg), UPNP_ROOTDEVICE, rtpsession->interface.ip_addr, HTTP_PORT, rtpsession->satpi_version, rtpsession->uuid, rtpsession->bootId, deviceId);
			if (sendto(udp_multi_send.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_multi_send.addr, sizeof(udp_multi_send.addr)) == -1) {
				PERROR("send UPNP_ROOTDEVICE");
			}
			usleep(UTIME_DEL);

			// broadcast message
			snprintf(msg, sizeof(msg), UPNP_ALIVE, rtpsession->interface.ip_addr, HTTP_PORT, rtpsession->satpi_version, rtpsession->uuid, rtpsession->uuid, rtpsession->bootId, deviceId);
			if (sendto(udp_multi_send.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_multi_send.addr, sizeof(udp_multi_send.addr)) == -1) {
				PERROR("send UPNP_ALIVE");
			}
			usleep(UTIME_DEL);

			// broadcast message
			snprintf(msg, sizeof(msg), UPNP_DEVICE, rtpsession->interface.ip_addr, HTTP_PORT, rtpsession->satpi_version, rtpsession->uuid, rtpsession->bootId, deviceId);
			if (sendto(udp_multi_send.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_multi_send.addr, sizeof(udp_multi_send.addr)) == -1) {
				PERROR("send UPNP_DEVICE");
			}
		}
	}
	return NULL;
}

/*
 *
 */
void start_ssdp(RtpSession_t *rtpsession) {
	if (pthread_create(&(rtpsession->rtsp_threadID), NULL, &thread_work_ssdp, rtpsession) != 0) {
		PERROR("thread_work_ssdp");
	}
	pthread_setname_np(rtpsession->rtsp_threadID, "thread_ssdp");
}

/*
 *
 */
int stop_ssdp(const RtpSession_t *rtpsession) {
	// Cancel and Join threads
	pthread_cancel(threadID);
	pthread_join(threadID, NULL);

	// thread is closed, send bye bye
	send_byebye(rtpsession->bootId, rtpsession->uuid);

	// last close fd
	CLOSE_FD(udp_multi_listen.fd);
	CLOSE_FD(udp_multi_send.fd);
	return 1;
}
