/* SocketClient.h

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
#ifndef SOCKET_CLIENT_H_INCLUDE
#define SOCKET_CLIENT_H_INCLUDE

#include "SocketAttr.h"

#include <stdio.h>

#define HTTP_PORT 8875
#define RTSP_PORT 554
#define SSDP_PORT 1900

///
class SocketClient :
		public SocketAttr {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		SocketClient() {;}
		virtual ~SocketClient() {;}

		int getFD() const { return _fd; }
		
		void closeFD()    { SocketAttr::closeFD(); }

		///
		void clearMessage() { _msg.clear(); }

		///
		void addMessage(const std::string &msg) { _msg += msg; }

		///
		const std::string &getMessage() const { return _msg; }

		///
		void setIPAddress(const std::string addr) { _ip_addr = addr; }
		const std::string &getIPAddress() const { return _ip_addr; }

		///
		void setSessionID(const std::string sessionID) { _sessionID = sessionID; }
		const std::string &getSessionID() const { return _sessionID; }
		
		///
		void setProtocol(int protocol) {
			switch (protocol) {
				case RTSP_PORT:
					_protocolString = "RTSP";
					break;
				case HTTP_PORT:
					_protocolString = "HTTP";
					break;
				case SSDP_PORT:
					_protocolString = "SSDP";
					break;
				default:
					_protocolString = "Unkown Proto";
					break;
			}
		}

		///
		const char *getProtocolString() const { return _protocolString.c_str();	}
	protected:
	private:
		// =======================================================================
		// Data members
		// =======================================================================
		std::string _msg;
		std::string _protocolString;
		std::string _sessionID;

}; // class SocketClient

#endif // SOCKET_CLIENT_H_INCLUDE