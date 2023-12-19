/* StreamClient.cpp

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
#include <output/StreamClient.h>

#include <base/TimeCounter.h>
#include <Log.h>
#include <socket/SocketClient.h>
#include <Stream.h>

#include <random>

extern const char* const satpi_version;

namespace output {

// =============================================================================
//  -- Constructors and destructor ---------------------------------------------
// =============================================================================

StreamClient::StreamClient(FeID feID) :
		_feID(feID),
		_streamActive(false),
		_socketClient(nullptr),
		_sessionTimeoutCheck(SessionTimeoutCheck::WATCHDOG),
		_ipAddressOfStream("0.0.0.0"),
		_watchdog(0),
		_sessionTimeout(60),
		_sessionID("-1"),
		_userAgent("None"),
		_commandSeq(0),
		_senderRtpPacketCnt(0),
		_senderOctectPayloadCnt(0),
		_payload(0.0) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::normal_distribution<> dist(0xffff, 0xffff);
	_ssrc = std::lround(dist(gen));
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void StreamClient::doAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "owner", _ipAddressOfStream);
	ADD_XML_ELEMENT(xml, "ownerSessionID", _sessionID);
	ADD_XML_ELEMENT(xml, "userAgent", _userAgent);
	ADD_XML_ELEMENT(xml, "rtpPort", _rtp.getSocketPort());
	ADD_XML_ELEMENT(xml, "rtcpPort", _rtcp.getSocketPort());
	ADD_XML_ELEMENT(xml, "httpPort", (_socketClient == nullptr) ? 0 : _socketClient->getSocketPort());
	ADD_XML_ELEMENT(xml, "spc", _senderRtpPacketCnt.load());
	ADD_XML_ELEMENT(xml, "clientPayload", _payload.load() / (1024.0 * 1024.0));
}

void StreamClient::doFromXML(const std::string &UNUSED(xml)) {}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

bool StreamClient::processStreamingRequest(const SocketClient &client) {
	// Split message into Headers
	HeaderVector headers = client.getHeaders();

	// Save clients seq number
	const std::string cseq = headers.getFieldParameter("CSeq");
	if (!cseq.empty()) {
		_commandSeq = std::stoi(cseq);
	}

	// Save clients User-Agent
	const std::string userAgent = headers.getFieldParameter("User-Agent");
	if (!userAgent.empty()) {
		_userAgent = userAgent;
	}
	doProcessStreamingRequest(client);

	// reset watchdog and give some extra timeout
	_watchdog = std::time(nullptr) + _sessionTimeout + 15;
	return true;
}

void StreamClient::startStreaming() {
	doStartStreaming();
	_streamActive = true;
}

bool StreamClient::writeData(mpegts::PacketBuffer& buffer) {
	const long timestamp = base::TimeCounter::getTicks() * 90;
	const size_t dataSize = buffer.getCurrentBufferSize();

	++_senderRtpPacketCnt;
	_senderOctectPayloadCnt += dataSize;
	_payload += dataSize;
	_timestamp = timestamp;
	buffer.tagRTPHeaderWith(_ssrc, _senderRtpPacketCnt, timestamp);

	return doWriteData(buffer);
}

void StreamClient::writeRTCPData(const std::string& attributeDescribeString) {
	const auto [sr, srlen]     = getSR();
	const auto [sdes, sdeslen] = getSDES();
	const auto [app, applen]   = getAPP(attributeDescribeString);

	doWriteRTCPData(sr, srlen, sdes, sdeslen, app, applen);
}

void StreamClient::teardown() {
	doTeardown();
	{
		base::MutexLock lock(_mutex);

		_streamActive = false;
		_watchdog = 0;
		_sessionID = "-1";
		_ipAddressOfStream = "0.0.0.0";
		_userAgent = "None";
		_sessionTimeoutCheck = SessionTimeoutCheck::WATCHDOG;

		// Do not delete
		_socketClient = nullptr;
	}
}

void StreamClient::selfDestruct() {
	base::MutexLock lock(_mutex);
	_watchdog = 1;
}

bool StreamClient::isSelfDestructing() const {
	base::MutexLock lock(_mutex);
	return _watchdog == 1;
}

bool StreamClient::sessionTimeout() const {
	base::MutexLock lock(_mutex);
	// See if we need to check watchdog or http FD
	switch (_sessionTimeoutCheck) {
		case SessionTimeoutCheck::WATCHDOG:
			return ((_watchdog != 0) && (_watchdog < std::time(nullptr)));
		case SessionTimeoutCheck::FILE_DESCRIPTOR:
			return (_socketClient == nullptr) ? false : (_socketClient->getFD() == -1);
		default:
			return false;
	};
}

void StreamClient::setSocketClient(SocketClient &socket) {
	base::MutexLock lock(_mutex);
	_socketClient = &socket;
}

std::string StreamClient::getSetupMethodReply(const StreamID UNUSED(streamID)) {
	static const char* RTSP_SETUP_REPLY =
		"RTSP/1.0 461 Unsupported Transport\r\n" \
		"Server: satpi/@#1\r\n" \
		"CSeq: @#2\r\n" \
		"\r\n";
	return StringConverter::stringFormat(RTSP_SETUP_REPLY,
		satpi_version,
		_commandSeq);
}

std::string StreamClient::getPlayMethodReply(
		StreamID streamID,
		const std::string& ipAddressOfServer) {
	static const char* RTSP_PLAY_OK =
		"RTSP/1.0 200 OK\r\n" \
		"Server: satpi/@#1\r\n" \
		"RTP-Info: url=rtsp://@#2/stream=@#3\r\n" \
		"CSeq: @#4\r\n" \
		"Session: @#5\r\n" \
		"Range: npt=0.000-\r\n" \
		"\r\n";
	return StringConverter::stringFormat(RTSP_PLAY_OK,
		satpi_version,
		ipAddressOfServer,
		streamID.getID(),
		_commandSeq,
		_sessionID);
}

std::string StreamClient::getOptionsMethodReply() const {
	static const char* RTSP_OPTIONS_OK =
		"RTSP/1.0 200 OK\r\n" \
		"Server: satpi/@#1\r\n" \
		"CSeq: @#2\r\n" \
		"Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n" \
		"Session: @#3\r\n" \
		"\r\n";
	return StringConverter::stringFormat(RTSP_OPTIONS_OK,
		satpi_version, _commandSeq, _sessionID);
}

std::string StreamClient::getTeardownMethodReply() const {
	static const char* RTSP_TEARDOWN_OK =
		"RTSP/1.0 200 OK\r\n" \
		"Server: satpi/@#1\r\n" \
		"CSeq: @#2\r\n" \
		"Session: @#3\r\n" \
		"\r\n";
	return StringConverter::stringFormat(RTSP_TEARDOWN_OK,
		satpi_version, _commandSeq, _sessionID);
}

// =============================================================================
//  -- RTCP member functions ---------------------------------------------------
// =============================================================================

std::pair<PacketPtr, int> StreamClient::getAPP(const std::string &desc) const {
	// total length and align on 32 bits
	int len = 16 + desc.size();
	if ((len % 4) != 0) {
		len += 4 - (len % 4);
	}
	// Application Defined packet  (APP Packet)
	// Alloc and copy data and adjust length
	PacketPtr app(new uint8_t[len]);
	app[0]  = 0x80;                // version: 2, padding: 0, subtype: 0
	app[1]  = 204;                 // payload type: 204 (0xcc) (APP)
	app[2]  = 0;                   // length (total in 32-bit words minus one)
	app[3]  = 0;                   // length (total in 32-bit words minus one)
	app[4]  = (_ssrc >> 24) & 0xff;// synchronization source
	app[5]  = (_ssrc >> 16) & 0xff;// synchronization source
	app[6]  = (_ssrc >>  8) & 0xff;// synchronization source
	app[7]  = (_ssrc >>  0) & 0xff;// synchronization source
	app[8]  = 'S';                 // name
	app[9]  = 'E';                 // name
	app[10] = 'S';                 // name
	app[11] = '1';                 // name
	app[12] = 0;                   // identifier (0000)
	app[13] = 0;                   // identifier
	app[14] = 0;                   // string length
	app[15] = 0;                   // string length
	                               // here the App defined data is added

	// Alloc and copy data and adjust length
	std::memcpy(app.get() + 16, desc.data(), desc.size());
	const int ws = (len / 4) - 1;
	app[2] = (ws >> 8) & 0xff;
	app[3] = (ws >> 0) & 0xff;
	const int ss = desc.size();
	app[14] = (ss >> 8) & 0xff;
	app[15] = (ss >> 0) & 0xff;
	return {std::move(app), len};
}

std::pair<PacketPtr, int> StreamClient::getSR() const {
	// Sender Report (SR Packet)
	int len = 28;
	PacketPtr sr(new uint8_t[len]);
	sr[0]  = 0x80;                         // version: 2, padding: 0, sr blocks: 0
	sr[1]  = 200;                          // payload type: 200 (0xc8) (SR)
	sr[2]  = 0;                            // length (total in 32-bit words minus one)
	sr[3]  = 0;                            // length (total in 32-bit words minus one)
	sr[4]  = (_ssrc >> 24) & 0xff;         // synchronization source
	sr[5]  = (_ssrc >> 16) & 0xff;         // synchronization source
	sr[6]  = (_ssrc >>  8) & 0xff;         // synchronization source
	sr[7]  = (_ssrc >>  0) & 0xff;         // synchronization source

	const std::time_t ntp = std::time(nullptr);
	                                       // NTP integer part
	sr[8]  = (ntp >> 24) & 0xff;           // NTP most sign word
	sr[9]  = (ntp >> 16) & 0xff;           // NTP most sign word
	sr[10] = (ntp >>  8) & 0xff;           // NTP most sign word
	sr[11] = (ntp >>  0) & 0xff;           // NTP most sign word
	                                       // NTP fractional part
	sr[12] = 0;                            // NTP least sign word
	sr[13] = 0;                            // NTP least sign word
	sr[14] = 0;                            // NTP least sign word
	sr[15] = 0;                            // NTP least sign word

	const long timestamp = _timestamp;
	sr[16] = (timestamp >> 24) & 0xff;     // RTP timestamp RTS
	sr[17] = (timestamp >> 16) & 0xff;     // RTP timestamp RTS
	sr[18] = (timestamp >>  8) & 0xff;     // RTP timestamp RTS
	sr[19] = (timestamp >>  0) & 0xff;     // RTP timestamp RTS
	const int32_t spc = _senderRtpPacketCnt;
	sr[20] = (spc >> 24) & 0xff;           // sender's packet count SPC
	sr[21] = (spc >> 16) & 0xff;           // sender's packet count SPC
	sr[22] = (spc >>  8) & 0xff;           // sender's packet count SPC
	sr[23] = (spc >>  0) & 0xff;           // sender's packet count SPC
	const uint32_t soc = _senderOctectPayloadCnt;
	sr[24] = (soc >> 24) & 0xff;           // sender's octet count SOC
	sr[25] = (soc >> 16) & 0xff;           // sender's octet count SOC
	sr[26] = (soc >>  8) & 0xff;           // sender's octet count SOC
	sr[27] = (soc >>  0) & 0xff;           // sender's octet count SOC

	// Alloc and copy data and adjust length
	const int ws = (len / 4) - 1;
	sr[2] = (ws >> 8) & 0xff;
	sr[3] = (ws >> 0) & 0xff;
	return {std::move(sr), len};
}

std::pair<PacketPtr, int> StreamClient::getSDES() const {
	// Source Description (SDES Packet)
	int len = 20;
	PacketPtr sdes(new uint8_t[len]);
	sdes[0]  = 0x81;                           // version: 2, padding: 0, sc blocks: 1
	sdes[1]  = 202;                            // payload type: 202 (0xca) (SDES)
	sdes[2]  = 0;                              // length (total in 32-bit words minus one)
	sdes[3]  = 3;                              // length (total in 32-bit words minus one)

	sdes[4]  = (_ssrc >> 24) & 0xff;           // synchronization source
	sdes[5]  = (_ssrc >> 16) & 0xff;           // synchronization source
	sdes[6]  = (_ssrc >>  8) & 0xff;           // synchronization source
	sdes[7]  = (_ssrc >>  0) & 0xff;           // synchronization source

	sdes[8]  = 1;                              // CNAME: 1
	sdes[9]  = 6;                              // length: 6
	sdes[10] = 'S';                            // data
	sdes[11] = 'a';                            // data

	sdes[12] = 't';                            // data
	sdes[13] = 'P';                            // data
	sdes[14] = 'I';                            // data
	sdes[15] = 0;                              // data

	sdes[16] = 0;                              // data
	sdes[17] = 0;                              // data
	sdes[18] = 0;                              // data
	sdes[19] = 0;                              // data

	const int ws = (len / 4) - 1;
	sdes[2] = (ws >> 8) & 0xff;
	sdes[3] = (ws >> 0) & 0xff;
	return {std::move(sdes), len};
}

// =============================================================================
//  -- HTTP member functions ---------------------------------------------------
// =============================================================================

bool StreamClient::sendHttpData(const void *buf, std::size_t len, int flags) {
//	base::MutexLock lock(_mutex);
	return (_socketClient == nullptr) ? false : _socketClient->sendData(buf, len, flags);
}

bool StreamClient::writeHttpData(const struct iovec *iov, int iovcnt) {
//	base::MutexLock lock(_mutex);
	return (_socketClient == nullptr) ? false : _socketClient->writeData(iov, iovcnt);
}

int StreamClient::getHttpSocketPort() const {
//	base::MutexLock lock(_mutex);
	return (_socketClient == nullptr) ? 0 : _socketClient->getSocketPort();
}

int StreamClient::getHttpNetworkSendBufferSize() const {
//	base::MutexLock lock(_mutex);
	return (_socketClient == nullptr) ? 0 : _socketClient->getNetworkSendBufferSize();
}

bool StreamClient::setHttpNetworkSendBufferSize(int size) {
//	base::MutexLock lock(_mutex);
	return (_socketClient == nullptr) ? false : _socketClient->setNetworkSendBufferSize(size);
}

}
