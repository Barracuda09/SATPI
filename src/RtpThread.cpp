/* RtpThread.cpp

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
#include "RtpThread.h"
#include "StreamClient.h"
#include "Log.h"
#include "StreamProperties.h"
#include "Utils.h"
#ifdef LIBDVBCSA
	#include "DvbapiClient.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <fcntl.h>

RtpThread::RtpThread(StreamClient *clients, StreamProperties &properties, DvbapiClient *dvbapi) :
		ThreadBase("RtpThread"),
		_socket_fd(-1),
		_clients(clients),
		_cseq(0),
		_properties(properties),
		_state(Running),
		_dvbapi(dvbapi) {
#ifdef LIBDVBCSA
	assert(_dvbapi);
#endif
	_pfd[0].fd = -1;

	// Initialize all RTP packets
	uint32_t ssrc = _properties.getSSRC();
	long timestamp = _properties.getTimestamp();
	for (size_t i = 0; i < MAX_BUF; ++i) {
		_rtpBuffer[i].initialize(ssrc, timestamp);
	}
}

RtpThread::~RtpThread() {
	cancelThread();
	joinThread();
}

bool RtpThread::startStreaming(int fd_dvr) {
	const int streamID = _properties.getStreamID();
	_pfd[0].fd = fd_dvr;
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
	setState(Running);
	if (!startThread()) {
		SI_LOG_ERROR("Stream: %d, ERROR   RTP Start stream to %s:%d", streamID,
		             _clients[0].getIPAddress().c_str(), _clients[0].getRtpSocketPort());
		return false;
	}
	// Set priority above normal for RTP Thread
	setPriority(AboveNormal);

	// Set affinity with CPU
//	setAffinity(3);

	SI_LOG_INFO("Stream: %d, Start   RTP stream to %s:%d (fd: %d)", streamID,
	            _clients[0].getIPAddress().c_str(), _clients[0].getRtpSocketPort(), _pfd[0].fd);

	return true;
}

void RtpThread::stopStreaming(int clientID) {
	// Check if thread is running
	if (running()) {
		stopThread();
		joinThread();
		SI_LOG_INFO("Stream: %d, Stop   RTP stream to %s:%d (Streamed %.3f MBytes)",
		            _properties.getStreamID(), _clients[clientID].getIPAddress().c_str(),
		            _clients[clientID].getRtpSocketPort(), (_properties.getRtpPayload() / (1024.0 * 1024.0)));
#ifdef LIBDVBCSA
		_dvbapi->stopDecrypt(_properties.getStreamID());
#endif
	}
}

bool RtpThread::restartStreaming(int fd_dvr, int clientID) {
	// Check if thread is running
	if (running()) {
		_pfd[0].fd = fd_dvr;
		setState(Running);
		SI_LOG_INFO("Stream: %d, Restart RTP stream to %s:%d", _properties.getStreamID(),
					_clients[clientID].getIPAddress().c_str(), _clients[clientID].getRtpSocketPort());
	}
	return true;
}

bool RtpThread::pauseStreaming(int clientID) {
	// Check if thread is running
	if (running()) {
		setState(Pause);
		// try waiting on pause
		size_t timeout = 0;
		while (getState() != Paused) {
			usleep(50000);
			++timeout;
			if (timeout > 5u) {
				SI_LOG_ERROR("Stream: %d, Pause  RTP stream to %s:%d  TIMEOUT (Streamed %.3f MBytes)",
				             _properties.getStreamID(), _clients[clientID].getIPAddress().c_str(),
				             _clients[clientID].getRtpSocketPort(), (_properties.getRtpPayload() / (1024.0 * 1024.0)));
				return false;
			}
		}
		SI_LOG_INFO("Stream: %d, Pause  RTP stream to %s:%d (Streamed %.3f MBytes)",
					_properties.getStreamID(), _clients[clientID].getIPAddress().c_str(),
					_clients[clientID].getRtpSocketPort(), (_properties.getRtpPayload() / (1024.0 * 1024.0)));
#ifdef LIBDVBCSA
		_dvbapi->stopDecrypt(_properties.getStreamID());
#endif
	}
	return true;
}

bool RtpThread::readFullRtpPacket(RtpPacketBuffer &buffer) {
	// try read maximum amount of bytes from DVR
	const int bytes = read(_pfd[0].fd, buffer.getWriteBufferPtr(), buffer.getAmountOfBytesToWrite());
	if (bytes > 0) {
		buffer.addAmountOfBytesWritten(bytes);
		return buffer.full();
	}
	return false;
}

long RtpThread::getmsec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

void RtpThread::threadEntry() {
	const StreamClient &client = _clients[0];

	// init
	_cseq = 0x0000;
	_pfd[0].events  = POLLIN | POLLPRI;
	_pfd[0].revents = 0;

	_writeIndex = 0;
	_readIndex  = 0;
	_rtpBuffer[_writeIndex].reset();

	while (running()) {
		switch (getState()) {
			case Pause:
				_writeIndex = 0;
				_readIndex  = 0;
				_rtpBuffer[_writeIndex].reset();
				setState(Paused);
				break;
			case Paused:
				// Do nothing here, just wait
				usleep(50000);
				break;
			case Running:
				{
					// call poll with a timeout of 100 ms
					const int pollRet = poll(_pfd, 1, 100);
					if (pollRet > 0) {
						if (readFullRtpPacket(_rtpBuffer[_writeIndex])) {
/*
							// sync byte then check cc
							if (_bufferPtrWrite[0] == 0x47 && bytes_read > 3) {
								// get PID and CC from TS
								const uint16_t pid = ((_bufferPtrWrite[1] & 0x1f) << 8) | _bufferPtrWrite[2];
								const uint8_t  cc  =   _bufferPtrWrite[3] & 0x0f;
								_properties.addPIDData(pid, cc);
							}
*/
#ifdef LIBDVBCSA
							//
							_dvbapi->decrypt(_properties.getStreamID(), _rtpBuffer[_writeIndex]);
