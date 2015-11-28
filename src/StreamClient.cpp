/* StreamClient.cpp

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

#include "Log.h"
#include "SocketClient.h"
#include "Stream.h"
#include "StreamClient.h"
#include "Utils.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

StreamClient::StreamClient() :
		_rtspFD(-1),
		_rtspMsg(""),
		_ip_addr("0.0.0.0"),
		_sessionID("-1"),
		_watchdog(0),
		_sessionTimeout(60),
		_cseq(0),
		_canClose(false) {;}

StreamClient::~StreamClient() {;}

void StreamClient::setCanClose(bool close) {
	MutexLock lock(_mutex);

	SI_LOG_DEBUG("Connection can close: %d", close);
	_canClose = close;
}

void StreamClient::teardown(bool gracefull) {
	MutexLock lock(_mutex);

	_watchdog = 0;
	_canClose = true;
	if (!gracefull) {
		_sessionID = "-1";
		_ip_addr = "0.0.0.0";

		CLOSE_FD(_rtspFD);
	}
	_rtspFD = -1;
}

void StreamClient::restartWatchDog() {
	MutexLock lock(_mutex);

	// reset watchdog and give some extra timeout
	_watchdog = time(NULL) + _sessionTimeout + 15;
}

bool StreamClient::checkWatchDogTimeout() {
	MutexLock lock(_mutex);

	return _watchdog != 0 && _watchdog < time(NULL);
}

void StreamClient::copySocketClientAttr(const SocketClient &socket) {
	MutexLock lock(_mutex);

	_rtspFD = socket.getFD();
	_ip_addr = socket.getIPAddress();
	_rtspMsg = socket.getMessage().c_str();
}

void StreamClient::setRtpSocketPort(int port) {
	MutexLock lock(_mutex);

	_rtp._addr.sin_family = AF_INET;
	_rtp._addr.sin_addr.s_addr = inet_addr(_ip_addr.c_str());
	_rtp._addr.sin_port = htons(port);
}

int StreamClient::getRtpSocketPort() const {
	MutexLock lock(_mutex);

	return ntohs(_rtp._addr.sin_port);
}

void StreamClient::setRtcpSocketPort(int port) {
	MutexLock lock(_mutex);

	_rtcp._addr.sin_family = AF_INET;
	_rtcp._addr.sin_addr.s_addr = inet_addr(_ip_addr.c_str());
	_rtcp._addr.sin_port = htons(port);
}

int StreamClient::getRtcpSocketPort() const {
	MutexLock lock(_mutex);

	return ntohs(_rtcp._addr.sin_port);
}

const struct sockaddr_in &StreamClient::getRtpSockAddr() const {
	MutexLock lock(_mutex);

	return _rtp._addr;
}

const struct sockaddr_in &StreamClient::getRtcpSockAddr() const {
	MutexLock lock(_mutex);

	return _rtcp._addr;
}
