/* TcpSocket.h

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
#ifndef TCP_SOCKET_H_INCLUDE
#define TCP_SOCKET_H_INCLUDE

#include "HttpcSocket.h"
#include "SocketAttr.h"

#include <poll.h>
#include <netinet/in.h>

// Forward declarations
class SocketClient;

/// TCP Socket
class TcpSocket :
		public HttpcSocket {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		TcpSocket(int maxClients, int port, bool nonblock);
		virtual ~TcpSocket();

		int poll(int timeout);

	protected:
		///
		virtual bool process(SocketClient &client) = 0;

		///
		virtual bool closeConnection(SocketClient &client) = 0;

	private:
		///
		bool initServerSocket(int maxClients, int port, bool nonblock);

		/// Accept an connection save client IP address
		bool acceptConnection(SocketClient &client, bool showLogInfo);

		// =======================================================================
		// Data members
		// =======================================================================
		size_t _MAX_CLIENTS;
		size_t _MAX_POLL;
		struct pollfd *_pfd;
		SocketAttr _server;
		SocketClient *_client;

}; // class TcpSocket

#endif // TCP_SOCKET_H_INCLUDE