/* StreamThreadBase.cpp

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
#include "StreamThreadBase.h"
#include "StreamClient.h"
#include "StringConverter.h"
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

StreamThreadBase::StreamThreadBase(std::string protocol, StreamClient *clients, StreamProperties &properties, DvbapiClient *dvbapi) :
		ThreadBase(StringConverter::getFormattedString("Streaming%d", properties.getStreamID())),
		_properties(properties),
		_clients(clients),
		_protocol(protocol),
		_state(Paused),
		_dvbapi(dvbapi) {
#ifdef LIBDVBCSA
	assert(_dvbapi);
#endif
	_pfd[0].fd = -1;

	// Initialize all TS packets
	uint32_t ssrc = _properties.getSSRC();
	long timestamp = _properties.getTimestamp();
	for (size_t i = 0; i < MAX_BUF; ++i) {
		_tsBuffer[i].initialize(ssrc, timestamp);
	}
}

StreamThreadBase::~StreamThreadBase() {
#ifdef LIBDVBCSA
	// Do not destroy _dvbapi here
	_dvbapi->stopDecrypt(_properties.getStreamID());
#endif
	const int streamID = _properties.getStreamID();
	SI_LOG_INFO("Stream: %d, Destroy %s stream", streamID, _protocol.c_str());
}

bool StreamThreadBase::startStreaming(int fd_dvr) {
	const int streamID = _properties.getStreamID();
	const int clientID = 0;

	_pfd[0].fd         = fd_dvr;
	_pfd[0].events     = POLLIN | POLLPRI;
	_pfd[0].revents    = 0;

	_writeIndex        = 0;
	_readIndex         = 0;
	_tsBuffer[_writeIndex].reset();

	if (!startThread()) {
		SI_LOG_ERROR("Stream: %d, Start %s Start stream to %s:%d ERROR", streamID, _protocol.c_str(),
			_clients[clientID].getIPAddress().c_str(), getStreamSocketPort(clientID));
		return false;
	}
	// Set priority above normal for this Thread
	setPriority(AboveNormal);

	// Set affinity with CPU
//	setAffinity(3);

	_state = Running;
	SI_LOG_INFO("Stream: %d, Start %s stream to %s:%d (Client fd: %d) (DVR fd: %d)", streamID, _protocol.c_str(),
		_clients[clientID].getIPAddress().c_str(), getStreamSocketPort(clientID), _clients[clientID].getHttpcFD(), _pfd[0].fd);

	return true;
}

bool StreamThreadBase::restartStreaming(int fd_dvr, int clientID) {
	// Check if thread is running
	if (running()) {
		_writeIndex = 0;
		_readIndex  = 0;
		_tsBuffer[_writeIndex].reset();
		_pfd[0].fd = fd_dvr;
		_state = Running;
		SI_LOG_INFO("Stream: %d, Restart %s stream to %s:%d (fd: %d)", _properties.getStreamID(),
			_protocol.c_str(), _clients[clientID].getIPAddress().c_str(), getStreamSocketPort(clientID), _pfd[0].fd);
	}
	return true;
}

bool StreamThreadBase::pauseStreaming(int clientID) {
	bool paused = true;
	// Check if thread is running
	if (running()) {
		_state = Pause;
		const double payload = _properties.getRtpPayload() / (1024.0 * 1024.0);
		// try waiting on pause
		auto timeout = 0;
		while (_state != Paused) {
			usleep(50000);
			++timeout;
			if (timeout > 50) {
				SI_LOG_ERROR("Stream: %d, Pause %s stream to %s:%d  TIMEOUT (Streamed %.3f MBytes)",
					_properties.getStreamID(), _protocol.c_str(), _clients[clientID].getIPAddress().c_str(),
					getStreamSocketPort(clientID), payload);
				paused = false;
				break;
			}
		}
		if (paused) {
			SI_LOG_INFO("Stream: %d, Pause %s stream to %s:%d (Streamed %.3f MBytes)",
				_properties.getStreamID(), _protocol.c_str(), _clients[clientID].getIPAddress().c_str(),
				getStreamSocketPort(clientID), payload);
		}
#ifdef LIBDVBCSA
		_dvbapi->stopDecrypt(_properties.getStreamID());
#endif
	}
	return paused;
}

bool StreamThreadBase::readFullTSPacket(TSPacketBuffer &buffer) {
	// try read maximum amount of bytes from DVR
	const int bytes = read(_pfd[0].fd, buffer.getWriteBufferPtr(), buffer.getAmountOfBytesToWrite());
	if (bytes > 0) {
		buffer.addAmountOfBytesWritten(bytes);
		return buffer.full();
	}
	return false;
}

long StreamThreadBase::getmsec() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

void StreamThreadBase::pollDVR(const StreamClient &client) {
	// call poll with a timeout of 100 ms
	const int pollRet = poll(_pfd, 1, 100);
	if (pollRet > 0) {
		if (readFullTSPacket(_tsBuffer[_writeIndex])) {
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
			_dvbapi->decrypt(_properties.getStreamID(), _tsBuffer[_writeIndex]);
#endif
			// goto next, so inc write index
			++_writeIndex;
			_writeIndex %= MAX_BUF;

			// reset next
			_tsBuffer[_writeIndex].reset();

			// should we send some packets
			while (_tsBuffer[_readIndex].isReadyToSend()) {
				sendTSPacket(_tsBuffer[_readIndex], client);

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
