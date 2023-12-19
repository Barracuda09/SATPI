/* StreamClientOutputRtp.cpp

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
#include <output/StreamClientOutputRtp.h>

extern const char* const satpi_version;

namespace output {

// =============================================================================
// -- StreamClient -------------------------------------------------------------
// =============================================================================

std::string StreamClientOutputRtp::getSetupMethodReply(const StreamID streamID) {
	if (_multicast) {
		static const char* RTSP_SETUP_OK =
			"RTSP/1.0 200 OK\r\n" \
			"Server: satpi/@#1\r\n" \
			"CSeq: @#2\r\n" \
			"Session: @#3;timeout=@#4\r\n" \
			"Transport: RTP/AVP;multicast;destination=@#5;port=@#6-@#7;ttl=5\r\n" \
			"com.ses.streamID: @#8\r\n" \
			"\r\n";
		return StringConverter::stringFormat(RTSP_SETUP_OK,
			satpi_version,
			_commandSeq,
			_sessionID,
			_sessionTimeout,
			_ipAddressOfStream,
			_rtp.getSocketPort(),
			_rtcp.getSocketPort(),
			streamID.getID());
	} else {
			static const char* RTSP_SETUP_OK =
				"RTSP/1.0 200 OK\r\n" \
				"Server: satpi/@#1\r\n" \
				"CSeq: @#2\r\n" \
				"Session: @#3;timeout=@#4\r\n" \
				"Transport: RTP/AVP;unicast;client_ip=@#5;client_port=@#6-@#7\r\n" \
				"com.ses.streamID: @#8\r\n" \
				"\r\n";
			return StringConverter::stringFormat(RTSP_SETUP_OK,
				satpi_version,
				_commandSeq,
				_sessionID,
				_sessionTimeout,
				_ipAddressOfStream,
				_rtp.getSocketPort(),
				_rtcp.getSocketPort(),
				streamID.getID());
		}
}

std::string StreamClientOutputRtp::getSDPMediaLevelString(
		StreamID streamID,
		const std::string& fmtp) const {
	static const char* SDP_MEDIA_LEVEL =
		"m=video @#1 RTP/AVP 33\r\n" \
		"c=IN IP4 @#2\r\n" \
		"a=control:stream=@#3\r\n" \
		"a=fmtp:33 @#4\r\n" \
		"a=@#5\r\n";
	if (_multicast) {
		return StringConverter::stringFormat(SDP_MEDIA_LEVEL,
			_rtp.getSocketPort(),
			_ipAddressOfStream + "/" + std::to_string(_rtp.getTimeToLive()),
			streamID.getID(), fmtp,
			(_streamActive) ? "sendonly" : "inactive");
	} else {
		return StringConverter::stringFormat(SDP_MEDIA_LEVEL,
			0, "0.0.0.0", streamID.getID(), fmtp,
			(_streamActive) ? "sendonly" : "inactive");
	}
}

bool StreamClientOutputRtp::doProcessStreamingRequest(const SocketClient& client) {
	// Split message into Headers
	HeaderVector headers = client.getHeaders();

	std::string ports;
	int ttl = 0;
	if (_multicast) {
		ttl = headers.getIntFieldParameter("Transport", "ttl");
		ports = headers.getStringFieldParameter("Transport", "port");
		const std::string dest = headers.getStringFieldParameter("Transport", "destination");
		if (!dest.empty()) {
			_ipAddressOfStream = dest;
		}
		_sessionTimeoutCheck = StreamClient::SessionTimeoutCheck::TEARDOWN;
	} else {
		ports = headers.getStringFieldParameter("Transport", "client_port");
		_ipAddressOfStream = client.getIPAddressOfSocket();
		_sessionTimeoutCheck = StreamClient::SessionTimeoutCheck::WATCHDOG;
	}

	const StringVector port = StringConverter::split(ports, "-");
	if (port.size() == 2) {
		const int rtp  = std::isdigit(port[0][0]) ? std::stoi(port[0]) : 15000;
		const int rtcp = std::isdigit(port[1][0]) ? std::stoi(port[1]) : 15001;
		_rtp.setupSocketStructure(_ipAddressOfStream, rtp, ttl);
		_rtcp.setupSocketStructure(_ipAddressOfStream, rtcp, ttl);
	}
	return true;
}

void StreamClientOutputRtp::doStartStreaming() {
	// setup sockets for RTP and RTCP
	if (!_rtp.setupSocketHandle(SOCK_DGRAM, IPPROTO_UDP)) {
		SI_LOG_ERROR("Frontend: @#1, Get RTP/UDP handle failed", _feID);
	}
	if (!_rtcp.setupSocketHandle(SOCK_DGRAM, IPPROTO_UDP)) {
		SI_LOG_ERROR("Frontend: @#1, Get RTCP/UDP handle failed", _feID);
	}

	// Get default buffer size and set it x times as big
	const int bufferSize = _rtp.getNetworkSendBufferSize() * 2;
	_rtp.setNetworkSendBufferSize(bufferSize);
	SI_LOG_INFO("Frontend: @#1, RTP/UDP set network buffer size: @#2 KBytes", _feID,
		bufferSize / 1024);
	SI_LOG_INFO("Frontend: @#1, Start RTP/UDP stream to @#2:@#3", _feID,
		_rtp.getIPAddressOfSocket(), _rtp.getSocketPort());
	SI_LOG_INFO("Frontend: @#1, Start RTCP/UDP stream to @#2:@#3", _feID,
		_rtcp.getIPAddressOfSocket(), _rtcp.getSocketPort());
}

void StreamClientOutputRtp::doTeardown() {
	SI_LOG_INFO("Frontend: @#1, Stop RTP/UDP stream to @#2:@#3", _feID,
		_rtp.getIPAddressOfSocket(), _rtp.getSocketPort());
	SI_LOG_INFO("Frontend: @#1, Stop RTCP/UDP stream to @#2:@#3", _feID,
		_rtcp.getIPAddressOfSocket(), _rtcp.getSocketPort());
}

bool StreamClientOutputRtp::doWriteData(mpegts::PacketBuffer& buffer) {
	const size_t dataSize = buffer.getCurrentBufferSize();
	const size_t lenRTP = dataSize + mpegts::PacketBuffer::RTP_HEADER_LEN;
	const unsigned char* rtpBuffer = buffer.getReadBufferPtr();
	if (!_rtp.sendDataTo(rtpBuffer, lenRTP, MSG_DONTWAIT)) {
		if (!isSelfDestructing()) {
			SI_LOG_ERROR("Frontend: @#1, Error sending RTP/UDP data to @#2:@#3", _feID,
				_rtp.getIPAddressOfSocket(), _rtp.getSocketPort());
			selfDestruct();
		}
	}
	return true;
}

void StreamClientOutputRtp::doWriteRTCPData(
		const PacketPtr& sr, const int srlen,
		const PacketPtr& sdes, const int sdeslen,
		const PacketPtr& app, const int applen) {
	const int len = srlen + sdeslen + applen;
	PacketPtr data(new uint8_t[len]);
	std::memcpy(data.get(), sr.get(), srlen);
	std::memcpy(data.get() + srlen, sdes.get(), sdeslen);
	std::memcpy(data.get() + srlen + sdeslen, app.get(), applen);

	// send the RTCP/UDP packet
	if (!_rtcp.sendDataTo(data.get(), len, 0)) {
		SI_LOG_ERROR("Frontend: @#1, Error sending RTCP/UDP data to @#2:@#3", _feID,
			_ipAddressOfStream, _rtcp.getSocketPort());
	}
}

}
