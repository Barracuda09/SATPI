/* SocketClient.h

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
#ifndef SOCKET_SOCKETCLIENT_H_INCLUDE
#define SOCKET_SOCKETCLIENT_H_INCLUDE SOCKET_SOCKETCLIENT_H_INCLUDE

#include <HeaderVector.h>
#include <StringConverter.h>
#include <TransportParamVector.h>
#include <socket/SocketAttr.h>

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

		/// Clear the HTTP message
		void clearMessage() {
			_msg.clear();
		}

		/// Add HTTP message data
		/// @param msg specifies the message to set/add
		void addMessage(const std::string &msg) {
			_msg += msg;
		}

		/// Get the Headers of the HTTP message
		HeaderVector getHeaders() const {
			return HeaderVector(StringConverter::split(_msg, "\r\n"));
		}

		/// Get the Raw HTTP message data
		std::string getRawMessage() const {
			return _msg;
		}

		/// Spoof HTTP message with given data header
		/// @param header specifies the header to spoof/add to HTTP message
		void spoofHeaderWith(const std::string &header) const {
			const std::string::size_type n = _msg.find("\r\n\r\n");
			if (n != std::string::npos) {
				_msg.insert(n + 2, header);
			}
		}

		/// Get the Method used for this HTTP message
		std::string getMethod() const {
			// request line should be in the first line (method)
			HeaderVector headers = getHeaders();
			const std::string& line = headers[0];
			if (!headers[0].empty()) {
				std::string::const_iterator it = line.begin();
				// remove any leading whitespace
				while (*it == ' ') ++it;

				// copy method (upper case)
				std::string method;
				while (*it != ' ') {
					method += std::toupper(*it);
					++it;
				}
				return method;
			}
			return std::string();
		}

		/// Get the content from HTTP message
		std::string getContentFrom() const {
			HeaderVector headers = getHeaders();
			const std::string p = headers.getFieldParameter("Content-Length");
			const std::size_t contentLength = (!p.empty() && std::isdigit(p[0])) ? std::stoi(p) : 0;
			if (contentLength > 0) {
				const std::string::size_type headerSize = _msg.find("\r\n\r\n", 0);
				if (headerSize != std::string::npos) {
					return _msg.substr(headerSize + 4);
				}
			}
			return std::string();
		}

		/// Get the requested resource from HTTP message
		std::string getRequestedFile() const {
			HeaderVector headers = getHeaders();
			const std::string& param = headers[0];
			if (!param.empty()) {
				const std::string::size_type begin = param.find_first_of("/");
				const std::string::size_type end   = param.find_first_of(" ", begin);
				return param.substr(begin, end - begin);
			}
			return std::string();
		}

		/// Is the request the root-resource
		bool isRootFile() const {
			return _msg.find("/ ") != std::string::npos;
		}

		/// Does the request have any Transport Parameters
		bool hasTransportParameters() const {
			// Transport Parameters should be in the first line (method)
			std::string::size_type nextline = 0;
			const std::string line = StringConverter::getline(_msg, nextline, "\r\n");
			if (!line.empty()) {
				const std::string::size_type size = line.size() - 1;
				const std::string::size_type found = line.find_first_of("?");
				if (found != std::string::npos && found < size && line[found + 1] != ' ') {
					return true;
				}
			}
			return false;
		}

		/// Get the Transport Parameters
		TransportParamVector getTransportParameters() const {
			HeaderVector headers = getHeaders();
			return TransportParamVector(StringConverter::split(
				StringConverter::getPercentDecoding(headers[0]), " /?&"));
		}

		/// Get the Percent Decoded HTTP message from this client
		std::string getPercentDecodedMessage() const {
			return StringConverter::getPercentDecoding(_msg);
		}

		/// Get the protocol specified in this HTTP message
		std::string getProtocol() const {
			// Protocol should be in the first line
			std::string::size_type nextline = 0;
			const std::string line = StringConverter::getline(_msg, nextline, "\r\n");
			if (!line.empty()) {
				const std::string::size_type begin = line.find_last_of(" ") + 1;
				const std::string::size_type end   = line.find_last_of("/");
				if (begin != std::string::npos && end != std::string::npos) {
					return line.substr(begin, end - begin);
				}
			}
			return std::string();
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

		mutable std::string _msg;
		std::string _protocolString;
};

#endif // SOCKET_SOCKETCLIENT_H_INCLUDE
