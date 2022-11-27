/* StreamThreadRtpTcp.cpp

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
#include <output/StreamThreadRtpTcp.h>

#include <StreamClient.h>
#include <Log.h>
#include <StreamInterface.h>
#include <InterfaceAttr.h>
#include <base/TimeCounter.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

namespace output {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

StreamThreadRtpTcp::StreamThreadRtpTcp(StreamInterface &stream) :
	StreamThreadBase("RTP/TCP", stream),
	_rtcp(stream) {
}

StreamThreadRtpTcp::~StreamThreadRtpTcp() {
	terminateThread();
	const FeID id = _stream.getFeID();
	StreamClient &client = _stream.getStreamClient(_clientID);
	SI_LOG_INFO("Frontend: @#1, Destroy @#2 stream to @#3:@#4", id, _protocol,
		client.getIPAddressOfStream(), getStreamSocketPort(_clientID));
}

// =============================================================================
//  -- output::StreamThreadBase ------------------------------------------------
// =============================================================================

void StreamThreadRtpTcp::doStartStreaming(const int clientID) {
	const FeID id = _stream.getFeID();
	StreamClient &client = _stream.getStreamClient(clientID);

	// Get default buffer size and set it x times as big
	const int bufferSize = client.getHttpNetworkSendBufferSize() * 20;
	client.setHttpNetworkSendBufferSize(bufferSize);
	SI_LOG_INFO("Frontend: @#1, @#2 set network buffer size: @#3 KBytes",
		id, _protocol, bufferSize / 1024);

	// RTCP/TCP
	_rtcp.startStreaming(clientID);
}

void StreamThreadRtpTcp::doPauseStreaming(const int clientID) {
	// RTCP/TCP
	_rtcp.pauseStreaming(clientID);
}

void StreamThreadRtpTcp::doRestartStreaming(const int clientID) {
	// RTCP/TCP
	_rtcp.restartStreaming(clientID);
}

int StreamThreadRtpTcp::getStreamSocketPort(const int clientID) const {
	return  _stream.getStreamClient(clientID).getHttpSocketPort();
}

bool StreamThreadRtpTcp::writeDataToOutputDevice(mpegts::PacketBuffer &buffer, StreamClient &client) {
	// update sequence number and timestamp
	const long timestamp = base::TimeCounter::getTicks() * 90;
	++_cseq;
	buffer.tagRTPHeaderWith(_cseq, timestamp);

	const size_t dataSize = buffer.getCurrentBufferSize();
	const size_t len = dataSize + mpegts::PacketBuffer::RTP_HEADER_LEN;

	// RTP packet octet count (Bytes)
	_stream.addRtpData(dataSize, timestamp);

	unsigned char header[4];
	header[0] = 0x24;
	header[1] = 0x00;
	header[2] = (len >> 8) & 0xFF;
	header[3] = (len >> 0) & 0xFF;

	iovec iov[2];
	iov[0].iov_base = header;
	iov[0].iov_len = 4;
	iov[1].iov_base = buffer.getReadBufferPtr();
	iov[1].iov_len = len;

	// send the RTP/TCP packet
	if (!client.writeHttpData(iov, 2)) {
		if (!client.isSelfDestructing()) {
			SI_LOG_ERROR("Frontend: @#1, Error sending RTP/TCP Stream Data to @#2",
				_stream.getFeID(), client.getIPAddressOfStream());
			client.selfDestruct();
		}
	}
	return true;
}

} // namespace output
