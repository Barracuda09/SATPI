/* StreamThreadHttp.cpp

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
#include <output/StreamThreadHttp.h>

#include <InterfaceAttr.h>
#include <Log.h>
#include <StreamInterface.h>
#include <StreamClient.h>
#include <base/TimeCounter.h>

#include <chrono>
#include <thread>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

namespace output {

// =========================================================================
// -- Constructors and destructor ------------------------------------------
// =========================================================================

StreamThreadHttp::StreamThreadHttp(
	StreamInterface &stream) :
	StreamThreadBase("HTTP", stream) {}

StreamThreadHttp::~StreamThreadHttp() {
	terminateThread();
	const FeID id = _stream.getFeID();
	StreamClient &client = _stream.getStreamClient(_clientID);
	SI_LOG_INFO("Frontend: %d, Destroy %s stream to %s:%d", id, _protocol.c_str(),
		client.getIPAddressOfStream().c_str(), getStreamSocketPort(_clientID));
}

// =========================================================================
//  -- output::StreamThreadBase --------------------------------------------
// =========================================================================

void StreamThreadHttp::doStartStreaming(const int clientID) {
	const FeID id = _stream.getFeID();
	StreamClient &client = _stream.getStreamClient(clientID);

	// Get default buffer size and set it x times as big
	const int bufferSize = client.getHttpNetworkSendBufferSize() * 20;
	client.setHttpNetworkSendBufferSize(bufferSize);
	SI_LOG_INFO("Frontend: %d, %s set network buffer size: %d KBytes", id,
		_protocol.c_str(), bufferSize / 1024);

//		client.setSocketTimeoutInSec(2);
}

int StreamThreadHttp::getStreamSocketPort(const int clientID) const {
	return _stream.getStreamClient(clientID).getHttpSocketPort();
}

bool StreamThreadHttp::writeDataToOutputDevice(mpegts::PacketBuffer &buffer, StreamClient &client) {
	const long timestamp = base::TimeCounter::getTicks() * 90;
	const size_t dataSize = buffer.getBufferSize();

	// RTP packet octet count (Bytes)
	_stream.addRtpData(dataSize, timestamp);

	iovec iov[1];
	iov[0].iov_base = buffer.getTSReadBufferPtr();
	iov[0].iov_len = dataSize;

	// send the HTTP packet
	if (!client.writeHttpData(iov, 1)) {
		if (!client.isSelfDestructing()) {
			SI_LOG_ERROR("Frontend: %d, Error sending HTTP Stream Data to %s", _stream.getFeID(),
				client.getIPAddressOfStream().c_str());
			client.selfDestruct();
		}
	}
	return true;
}

} // namespace output
