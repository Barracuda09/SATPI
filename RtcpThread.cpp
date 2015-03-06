/* RtcpThread.cpp

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#include "RtcpThread.h"
#include "StreamClient.h"
#include "StreamProperties.h"
#include "Log.h"

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <fcntl.h>


RtcpThread::RtcpThread(StreamClient *clients, StreamProperties &properties) :
		_socket_fd(-1),
		_clients(clients),
		_properties(properties),
		_fd_fe(-1) {
}

RtcpThread::~RtcpThread() {
	stopThread();
	joinThread();
}

bool RtcpThread::startStreaming(int fd_fe) {
	_fd_fe = fd_fe;
	if (_socket_fd == -1) {
		if ((_socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			PERROR("socket RTP ");
			return false;
		}

		const int val = 1;
		if (setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
			PERROR("RTP setsockopt SO_REUSEADDR");
			return false;
		}
	}
	if (!startThread()) {
		SI_LOG_ERROR("Stream: %d, ERROR Start RTCP streaming to %s (%d - %d)", 
		             _properties.getStreamID(), _clients[0].getIPAddress().c_str(),
		             _clients[0].getRtpSocketPort(), _clients[0].getRtcpSocketPort());
		return false;
	}
	SI_LOG_INFO("Stream: %d, Start RTCP streaming to %s (%d - %d)", 
	            _properties.getStreamID(), _clients[0].getIPAddress().c_str(),
	            _clients[0].getRtpSocketPort(), _clients[0].getRtcpSocketPort());
	return true;
}

void RtcpThread::stopStreaming(int clientID) {
	if (running()) {
		stopThread();
		joinThread();
		SI_LOG_INFO("Stream: %d, Stop RTCP stream to %s (%d - %d)", 
		            _properties.getStreamID(), _clients[clientID].getIPAddress().c_str(),
		            _clients[clientID].getRtpSocketPort(), _clients[clientID].getRtcpSocketPort());
	}
}

void RtcpThread::monitorFrontend(bool showStatus) {
	fe_status_t status = FE_REINIT;
	
	// first read status
	if (ioctl(_fd_fe, FE_READ_STATUS, &status) == 0) {
		uint16_t strength;
		uint16_t snr;
		uint32_t ber;
		uint32_t ublocks;

		// some frontends might not support all these ioctls
		if (ioctl(_fd_fe, FE_READ_SIGNAL_STRENGTH, &strength) != 0) {
			strength = 0;
		}
		if (ioctl(_fd_fe, FE_READ_SNR, &snr) != 0) {
			snr = 0;
		}
		if (ioctl(_fd_fe, FE_READ_BER, &ber) != 0) {
			ber = -2;
		}
		if (ioctl(_fd_fe, FE_READ_UNCORRECTED_BLOCKS, &ublocks) != 0) {
			ublocks = -2;
		}
		strength = (strength * 255) / 0xffff;
		snr = (snr * 15) / 0xffff;
		
		_properties.setFrontendMonitorData(status, strength, snr, ber, ublocks);

		// Print Status
		if (showStatus) {
			SI_LOG_INFO("status %02x | signal %3u%% | snr %3u%% | ber %d | unc %d | Locked %d",
				status, strength, snr, ber, ublocks,
				(status & FE_HAS_LOCK) ? 1 : 0);
		}
	} else {
		PERROR("FE_READ_STATUS failed");
	}
}

uint8_t *RtcpThread::get_app_packet(size_t *len) {
	uint8_t app[1024 * 2];
	
	uint32_t ssrc = _properties.getSSRC();

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

	bool active = false;
	std::string desc = _properties.attribute_describe_string(active);
	const char *attr_desc_str = desc.c_str();
	memcpy(app + 16, attr_desc_str, desc.size());

	// total length and align on 32 bits
	*len = 16 + desc.size();
	if ((*len % 4) != 0) {
		*len += 4 - (*len % 4);
	}

	// Alloc and copy data and adjust length
	uint8_t *appPtr = new uint8_t[*len];
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
uint8_t *RtcpThread::get_sr_packet(size_t *len) {
	uint8_t sr[28];

	long     timestamp = _properties.getTimestamp();
	uint32_t ssrc = _properties.getSSRC();
	uint32_t spc = _properties.getSPC();
	uint32_t soc = _properties.getSOC();

	// Sender Report (SR Packet)
	sr[0]  = 0x80;                         // version: 2, padding: 0, sr blocks: 0
	sr[1]  = 200;                          // payload type: 200 (0xc8) (SR)
	sr[2]  = 0;                            // length (total in 32-bit words minus one)
	sr[3]  = 0;                            // length (total in 32-bit words minus one)
	sr[4]  = (ssrc >> 24) & 0xff;          // synchronization source
	sr[5]  = (ssrc >> 16) & 0xff;          // synchronization source
	sr[6]  = (ssrc >>  8) & 0xff;          // synchronization source
	sr[7]  = (ssrc >>  0) & 0xff;          // synchronization source
	
//	const time_t ntp = time(NULL);
	const time_t ntp = 0;
	                                       // NTP integer part
	sr[8]  = (ntp >> 24) & 0xff;           // NTP most sign word
	sr[9]  = (ntp >> 16) & 0xff;           // NTP most sign word
	sr[10] = (ntp >>  8) & 0xff;           // NTP most sign word
	sr[11] = (ntp >>  0) & 0xff;           // NTP most sign word
	                                       // NTP fractional part
	sr[12] = 0;                            // NTP least sign word
	sr[13] = 0;                            // NTP least sign word
	sr[14] = 0;                            // NTP least sign word
	sr[15] = 0;                            // NTP least sign word

	sr[16] = (timestamp >> 24) & 0xff;     // RTP timestamp RTS
	sr[17] = (timestamp >> 16) & 0xff;     // RTP timestamp RTS
	sr[18] = (timestamp >>  8) & 0xff;     // RTP timestamp RTS
	sr[19] = (timestamp >>  0) & 0xff;     // RTP timestamp RTS
	sr[20] = (spc >> 24) & 0xff;           // sender's packet count SPC
	sr[21] = (spc >> 16) & 0xff;           // sender's packet count SPC
	sr[22] = (spc >>  8) & 0xff;           // sender's packet count SPC
	sr[23] = (spc >>  0) & 0xff;           // sender's packet count SPC
	sr[24] = (soc >> 24) & 0xff;           // sender's octet count SOC
	sr[25] = (soc >> 16) & 0xff;           // sender's octet count SOC
	sr[26] = (soc >>  8) & 0xff;           // sender's octet count SOC
	sr[27] = (soc >>  0) & 0xff;           // sender's octet count SOC

	*len = sizeof(sr);

	// Alloc and copy data and adjust length
	uint8_t *srPtr = new uint8_t[*len];
	if (srPtr != NULL) {
		memcpy(srPtr, sr, sizeof(sr));
		int ws = (*len / 4) - 1;
		srPtr[2] = (ws >> 8) & 0xff;
		srPtr[3] = (ws >> 0) & 0xff;
	}
	return srPtr;
}

/*
 *
 */
