/* RtspServer.cpp

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
#include <RtspServer.h>

#include <Log.h>
#include <Stream.h>
#include <StreamManager.h>
#include <socket/SocketClient.h>
#include <StreamClient.h>
#include <StringConverter.h>

extern const char* const satpi_version;

RtspServer::RtspServer(StreamManager &streamManager, const std::string &bindIPAddress) :
		ThreadBase("RtspServer"),
		HttpcServer(20, "RTSP", streamManager, bindIPAddress) {}

RtspServer::~RtspServer() {
	cancelThread();
	joinThread();
}

void RtspServer::initialize(
		int port,
		bool nonblock) {
	HttpcServer::initialize(port, nonblock);
	startThread();
}

void RtspServer::threadEntry() {
	SI_LOG_INFO("Setting up RTSP server");

	for (;;) {
		// call poll with a timeout of 500 ms
		poll(500);

		_streamManager.checkForSessionTimeout();
	}
}

void RtspServer::methodSetup(const Stream &stream, const int clientID, std::string &htmlBody) {
	StreamClient &client = stream.getStreamClient(clientID);
	switch (stream.getStreamingType()) {
		case Stream::StreamingType::RTP_TCP: {
			static const char *RTSP_SETUP_OK =
				"RTSP/1.0 200 OK\r\n" \
				"Server: satpi/@#1\r\n" \
				"CSeq: @#2\r\n" \
				"Session: @#3;timeout=@#4\r\n" \
				"Transport: RTP/AVP/TCP;unicast;client_ip=@#5;interleaved=0-1\r\n" \
				"com.ses.streamID: @#6\r\n" \
				"\r\n";

			// setup reply
			htmlBody = StringConverter::stringFormat(RTSP_SETUP_OK,
				satpi_version,
				client.getCSeq(),
				client.getSessionID(),
				client.getSessionTimeout(),
				client.getIPAddressOfStream(),
				stream.getStreamID().getID());
			break;
		}
		case Stream::StreamingType::RTSP_UNICAST: {
			static const char *RTSP_SETUP_OK =
				"RTSP/1.0 200 OK\r\n" \
				"Server: satpi/@#1\r\n" \
				"CSeq: @#2\r\n" \
				"Session: @#3;timeout=@#4\r\n" \
				"Transport: RTP/AVP;unicast;client_ip=@#5;client_port=@#6-@#7\r\n" \
				"com.ses.streamID: @#8\r\n" \
				"\r\n";

			// setup reply
			htmlBody = StringConverter::stringFormat(RTSP_SETUP_OK,
				satpi_version,
				client.getCSeq(),
				client.getSessionID(),
				client.getSessionTimeout(),
				client.getIPAddressOfStream(),
				client.getRtpSocketAttr().getSocketPort(),
				client.getRtcpSocketAttr().getSocketPort(),
				stream.getStreamID().getID());
			break;
		}
		case Stream::StreamingType::RTSP_MULTICAST: {
			static const char *RTSP_SETUP_OK =
				"RTSP/1.0 200 OK\r\n" \
				"Server: satpi/@#1\r\n" \
				"CSeq: @#2\r\n" \
				"Session: @#3;timeout=@#4\r\n" \
				"Transport: RTP/AVP;multicast;destination=@#5;port=@#6-@#7;ttl=5\r\n" \
				"com.ses.streamID: @#8\r\n" \
				"\r\n";

			// setup reply
			htmlBody = StringConverter::stringFormat(RTSP_SETUP_OK,
				satpi_version,
				client.getCSeq(),
				client.getSessionID(),
				client.getSessionTimeout(),
				client.getIPAddressOfStream(),
				client.getRtpSocketAttr().getSocketPort(),
				client.getRtcpSocketAttr().getSocketPort(),
				stream.getStreamID().getID());
			break;
		}
		default:
			static const char *RTSP_SETUP_REPLY =
				"RTSP/1.0 461 Unsupported Transport\r\n" \
				"Server: satpi/@#1\r\n" \
				"CSeq: @#2\r\n" \
				"\r\n";

			// setup reply
			htmlBody = StringConverter::stringFormat(RTSP_SETUP_REPLY,
				satpi_version,
				client.getCSeq());
			return;
	};
}

void RtspServer::methodPlay(const Stream &stream, const int clientID, std::string &htmlBody) {
	const StreamClient &client = stream.getStreamClient(clientID);
	static const char *RTSP_PLAY_OK =
		"RTSP/1.0 200 OK\r\n" \
		"Server: satpi/@#1\r\n" \
		"RTP-Info: url=rtsp://@#2/stream=@#3\r\n" \
		"CSeq: @#4\r\n" \
		"Session: @#5\r\n" \
		"Range: npt=0.000-\r\n" \
		"\r\n";

	htmlBody = StringConverter::stringFormat(RTSP_PLAY_OK, satpi_version, _bindIPAddress,
		stream.getStreamID().getID(), client.getCSeq(), client.getSessionID());
}

void RtspServer::methodTeardown(
	const std::string &sessionID, const int cseq, std::string &htmlBody) {
	static const char *RTSP_TEARDOWN_OK =
		"RTSP/1.0 200 OK\r\n" \
		"Server: satpi/@#1\r\n" \
		"CSeq: @#2\r\n" \
		"Session: @#3\r\n" \
		"\r\n";

	htmlBody = StringConverter::stringFormat(RTSP_TEARDOWN_OK, satpi_version, cseq, sessionID);
}

void RtspServer::methodOptions(
	const std::string &sessionID, const int cseq, std::string &htmlBody) {
	static const char *RTSP_OPTIONS_OK =
		"RTSP/1.0 200 OK\r\n" \
		"Server: satpi/@#1\r\n" \
		"CSeq: @#2\r\n" \
		"Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n" \
		"@#3" \
		"\r\n";

	// check if we are in session, then we need to send the Session ID
	const std::string sessionIDHeaderField = (sessionID.size() > 2) ?
		StringConverter::stringFormat("Session: @#1\r\n", sessionID) : "";

	htmlBody = StringConverter::stringFormat(RTSP_OPTIONS_OK, satpi_version, cseq, sessionIDHeaderField);
}

void RtspServer::methodDescribe(
	const std::string &sessionID, const int cseq, const FeIndex feIndex, std::string &htmlBody) {
	static const char *RTSP_DESCRIBE  =
		"@#1" \
		"Server: satpi/@#2\r\n" \
		"CSeq: @#3\r\n" \
		"Content-Type: application/sdp\r\n" \
		"Content-Base: rtsp://@#4/\r\n" \
		"Content-Length: @#5\r\n" \
		"@#6" \
		"\r\n" \
		"@#7";

	static const char *RTSP_DESCRIBE_SESSION_LEVEL =
		"v=0\r\n" \
		"o=- @#1 @#2 IN IP4 @#3\r\n" \
		"s=SatIPServer:1 @#4\r\n" \
		"t=0 0\r\n";

	// Describe streams
	std::string desc = StringConverter::stringFormat(RTSP_DESCRIBE_SESSION_LEVEL,
		(sessionID.size() > 2) ? sessionID : "0",
		(sessionID.size() > 2) ? sessionID : "0",
		_bindIPAddress,
		_streamManager.getRTSPDescribeString());

	std::size_t streamsSetup = 0;

	// Lambda Expression 'setupDescribeMediaLevelString'
	const auto setupDescribeMediaLevelString = [&](const FeIndex feIndex) {
		const std::string mediaLevel = _streamManager.getDescribeMediaLevelString(feIndex);
		if (mediaLevel.size() > 5) {
			++streamsSetup;
			desc += mediaLevel;
		}
	};
	// Check if there is a specific Frontend ID index requested
	if (feIndex == -1) {
		for (std::size_t i = 0; i < _streamManager.getMaxStreams(); ++i) {
			setupDescribeMediaLevelString(i);
		}
	} else {
		setupDescribeMediaLevelString(feIndex);
	}

	// check if we are in session, then we need to send the Session ID
	const std::string sessionIDHeaderField = (sessionID.size() > 2) ?
		StringConverter::stringFormat("Session: @#1\r\n", sessionID) : "";

	// Are there any streams setup already
	if (streamsSetup != 0) {
		htmlBody = StringConverter::stringFormat(RTSP_DESCRIBE, "RTSP/1.0 200 OK\r\n", satpi_version,
			cseq, _bindIPAddress, desc.size(), sessionIDHeaderField, desc);
	} else {
		htmlBody = StringConverter::stringFormat(RTSP_DESCRIBE, "RTSP/1.0 404 Not Found\r\n", satpi_version,
			cseq, _bindIPAddress, 0, sessionIDHeaderField, "");
	}
}

