/* StreamClientOutputHttp.cpp

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
#include <output/StreamClientOutputHttp.h>

namespace output {

// =============================================================================
// -- StreamClient -------------------------------------------------------------
// =============================================================================

bool StreamClientOutputHttp::doProcessStreamingRequest(const SocketClient& client) {
	_sessionTimeoutCheck = StreamClient::SessionTimeoutCheck::FILE_DESCRIPTOR;
	_ipAddressOfStream = client.getIPAddressOfSocket();
	return true;
}

void StreamClientOutputHttp::doStartStreaming() {
	// Get default buffer size and set it x times as big
	const int bufferSize = getHttpNetworkSendBufferSize() * 2;
	setHttpNetworkSendBufferSize(bufferSize);
	SI_LOG_INFO("Frontend: @#1, HTTP set network buffer size: @#2 KBytes", _feID,
		bufferSize / 1024);
	SI_LOG_INFO("Frontend: @#1, Start HTTP stream to @#2:@#3", _feID,
		_ipAddressOfStream, getHttpSocketPort());
}

void StreamClientOutputHttp::doTeardown() {
	SI_LOG_INFO("Frontend: @#1, Stop HTTP stream to @#2:@#3", _feID,
		_ipAddressOfStream, getHttpSocketPort());
}

bool StreamClientOutputHttp::doWriteData(mpegts::PacketBuffer& buffer) {
	const size_t dataSize = buffer.getCurrentBufferSize();
	iovec iovHTTP[1];
	iovHTTP[0].iov_base = buffer.getTSReadBufferPtr();
	iovHTTP[0].iov_len = dataSize;
	// send the HTTP packet
	if (!writeHttpData(iovHTTP, 1)) {
		if (!isSelfDestructing()) {
			SI_LOG_ERROR("Frontend: @#1, Error sending HTTP Stream Data to @#2:@#3", _feID,
				_ipAddressOfStream, getHttpSocketPort());
			selfDestruct();
		}
	}
	return true;
}

}
