/* SocketClient.h

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
#ifndef SOCKET_SOCKETCLIENT_H_INCLUDE
#define SOCKET_SOCKETCLIENT_H_INCLUDE SOCKET_SOCKETCLIENT_H_INCLUDE

#include <socket/SocketAttr.h>

#include <string>

///
class SocketClient :
	public SocketAttr {
	public:

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		SocketClient() :
			_msg(""),
			_protocolString("None"),
			_sessionID("-1"),
			_userAgent("None") {}

		virtual ~SocketClient() {}

		// =======================================================================
		//  -- Other member functions --------------------------------------------
		// =======================================================================

	public:

		/// Clear the HTTP message from this client
		void clearMessage() {
			_msg.clear();
		}

		/// Add HTTP message to this client
		/// @param msg specifiet the message to set/add
		void addMessage(const std::string &msg) {
			_msg += msg;
		}

		/// Get the HTTP message from this client
		const std::string &getMessage() const {
			return _msg;
		}

		/// Set the User-Agent of this client
		/// @param userAgent specifies the IP address of this client
		void setUserAgent(const std::string &userAgent) {
			_userAgent = userAgent;
		}

		/// Get the User-Agent of this client
		const std::string &getUserAgent() const {
			return _userAgent;
		}

		/// Set the IP address of this client
		/// @param addr specifies the IP address of this client
		void setIPAddress(const std::string &addr) {
			_ip_addr = addr;
		}

		/// Get the IP address of this client
		const std::string &getIPAddress() const {
			return _ip_addr;
		}

		/// Set the session ID for this client
		/// @param specifies the the session ID to use
		void setSessionID(const std::string &sessionID) {
			_sessionID = sessionID;
		}

		/// Get the session ID for this client
		const std::string &getSessionID() const {
			return _sessionID;
		}

		/// Set protocol string
		/// @param protocol specifies the protocol this client is using
		void setProtocol(const std::string &protocol) {
			_protocolString = protocol;
		}

		/// Get the protocol string
		const std::string &getProtocolString() const {
			return _protocolString;
		}

		/// Close the file descriptor of this Socket
		virtual void closeFD() override {
			SocketAttr::closeFD();
			_sessionID = "-1";
			_userAgent = "None";
			_msg.clear();
		}

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================

	private:

		std::string _msg;
		std::string _protocolString;
		std::string _sessionID;
		std::string _userAgent;

};

#endif // SOCKET_SOCKETCLIENT_H_INCLUDE
