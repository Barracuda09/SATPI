/* UdpSocket.cpp

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
#include <socket/UdpSocket.h>
#include <Log.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

UdpSocket::UdpSocket() {}

UdpSocket::~UdpSocket() {}

bool UdpSocket::init_udp_socket(SocketClient &server, int port, const char *ip_addr) {
	// fill in the socket structure with host information
	server.setupSocketStructure(port, ip_addr);

	if (!server.setupSocketHandle(SOCK_DGRAM, IPPROTO_UDP)) {
		SI_LOG_ERROR("UDP Server handle failed");
		return false;
	}
	return true;
}

bool UdpSocket::init_mutlicast_udp_socket(SocketClient &server,
		const char *multicastIPAddr,
		int port,
		const char *interfaceIPaddr) {
	// fill in the socket structure with host information
	server.setupSocketStructureWithAnyAddress(port);

	if (!server.setupSocketHandle(SOCK_DGRAM, IPPROTO_UDP)) {
		SI_LOG_ERROR("UDP Multicast Server handle failed");
		return false;
	}

	// bind the socket to the port number
	if (!server.bind()) {
		SI_LOG_ERROR("UDP Multicast Bind failed");
		return false;
	}

	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(multicastIPAddr);
	mreq.imr_interface.s_addr = inet_addr(interfaceIPaddr);
	if (setsockopt(server.getFD(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
		PERROR("IP_ADD_MEMBERSHIP");
		return false;
	}
	return true;
}
