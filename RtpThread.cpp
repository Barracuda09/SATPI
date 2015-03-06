/* RtpThread.cpp

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
#include "RtpThread.h"
#include "StreamClient.h"
#include "Log.h"
#include "StreamProperties.h"

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <fcntl.h>


RtpThread::RtpThread(StreamClient *clients, StreamProperties &properties) :
		_socket_fd(-1),
		_fd_dvr(-1),
		_clients(clients),
		_send_interval(0),
		_cseq(0),
		_properties(properties) {

	uint32_t ssrc = _properties.getSSRC();
	long timestamp = _properties.getTimestamp();

	_buffer[0]  = 0x80;                         // version: 2, padding: 0, extension: 0, CSRC: 0
	_buffer[1]  = 33;                           // marker: 0, payload type: 33 (MP2T)
	_buffer[2]  = (_cseq >> 8) & 0xff;          // sequence number
	_buffer[3]  = (_cseq >> 0) & 0xff;          // sequence number
	_buffer[4]  = (timestamp >> 24) & 0xff;     // timestamp
	_buffer[5]  = (timestamp >> 16) & 0xff;     // timestamp
	_buffer[6]  = (timestamp >>  8) & 0xff;     // timestamp
	_buffer[7]  = (timestamp >>  0) & 0xff;     // timestamp
	_buffer[8]  = (ssrc >> 24) & 0xff;          // synchronization source
	_buffer[9]  = (ssrc >> 16) & 0xff;          // synchronization source
	_buffer[10] = (ssrc >>  8) & 0xff;          // synchronization source
	_buffer[11] = (ssrc >>  0) & 0xff;          // synchronization source
}

RtpThread::~RtpThread() {
	stopThread();
	joinThread();
}

bool RtpThread::startStreaming(int fd_dvr) {
	_fd_dvr = fd_dvr;
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
		SI_LOG_ERROR("Stream: %d, ERROR RTP Start streaming to %s (%d - %d)", _properties.getStreamID(), _clients[0].getIPAddress().c_str(),
		             _clients[0].getRtpSocketPort(), _clients[0].getRtcpSocketPort());
		return false;
	}
	SI_LOG_INFO("Stream: %d, Start RTP streaming to %s (%d - %d)", _properties.getStreamID(), _clients[0].getIPAddress().c_str(),
	            _clients[0].getRtpSocketPort(), _clients[0].getRtcpSocketPort());
	return true;
}

void RtpThread::stopStreaming(int clientID) {
	if (running()) {
		stopThread();
		joinThread();
		SI_LOG_INFO("Stream: %d, Stop RTP streaming to %s (%d - %d) (Streamed %.3f MBytes)", 
		            _properties.getStreamID(), _clients[clientID].getIPAddress().c_str(),
		            _clients[clientID].getRtpSocketPort(), _clients[clientID].getRtcpSocketPort(),
		            (_properties.getRtpPayload() / (1024.0 * 1024.0)));
	}
}

long RtpThread::getmsec() {
  struct timeval tv;
  gettimeofday(&tv,(struct timezone*) NULL);
  return(tv.tv_sec%1000000)*1000 + tv.tv_usec/1000;
}

void RtpThread::threadEntry() {
	struct pollfd pfd[1];

	const StreamClient &client = _clients[0];

	// init
	_cseq = 0x0000;
	_bufPtr = _buffer + RTP_HEADER_LEN;
	pfd[0].fd = _fd_dvr;
	pfd[0].events = POLLIN | POLLPRI;
	pfd[0].revents = 0;

	while (running()) {
		// call poll with a timeout of 100 ms
		const int pollRet = poll(pfd, 1, 100);
		if (pollRet > 0) {
			// try read TS_PACKET_SIZE from DVR
			const int bytes_read = read(pfd[0].fd, _bufPtr, TS_PACKET_SIZE);
			if (bytes_read > 0) {
				// sync byte then check cc
				if (_bufPtr[0] == 0x47 && bytes_read > 3) {
					// get PID from TS
					const uint16_t pid = ((_bufPtr[1] & 0x1f) << 8) | _bufPtr[2];
					// get CC from TS
					const uint8_t cc  = _bufPtr[3] & 0x0f;

					_properties.addPIDCounterAndSetCC(pid, cc);
				}
				// inc buffer pointer
				_bufPtr += bytes_read;
				const int len = _bufPtr - _buffer;

				// rtp buffer full
				const long time_ms = getmsec();
				if ((len + TS_PACKET_SIZE) > MTU || _send_interval < time_ms) {
					// reset the time interval
					_send_interval = time_ms + 100;
					
					// update sequence number
					++_cseq;
					_buffer[2] = ((_cseq >> 8) & 0xFF); // sequence number
					_buffer[3] =  (_cseq & 0xFF);       // sequence number

					// update timestamp
					long timestamp = time_ms * 90;
					_buffer[4] = (timestamp >> 24) & 0xFF; // timestamp
					_buffer[5] = (timestamp >> 16) & 0xFF; // timestamp
					_buffer[6] = (timestamp >>  8) & 0xFF; // timestamp
					_buffer[7] = (timestamp >>  0) & 0xFF; // timestamp

					// RTP packet octet count (Bytes)
					const uint32_t byte = (len - RTP_HEADER_LEN);
					_properties.addIncRtpOctetCount(byte, timestamp);

					// send the RTP packet
					if (sendto(_socket_fd, _buffer, len, MSG_DONTWAIT,
					          (struct sockaddr *)&client.getRtpSockAddr(), sizeof(client.getRtpSockAddr())) == -1) {
						PERROR("send RTP");
					}
					// set bufPtr to begin of RTP data (after Header)
					_bufPtr = _buffer + RTP_HEADER_LEN;
				}
			}
		}
	}
}
