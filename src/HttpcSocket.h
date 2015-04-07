/* HttpcSocket.h

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
#ifndef HTTPC_SOCKET_H_INCLUDE
#define HTTPC_SOCKET_H_INCLUDE

#include "SocketAttr.h"

#include <poll.h>
#include <netinet/in.h>

// Forward declarations
class SocketClient;

/// HTTPC Socket related functions
class HttpcSocket  {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		HttpcSocket();
		virtual ~HttpcSocket();

	protected:
		///
		ssize_t recvHttpcMessage(SocketClient &client, int recv_flags);

		///
		ssize_t recvfromHttpcMessage(SocketClient &client, int recv_flags, struct sockaddr_in *si_other, socklen_t *addrlen);

	private:
		///
		ssize_t recv_recvfrom_httpc_message(SocketClient &client, int recv_flags, struct sockaddr_in *si_other, socklen_t *addrlen);

}; // class HttpcSocket

#endif // HTTPC_SOCKET_H_INCLUDE