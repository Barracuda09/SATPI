/* StreamClient.cpp

   Copyright (C) 2014 - 2023 Marc Postema (mpostema09 -at- gmail.com)

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
#include <StreamClient.h>

#include <Log.h>
#include <socket/SocketClient.h>
#include <Stream.h>

// ============================================================================
//  -- Constructors and destructor --------------------------------------------
// ============================================================================

StreamClient::StreamClient() :
		_socketClient(nullptr),
		_sessionTimeoutCheck(SessionTimeoutCheck::WATCHDOG),
		_ipAddress("0.0.0.0"),
		_watchdog(0),
		_sessionTimeout(60),
		_sessionID("-1"),
		_userAgent("None"),
		_cseq(0) {}

StreamClient::~StreamClient() {}

// =======================================================================
//  -- base::XMLSupport --------------------------------------------------
// =======================================================================

void StreamClient::doAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "owner", _ipAddress);
	ADD_XML_ELEMENT(xml, "ownerSessionID", _sessionID);
	ADD_XML_ELEMENT(xml, "userAgent", _userAgent);
	ADD_XML_ELEMENT(xml, "rtpPort", _rtp.getSocketPort());
	ADD_XML_ELEMENT(xml, "rtcpPort", _rtcp.getSocketPort());
	ADD_XML_ELEMENT(xml, "httpPort", (_socketClient == nullptr) ? 0 : _socketClient->getSocketPort());
}

void StreamClient::doFromXML(const std::string &UNUSED(xml)) {}

// ============================================================================
//  -- Other member functions -------------------------------------------------
// ============================================================================

void StreamClient::teardown() {
	base::MutexLock lock(_mutex);

	_watchdog = 0;
	_sessionID = "-1";
	_ipAddress = "0.0.0.0";
	_userAgent = "None";
	_sessionTimeoutCheck = SessionTimeoutCheck::WATCHDOG;

	// Do not delete
	_socketClient = nullptr;
}

void StreamClient::restartWatchDog() {
	base::MutexLock lock(_mutex);

	// reset watchdog and give some extra timeout
	_watchdog = std::time(nullptr) + _sessionTimeout + 15;
}

void StreamClient::selfDestruct() {
	base::MutexLock lock(_mutex);
	_watchdog = 1;
}

bool StreamClient::isSelfDestructing() const {
	base::MutexLock lock(_mutex);
	return _watchdog == 1;
}

bool StreamClient::sessionTimeout() const {
	base::MutexLock lock(_mutex);
	// See if we need to check watchdog or http FD
	switch (_sessionTimeoutCheck) {
		case SessionTimeoutCheck::WATCHDOG:
			return ((_watchdog != 0) && (_watchdog < std::time(nullptr)));
		case SessionTimeoutCheck::FILE_DESCRIPTOR:
			return (_socketClient->getFD() == -1);
		case SessionTimeoutCheck::TEARDOWN:
		default:
			return false;
	};
}

void StreamClient::setSocketClient(SocketClient &socket) {
	base::MutexLock lock(_mutex);
	_socketClient = &socket;
}

SocketAttr &StreamClient::getRtpSocketAttr() {
	return _rtp;
}

SocketAttr &StreamClient::getRtcpSocketAttr() {
	return _rtcp;
}

// ============================================================================
//  -- HTTP member functions --------------------------------------------------
// ============================================================================

bool StreamClient::sendHttpData(const void *buf, std::size_t len, int flags) {
	base::MutexLock lock(_mutex);
	return (_socketClient == nullptr) ? false : _socketClient->sendData(buf, len, flags);
}

bool StreamClient::writeHttpData(const struct iovec *iov, int iovcnt) {
	base::MutexLock lock(_mutex);
	return (_socketClient == nullptr) ? false : _socketClient->writeData(iov, iovcnt);
}

int StreamClient::getHttpSocketPort() const {
	base::MutexLock lock(_mutex);
	return (_socketClient == nullptr) ? 0 : _socketClient->getSocketPort();
}

int StreamClient::getHttpNetworkSendBufferSize() const {
	base::MutexLock lock(_mutex);
	return (_socketClient == nullptr) ? 0 : _socketClient->getNetworkSendBufferSize();
}

bool StreamClient::setHttpNetworkSendBufferSize(int size) {
	base::MutexLock lock(_mutex);
	return (_socketClient == nullptr) ? false : _socketClient->setNetworkSendBufferSize(size);
}
