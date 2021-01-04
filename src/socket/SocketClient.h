/* SocketClient.h

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
#include <StringConverter.h>

#include <string>

///
class SocketClient :
	public SocketAttr {
		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:

		SocketClient() :
			_msg(""),
			_protocolString("None") {}

		virtual ~SocketClient() {}

		// =====================================================================
		// -- socket::SocketAttr -----------------------------------------------
		// =====================================================================
	public:

		/// Close the file descriptor of this Socket
		virtual void closeFD() final {
			SocketAttr::closeFD();
			_msg.clear();
		}

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
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

		/// Get the Raw HTTP message from this client
		std::string getMessage() const {
			return _msg;
		}

		/// Get the Percent Decoded HTTP message from this client
		std::string getPercentDecodedMessage() const {
			return StringConverter::getPercentDecoding(_msg);
		}

		/// Set protocol string
		/// @param protocol specifies the protocol this client is using
		void setProtocol(const std::string &protocol) {
			_protocolString = protocol;
		}

		/// Get the protocol string
		std::string getProtocolString() const {
			return _protocolString;
		}

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		std::string _msg;
		std::string _protocolString;
};

#endif // SOCKET_SOCKETCLIENT_H_INCLUDE
