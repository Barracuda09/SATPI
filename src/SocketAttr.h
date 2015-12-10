/* SocketAttr.h

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
#ifndef SOCKET_ATTR_H_INCLUDE
#define SOCKET_ATTR_H_INCLUDE SOCKET_ATTR_H_INCLUDE

#include "Utils.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>

/// Socket attributes
class SocketAttr {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		SocketAttr() {}

		virtual ~SocketAttr() {}

		/// Get the file descriptor of this Socket
		int getFD() const {
			return _fd;
		}

		/// Get the pointer to file descriptor of this Socket
		const int *getFDPtr() const {
			return &_fd;
		}

		/// Set the file descriptor for this Socket
		/// @param fd specifies the file descriptor to set
		void setFD(int fd) {
			_fd = fd;
		}

		/// Close the file descriptor of this Socket
		virtual void closeFD() {
			CLOSE_FD(_fd);
			_ip_addr = "0.0.0.0";
		}

		/// Set the Receive and Send timeout in Sec for this socket
		void setSocketTimeoutInSec(int timeout) {
			struct timeval tv;
			tv.tv_sec = timeout;
			tv.tv_usec = 0;

			SI_LOG_DEBUG("Set Timeout (fd: %d)", _fd);

			if (setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval))) {
				PERROR("setsockopt: SO_RCVTIMEO");
			}
			if (setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval))) {
				PERROR("setsockopt: SO_SNDTIMEO");
			}
		}

		// =======================================================================
		// Data members
		// =======================================================================
		struct sockaddr_in _addr;
	protected:
		int _fd;
		std::string _ip_addr;
}; // class SocketAttr

#endif // SOCKET_ATTR_H_INCLUDE