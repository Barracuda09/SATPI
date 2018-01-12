/* RtspServer.cpp

   Copyright (C) 2014 - 2018 Marc Postema (mpostema09 -at- gmail.com)

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
#include <InterfaceAttr.h>
#include <Stream.h>
#include <StreamManager.h>
#include <socket/SocketClient.h>
#include <StreamClient.h>
#include <StringConverter.h>

RtspServer::RtspServer(StreamManager &streamManager, const InterfaceAttr &interface) :
		ThreadBase("RtspServer"),
		HttpcServer(20, "RTSP", streamManager, interface) {}

RtspServer::~RtspServer() {
	cancelThread();
	joinThread();
}

void RtspServer::initialize(int port, bool nonblock) {
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

bool RtspServer::methodSetup(Stream &stream, int clientID, std::string &htmlBody) {
	if (stream.getStreamingType() == Stream::StreamingType::RTP_TCP) {
		static const char *RTSP_SETUP_OK =
			"RTSP/1.0 200 OK\r\n" \
			"CSeq: %1\r\n" \
			"Session: %2;timeout=%3\r\n" \
			"Transport: RTP/AVP/TCP;unicast;client_ip=%4;interleaved=0-1\r\n" \
			"com.ses.streamID: %5\r\n" \
			"\r\n";

		// setup reply
		htmlBody = StringConverter::stringFormat(RTSP_SETUP_OK, stream.getCSeq(clientID), stream.getSessionID(clientID),
			stream.getSessionTimeout(clientID), stream.getIPAddress(clientID), stream.getStreamID());

	} else {
		static const char *RTSP_SETUP_OK =
			"RTSP/1.0 200 OK\r\n" \
			"CSeq: %1\r\n" \
			"Session: %2;timeout=%3\r\n" \
			"Transport: RTP/AVP;unicast;client_ip=%4;client_port=%5-%6\r\n" \
			"com.ses.streamID: %7\r\n" \
			"\r\n";

		// setup reply
		htmlBody = StringConverter::stringFormat(RTSP_SETUP_OK, stream.getCSeq(clientID), stream.getSessionID(clientID),
			stream.getSessionTimeout(clientID), stream.getIPAddress(clientID),
			stream.getRtpSocketPort(clientID), stream.getRtcpSocketPort(clientID), stream.getStreamID());
	}

// @TODO  check return of update();
	stream.update(clientID, true);

	return true;
}

bool RtspServer::methodPlay(Stream &stream, int clientID, std::string &htmlBody) {
	static const char *RTSP_PLAY_OK =
		"RTSP/1.0 200 OK\r\n" \
		"RTP-Info: url=rtsp://%1/stream=%2\r\n" \
		"CSeq: %3\r\n" \
		"Session: %4\r\n" \
		"Range: npt=0.000-\r\n" \
		"\r\n";

// @TODO  check return of update();
	stream.update(clientID, true);

	htmlBody = StringConverter::stringFormat(RTSP_PLAY_OK, _interface.getIPAddress(),
		stream.getStreamID(), stream.getCSeq(clientID), stream.getSessionID(clientID));

	return true;
}

bool RtspServer::methodOptions(Stream &stream, int clientID, std::string &htmlBody) {
	static const char *RTSP_OPTIONS_OK =
		"RTSP/1.0 200 OK\r\n" \
		"CSeq: %1\r\n" \
		"Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n" \
		"%2" \
		"\r\n";

	// check if we are in session, then we need to send the Session ID
	const std::string sessionID = (stream.getSessionID(clientID).size() > 2) ?
		StringConverter::stringFormat("Session: %1\r\n", stream.getSessionID(clientID)) : "";

	htmlBody = StringConverter::stringFormat(RTSP_OPTIONS_OK, stream.getCSeq(clientID), sessionID);

	return true;
}

bool RtspServer::methodDescribe(Stream &stream, int clientID, std::string &htmlBody) {
	static const char *RTSP_DESCRIBE  =
		"%1" \
		"CSeq: %2\r\n" \
		"Content-Type: application/sdp\r\n" \
		"Content-Base: rtsp://%3/\r\n" \
		"Content-Length: %4\r\n" \
		"%5" \
		"\r\n" \
		"%6";

	static const char *RTSP_DESCRIBE_CONT1 =
		"v=0\r\n" \
		"o=- %1 %2 IN IP4 %3\r\n" \
		"s=SatIPServer:1 %4\r\n" \
		"t=0 0\r\n";

	static const char *RTSP_DESCRIBE_CONT2 =
		"m=video 0 RTP/AVP 33\r\n" \
		"c=IN IP4 0.0.0.0\r\n" \
		"a=control:stream=%1\r\n" \
		"a=fmtp:33 %2\r\n" \
		"a=%3\r\n";

	std::size_t streams_setup = 0;

	// Describe streams
	std::string desc = StringConverter::stringFormat(RTSP_DESCRIBE_CONT1,
		(stream.getSessionID(clientID).size() > 2) ? stream.getSessionID(clientID) : "0",
		(stream.getSessionID(clientID).size() > 2) ? stream.getSessionID(clientID) : "0",
		_interface.getIPAddress(),
		_streamManager.getRTSPDescribeString());

	for (auto i = 0u; i < _streamManager.getMaxStreams(); ++i) {
		bool active = false;
		const std::string desc_attr = _streamManager.attributeDescribeString(i, active);
		if (desc_attr.size() > 5) {
			++streams_setup;
			desc += StringConverter::stringFormat(RTSP_DESCRIBE_CONT2, i, desc_attr, (active) ? "sendonly" : "inactive");
		}
	}

	// check if we are in session, then we need to send the Session ID
	const std::string sessionID = (stream.getSessionID(clientID).size() > 2) ?
		StringConverter::stringFormat("Session: %1\r\n", stream.getSessionID(clientID)) : "";

	// Are there any streams setup already
	if (streams_setup != 0) {
		htmlBody = StringConverter::stringFormat(RTSP_DESCRIBE, "RTSP/1.0 200 OK\r\n",
			stream.getCSeq(clientID), _interface.getIPAddress(), desc.size(), sessionID, desc);
	} else {
		htmlBody = StringConverter::stringFormat(RTSP_DESCRIBE, "RTSP/1.0 404 Not Found\r\n",
			stream.getCSeq(clientID), _interface.getIPAddress(), 0, sessionID, "");
	}
	return true;
}

bool RtspServer::methodTeardown(Stream &stream, int clientID, std::string &htmlBody) {
	static const char *RTSP_TEARDOWN_OK =
		"RTSP/1.0 200 OK\r\n" \
		"CSeq: %1\r\n" \
		"Session: %2\r\n" \
		"\r\n";

	htmlBody = StringConverter::stringFormat(RTSP_TEARDOWN_OK, stream.getCSeq(clientID), stream.getSessionID(clientID));

	// after setting up reply, else sessionID is reset
	stream.teardown(clientID, true);

	return true;
}
