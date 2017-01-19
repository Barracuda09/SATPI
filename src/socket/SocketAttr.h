/* SocketAttr.h

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef SOCKET_SOCKETATTR_H_INCLUDE
#define SOCKET_SOCKETATTR_H_INCLUDE SOCKET_SOCKETATTR_H_INCLUDE

#include <FwDecl.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <string>

FW_DECL_NS0(SocketClient);

	/// Socket attributes
	class SocketAttr {
		public:
			// ===================================================================
			//  -- Constructors and destructor -----------------------------------
			// ===================================================================
			SocketAttr();

			virtual ~SocketAttr();

			// ===================================================================
			//  -- Other member functions ----------------------------------------
			// ===================================================================

		public:

			///
			void setupSocketStructure(int port, const char *ip_addr);

			///
			void setupSocketStructureWithAnyAddress(int port);

			///
			bool setupSocketHandle(int type, int protocol);

			///
			std::string getIPAddress() const {
				return _ip_addr;
			}

			/// bind the socket to the port number
			bool bind();

			/// Set listen for maxClients on this Socket
			bool listen(std::size_t maxClients);

			/// Set connect on this Socket
			bool connectTo();

			/// Accept an connection on this Socket and save client IP address etc.
			/// in client
			bool acceptConnection(SocketClient &client, bool showLogInfo);

			/// Get the port of this Socket
			int getSocketPort() const;

			/// Use this function when the socket is in connected state
			bool sendData(const void *buf, std::size_t len, int flags);

			///
			bool writeData(const struct iovec *iov, int iovcnt);

			/// Use this function when the socket is on a
			/// connection-mode (SOCK_STREAM)
			bool sendDataTo(const void *buf, std::size_t len, int flags);

			///
			ssize_t recvDatafrom(void *buf, std::size_t len, int flags);

			/// Get the file descriptor of this Socket
			int getFD() const;

			/// Set the file descriptor for this Socket
			/// @param fd specifies the file descriptor to set
			void setFD(int fd);

			/// Close the file descriptor of this Socket
			virtual void closeFD();

			/// Set the Receive and Send timeout in Sec for this socket
			void setSocketTimeoutInSec(unsigned int timeout);

			///
			void setKeepAlive();

			/// Get the network send buffer size for this Socket
			int getNetworkSendBufferSize() const;

			/// Set the network send buffer size for this Socket
			bool setNetworkSendBufferSize(int size);

			/// Set the network receive buffer size for this Socket
			bool setNetworkReceiveBufferSize(int size);

			// ===================================================================
			//  -- Data members --------------------------------------------------
			// ===================================================================

		protected:

			int _fd;
			struct sockaddr_in _addr;
			std::string _ip_addr;

	};

#endif // SOCKET_SOCKETATTR_H_INCLUDE
