/* StreamThreadRtp.cpp

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
#include <output/StreamThreadRtp.h>

#include <StreamClient.h>
#include <Log.h>
#include <StreamInterface.h>
#include <InterfaceAttr.h>
#include <base/TimeCounter.h>

#include <sys/socket.h>

namespace output {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

StreamThreadRtp::StreamThreadRtp(StreamInterface &stream) :
	StreamThreadBase("RTP/UDP", stream),
	_rtcp(stream) {}

StreamThreadRtp::~StreamThreadRtp() {
	terminateThread();
	const int streamID = _stream.getStreamID();
	StreamClient &client = _stream.getStreamClient(_clientID);
	SI_LOG_INFO("Stream: %d, Destroy %s stream to %s:%d", streamID, _protocol.c_str(),
		client.getIPAddressOfStream().c_str(), getStreamSocketPort(_clientID));
	client.getRtpSocketAttr().closeFD();
}

// =============================================================================
//  -- output::StreamThreadBase ------------------------------------------------
// =============================================================================

void StreamThreadRtp::doStartStreaming(const int clientID) {
	const int streamID = _stream.getStreamID();
	SocketAttr &rtp = _stream.getStreamClient(clientID).getRtpSocketAttr();

	// RTP
	if (!rtp.setupSocketHandle(SOCK_DGRAM, IPPROTO_UDP)) {
		SI_LOG_ERROR("Stream: %d, Get RTP handle failed", streamID);
	}

	// Get default buffer size and set it x times as big
	const int bufferSize = rtp.getNetworkSendBufferSize() * 20;
	rtp.setNetworkSendBufferSize(bufferSize);
	SI_LOG_INFO("Stream: %d, %s set network buffer size: %d KBytes", streamID,
		_protocol.c_str(), bufferSize / 1024);

	// RTCP
	_rtcp.startStreaming(clientID);
}

void StreamThreadRtp::doPauseStreaming(const int clientID) {
	// RTCP
	_rtcp.pauseStreaming(clientID);
}

void StreamThreadRtp::doRestartStreaming(const int clientID) {
	// RTCP
	_rtcp.restartStreaming(clientID);
}

int StreamThreadRtp::getStreamSocketPort(const int clientID) const {
	return  _stream.getStreamClient(clientID).getRtpSocketAttr().getSocketPort();
}

bool StreamThreadRtp::writeDataToOutputDevice(mpegts::PacketBuffer &buffer, StreamClient &client) {
	// update sequence number and timestamp
	const long timestamp = base::TimeCounter::getTicks() * 90;
	++_cseq;
	buffer.tagRTPHeaderWith(_cseq, timestamp);

	static constexpr size_t dataSize = mpegts::PacketBuffer::getBufferSize();
	static constexpr size_t len = dataSize + mpegts::PacketBuffer::RTP_HEADER_LEN;

	// RTP packet octet count (Bytes)
	_stream.addRtpData(dataSize, timestamp);

	// send the RTP/UDP packet
	const unsigned char *rtpBuffer = buffer.getReadBufferPtr();
	SocketAttr &rtp = client.getRtpSocketAttr();
	if (!rtp.sendDataTo(rtpBuffer, len, MSG_DONTWAIT)) {
		if (!client.isSelfDestructing()) {
			SI_LOG_ERROR("Stream: %d, Error sending RTP/UDP data to %s:%d", _stream.getStreamID(),
				rtp.getIPAddressOfSocket().c_str(), rtp.getSocketPort());
			client.selfDestruct();
		}
	}
	return true;
}

} // namespace output
