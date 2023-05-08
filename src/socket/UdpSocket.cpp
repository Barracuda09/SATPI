/* UdpSocket.cpp

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
#include <socket/UdpSocket.h>

#include <socket/SocketClient.h>
#include <Log.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>


bool UdpSocket::initUDPSocket(
		SocketClient& server,
		const std::string_view ipAddr,
		int port,
		int ttl) {
	// fill in the socket structure with host information
	server.setupSocketStructure(ipAddr, port);

	if (!server.setupSocketHandle(SOCK_DGRAM, IPPROTO_UDP)) {
		SI_LOG_ERROR("UDP Server handle failed");
		return false;
	}
	server.setSocketMutlicastTTL(ttl);
	server.setSocketUnicastTTL(ttl);
	return true;
}

bool UdpSocket::initMutlicastUDPSocket(
		SocketClient &server,
		const std::string &multicastIPAddr,
		const std::string &interfaceIPaddr,
		int port) {
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

	// request that the kernel joins a multicast group
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(multicastIPAddr.c_str());
	mreq.imr_interface.s_addr = inet_addr(interfaceIPaddr.c_str());
	if (setsockopt(server.getFD(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
		SI_LOG_PERROR("IP_ADD_MEMBERSHIP");
		return false;
	}
	return true;
}
