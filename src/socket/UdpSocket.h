/* UdpSocket.h

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
#ifndef SOCKET_UDPSOCKET_H_INCLUDE
#define SOCKET_UDPSOCKET_H_INCLUDE SOCKET_UDPSOCKET_H_INCLUDE

#include <FwDecl.h>
#include <socket/HttpcSocket.h>
#include <socket/SocketClient.h>
#include <socket/SocketAttr.h>

FW_DECL_NS0(SocketAttr);

/// UDP Socket
class UdpSocket :
	public HttpcSocket {
	public:

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		UdpSocket();

		virtual ~UdpSocket();

		// =======================================================================
		//  -- Other member functions --------------------------------------------
		// =======================================================================

	public:

		/// Initialize an UDP socket
		/// @param server
		/// @param port
		/// @param s_addr
		bool init_udp_socket(SocketClient &server, int port, const char *ip_addr);

		/// Initialize an Multicast UDP socket
		/// @param server
		/// @param multicastIPAddr
		/// @param port
		/// @param interfaceIPaddr
		bool init_mutlicast_udp_socket(
			SocketClient &server,
			const char *multicastIPAddr,
			int port,
			const char *interfaceIPaddr);

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================

	private:

};

#endif // SOCKET_UDPSOCKET_H_INCLUDE