#endif
							// goto next, so inc write index
							++_writeIndex;
							_writeIndex %= MAX_BUF;

							// reset next
							_rtpBuffer[_writeIndex].reset();

							// should we send some packets
							while (_rtpBuffer[_readIndex].isReadyToSend()) {
								sendRtpPacket(_rtpBuffer[_readIndex], client);

								// inc read index
								++_readIndex;
								_readIndex %= MAX_BUF;

								// should we stop searching
								if (_readIndex == _writeIndex) {
									break;
								}
							}
						}
					}
				}
				break;
			default:
				PERROR("RTP Wrong State");
				usleep(50000);
				break;
		}
	}
}

void RtpThread::sendRtpPacket(RtpPacketBuffer &buffer, const StreamClient &client) {
	unsigned char *rtpBuffer = buffer.getReadBufferPtr();

	// update sequence number
	++_cseq;
	rtpBuffer[2] = ((_cseq >> 8) & 0xFF); // sequence number
	rtpBuffer[3] =  (_cseq & 0xFF);       // sequence number

	// update timestamp
	const long timestamp = getmsec();
	rtpBuffer[4] = (timestamp >> 24) & 0xFF; // timestamp
	rtpBuffer[5] = (timestamp >> 16) & 0xFF; // timestamp
	rtpBuffer[6] = (timestamp >>  8) & 0xFF; // timestamp
	rtpBuffer[7] = (timestamp >>  0) & 0xFF; // timestamp

	const unsigned int size = buffer.getBufferSize();

	// RTP packet octet count (Bytes)
	_properties.addRtpData(size, timestamp);

	// send the RTP packet
	if (sendto(_socket_fd, rtpBuffer, size + RTP_HEADER_LEN, MSG_DONTWAIT,
			  (struct sockaddr *)&client.getRtpSockAddr(), sizeof(client.getRtpSockAddr())) == -1) {
		PERROR("send RTP");
	}
}
