/* UdpSocket.cpp

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
#include "UdpSocket.h"
#include "Log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

UdpSocket::UdpSocket() {;}

UdpSocket::~UdpSocket() {;}

bool UdpSocket::init_udp_socket(SocketClient &server, int port, in_addr_t s_addr) {
	// fill in the socket structure with host information
	memset(&server._addr, 0, sizeof(server._addr));
	server._addr.sin_family      = AF_INET;
	server._addr.sin_addr.s_addr = s_addr;
	server._addr.sin_port        = htons(port);

	if ((server._fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		PERROR("socket send");
		return false;
	}
	int val = 1;
	if (setsockopt(server._fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		PERROR("Server setsockopt SO_REUSEADDR");
		return false;
	}
	return true;
}

bool UdpSocket::init_mutlicast_udp_socket(SocketClient &server, int port, const char *ip_addr) {
	// fill in the socket structure with host information
	memset(&server._addr, 0, sizeof(server._addr));
	server._addr.sin_family      = AF_INET;
	server._addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server._addr.sin_port        = htons(port);

	if ((server._fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		PERROR("socket listen");
		return false;
	}

	int val = 1;
	if (setsockopt(server._fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		PERROR("Server setsockopt SO_REUSEADDR");
		return false;
	}

	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
	mreq.imr_interface.s_addr = inet_addr(ip_addr);
	if (setsockopt(server._fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
		PERROR("IP_ADD_MEMBERSHIP");
		return false;
	}

	// bind the socket to the port number 
	if (bind(server._fd, (struct sockaddr *) &server._addr, sizeof(server._addr)) == -1) {
		PERROR("udp multi bind");
		return false;
	}
	return true;
}
