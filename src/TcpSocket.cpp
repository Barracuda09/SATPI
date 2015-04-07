/* TcpSocket.cpp

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
#include "TcpSocket.h"
#include "SocketClient.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "Log.h"
#include "Utils.h"

TcpSocket::TcpSocket(int maxClients, int port, bool nonblock) :
		_MAX_CLIENTS(maxClients),
		_MAX_POLL(maxClients+1),
		_pfd(new pollfd[maxClients+1]),
		_client(new SocketClient[maxClients]) {
	if (initServerSocket(maxClients, port, nonblock)) {
		_pfd[0].fd = _server.getFD();
		_pfd[0].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
		_pfd[0].revents = 0;
		for (size_t i = 1; i < _MAX_POLL; ++i) {
			_pfd[i].fd = -1;
			_pfd[i].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
			_pfd[i].revents = 0;
		}
	}
}

TcpSocket::~TcpSocket() {
	delete[] _pfd;
	delete[] _client;
	_server.closeFD();
}

int TcpSocket::poll(int timeout) {
	if (::poll(_pfd, _MAX_POLL, timeout) > 0) {
		// Check who is sending data, so iterate over pfd
		for (size_t i = 0; i < _MAX_POLL; ++i) {
			if (_pfd[i].revents != 0) {
				if (i == 0) {
					// Try to find a free poll entry
					for (size_t j = 0; j < _MAX_CLIENTS; ++j) {
						if (_pfd[j+1].fd == -1) {
							if (acceptConnection(_client[j], 1) == 1) {
								// setup polling
								_pfd[j+1].fd = _client[j].getFD();
								_pfd[j+1].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
								_pfd[j+1].revents = 0;
								break;
							}
						}
					}
				} else {
					// receive httpc messages
					const int dataSize = recvHttpcMessage(_client[i-1], MSG_DONTWAIT);
					if (dataSize > 0) {
						process(_client[i-1]);
					} else {
						// Try to find the client that requested to close
						size_t j;
						for (j = 0; j < _MAX_CLIENTS; ++j) {
							if (_client[j].getFD() == _pfd[i].fd) {
								SI_LOG_DEBUG("%s Client %s: Connection closed with fd: %d", _client[j].getProtocolString(),
								                    _client[j].getIPAddress().c_str(), _client[j].getFD());
								closeConnection(_client[j]);
								_client[j].closeFD();
								break;
							}
						}
						_pfd[i].fd = -1;
					}
				}
			}
		}
	}
	return 1;
}

bool TcpSocket::initServerSocket(int maxClients, int port, bool nonblock) {
	// fill in the socket structure with host information
	_server._addr.sin_family      = AF_INET;
	_server._addr.sin_addr.s_addr = INADDR_ANY;
	_server._addr.sin_port        = htons(port);

	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM | ((nonblock) ? SOCK_NONBLOCK : 0), 0)) == -1) {
		PERROR("Server socket");
		return false;
	}
	_server.setFD(fd);

	int val = 1;
	if (setsockopt(_server.getFD(), SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		PERROR("Server setsockopt SO_REUSEADDR");
		return false;
	}

	// bind the socket to the port number 
	if (bind(_server.getFD(), (struct sockaddr *) &_server._addr, sizeof(_server._addr)) == -1) {
		PERROR("Server bind");
		return false;
	}

	// start to listen
	if (listen(_server.getFD(), maxClients) == -1) {
		PERROR("Server listen");
		return false;
	}
	return true;
}

bool TcpSocket::acceptConnection(SocketClient &client, bool showLogInfo) {
	// check if we have a client?
	socklen_t addrlen = sizeof(client._addr);
	const int fd_conn = accept(_server.getFD(), (struct sockaddr *) &client._addr, &addrlen);
	if (fd_conn > 0) {
		// save connected file descriptor
		client.setFD(fd_conn);

		// save client ip address
		client.setIPAddress(inet_ntoa(client._addr.sin_addr));
		
		client.setProtocol(ntohs(_server._addr.sin_port));

		// Show who is connected
		if (showLogInfo) {
			SI_LOG_INFO("%s Connection from %s Port %d (fd: %d)", client.getProtocolString(), client.getIPAddress().c_str(),
			                  ntohs(client._addr.sin_port), fd_conn);
		}
		return true;
	}
	return false;
}
