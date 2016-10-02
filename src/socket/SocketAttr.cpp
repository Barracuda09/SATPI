/* SocketAttr.cpp

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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

#include <socket/SocketClient.h>

#include <string>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

	// ===================================================================
	//  -- Constructors and destructor -----------------------------------
	// ===================================================================
	SocketAttr::SocketAttr() :
		_fd(-1),
		_ip_addr("0.0.0.0") {
		memset(&_addr, 0, sizeof(_addr));
	}

	SocketAttr::~SocketAttr() {
		closeFD();
	}

	// ===================================================================
	//  -- Other member functions ----------------------------------------
	// ===================================================================

	void SocketAttr::setupSocketStructure(int port, const char *ip_addr) {
		// fill in the socket structure with host information
		memset(&_addr, 0, sizeof(_addr));
		_addr.sin_family      = AF_INET;
		_addr.sin_addr.s_addr = inet_addr(ip_addr);
		_addr.sin_port        = htons(port);

		_ip_addr = ip_addr;
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
				PERROR("socket");
				return false;
			}

			const int val = 1;
			if (::setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
				PERROR("setsockopt: SO_REUSEADDR");
				return false;
			}
		}
		return true;
	}

	bool SocketAttr::bind() {
		// bind the socket to the port number
		if (::bind(_fd, (struct sockaddr *) &_addr, sizeof(_addr)) == -1) {
			PERROR("bind");
			return false;
		}
		return true;
	}

	bool SocketAttr::listen(std::size_t maxClients) {
		if (::listen(_fd, maxClients) == -1) {
			PERROR("listen");
			return false;
		}
		return true;
	}

	bool SocketAttr::connectTo() {
		if (::connect(_fd, (struct sockaddr *)&_addr, sizeof(_addr)) == -1) {
			if (errno != ECONNREFUSED && errno != EINPROGRESS) {
				PERROR("connect");
			}
			return false;
		}
		return true;
	}

	bool SocketAttr::acceptConnection(SocketClient &client, bool showLogInfo) {
		// check if we have a client?
		socklen_t addrlen = sizeof(client._addr);
		const int fd_conn = ::accept(_fd, (struct sockaddr *) &client._addr, &addrlen);
		if (fd_conn > 0) {
			// save connected file descriptor
			client.setFD(fd_conn);

			client.setSocketTimeoutInSec(2);

			// save client ip address
			client.setIPAddress(inet_ntoa(client._addr.sin_addr));

			// Show who is connected
			if (showLogInfo) {
				SI_LOG_INFO("%s Connection from %s Port %d (fd: %d)", client.getProtocolString().c_str(),
							client.getIPAddress().c_str(), ntohs(client._addr.sin_port), fd_conn);
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
			PERROR("send");
			return false;
		}
		return true;
	}

	bool SocketAttr::writeData(const struct iovec *iov, const int iovcnt) {
		if (::writev(_fd, iov, iovcnt) == -1) {
			if (errno != EBADF) {
				PERROR("writev");
			}
			return false;
		}
		return true;
	}

	bool SocketAttr::sendDataTo(const void *buf, std::size_t len, int flags) {
		if (::sendto(_fd, buf, len, flags, reinterpret_cast<struct sockaddr *>(&_addr),
				   sizeof(_addr)) == -1) {
			PERROR("sendto");
			return false;
		}
		return true;
	}

	ssize_t SocketAttr::recvDatafrom(void *buf, std::size_t len, int flags) {
		struct sockaddr_in si_other;
		socklen_t addrlen = sizeof(si_other);
		const ssize_t size = ::recvfrom(_fd, buf, len, flags, (struct sockaddr *)&si_other, &addrlen);
		return size;
	}

	int SocketAttr::getFD() const {
		return _fd;
	}

	void SocketAttr::setFD(int fd) {
		_fd = fd;
	}

	void SocketAttr::closeFD() {
		CLOSE_FD(_fd);
		_ip_addr = "0.0.0.0";
	}

	void SocketAttr::setSocketTimeoutInSec(unsigned int timeout) {
		struct timeval tv;
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

//		SI_LOG_DEBUG("Set Timeout to %d Sec (fd: %d)", timeout, _fd);

		if (::setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval))) {
			PERROR("setsockopt: SO_RCVTIMEO");
		}
		if (::setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval))) {
			PERROR("setsockopt: SO_SNDTIMEO");
		}
	}

	int SocketAttr::getNetworkBufferSize() const {
		int bufferSize;
		socklen_t optlen = sizeof(bufferSize);
		if (::getsockopt(_fd, SOL_SOCKET, SO_SNDBUF, &bufferSize, &optlen) == -1) {
			PERROR("getsockopt: SO_SNDBUF");
			bufferSize = 0;
		}
		return bufferSize / 2;
	}

	bool SocketAttr::setNetworkBufferSize(int size) {
		if (::setsockopt(_fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) == -1) {
			PERROR("setsockopt: SO_SNDBUF");
			return false;
		}
		return true;
	}
