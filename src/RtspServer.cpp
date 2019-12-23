/* RtspServer.cpp

   Copyright (C) 2014 - 2019 Marc Postema (mpostema09 -at- gmail.com)

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

void RtspServer::methodSetup(Stream &stream, int clientID, std::string &htmlBody) {
	StreamClient &client = stream.getStreamClient(clientID);
	switch (stream.getStreamingType()) {
		case Stream::StreamingType::RTP_TCP: {
			static const char *RTSP_SETUP_OK =
				"RTSP/1.0 200 OK\r\n" \
				"CSeq: %1\r\n" \
				"Session: %2;timeout=%3\r\n" \
				"Transport: RTP/AVP/TCP;unicast;client_ip=%4;interleaved=0-1\r\n" \
				"com.ses.streamID: %5\r\n" \
				"\r\n";

			// setup reply
			htmlBody = StringConverter::stringFormat(RTSP_SETUP_OK,
				client.getCSeq(),
				client.getSessionID(),
				client.getSessionTimeout(),
				client.getIPAddressOfStream(),
				stream.getStreamID());
			break;
		}
		case Stream::StreamingType::RTSP_UNICAST: {
			static const char *RTSP_SETUP_OK =
				"RTSP/1.0 200 OK\r\n" \
				"CSeq: %1\r\n" \
				"Session: %2;timeout=%3\r\n" \
				"Transport: RTP/AVP;unicast;client_ip=%4;client_port=%5-%6\r\n" \
				"com.ses.streamID: %7\r\n" \
				"\r\n";

			// setup reply
			htmlBody = StringConverter::stringFormat(RTSP_SETUP_OK,
				client.getCSeq(),
				client.getSessionID(),
				client.getSessionTimeout(),
				client.getIPAddressOfStream(),
				client.getRtpSocketAttr().getSocketPort(),
				client.getRtcpSocketAttr().getSocketPort(),
				stream.getStreamID());
			break;
		}
		case Stream::StreamingType::RTSP_MULTICAST: {
			static const char *RTSP_SETUP_OK =
				"RTSP/1.0 200 OK\r\n" \
				"CSeq: %1\r\n" \
				"Session: %2;timeout=%3\r\n" \
				"Transport: RTP/AVP;multicast;destination=%4;port=%5-%6;ttl=5\r\n" \
				"com.ses.streamID: %7\r\n" \
				"\r\n";

			// setup reply
			htmlBody = StringConverter::stringFormat(RTSP_SETUP_OK,
				client.getCSeq(),
				client.getSessionID(),
				client.getSessionTimeout(),
				client.getIPAddressOfStream(),
				client.getRtpSocketAttr().getSocketPort(),
				client.getRtcpSocketAttr().getSocketPort(),
				stream.getStreamID());
			break;
		}
		default:
			static const char *RTSP_SETUP_REPLY =
				"RTSP/1.0 461 Unsupported Transport\r\n" \
				"CSeq: %1\r\n" \
				"\r\n";

			// setup reply
			htmlBody = StringConverter::stringFormat(RTSP_SETUP_REPLY,
				client.getCSeq());
			return;
	};

// @TODO  check return of update();
	stream.update(clientID, true);
}

void RtspServer::methodPlay(
	const std::string &sessionID, const int cseq, const int streamID, std::string &htmlBody) {
	static const char *RTSP_PLAY_OK =
		"RTSP/1.0 200 OK\r\n" \
		"RTP-Info: url=rtsp://%1/stream=%2\r\n" \
		"CSeq: %3\r\n" \
		"Session: %4\r\n" \
		"Range: npt=0.000-\r\n" \
		"\r\n";

	htmlBody = StringConverter::stringFormat(RTSP_PLAY_OK, _bindIPAddress,
		streamID, cseq, sessionID);
}

void RtspServer::methodTeardown(
	const std::string &sessionID, const int cseq, std::string &htmlBody) {
	static const char *RTSP_TEARDOWN_OK =
		"RTSP/1.0 200 OK\r\n" \
		"CSeq: %1\r\n" \
		"Session: %2\r\n" \
		"\r\n";

	htmlBody = StringConverter::stringFormat(RTSP_TEARDOWN_OK, cseq, sessionID);
}

void RtspServer::methodOptions(
	const std::string &sessionID, const int cseq, std::string &htmlBody) {
	static const char *RTSP_OPTIONS_OK =
		"RTSP/1.0 200 OK\r\n" \
		"CSeq: %1\r\n" \
		"Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n" \
		"%2" \
		"\r\n";

	// check if we are in session, then we need to send the Session ID
	const std::string sessionIDHeaderField = (sessionID.size() > 2) ?
		StringConverter::stringFormat("Session: %1\r\n", sessionID) : "";

	htmlBody = StringConverter::stringFormat(RTSP_OPTIONS_OK, cseq, sessionIDHeaderField);
}

void RtspServer::methodDescribe(
	const std::string &sessionID, const int cseq, const int streamID, std::string &htmlBody) {
	static const char *RTSP_DESCRIBE  =
		"%1" \
		"CSeq: %2\r\n" \
		"Content-Type: application/sdp\r\n" \
		"Content-Base: rtsp://%3/\r\n" \
		"Content-Length: %4\r\n" \
		"%5" \
		"\r\n" \
		"%6";

	static const char *RTSP_DESCRIBE_SESSION_LEVEL =
		"v=0\r\n" \
		"o=- %1 %2 IN IP4 %3\r\n" \
		"s=SatIPServer:1 %4\r\n" \
		"t=0 0\r\n";

	// Describe streams
	std::string desc = StringConverter::stringFormat(RTSP_DESCRIBE_SESSION_LEVEL,
		(sessionID.size() > 2) ? sessionID : "0",
		(sessionID.size() > 2) ? sessionID : "0",
		_bindIPAddress,
		_streamManager.getRTSPDescribeString());

	std::size_t streamsSetup = 0;

	// Lambda Expression 'setupDescribeMediaLevelString'
	const auto setupDescribeMediaLevelString = [&](const int streamID) {
		const std::string mediaLevel = _streamManager.getDescribeMediaLevelString(streamID);
		if (mediaLevel.size() > 5) {
			++streamsSetup;
			desc += mediaLevel;
		}
	};
	// Check if there is a specific stream ID requested
	if (streamID == -1) {
		for (std::size_t i = 0; i < _streamManager.getMaxStreams(); ++i) {
			setupDescribeMediaLevelString(i);
		}
	} else {
		setupDescribeMediaLevelString(streamID);
	}

	// check if we are in session, then we need to send the Session ID
	const std::string sessionIDHeaderField = (sessionID.size() > 2) ?
		StringConverter::stringFormat("Session: %1\r\n", sessionID) : "";

	// Are there any streams setup already
	if (streamsSetup != 0) {
		htmlBody = StringConverter::stringFormat(RTSP_DESCRIBE, "RTSP/1.0 200 OK\r\n",
			cseq, _bindIPAddress, desc.size(), sessionIDHeaderField, desc);
	} else {
		htmlBody = StringConverter::stringFormat(RTSP_DESCRIBE, "RTSP/1.0 404 Not Found\r\n",
			cseq, _bindIPAddress, 0, sessionIDHeaderField, "");
	}
}

