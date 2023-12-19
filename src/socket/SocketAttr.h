/* SocketAttr.h

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
#ifndef SOCKET_SOCKETATTR_H_INCLUDE
#define SOCKET_SOCKETATTR_H_INCLUDE SOCKET_SOCKETATTR_H_INCLUDE

#include <FwDecl.h>
#include <base/Mutex.h>

#include <string>
#include <string_view>

#include <netinet/in.h>

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

		/// Close the file descriptor of this Socket
		virtual void closeFD();

		/// Setup Socket address struct with IP and Port
		/// @param ipAddr
		/// @param port
		/// @param ttl
		void setupSocketStructure(std::string_view ipAddr, int port, int ttl);

		///
		void setupSocketStructureWithAnyAddress(int port, int ttl);

		/// Setup Socket type with protocol and open File Descriptor
		/// @param type
		/// @param protocol
		bool setupSocketHandle(int type, int protocol);

		/// Set the IP address of this client
		/// @param addr specifies the IP address of this client
		void setIPAddressOfSocket(const std::string& ipAddr) {
			_ipAddr = ipAddr;
		}

		/// Get the IP address of this client
		const std::string &getIPAddressOfSocket() const {
			return _ipAddr;
		}

		///
		bool writeData(const struct iovec* iov, int iovcnt);

		/// Use this function when the socket is in connected state
		bool sendData(const void* buf, std::size_t len, int flags);

		/// Use this function when the socket is on a
		/// connection-mode (SOCK_STREAM)
		bool sendDataTo(const void* buf, std::size_t len, int flags);

		/// Get the port of this Socket
		int getSocketPort() const;

		/// Get the network send buffer size for this Socket
		int getNetworkSendBufferSize() const;

		/// Set the network send buffer size for this Socket
		bool setNetworkSendBufferSize(int size);

		/// Set the network receive buffer size for this Socket
		bool setNetworkReceiveBufferSize(int size);

		/// Set the Receive and Send timeout in Sec for this socket
		void setSocketTimeoutInSec(unsigned int timeout);

		/// Get the Time(hops) that a packet exists inside network
		int getTimeToLive() const {
			return _ttl;
		}

		/// Get the file descriptor of this Socket
		int getFD() const;

		///
		ssize_t recvDatafrom(void* buf, std::size_t len, int flags);

		/// bind the socket to the port number
		bool bind();

		/// Set listen for maxClients on this Socket
		/// @param backlog defines the maximum length to which the queue
		/// of pending connections for this server may grow
		bool listen(std::size_t backlog);

		/// Set connect on this Socket
		bool connectTo();

		/// Accept an connection on this Socket and save client IP address etc.
		/// in client
		bool acceptConnection(SocketClient& client, bool showLogInfo);

	protected:

		/// Set the file descriptor for this Socket
		/// @param fd specifies the file descriptor to set
		void setFD(int fd);

		///
		void setKeepAlive();

		// ===================================================================
		//  -- Data members --------------------------------------------------
		// ===================================================================

	protected:

		base::Mutex _mutex;
		int _fd;
		struct sockaddr_in _addr;
		std::string _ipAddr;
		int _ttl;

};

#endif // SOCKET_SOCKETATTR_H_INCLUDE
