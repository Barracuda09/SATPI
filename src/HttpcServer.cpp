/* HttpcServer.cpp

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
#include <HttpcServer.h>

#include <base/StopWatch.h>
#include <Log.h>
#include <Properties.h>
#include <Stream.h>
#include <StreamManager.h>
#include <StringConverter.h>
#include <socket/SocketClient.h>

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <fcntl.h>

const char *HttpcServer::HTML_BODY_WITH_CONTENT =
	"@#1 @#2\r\n" \
	"Server: SatPI WebServer v0.1\r\n" \
	"Location: @#3\r\n" \
	"CSeq: @#4\r\n" \
	"cache-control: no-cache\r\n" \
	"Content-Type: @#5\r\n" \
	"Content-Length: @#6\r\n" \
	"@#7" \
	"\r\n";

const char *HttpcServer::HTML_BODY_NO_CONTENT =
	"@#1 @#2\r\n" \
	"Server: SatPI WebServer v0.1\r\n" \
	"Location: @#3\r\n" \
	"CSeq: @#4\r\n" \
	"cache-control: no-cache\r\n" \
	"Content-Type: @#5\r\n" \
	"\r\n";

const std::string HttpcServer::HTML_PROTOCOL_RTSP_VERSION = "RTSP/1.0";
const std::string HttpcServer::HTML_PROTOCOL_HTTP_VERSION = "HTTP/1.1";

const std::string HttpcServer::HTML_OK                  = "200 OK";
const std::string HttpcServer::HTML_NO_RESPONSE         = "204 No Response";
const std::string HttpcServer::HTML_NOT_FOUND           = "404 Not Found";
const std::string HttpcServer::HTML_MOVED_PERMA         = "301 Moved Permanently";
const std::string HttpcServer::HTML_REQUEST_TIMEOUT     = "408 Request Timeout";
const std::string HttpcServer::HTML_SERVICE_UNAVAILABLE = "503 Service Unavailable";

//const std::string HttpcServer::CONTENT_TYPE_XML         = "application/xml; charset=UTF-8";
const std::string HttpcServer::CONTENT_TYPE_JSON        = "application/json; charset=UTF-8";
const std::string HttpcServer::CONTENT_TYPE_JS          = "application/javascript; charset=UTF-8";
const std::string HttpcServer::CONTENT_TYPE_XML         = "text/xml; charset=UTF-8";
const std::string HttpcServer::CONTENT_TYPE_HTML        = "text/html; charset=UTF-8";
const std::string HttpcServer::CONTENT_TYPE_CSS         = "text/css; charset=UTF-8";
const std::string HttpcServer::CONTENT_TYPE_PNG         = "image/png";
const std::string HttpcServer::CONTENT_TYPE_ICO         = "image/x-icon";
const std::string HttpcServer::CONTENT_TYPE_VIDEO       = "video/MP2T";
const std::string HttpcServer::CONTENT_TYPE_TEXT        = "text/parameters";

HttpcServer::HttpcServer(
		int maxClients,
		const std::string &protocol,
		StreamManager &streamManager,
		const std::string &bindIPAddress) :
		TcpSocket(maxClients, protocol),
		_streamManager(streamManager),
		_bindIPAddress(bindIPAddress) {}

HttpcServer::~HttpcServer() {}

void HttpcServer::initialize(
		int port,
		bool nonblock) {
	TcpSocket::initialize(_bindIPAddress, port, nonblock);
}

void HttpcServer::getHtmlBodyWithContent(std::string &htmlBody,
		const std::string &html, const std::string &location,
		const std::string &contentType, std::size_t docTypeSize,
		std::size_t cseq, const unsigned int rtspPort) const {
	// Check do we need to add TvHeadend specific "X-SATIP-RTSP-Port"
	const std::string satipRtspPort = (rtspPort == 0) ? "" :
		StringConverter::stringFormat("X-SATIP-RTSP-Port: @#1\r\n", rtspPort);

	htmlBody = StringConverter::stringFormat(HTML_BODY_WITH_CONTENT,
		getProtocolVersionString(), html, location, cseq, contentType,
		docTypeSize, satipRtspPort);
}

void HttpcServer::getHtmlBodyNoContent(std::string &htmlBody, const std::string &html,
		const std::string &location, const std::string &contentType, std::size_t cseq) const {
	htmlBody = StringConverter::stringFormat(HTML_BODY_NO_CONTENT, getProtocolVersionString(), html,
		location, cseq, contentType);
}

bool HttpcServer::process(SocketClient &client) {
	std::string msg = client.getMessage();

//	SI_LOG_DEBUG("@#1 HTML data from client @#2: @#3", client.getProtocolString(), client.getIPAddress(), msg);

	// parse HTML
	const std::string method = StringConverter::getMethod(msg);
	const std::string protocol = StringConverter::getProtocol(msg);
	if (method.empty() || protocol.empty()) {
		SI_LOG_ERROR("Unknown Data: @#1", msg);
		return false;
	}
	if (protocol == "RTSP") {
		processStreamingRequest(client);
	} else if (protocol == "HTTP") {
		if (method == "GET" || method == "HEAD") {
			if (StringConverter::hasTransportParameters(client.getMessage())) {
				processStreamingRequest(client);
			} else {
				methodGet(client, method == "HEAD");
			}
		} else if (method == "POST") {
			methodPost(client);
		} else {
			SI_LOG_ERROR("Unknown HTML message: @#1", msg);
			return false;
		}
	} else {
		SI_LOG_ERROR("@#1: Unknown HTML protocol: @#2", protocol, msg);
		return false;
	}
	return true;
}

void HttpcServer::processStreamingRequest(SocketClient &client) {
	base::StopWatch sw;
	sw.start();
	const std::string msg = client.getPercentDecodedMessage();
	SI_LOG_DEBUG("@#1 Stream data from client @#2 with IP @#3 on Port @#4: @#5",
		client.getProtocolString(), "None", client.getIPAddressOfSocket(),
		client.getSocketPort(), msg);

	const std::string method = StringConverter::getMethod(msg);
	if (method.empty()) {
		return;
	}
	// Save clients seq number
	const std::string fieldCSeq = StringConverter::getHeaderFieldParameter(msg, "CSeq:");
	const int cseq =  fieldCSeq.empty() ? 0 : std::stoi(fieldCSeq);
	// Save sessionID and StreamID
	const std::string sessionID = StringConverter::getHeaderFieldParameter(msg, "Session:");
	const StreamID streamID = StringConverter::getIntParameter(msg, method, "stream=");
	// Find the FeID with requesed StreamID
	const FeID feID = _streamManager.findFrontendIDWithStreamID(streamID);

	std::string httpcReply;
	if (sessionID.empty() && method == "OPTIONS") {
		methodOptions("", cseq, httpcReply);
	} else if (sessionID.empty() && method == "DESCRIBE") {
		methodDescribe("", cseq, feID, httpcReply);
	} else {
		int clientID;
		SpStream stream = _streamManager.findStreamAndClientIDFor(client, clientID);
		if (stream != nullptr) {
			stream->processStreamingRequest(msg, clientID, method);

			// Check the Method
			if (method == "GET") {
				stream->update(clientID);
				getHtmlBodyNoContent(httpcReply, HTML_OK, "", CONTENT_TYPE_VIDEO, 0);

			} else if (method == "SETUP") {
				methodSetup(*stream, clientID, httpcReply);

				if (!stream->update(clientID)) {
					// something wrong here... send 408 error
					getHtmlBodyNoContent(httpcReply, HTML_REQUEST_TIMEOUT, "", CONTENT_TYPE_VIDEO, cseq);
					stream->teardown(clientID);
				}
			} else if (method == "PLAY") {
				methodPlay(*stream, clientID, httpcReply);

				if (!stream->update(clientID)) {
					// something wrong here... send 408 error
					getHtmlBodyNoContent(httpcReply, HTML_REQUEST_TIMEOUT, "", CONTENT_TYPE_VIDEO, cseq);
					stream->teardown(clientID);
				}
			} else if (method == "TEARDOWN") {
				methodTeardown(sessionID, cseq, httpcReply);

				stream->teardown(clientID);
			} else if (method == "OPTIONS") {
				methodOptions(sessionID, cseq, httpcReply);
			} else if (method == "DESCRIBE") {
				methodDescribe(sessionID, cseq, feID, httpcReply);
			} else {
				// method not supported
				SI_LOG_ERROR("@#1: Method not allowed: @#2", method, msg);
			}
		} else {
			// something wrong here... send 503 error with 'No-More: frontends'
			static const std::string content("No-More: frontends\r\n");
			getHtmlBodyWithContent(httpcReply, HTML_SERVICE_UNAVAILABLE, "", CONTENT_TYPE_TEXT, content.size(), cseq);
			httpcReply += content;
		}
	}
	const unsigned long time = sw.getIntervalMS();
	SI_LOG_DEBUG("Send reply in @#1 ms\r\n@#2", time, httpcReply);
	if (!client.sendData(httpcReply.c_str(), httpcReply.size(), MSG_NOSIGNAL)) {
		SI_LOG_ERROR("Send Streaming reply failed");
	}
}

const std::string &HttpcServer::getProtocolVersionString() const {
	if (getProtocolString() == "RTSP") {
		return HTML_PROTOCOL_RTSP_VERSION;
	} else {
		return HTML_PROTOCOL_HTTP_VERSION;
	}
}
