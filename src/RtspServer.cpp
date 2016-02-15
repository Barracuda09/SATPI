/* RtspServer.cpp

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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

#include <stdio.h>
#include <stdlib.h>

static const char *RTSP_OPTIONS_SESSION = "Session: %s\r\n";

RtspServer::RtspServer(StreamManager &streamManager, const Properties &properties, const InterfaceAttr &interface) :
		ThreadBase("RtspServer"),
		HttpcServer(5, "RTSP", properties.getRtspPort(), true, streamManager, interface) {
	startThread();
}

RtspServer::~RtspServer() {
	cancelThread();
	joinThread();
}

void RtspServer::threadEntry() {
	SI_LOG_INFO("Setting up RTSP server");

	for (;;) {
		// call poll with a timeout of 500 ms
		poll(500);

		_streamManager.checkStreamClientsWithTimeout();
	}
}

bool RtspServer::methodSetup(Stream &stream, int clientID, std::string &htmlBody) {
	static const char *RTSP_SETUP_OK =
		"RTSP/1.0 200 OK\r\n" \
		"CSeq: %d\r\n" \
		"Session: %s;timeout=%d\r\n" \
		"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n" \
		"com.ses.streamID: %d\r\n" \
		"\r\n";

// @TODO  check return of update();
	stream.update(clientID);

	// setup reply
	StringConverter::addFormattedString(htmlBody, RTSP_SETUP_OK, stream.getCSeq(clientID), stream.getSessionID(clientID).c_str(),
	                                    stream.getSessionTimeout(clientID), stream.getRtpSocketPort(clientID),
	                                    stream.getRtcpSocketPort(clientID), stream.getStreamID());

	return true;
}

bool RtspServer::methodPlay(Stream &stream, int clientID, std::string &htmlBody) {
	static const char *RTSP_PLAY_OK =
		"RTSP/1.0 200 OK\r\n" \
		"RTP-Info: url=rtsp://%s/stream=%d\r\n" \
		"CSeq: %d\r\n" \
		"Session: %s\r\n" \
		"\r\n";

// @TODO  check return of update();
	stream.update(clientID);

	StringConverter::addFormattedString(htmlBody, RTSP_PLAY_OK, _interface.getIPAddress().c_str(), stream.getStreamID(),
	                   stream.getCSeq(clientID), stream.getSessionID(clientID).c_str());

	return true;
}

bool RtspServer::methodOptions(Stream &stream, int clientID, std::string &htmlBody) {
	static const char *RTSP_OPTIONS_OK =
		"RTSP/1.0 200 OK\r\n" \
		"CSeq: %d\r\n" \
		"Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n" \
		"%s" \
		"\r\n";

	std::string rtsp;

	// check if we are in session, then we need to send the Session ID
	char sessionID[50] = "";
	if (stream.getSessionID(clientID).size() > 2) {
		snprintf(sessionID, sizeof(sessionID), RTSP_OPTIONS_SESSION, stream.getSessionID(clientID).c_str());
	}

	StringConverter::addFormattedString(htmlBody, RTSP_OPTIONS_OK, stream.getCSeq(clientID), sessionID);

	return true;
}

bool RtspServer::methodDescribe(Stream &stream, int clientID, std::string &htmlBody) {
	static const char *RTSP_200_OK    = "RTSP/1.0 200 OK\r\n";
	static const char *RTSP_404_ERROR = "RTSP/1.0 404 Not Found\r\n";
	static const char *RTSP_DESCRIBE  =
		"%s" \
		"CSeq: %d\r\n" \
		"Content-Type: application/sdp\r\n" \
		"Content-Base: rtsp://%s/\r\n" \
		"Content-Length: %zu\r\n" \
		"%s" \
		"\r\n" \
		"%s";

	static const char *RTSP_DESCRIBE_CONT1 =
		"v=0\r\n" \
		"o=- 2 3 IN IP4 %s\r\n" \
		"s=SatIPServer:1 %zu,%zu,%zu\r\n" \
		"t=0 0\r\n";

	static const char *RTSP_DESCRIBE_CONT2 =
		"m=video 0 RTP/AVP 33\r\n" \
		"c=IN IP4 0.0.0.0\r\n" \
		"a=control:stream=%zu\r\n" \
		"a=fmtp:33 %s\r\n" \
		"a=%s\r\n";

	std::string desc;
	unsigned int streams_setup = 0;

	// Describe streams
	std::string cont1;
	StringConverter::addFormattedString(desc, RTSP_DESCRIBE_CONT1, _interface.getIPAddress().c_str(),
	         _streamManager.getMaxDvbSat(), _streamManager.getMaxDvbTer(), _streamManager.getMaxDvbCable());

	for (auto i = 0u; i < _streamManager.getMaxStreams(); ++i) {
		bool active = false;
		std::string desc_attr = _streamManager.attributeDescribeString(i, active);
		if (desc_attr.size() > 5) {
			++streams_setup;
			StringConverter::addFormattedString(desc, RTSP_DESCRIBE_CONT2, i, desc_attr.c_str(), (active) ? "sendonly" : "inactive");
		}
	}

	// check if we are in session, then we need to send the Session ID
	char sessionID[50] = "";
	if (stream.getSessionID(clientID).size() > 2) {
		snprintf(sessionID, sizeof(sessionID), RTSP_OPTIONS_SESSION, stream.getSessionID(clientID).c_str());
	}

	// Are there any streams setup already
	if (streams_setup != 0) {
		StringConverter::addFormattedString(htmlBody, RTSP_DESCRIBE, RTSP_200_OK, stream.getCSeq(clientID),
		                                    _interface.getIPAddress().c_str(), desc.size(), sessionID, desc.c_str());
	} else {
		StringConverter::addFormattedString(htmlBody, RTSP_DESCRIBE, RTSP_404_ERROR, stream.getCSeq(clientID),
		                                    _interface.getIPAddress().c_str(), 0, sessionID, "");
	}
	return true;
}

bool RtspServer::methodTeardown(Stream &stream, int clientID, std::string &htmlBody) {
	static const char *RTSP_TEARDOWN_OK =
		"RTSP/1.0 200 OK\r\n" \
		"CSeq: %d\r\n" \
		"Session: %s\r\n" \
		"\r\n";

	// after setting up reply, else sessionID is reset
	stream.teardown(clientID, true);

	StringConverter::addFormattedString(htmlBody, RTSP_TEARDOWN_OK, stream.getCSeq(clientID), stream.getSessionID(clientID).c_str());

	return true;
}
