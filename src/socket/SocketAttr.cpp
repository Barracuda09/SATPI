/* SocketAttr.cpp

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
#include <socket/SocketAttr.h>

#include <Utils.h>
#include <StringConverter.h>
#include <socket/SocketClient.h>

#include <string>
#include <cstring>

#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/socket.h>

	// ===================================================================
	//  -- Constructors and destructor -----------------------------------
	// ===================================================================
	SocketAttr::SocketAttr() :
		_fd(-1),
		_ipAddr("0.0.0.0") {
		std::memset(&_addr, 0, sizeof(_addr));
	}

	SocketAttr::~SocketAttr() {
		closeFD();
	}

	// ===================================================================
	//  -- Other member functions ----------------------------------------
	// ===================================================================

	void SocketAttr::closeFD() {
		CLOSE_FD(_fd);
		_ipAddr = "0.0.0.0";
		_addr.sin_port = 0;
	}

	void SocketAttr::setupSocketStructure(const std::string_view ipAddr, int port) {
		// fill in the socket structure with host information
		memset(&_addr, 0, sizeof(_addr));
		_addr.sin_family      = AF_INET;
		_addr.sin_addr.s_addr = inet_addr(ipAddr.data());
		_addr.sin_port        = htons(port);
		_ipAddr = ipAddr.data();
	}

	void SocketAttr::setupSocketStructureWithAnyAddress(int port) {
		// fill in the socket structure with host information
		memset(&_addr, 0, sizeof(_addr));
		_addr.sin_family      = AF_INET;
		_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		_addr.sin_port        = htons(port);
	}

	bool SocketAttr::setupSocketHandle(int type, int protocol) {
		if (_fd == -1) {
			_fd = ::socket(AF_INET, type, protocol);
			if (_fd == -1) {
				SI_LOG_PERROR("socket");
				return false;
			}

			const int val = 1;
			if (::setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
				SI_LOG_PERROR("setsockopt: SO_REUSEADDR");
				return false;
			}
		}
		return true;
	}

	bool SocketAttr::bind() {
		// bind the socket to the port number
		if (::bind(_fd, reinterpret_cast<sockaddr *>(&_addr), sizeof(_addr)) == -1) {
			SI_LOG_PERROR("bind");
			return false;
		}
		return true;
	}

	bool SocketAttr::listen(std::size_t backlog) {
		if (::listen(_fd, backlog) == -1) {
			SI_LOG_PERROR("listen");
			return false;
		}
		return true;
	}

	bool SocketAttr::connectTo() {
		if (::connect(_fd, reinterpret_cast<sockaddr *>(&_addr), sizeof(_addr)) == -1) {
			if (errno != ECONNREFUSED && errno != EINPROGRESS) {
				SI_LOG_PERROR("connect");
			}
			return false;
		}
		return true;
	}

	bool SocketAttr::acceptConnection(SocketClient &client, bool showLogInfo) {
		// check if we have a client?
		socklen_t addrlen = sizeof(client._addr);
		const int fdAccept = ::accept(_fd, reinterpret_cast<sockaddr *>(&client._addr), &addrlen);
		if (fdAccept >= 0) {

// @todo should 'client' handle this himself?

			// save connected file descriptor
			client.setFD(fdAccept);

			client.setSocketTimeoutInSec(2);

			client.setKeepAlive();

			// save client ip address
			client.setIPAddressOfSocket(inet_ntoa(client._addr.sin_addr));

			// Show who is connected
			if (showLogInfo) {
				SI_LOG_INFO("@#1 Connection from @#2 Port @#3 with fd: @#4", client.getProtocolString(),
					client.getIPAddressOfSocket(), ntohs(client._addr.sin_port), fdAccept);
			}
			return true;
		}
		return false;
	}

	int SocketAttr::getSocketPort() const {
		return ntohs(_addr.sin_port);
	}

	bool SocketAttr::sendData(const void *buf, std::size_t len, int flags) {
		if (::send(_fd, buf, len, flags) == -1) {
			SI_LOG_PERROR("send");
			return false;
		}
		return true;
	}

	bool SocketAttr::writeData(const iovec *iov, const int iovcnt) {
		if (::writev(_fd, iov, iovcnt) == -1) {
			if (errno != EBADF) {
				SI_LOG_PERROR("writev");
			}
			return false;
		}
		return true;
	}

	bool SocketAttr::sendDataTo(const void *buf, std::size_t len, int flags) {
		if (::sendto(_fd, buf, len, flags, reinterpret_cast<sockaddr *>(&_addr),
				   sizeof(_addr)) == -1) {
			SI_LOG_PERROR("sendto");
			return false;
		}
		return true;
	}

	ssize_t SocketAttr::recvDatafrom(void *buf, std::size_t len, int flags) {
		struct sockaddr_in si_other;
		socklen_t addrlen = sizeof(si_other);
		const ssize_t size = ::recvfrom(_fd, buf, len, flags, reinterpret_cast<sockaddr *>(&si_other), &addrlen);
		return size;
	}

	int SocketAttr::getFD() const {
		return _fd;
	}

	void SocketAttr::setFD(int fd) {
		_fd = fd;
	}

	void SocketAttr::setSocketTimeoutInSec(unsigned int timeout) {
		struct timeval tv;
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

//		SI_LOG_DEBUG("Set Timeout to @#1 Sec (fd: @#2)", timeout, _fd);

		if (::setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval))) {
			SI_LOG_PERROR("setsockopt: SO_RCVTIMEO");
		}
		if (::setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval))) {
			SI_LOG_PERROR("setsockopt: SO_SNDTIMEO");
		}
	}

	void SocketAttr::setKeepAlive() {
		// /proc/sys/net/ipv4/tcp_keepalive_time
		// /proc/sys/net/ipv4/tcp_keepalive_intvl
		// /proc/sys/net/ipv4/tcp_keepalive_probes
		// sysctl -p
		int optval = 1;
		if (::setsockopt(_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == -1) {
			SI_LOG_PERROR("setsockopt: SO_KEEPALIVE");
		}
	}

	int SocketAttr::getNetworkSendBufferSize() const {
		int bufferSize;
		socklen_t optlen = sizeof(bufferSize);
		if (::getsockopt(_fd, SOL_SOCKET, SO_SNDBUF, &bufferSize, &optlen) == -1) {
			SI_LOG_PERROR("getsockopt: SO_SNDBUF");
			bufferSize = 0;
		}
		return bufferSize / 2;
	}

	bool SocketAttr::setNetworkSendBufferSize(int size) {
		if (::setsockopt(_fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) == -1) {
			SI_LOG_PERROR("setsockopt: SO_SNDBUF");
			return false;
		}
		return true;
	}

	bool SocketAttr::setNetworkReceiveBufferSize(int size) {
		if (::setsockopt(_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) == -1) {
			SI_LOG_PERROR("setsockopt: SO_RCVBUF");
			return false;
		}
		return true;
	}

	bool SocketAttr::setSocketMutlicastTTL(int ttl) {
		if (::setsockopt(_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == -1) {
			SI_LOG_PERROR("setsockopt: IP_MULTICAST_TTL");
			return false;
		}
		return true;
	}

	bool SocketAttr::setSocketUnicastTTL(int ttl) {
		if (::setsockopt(_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) == -1) {
			SI_LOG_PERROR("setsockopt: IP_MULTICAST_TTL");
			return false;
		}
		return true;
	}
