/* TcpSocket.cpp

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
#include <socket/TcpSocket.h>

#include <socket/SocketClient.h>
#include <Log.h>
#include <Utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

// ============================================================================
//  -- Constructors and destructor --------------------------------------------
// ============================================================================

TcpSocket::TcpSocket(int maxClients, const std::string &protocol) :
		_MAX_CLIENTS(maxClients),
		_MAX_POLL(maxClients + 1),
		_pfd(new pollfd[maxClients + 1]),
		_client(new SocketClient[maxClients]),
		_protocolString(protocol) {}

TcpSocket::~TcpSocket() {
	DELETE_ARRAY(_pfd);
	DELETE_ARRAY(_client);
	_server.closeFD();
}

// ============================================================================
//  -- Other member functions -------------------------------------------------
// ============================================================================

void TcpSocket::initialize(const std::string &ipAddr, int port, bool nonblock) {
	for (std::size_t i = 0; i < _MAX_CLIENTS; ++i) {
		_client[i].setProtocol(_protocolString);
	}
	if (initServerSocket(ipAddr, port, _MAX_CLIENTS, nonblock)) {
		_pfd[0].fd = _server.getFD();
		_pfd[0].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
		_pfd[0].revents = 0;

		for (std::size_t i = 1; i < _MAX_POLL; ++i) {
			_pfd[i].fd = -1;
			_pfd[i].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
			_pfd[i].revents = 0;
		}
	}
}

int TcpSocket::poll(int timeout) {
	if (::poll(_pfd, _MAX_POLL, timeout) > 0) {
		// Check who is sending data, so iterate over pfd
		for (std::size_t i = 0; i < _MAX_POLL; ++i) {
			if (_pfd[i].revents == 0) {
				continue;
			}
			// index 0 is server
			if (i == 0) {
				// Try to find a free poll entry
				for (std::size_t j = 0; j < _MAX_CLIENTS; ++j) {
					if (_pfd[j + 1].fd != -1) {
						continue;
					}
					if (acceptConnection(_client[j], true)) {
						// setup polling
						_pfd[j + 1].fd = _client[j].getFD();
						_pfd[j + 1].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
						_pfd[j + 1].revents = 0;
						break;
					}
				}
			} else {
				// receive httpc messages
				const auto dataSize = recvHttpcMessage(_client[i-1], MSG_DONTWAIT);
				if (dataSize > 0) {
					process(_client[i - 1]);
					continue;
				}
				// Try to find the client that requested to close
				for (std::size_t j = 0; j < _MAX_CLIENTS; ++j) {
					if (_client[j].getFD() != _pfd[i].fd) {
						continue;
					}
					SI_LOG_INFO("@#1 Client @#2:@#3 Connection closed with fd: @#4",
						_client[j].getProtocolString(),
						_client[j].getIPAddressOfSocket(),
						_client[j].getSocketPort(), _client[j].getFD());
					_client[j].closeFD();
					break;
				}
				_pfd[i].fd = -1;
			}
		}
	}
	return 1;
}

bool TcpSocket::initServerSocket(
		const std::string &ipAddr,
		int port,
		int maxClients,
		bool nonblock) {
	// fill in the socket structure with host information
	_server.setupSocketStructure(ipAddr, port, 0);

	if (!_server.setupSocketHandle(SOCK_STREAM | ((nonblock) ? SOCK_NONBLOCK : 0), 0)) {
		SI_LOG_ERROR("TCP Server handle failed");
		return false;
	}

	_server.setSocketTimeoutInSec(2);

	if (!_server.bind()) {
		SI_LOG_ERROR("TCP Bind failed");
		return false;
	}
	if (!_server.listen(maxClients)) {
		SI_LOG_ERROR("TCP Listen failed");
		return false;
	}
	return true;
}

bool TcpSocket::acceptConnection(SocketClient &client, bool showLogInfo) {
	// check if we have a client?
	return _server.acceptConnection(client, showLogInfo);
}