uint8_t *RtcpThread::get_sdes_packet(size_t *len) {
	uint8_t sdes[20];
	uint32_t ssrc = _properties.getSSRC();

	// Source Description (SDES Packet)
	sdes[0]  = 0x81;                           // version: 2, padding: 0, sc blocks: 1
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
	sdes[11] = 'a';                            // data
	
	sdes[12] = 't';                            // data
	sdes[13] = 'P';                            // data
	sdes[14] = 'I';                            // data
	sdes[15] = 0;                              // data
	
	sdes[16] = 0;                              // data
	sdes[17] = 0;                              // data
	sdes[18] = 0;                              // data
	sdes[19] = 0;                              // data

	*len = sizeof(sdes);

	// Alloc and copy data and adjust length
	uint8_t *sdesPtr = new uint8_t[*len];
	if (sdesPtr != NULL) {
		memcpy(sdesPtr, sdes, sizeof(sdes));
		int ws = (*len / 4) - 1;
		sdesPtr[2] = (ws >> 8) & 0xff;
		sdesPtr[3] = (ws >> 0) & 0xff;
	}
	return sdesPtr;
}

void RtcpThread::threadEntry() {
	size_t mon_update = 0;
	const StreamClient &client = _clients[0];

	while (running()) {
		// RTCP compound packets must start with a SR, SDES then APP
		size_t srlen   = 0;
		size_t sdeslen = 0;
		size_t applen  = 0;
		uint8_t *sdes  = get_sdes_packet(&sdeslen);
		uint8_t *sr    = get_sr_packet(&srlen);
		uint8_t *app   = get_app_packet(&applen);

		if (sr && sdes && app) {
			// check do we need to update frontend monitor
			if (mon_update == 0) {
				monitorFrontend(false);
				mon_update = 1;
			} else {
				--mon_update;
			}

			const size_t rtcplen = srlen + sdeslen + applen;

			uint8_t *rtcp = new uint8_t[rtcplen];
			if (rtcp) {
				memcpy(rtcp, sr, srlen);
				memcpy(rtcp + srlen, sdes, sdeslen);
				memcpy(rtcp + srlen + sdeslen, app, applen);

				if (sendto(_socket_fd, rtcp, rtcplen, 0,
				           (struct sockaddr *)&client.getRtcpSockAddr(), sizeof(client.getRtcpSockAddr())) == -1) {
					PERROR("send RTCP");
				}
			}
			delete[] rtcp;
		}
		delete[] sr;
		delete[] sdes;
		delete[] app;

		// update 5 Hz
		usleep(200000);
	}
}
