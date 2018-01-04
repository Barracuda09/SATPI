/* TcpSocket.h

   Copyright (C) 2014 - 2018 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef SOCKET_TCPSOCKET_H_INCLUDE
#define SOCKET_TCPSOCKET_H_INCLUDE SOCKET_TCPSOCKET_H_INCLUDE

#include <FwDecl.h>
#include <socket/HttpcSocket.h>
#include <socket/SocketAttr.h>

#include <vector>

#include <poll.h>

FW_DECL_NS0(SocketClient);

	/// TCP Socket
	class TcpSocket :
		public HttpcSocket {
		public:
			// ===================================================================
			// -- Constructors and destructor ------------------------------------
			// ===================================================================
			TcpSocket(int maxClients, const std::string &protocol);

			virtual ~TcpSocket();

			/// Call this to initialize and setup this socket(s)
			virtual void initialize(int port, bool nonblock);

			/// Call this function periodically to check for messages
			/// @param timeout specifies the timeout 'poll' should use
			int poll(int timeout);

		protected:
			/// Callback function if an messages was received
			/// @param client specifies the client that sended the message etc.
			virtual bool process(SocketClient &client) = 0;

			/// Callback when an connection is closed
			/// @param client specifies the client that closed the connection
			virtual bool closeConnection(SocketClient &client) = 0;

			/// Get the protocol string
			const std::string &getProtocolString() const {
				return _protocolString;
			}

		private:
			///
			bool initServerSocket(int maxClients, int port, bool nonblock);

			/// Accept an connection and save client IP address etc.
			bool acceptConnection(SocketClient &client, bool showLogInfo);

			// ===================================================================
			// -- Data members ---------------------------------------------------
			// ===================================================================

		private:
			typedef std::vector<pollfd> PollfdVector;
			typedef std::vector<SocketClient> SocketClientVector;
			PollfdVector       _pfdVector;
			SocketClientVector _socketClientVector;

			std::size_t        _MAX_CLIENTS;   //
			std::size_t        _MAX_POLL;      //
			struct pollfd     *_pfd;           //
			SocketAttr         _server;        //
			SocketClient      *_client;        //
			const std::string  _protocolString;//

	}; // class TcpSocket

#endif // SOCKET_TCPSOCKET_H_INCLUDE
