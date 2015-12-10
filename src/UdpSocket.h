/* UdpSocket.h

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
#ifndef UDP_SOCKET_H_INCLUDE
#define UDP_SOCKET_H_INCLUDE UDP_SOCKET_H_INCLUDE

#include "HttpcSocket.h"
#include "SocketClient.h"
#include "SocketAttr.h"

#include <netinet/in.h>
#include <net/if.h>

// Forward declarations
class SocketAttr;

/// UDP Socket
class UdpSocket :
		public HttpcSocket {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		UdpSocket();
		virtual ~UdpSocket();

		/// Initialize an UDP socket
		/// @param server
		/// @param port
		/// @param s_addr
		bool init_udp_socket(SocketClient &server, int port, in_addr_t s_addr);

		/// Initialize an Multicast UDP socket
		/// @param server
		/// @param port
		/// @param ip_addr
		bool init_mutlicast_udp_socket(SocketClient &server, int port, const char *ip_addr);

	protected:

	private:

		// =======================================================================
		// Data members
		// =======================================================================

}; // class UdpSocket

#endif // UDP_SOCKET_H_INCLUDE