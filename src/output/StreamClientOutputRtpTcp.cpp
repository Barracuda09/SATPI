/* StreamClientOutputRtpTcp.cpp

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
#include <output/StreamClientOutputRtpTcp.h>

extern const char* const satpi_version;

namespace output {

// =============================================================================
// -- StreamClient -------------------------------------------------------------
// =============================================================================

std::string StreamClientOutputRtpTcp::getSetupMethodReply(const StreamID streamID) {
	static const char* RTSP_SETUP_OK =
		"RTSP/1.0 200 OK\r\n" \
		"Server: satpi/@#1\r\n" \
		"CSeq: @#2\r\n" \
		"Session: @#3;timeout=@#4\r\n" \
		"Transport: RTP/AVP/TCP;unicast;client_ip=@#5;interleaved=0-1\r\n" \
		"com.ses.streamID: @#6\r\n" \
		"\r\n";
	return StringConverter::stringFormat(RTSP_SETUP_OK,
		satpi_version,
		_commandSeq,
		_sessionID,
		_sessionTimeout,
		_ipAddressOfStream,
		streamID.getID());
}

bool StreamClientOutputRtpTcp::doProcessStreamingRequest(const SocketClient& client) {
	// Split message into Headers
	HeaderVector headers = client.getHeaders();

	const int interleaved = headers.getIntFieldParameter("Transport", "interleaved");
	if (interleaved != -1) {
		_ipAddressOfStream = client.getIPAddressOfSocket();
	}
	_sessionTimeoutCheck = StreamClient::SessionTimeoutCheck::WATCHDOG;
	return true;
}

void StreamClientOutputRtpTcp::doStartStreaming() {
	// Get default buffer size and set it x times as big
	const int bufferSize = getHttpNetworkSendBufferSize() * 2;
	setHttpNetworkSendBufferSize(bufferSize);
	SI_LOG_INFO("Frontend: @#1, RTP/TCP set network buffer size: @#2 KBytes",
		_feID, bufferSize / 1024);

	SI_LOG_INFO("Frontend: @#1, Start RTP/TCP stream to @#2:@#3", _feID,
		_ipAddressOfStream, getHttpSocketPort());
	SI_LOG_INFO("Frontend: @#1, Start RTCP/TCP stream to @#2:@#3", _feID,
		_ipAddressOfStream, getHttpSocketPort());
}

void StreamClientOutputRtpTcp::doTeardown() {
	SI_LOG_INFO("Frontend: @#1, Stop RTP/TCP stream to @#2:@#3", _feID,
		_ipAddressOfStream, getHttpSocketPort());
	SI_LOG_INFO("Frontend: @#1, Stop RTCP/TCP stream to @#2:@#3", _feID,
		_ipAddressOfStream, getHttpSocketPort());
}

bool StreamClientOutputRtpTcp::doWriteData(mpegts::PacketBuffer& buffer) {
	const size_t dataSize = buffer.getCurrentBufferSize();
	const size_t lenRTP = dataSize + mpegts::PacketBuffer::RTP_HEADER_LEN;

	unsigned char header[4];
	header[0] = 0x24;
	header[1] = 0x00;
	header[2] = (lenRTP >> 8) & 0xFF;
	header[3] = (lenRTP >> 0) & 0xFF;

	iovec iov[2];
	iov[0].iov_base = header;
	iov[0].iov_len = 4;
	iov[1].iov_base = buffer.getReadBufferPtr();
	iov[1].iov_len = lenRTP;

	// send the RTP/TCP packet
	if (!writeHttpData(iov, 2)) {
		if (!isSelfDestructing()) {
			SI_LOG_ERROR("Frontend: @#1, Error sending RTP/TCP Stream Data to @#2:@#3", _feID,
				_ipAddressOfStream, getHttpSocketPort());
			selfDestruct();
		}
	}
	return true;
}

void StreamClientOutputRtpTcp::doWriteRTCPData(
		const PacketPtr& sr, const int srlen,
		const PacketPtr& sdes, const int sdeslen,
		const PacketPtr& app, const int applen) {
	const int len = srlen + sdeslen + applen;

	unsigned char header[4];
	header[0] = 0x24;
	header[1] = 0x01;
	header[2] = (len >> 8) & 0xFF;
	header[3] = (len >> 0) & 0xFF;

	iovec iov[4];
	iov[0].iov_base = header;
	iov[0].iov_len = 4;
	iov[1].iov_base = sr.get();
	iov[1].iov_len = srlen;
	iov[2].iov_base = sdes.get();
	iov[2].iov_len = sdeslen;
	iov[3].iov_base = app.get();
	iov[3].iov_len = applen;

	// send the RTCP/TCP packet
	if (!writeHttpData(iov, 4)) {
		SI_LOG_ERROR("Frontend: @#1, Error sending RTCP/TCP Stream Data to @#2:@#3", _feID,
			_ipAddressOfStream, getHttpSocketPort());
	}
}

}

