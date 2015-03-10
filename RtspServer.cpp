/* RtspServer.cpp

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#include "RtspServer.h"
#include "Log.h"
#include "SocketClient.h"
#include "StreamClient.h"
#include "StringConverter.h"
#include "Stream.h"
#include "Streams.h"

#include <stdio.h>
#include <stdlib.h>

#define RTSP_200_OK     "RTSP/1.0 200 OK\r\n"

#define RTSP_404_ERROR  "RTSP/1.0 404 Not Found\r\n"

#define RTSP_503_ERROR  "RTSP/1.0 503 Service Unavailable\r\n" \
                        "CSeq: %d\r\n" \
                        "Content-Type: text/parameters\r\n" \
                        "Content-Length: %d\r\n" \
                        "\r\n"

#define RTSP_500_ERROR  "RTSP/1.0 500 Internal Server Error\r\n" \
                        "CSeq: %d\r\n" \
                        "\r\n"

RtspServer::RtspServer(Streams &streams, const std::string &server_ip_addr) :
		ThreadBase("RtspServer"),
		TcpSocket(5, RTSP_PORT, true),
		_streams(streams),
		_server_ip_addr(server_ip_addr) {
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

		_streams.checkStreamClientsWithTimeout();
	}
}

bool RtspServer::process(SocketClient &socketclient) {
	std::string msg = socketclient.getMessage();
	SI_LOG_DEBUG("Data from client %s: %s", socketclient.getIPAddress().c_str(), msg.c_str());

	int clientID;
	Stream *stream = _streams.findStreamAndClientIDFor(socketclient, clientID);
	if (stream) {
		std::string method;
		if (StringConverter::getMethod(msg, method)) {
			stream->processStream(msg, clientID, method);
			processReply(*stream, clientID, method);
			return true;
		}
	} else {
		// something wrong here... send 503 error
		std::string cseq;
		StringConverter::getHeaderFieldParameter(msg, "CSeq:", cseq);
		std::string rtsp;
		StringConverter::addFormattedString(rtsp, RTSP_503_ERROR, atoi(cseq.c_str()), 0);
		// send reply to client
		if (send(socketclient.getFD(), rtsp.c_str(), rtsp.size(), MSG_NOSIGNAL) == -1) {
			PERROR("error sending: stream not found");
			return false;
		}
	}
	return false;
}

bool RtspServer::closeConnection(SocketClient &socketclient) {
	int clientID;
	Stream *stream = _streams.findStreamAndClientIDFor(socketclient, clientID);
	if (stream) {
		stream->close(clientID);
	}
	socketclient.setSessionID("-1");
	return true;
}

bool RtspServer::processReply(Stream &stream, int clientID, const std::string method) {
	// Check the Method
	if (method.compare("SETUP") == 0) {
		methodSetup(stream, clientID);
	} else if (method.compare("PLAY") == 0) {
		methodPlay(stream, clientID);
	} else if (method.compare("OPTIONS") == 0) {
		methodOptions(stream, clientID);
	} else if (method.compare("DESCRIBE") == 0) {
		methodDescribe(stream, clientID);
	} else if (method.compare("TEARDOWN") == 0) {
		methodTeardown(stream, clientID);
	}
	return true;
}

bool RtspServer::methodSetup(Stream &stream, int clientID) {
#define RTSP_SETUP_OK	"RTSP/1.0 200 OK\r\n" \
						"CSeq: %d\r\n" \
						"Session: %s;timeout=%d\r\n" \
						"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n" \
						"com.ses.streamID: %d\r\n" \
						"\r\n"
	std::string rtsp;

// @TODO  check return of updateFrontend();
	stream.updateFrontend();
	
	stream.startStreaming();

	// setup reply
	StringConverter::addFormattedString(rtsp, RTSP_SETUP_OK, stream.getCSeq(clientID), stream.getSessionID(clientID).c_str(),
	                                    stream.getSessionTimeout(clientID), stream.getRtpSocketPort(clientID),
	                                    stream.getRtcpSocketPort(clientID), stream.getStreamID());

	SI_LOG_DEBUG("%s", rtsp.c_str());

	// send reply to client
	if (send(stream.getRtspFD(clientID), rtsp.c_str(), rtsp.size(), MSG_NOSIGNAL) == -1) {
		PERROR("error sending: methodSetup");
		return false;
	}
	return true;
}

bool RtspServer::methodPlay(Stream &stream, int clientID) {
#define RTSP_PLAY_OK	"RTSP/1.0 200 OK\r\n" \
						"RTP-Info: url=rtsp://%s/stream=%d\r\n" \
						"CSeq: %d\r\n" \
						"Session: %s\r\n" \
						"\r\n"
	std::string rtsp;

// @TODO  check maybe return of updateFrontend();
	stream.updateFrontend();

	stream.startStreaming();

	StringConverter::addFormattedString(rtsp, RTSP_PLAY_OK, _server_ip_addr.c_str(), stream.getStreamID(),
	                   stream.getCSeq(clientID), stream.getSessionID(clientID).c_str());

	SI_LOG_DEBUG("%s", rtsp.c_str());

	// send reply to client
	if (send(stream.getRtspFD(clientID), rtsp.c_str(), rtsp.size(), MSG_NOSIGNAL) == -1) {
		PERROR("error sending: methodPlay");
		return false;
	}
	return true;
}

bool RtspServer::methodOptions(Stream &stream, int clientID) {
#define RTSP_OPTIONS_OK      "RTSP/1.0 200 OK\r\n" \
                             "CSeq: %d\r\n" \
                             "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n" \
                             "%s" \
                             "\r\n"

#define RTSP_OPTIONS_SESSION "Session: %s\r\n"
	std::string rtsp;

	// check if we are in session, then we need to send the Session ID
	char sessionID[50] = "";
	if (stream.getSessionID(clientID).size() > 2) {
		snprintf(sessionID, sizeof(sessionID), RTSP_OPTIONS_SESSION, stream.getSessionID(clientID).c_str());
	}

	StringConverter::addFormattedString(rtsp, RTSP_OPTIONS_OK, stream.getCSeq(clientID), sessionID);

	SI_LOG_DEBUG("%s", rtsp.c_str());

	// send reply to client
	if (send(stream.getRtspFD(clientID), rtsp.c_str(), rtsp.size(), MSG_NOSIGNAL) == -1) {
		PERROR("error sending: methodOptions");
		return false;
	}
	return true;
}

bool RtspServer::methodDescribe(Stream &stream, int clientID) {
#define RTSP_DESCRIBE  "%s" \
                       "CSeq: %d\r\n" \
                       "Content-Type: application/sdp\r\n" \
                       "Content-Base: rtsp://%s/\r\n" \
                       "Content-Length: %zu\r\n" \
                       "%s" \
                       "\r\n" \
                       "%s"

#define RTSP_DESCRIBE_IN_SESSION "Session: %s\r\n"

#define RTSP_DESCRIBE_CONT1 "v=0\r\n" \
                            "o=- 2 3 IN IP4 %s\r\n" \
                            "s=SatIPServer:1 %d,%d,%d\r\n" \
                            "t=0 0\r\n"

#define RTSP_DESCRIBE_CONT2 "m=video 0 RTP/AVP 33\r\n" \
                            "c=IN IP4 0.0.0.0\r\n" \
                            "a=control:stream=%zu\r\n" \
                            "a=fmtp:33 %s\r\n" \
                            "a=%s\r\n"
	std::string desc;
	std::string reply;
	unsigned int streams_setup = 0;

	// Describe streams
	std::string cont1;
	StringConverter::addFormattedString(desc, RTSP_DESCRIBE_CONT1, _server_ip_addr.c_str(),
	         _streams.getMaxDvbSat(), _streams.getMaxDvbTer(), _streams.getMaxDvbCable());

	for (size_t i = 0; i < static_cast<size_t>(_streams.getMaxStreams()); ++i) {
		bool active = false;
		std::string desc_attr = _streams.attribute_describe_string(i, active);
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
	if (streams_setup) {
		StringConverter::addFormattedString(reply, RTSP_DESCRIBE, RTSP_200_OK, stream.getCSeq(clientID),
		                                    _server_ip_addr.c_str(), desc.size(), sessionID, desc.c_str());
	} else {
		StringConverter::addFormattedString(reply, RTSP_DESCRIBE, RTSP_404_ERROR, stream.getCSeq(clientID),
		                                    _server_ip_addr.c_str(), 0, sessionID, "");
	}

	SI_LOG_DEBUG("%s", reply.c_str());

	// send reply to client
	if (send(stream.getRtspFD(clientID), reply.c_str(), reply.size(), MSG_NOSIGNAL) == -1) {
		PERROR("error sending: methodDescribe");
		return false;
	}
	return true;
}

bool RtspServer::methodTeardown(Stream &stream, int clientID) {
#define RTSP_TEARDOWN_OK "RTSP/1.0 200 OK\r\n" \
                         "CSeq: %d\r\n" \
                         "Session: %s\r\n" \
                         "\r\n"
	std::string rtsp;
	int fd = stream.getRtspFD(clientID);

	StringConverter::addFormattedString(rtsp, RTSP_TEARDOWN_OK, stream.getCSeq(clientID), stream.getSessionID(clientID).c_str());

	// after setting up reply, else sessionID is reset
	stream.teardown(clientID, true);

	SI_LOG_DEBUG("%s", rtsp.c_str());

	// send reply to client
	if (send(fd, rtsp.c_str(), rtsp.size(), MSG_NOSIGNAL) == -1) {
		PERROR("error sending: methodTeardown");
		return false;
	}
	return true;
}

