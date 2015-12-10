/* HttpcServer.cpp

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#include "HttpcServer.h"
#include "InterfaceAttr.h"
#include "Properties.h"
#include "Streams.h"
#include "Stream.h"
#include "Log.h"
#include "SocketClient.h"
#include "StringConverter.h"
#include "Configure.h"
#include "XMLSupport.h"

#ifdef LIBDVBCSA
	#include "DvbapiClient.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <fcntl.h>

const char *HttpcServer::HTML_BODY_WITH_CONTENT =
	"%s %s\r\n" \
	"Server: SatPI WebServer v0.1\r\n" \
	"Location: %s\r\n" \
	"CSeq: %zu\r\n" \
	"cache-control: no-cache\r\n" \
	"Content-Type: %s\r\n" \
	"Content-Length: %d\r\n" \
	"\r\n";

const char *HttpcServer::HTML_BODY_NO_CONTENT =
	"%s %s\r\n" \
	"Server: SatPI WebServer v0.1\r\n" \
	"Location: %s\r\n" \
	"CSeq: %zu\r\n" \
	"cache-control: no-cache\r\n" \
	"Content-Type: %s\r\n" \
	"\r\n";

const std::string HttpcServer::HTML_PROTOCOL_RTSP_VERSION = "RTSP/1.0";
const std::string HttpcServer::HTML_PROTOCOL_HTTP_VERSION = "HTTP/1.1";

const std::string HttpcServer::HTML_OK                  = "200 OK";
const std::string HttpcServer::HTML_NO_RESPONSE         = "204 No Response";
const std::string HttpcServer::HTML_NOT_FOUND           = "404 Not Found";
const std::string HttpcServer::HTML_MOVED_PERMA         = "301 Moved Permanently";
const std::string HttpcServer::HTML_SERVICE_UNAVAILABLE = "503 Service Unavailable";

const std::string HttpcServer::CONTENT_TYPE_XML         = "text/xml; charset=UTF-8";
const std::string HttpcServer::CONTENT_TYPE_HTML        = "text/html; charset=UTF-8";
const std::string HttpcServer::CONTENT_TYPE_JS          = "text/javascript; charset=UTF-8";
const std::string HttpcServer::CONTENT_TYPE_CSS         = "text/css; charset=UTF-8";
const std::string HttpcServer::CONTENT_TYPE_PNG         = "image/png";
const std::string HttpcServer::CONTENT_TYPE_ICO         = "image/x-icon";
const std::string HttpcServer::CONTENT_TYPE_VIDEO       = "video/MP2T";

HttpcServer::HttpcServer(int maxClients, const std::string &protocol, int port, bool nonblock,
                         Streams &streams,
                         const InterfaceAttr &interface) :
		TcpSocket(maxClients, protocol, port, nonblock),
		_streams(streams),
		_interface(interface) {}

HttpcServer::~HttpcServer() {}

const std::string &HttpcServer::getProtocolVersionString() const {
	if (getProtocolString() == "RTSP") {
		return HTML_PROTOCOL_RTSP_VERSION;
	} else {
		return HTML_PROTOCOL_HTTP_VERSION;
	}
}

void HttpcServer::getHtmlBodyWithContent(std::string &htmlBody, const std::string &html,
		const std::string &location, const std::string &contentType, std::size_t docTypeSize, std::size_t cseq) const {
	StringConverter::addFormattedString(htmlBody, HTML_BODY_WITH_CONTENT, getProtocolVersionString().c_str(), html.c_str(),
		location.c_str(), cseq, contentType.c_str(), docTypeSize);
}

void HttpcServer::getHtmlBodyNoContent(std::string &htmlBody, const std::string &html,
		const std::string &location, const std::string &contentType, std::size_t cseq) const {
	StringConverter::addFormattedString(htmlBody, HTML_BODY_NO_CONTENT, getProtocolVersionString().c_str(), html.c_str(),
		location.c_str(), cseq, contentType.c_str());
}

bool HttpcServer::process(SocketClient &client) {
	std::string msg = client.getMessage();

//	SI_LOG_DEBUG("%s HTML data from client %s: %s", client.getProtocolString().c_str(), client.getIPAddress().c_str(), msg.c_str());

	// parse HTML
	std::string method;
	std::string protocol;
	if (StringConverter::getMethod(msg.c_str(), method) &&
	    StringConverter::getProtocol(msg.c_str(), protocol)) {
		if (protocol.compare("RTSP") == 0) {
			processStreamingRequest(client);
		} else if (protocol.compare("HTTP") == 0) {
			if (method.compare("GET") == 0) {
				methodGet(client);
			} else if (method.compare("POST") == 0) {
				methodPost(client);
			} else {
				SI_LOG_ERROR("Unknown HTML message: %s", msg.c_str());
			}
		} else {
			SI_LOG_ERROR("%s: Unknown HTML protocol: %s", protocol.c_str(), msg.c_str());
		}
	}
	return true;
}

bool HttpcServer::closeConnection(SocketClient &socketclient) {
	int clientID;
	Stream *stream = _streams.findStreamAndClientIDFor(socketclient, clientID);
	if (stream != nullptr) {
		stream->close(clientID);
	}
	return true;
}

void HttpcServer::processStreamingRequest(SocketClient &client) {
	std::string msg = client.getMessage();
	SI_LOG_DEBUG("%s Stream data from client %s: %s", client.getProtocolString().c_str(), client.getIPAddress().c_str(), msg.c_str());

	std::string httpc;
	int clientID;
	Stream *stream = _streams.findStreamAndClientIDFor(client, clientID);
	if (stream != nullptr) {
		std::string method;
		if (StringConverter::getMethod(msg, method)) {
			stream->processStream(msg, clientID, method);

			// Check the Method
			if (method == "GET") {
				stream->update(clientID);
				getHtmlBodyNoContent(httpc, HTML_OK, "", CONTENT_TYPE_VIDEO, 0);
			} else if (method == "SETUP") {
				methodSetup(*stream, clientID, httpc);
			} else if (method == "PLAY") {
				methodPlay(*stream, clientID, httpc);
			} else if (method == "OPTIONS") {
				methodOptions(*stream, clientID, httpc);
			} else if (method == "DESCRIBE") {
				methodDescribe(*stream, clientID, httpc);
			} else if (method == "TEARDOWN") {
				methodTeardown(*stream, clientID, httpc);
			}
		}
	} else {
		std::string cseq("0");
		StringConverter::getHeaderFieldParameter(msg, "CSeq:", cseq);

		// something wrong here... send 503 error
		getHtmlBodyNoContent(httpc, HTML_SERVICE_UNAVAILABLE, "", CONTENT_TYPE_VIDEO, atoi(cseq.c_str()));
	}

	SI_LOG_DEBUG("%s", httpc.c_str());

	// send reply to client
	if (send(client.getFD(), httpc.c_str(), httpc.size(), MSG_NOSIGNAL) == -1) {
		PERROR("error sending: streaming reply");
	}
}
