/* RtpStreamThread.cpp

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#include "RtpStreamThread.h"
#include "StreamClient.h"
#include "Log.h"
#include "StreamProperties.h"
#include "InterfaceAttr.h"
#include "Utils.h"
#include "TimeCounter.h"
#ifdef LIBDVBCSA
	#include "DvbapiClient.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <fcntl.h>

RtpStreamThread::RtpStreamThread(StreamClient *clients, StreamProperties &properties, DvbapiClient *dvbapi) :
		StreamThreadBase("RTP", clients, properties, dvbapi),
		_socket_fd_rtp(-1),
		_cseq(0),
		_rtcp(clients, properties) {}

RtpStreamThread::~RtpStreamThread() {
	terminateThread();
	CLOSE_FD(_socket_fd_rtp);
}

bool RtpStreamThread::startStreaming(int fd_dvr, int fd_fe) {
	const int streamID = _properties.getStreamID();

	// RTP
	if (_socket_fd_rtp == -1) {
		if ((_socket_fd_rtp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			PERROR("socket RTP");
			return false;
		}

		const int val = 1;
		if (setsockopt(_socket_fd_rtp, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
			PERROR("RTP setsockopt SO_REUSEADDR");
			return false;
		}

		// Get default buffer size and set it twice as big
		int bufferSize = InterfaceAttr::getNetworkUDPBufferSize(_socket_fd_rtp);
		bufferSize *= 2;
		InterfaceAttr::setNetworkUDPBufferSize(_socket_fd_rtp, bufferSize);
		SI_LOG_INFO("Stream: %d, %s set network buffer size: %d KBytes", streamID, _protocol.c_str(), bufferSize / 1024);
	}
	// RTCP
	_rtcp.startStreaming(fd_fe);

	_cseq = 0x0000;
	StreamThreadBase::startStreaming(fd_dvr);
	return true;
}

int RtpStreamThread::getStreamSocketPort(int clientID) const {
	return _clients[clientID].getRtpSocketPort();
}

void RtpStreamThread::threadEntry() {
	const StreamClient &client = _clients[0];
	while (running()) {
		switch (_state) {
			case Pause:
				_state = Paused;
				break;
			case Paused:
				// Do nothing here, just wait
				usleep(50000);
				break;
			case Running:
				pollDVR(client);
				break;
			default:
				PERROR("Wrong State");
				usleep(50000);
				break;
		}
	}
}

void RtpStreamThread::sendTSPacket(TSPacketBuffer &buffer, const StreamClient &client) {
	unsigned char *rtpBuffer = buffer.getReadBufferPtr();

	// update sequence number
	++_cseq;
	rtpBuffer[2] = ((_cseq >> 8) & 0xFF); // sequence number
	rtpBuffer[3] =  (_cseq & 0xFF);       // sequence number

	// update timestamp
	const long timestamp = TimeCounter::getTicks() * 90;
	rtpBuffer[4] = (timestamp >> 24) & 0xFF; // timestamp
	rtpBuffer[5] = (timestamp >> 16) & 0xFF; // timestamp
	rtpBuffer[6] = (timestamp >>  8) & 0xFF; // timestamp
	rtpBuffer[7] = (timestamp >>  0) & 0xFF; // timestamp

	const unsigned int size = buffer.getBufferSize();

	// RTP packet octet count (Bytes)
	_properties.addRtpData(size, timestamp);

	// send the RTP packet
	if (sendto(_socket_fd_rtp, rtpBuffer, size + RTP_HEADER_LEN, MSG_DONTWAIT,
	           (struct sockaddr *)&client.getRtpSockAddr(), sizeof(client.getRtpSockAddr())) == -1) {

		PERROR("send RTP");
	}
}
