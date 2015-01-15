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
#include <sys/types.h>
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

#define UTIME_DEL         200000
#define ANNOUNCE_TIME_SEC 60

static SocketAttr_t udp_conn;
static pthread_t threadID;

/*
 *
 */
static void * thread_work_ssdp(void *arg) {
#define UUID "50c958a8-e839-4b96-b7ae-%s"
#define UPNP_ROOTDEVICE	"NOTIFY * HTTP/1.1\r\n" \
						"HOST: 239.255.255.250:1900\r\n" \
						"CACHE-CONTROL: max-age=1800\r\n" \
						"LOCATION: http://%s:%d/desc.xml\r\n" \
						"NT: upnp:rootdevice\r\n" \
						"NTS: ssdp:alive\r\n" \
						"SERVER: Linux/1.0 UPnP/1.1 SAT>IP/1.0\r\n" \
						"USN: uuid:%s::upnp:rootdevice\r\n" \
						"BOOTID.UPNP.ORG: 2318\r\n" \
						"CONFIGID.UPNP.ORG: 0\r\n" \
						"DEVICEID.SES.COM: %d\r\n" \
						"\r\n"

#define UPNP_ALIVE	"NOTIFY * HTTP/1.1\r\n" \
					"HOST: 239.255.255.250:1900\r\n" \
					"CACHE-CONTROL: max-age=1800\r\n" \
					"LOCATION: http://%s:%d/desc.xml\r\n" \
					"NT: uuid:%s\r\n" \
					"NTS: ssdp:alive\r\n" \
					"SERVER: Linux/1.0 UPnP/1.1 SAT>IP/1.0\r\n" \
					"USN: uuid:%s\r\n" \
					"BOOTID.UPNP.ORG: 2318\r\n" \
					"CONFIGID.UPNP.ORG: 0\r\n" \
					"DEVICEID.SES.COM: %d\r\n" \
					"\r\n"

#define UPNP_DEVICE "NOTIFY * HTTP/1.1\r\n" \
					"HOST: 239.255.255.250:1900\r\n" \
					"CACHE-CONTROL: max-age=1800\r\n" \
					"LOCATION: http://%s:%d/desc.xml\r\n" \
					"NT: urn:ses-com:device:SatIPServer:1\r\n" \
					"NTS: ssdp:alive\r\n" \
					"SERVER: Linux/1.0 UPnP/1.1 SAT>IP/1.0\r\n" \
					"USN: uuid:%s::urn:ses-com:device:SatIPServer:1\r\n" \
					"BOOTID.UPNP.ORG: 2318\r\n" \
					"CONFIGID.UPNP.ORG: 0\r\n" \
					"DEVICEID.SES.COM: %d\r\n" \
					"\r\n"
	RtpSession_t *rtpsession = (RtpSession_t *)arg;
	char msg[500];
	char uuid[40];
	unsigned int deviceId = 0xbeef; // just for now a different value

	SI_LOG_INFO("Setting up SSDP server");
	
	// Make UUID with MAC address
	snprintf(uuid, sizeof(uuid), UUID, rtpsession->interface.mac_addr);

	// fill in the socket structure with host information
	memset(&udp_conn.addr, 0, sizeof(udp_conn.addr));
	udp_conn.addr.sin_family      = AF_INET;
	udp_conn.addr.sin_addr.s_addr = inet_addr("239.255.255.250");
	udp_conn.addr.sin_port        = htons(SSDP_PORT);

	if ((udp_conn.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		PERROR("socket");
		return NULL;
	}
	
	for (;;) {
//		SI_LOG_INFO("Announcement SAT>IP server");

		snprintf(msg, sizeof(msg), UPNP_ROOTDEVICE, rtpsession->interface.ip_addr, HTTP_PORT, uuid, deviceId);

		// broadcast message
		if (sendto(udp_conn.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_conn.addr, sizeof(udp_conn.addr)) == -1) {
			PERROR("send");
			return NULL;
		}
		usleep(UTIME_DEL);
		// broadcast message
		if (sendto(udp_conn.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_conn.addr, sizeof(udp_conn.addr)) == -1) {
			PERROR("send");
			return NULL;
		}
		usleep(UTIME_DEL);

		snprintf(msg, sizeof(msg), UPNP_ALIVE, rtpsession->interface.ip_addr, HTTP_PORT, uuid, uuid, deviceId);

		// broadcast message
		if (sendto(udp_conn.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_conn.addr, sizeof(udp_conn.addr)) == -1) {
			PERROR("send");
			return NULL;
		}
		usleep(UTIME_DEL);
		// broadcast message
		if (sendto(udp_conn.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_conn.addr, sizeof(udp_conn.addr)) == -1) {
			PERROR("send");
			return NULL;
		}
		usleep(UTIME_DEL);

		snprintf(msg, sizeof(msg), UPNP_DEVICE, rtpsession->interface.ip_addr, HTTP_PORT, uuid, deviceId);

		// broadcast message
		if (sendto(udp_conn.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_conn.addr, sizeof(udp_conn.addr)) == -1) {
			PERROR("send");
			return NULL;
		}
		usleep(UTIME_DEL);
		// broadcast message
		if (sendto(udp_conn.fd, msg, strlen(msg), 0, (struct sockaddr *)&udp_conn.addr, sizeof(udp_conn.addr)) == -1) {
			PERROR("send");
			return NULL;
		}
		sleep(ANNOUNCE_TIME_SEC);
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
int stop_ssdp() {
	CLOSE_FD(udp_conn.fd);

	// Cancel and Join threads
	pthread_cancel(threadID);
	pthread_join(threadID, NULL);
	return 1;
}
