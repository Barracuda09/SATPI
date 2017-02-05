/* HttpcSocket.cpp

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
#include <socket/HttpcSocket.h>

#include <Log.h>
#include <socket/SocketClient.h>
#include <StringConverter.h>

#include <chrono>
#include <thread>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

const int HttpcSocket::HTTPC_TIMEOUT = 500;

	// =========================================================================
	//  -- Constructors and destructor -----------------------------------------
	// =========================================================================
	HttpcSocket::HttpcSocket() {}

	HttpcSocket::~HttpcSocket() {}

	// =========================================================================
	//  -- Other member functions ----------------------------------------------
	// =========================================================================
	
	ssize_t HttpcSocket::recvHttpcMessage(SocketClient &client, int recv_flags) {
		return recv_recvfrom_httpc_message(client, recv_flags, nullptr, nullptr);
	}

	ssize_t HttpcSocket::recvfromHttpcMessage(SocketClient &client, int recv_flags,
		struct sockaddr_in *si_other, socklen_t *addrlen) {
		return recv_recvfrom_httpc_message(client, recv_flags, si_other, addrlen);
	}

	ssize_t HttpcSocket::recv_recvfrom_httpc_message(SocketClient &client,
		int recv_flags, struct sockaddr_in *si_other, socklen_t *addrlen) {
		std::size_t read_len = 0;
		std::size_t timeout = HTTPC_TIMEOUT;
		bool done = false;
		bool end = false;
		std::size_t actualSize = 0;

		client.clearMessage();

		// read until we have '\r\n\r\n' (end of HTTP/RTSP message) then check
		// if the header field 'Content-Length' is present
		do {
			char buf[1024];
			const ssize_t size = ::recvfrom(client.getFD(), buf, sizeof(buf)-1, recv_flags, (struct sockaddr *)si_other, addrlen);
			if (size > 0) {
				read_len += size;
				// terminate
				buf[size] = 0;
				client.addMessage(buf);
				// reset timeout again
				timeout = HTTPC_TIMEOUT;
				// end of message "\r\n\r\n" ?
				if (!end) {
					const std::string::size_type headerSize = client.getMessage().find("\r\n\r\n");
					end = headerSize != std::string::npos;

					// check do we need to read more?
					std::string parameter;
					const std::size_t contentLength = StringConverter::getHeaderFieldParameter(client.getMessage(), "Content-Length:", parameter) ?
						atoi(parameter.c_str()) : 0;
					if (contentLength > 0) {
						// now check did we read it all
						actualSize = headerSize + contentLength + 4; // 4 = "\r\n\r\n"
						done = actualSize == client.getMessage().size();
					} else if (end) {
						done = true;
					}
				} else {
					// now check did we read it all this time around?
					done = actualSize == client.getMessage().size();
				}
			} else {
				if (timeout != 0 && size == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
					std::this_thread::sleep_for(std::chrono::microseconds(1000));
					--timeout;
				} else {
					return size;
				}
			}
		} while (!done);

		return read_len;
	}
