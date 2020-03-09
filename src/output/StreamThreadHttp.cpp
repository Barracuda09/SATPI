/* StreamThreadHttp.cpp

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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
#include <output/StreamThreadHttp.h>

#include <InterfaceAttr.h>
#include <Log.h>
#include <StreamInterface.h>
#include <StreamClient.h>
#include <base/TimeCounter.h>

#include <chrono>
#include <thread>

namespace output {

// =========================================================================
// -- Constructors and destructor ------------------------------------------
// =========================================================================

StreamThreadHttp::StreamThreadHttp(
	StreamInterface &stream) :
	StreamThreadBase("HTTP", stream) {}

StreamThreadHttp::~StreamThreadHttp() {
	terminateThread();
	const int streamID = _stream.getStreamID();
	StreamClient &client = _stream.getStreamClient(_clientID);
	SI_LOG_INFO("Stream: %d, Destroy %s stream to %s:%d", streamID, _protocol.c_str(),
		client.getIPAddressOfStream().c_str(), getStreamSocketPort(_clientID));
}

// =========================================================================
//  -- output::StreamThreadBase --------------------------------------------
// =========================================================================

void StreamThreadHttp::doStartStreaming(const int clientID) {
	const int streamID = _stream.getStreamID();
	StreamClient &client = _stream.getStreamClient(clientID);

	// Get default buffer size and set it x times as big
	const int bufferSize = client.getHttpNetworkSendBufferSize() * 20;
	client.setHttpNetworkSendBufferSize(bufferSize);
	SI_LOG_INFO("Stream: %d, %s set network buffer size: %d KBytes", streamID,
		_protocol.c_str(), bufferSize / 1024);

//		client.setSocketTimeoutInSec(2);
}

int StreamThreadHttp::getStreamSocketPort(const int clientID) const {
	return _stream.getStreamClient(clientID).getHttpSocketPort();
}

bool StreamThreadHttp::writeDataToOutputDevice(mpegts::PacketBuffer &buffer, StreamClient &client) {
	static constexpr unsigned int dataSize = buffer.getBufferSize();
	const long timestamp = base::TimeCounter::getTicks() * 90;

	// RTP packet octet count (Bytes)
	_stream.addRtpData(dataSize, timestamp);

	iovec iov[1];
	iov[0].iov_base = buffer.getTSReadBufferPtr();
	iov[0].iov_len = dataSize;

	// send the HTTP packet
	if (!client.writeHttpData(iov, 1)) {
		if (!client.isSelfDestructing()) {
			SI_LOG_ERROR("Stream: %d, Error sending HTTP Stream Data to %s", _stream.getStreamID(),
				client.getIPAddressOfStream().c_str());
			client.selfDestruct();
		}
	}
	return true;
}

} // namespace output
