/* HttpServer.cpp

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
#include "HttpServer.h"
#include "InterfaceAttr.h"
#include "Properties.h"
#include "Streams.h"
#include "Log.h"
#include "SocketClient.h"
#include "StringConverter.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <fcntl.h>


#define HTML_BODY_CONT	"HTTP/1.1 %s\r\n" \
						"Server: SatPI WebServer v0.1\r\n" \
						"Location: %s\r\n" \
						"Content-Type: %s\r\n" \
						"Content-Length: %d\r\n" \
						"\r\n"

#define HTML_OK            "200 OK"
#define HTML_NO_RESPONSE   "204 No Response"
#define HTML_NOT_FOUND     "404 Not Found"
#define HTML_MOVED_PERMA   "301 Moved Permanently"


#define CONTENT_TYPE_XML   "text/xml; charset=UTF-8"
#define CONTENT_TYPE_HTML  "text/html; charset=UTF-8"
#define CONTENT_TYPE_JS    "text/javascript; charset=UTF-8"
#define CONTENT_TYPE_CSS   "text/css; charset=UTF-8"
#define CONTENT_TYPE_PNG   "image/png"
#define CONTENT_TYPE_ICO   "image/x-icon"

#define SSDP_INTERVAL_VAR  "ssdp_interval"
#define DVR_BUFFER_VAR     "dvr_buffer"
#define SYSLOG_ON_VAR      "syslog_on"
#define RESET_FRONTEND     "reset_fe"

HttpServer::HttpServer(const InterfaceAttr &interface,
                       Streams &streams,
                       Properties &properties) :
		ThreadBase("HttpServer"),
		TcpSocket(20, HTTP_PORT, true),
		_interface(interface),
		_streams(streams),
		_properties(properties) {
	startThread();
}

HttpServer::~HttpServer() {
	cancelThread();
	joinThread();
}

void HttpServer::threadEntry() {
	SI_LOG_INFO("Setting up HTTP server");

	for (;;) {
		// call poll with a timeout of 500 ms
		poll(500);
	}
}

bool HttpServer::process(SocketClient &client) {
//	SI_LOG_INFO("%s", client.getMessage().c_str());

	// parse HTML
	std::string method;
	if (StringConverter::getMethod(client.getMessage().c_str(), method)) {
		if (method.compare("GET") == 0) {
			getMethod(client);
		} else if (method.compare("POST") == 0) {
			postMethod(client);
		} else {
			SI_LOG_ERROR("Unknown HTML message connection: %s", client.getMessage().c_str());
		}
	}
	return true;
}

int read_file(const char *file, std::string &data) {
	int file_size = 0;
	int fd;
	if ((fd = open(file, O_RDONLY | O_NONBLOCK)) < 0) {
		SI_LOG_ERROR("GET %s", file);
		PERROR("File not found");
		return 0;
	}
	const off_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	char *buf = new char[size];
	if (buf) {
		file_size = read(fd, buf, size);
		data.assign(buf, file_size);
		delete[] buf;
	}
	CLOSE_FD(fd);
//	SI_LOG_DEBUG("GET %s (size %d)", file, file_size);
	return file_size;
}

bool HttpServer::postMethod(const SocketClient &client) {
	std::string content;
	if (StringConverter::getContentTypeFrom(client.getMessage().c_str(), content)) {
		std::string::size_type found = content.find_first_of("=");
		if (found != std::string::npos) {
			std::string id = content.substr(0, found);
			std::string val = content.substr(found + 1);
			if (id.compare(SSDP_INTERVAL_VAR) == 0) {
				_properties.setSsdpAnnounceTimeSec( atoi(val.c_str()) );
				SI_LOG_INFO("Setting SSDP annouce interval to: %d Sec", _properties.getSsdpAnnounceTimeSec());
			} else if (id.compare(DVR_BUFFER_VAR) == 0) {
				_streams.setDVRBufferSize(0, atoi(val.c_str()));
				SI_LOG_INFO("Setting DVR buffer to: %d Bytes", _streams.getDVRBufferSize(0));
			}
		}
	}
	// setup reply
	std::string htmlBody;
	StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_NO_RESPONSE, "", CONTENT_TYPE_HTML, 0);

	// send 'htmlBody' to client
	if (send(client.getFD(), htmlBody.c_str(), htmlBody.size(), 0) == -1) {
		PERROR("send htmlBody");
		return false;
	}
	return true;
}

bool HttpServer::getMethod(const SocketClient &client) {
	std::string htmlBody;
	std::string docType;
	std::string file;
	int docTypeSize = 0;

	// Parse what to get
	if (StringConverter::isRootFile(client.getMessage())) {
#define HTML_MOVED "<html>\r\n" \
                   "<head>\r\n" \
                   "<title>Page is moved</title>\r\n" \
                   "</head>\r\n" \
                   "<body>\r\n" \
                   "<h1>Moved</h1>\r\n" \
                   "<p>This page is moved:\r\n" \
                   "<a href=\"%s:%d%s\">link</a>.</p>\r\n" \
                   "</body>\r\n" \
                   "</html>"
		StringConverter::addFormattedString(docType, HTML_MOVED, _interface.getIPAddress().c_str(), HTTP_PORT, "/index.html");
		docTypeSize = docType.size();
		StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_MOVED_PERMA, "/index.html", CONTENT_TYPE_HTML, docTypeSize);
	} else {
		if (StringConverter::getRequestedFile(client.getMessage(), file)) {
			std::string path = _properties.getStartPath() + "/web";
			path += file;
			if (file.compare("/data.xml") == 0) {
				make_data_xml(docType);
				docTypeSize = docType.size();

				StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_OK, file.c_str(), CONTENT_TYPE_XML, docTypeSize);
			} else if (file.compare("/streams.xml") == 0) {
				make_streams_xml(docType);
				docTypeSize = docType.size();

				StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_OK, file.c_str(), CONTENT_TYPE_XML, docTypeSize);
			} else if (file.compare("/config.xml") == 0) {
				make_config_xml(docType);
				docTypeSize = docType.size();

				StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_OK, file.c_str(), CONTENT_TYPE_XML, docTypeSize);
			} else if (file.compare("/log.xml") == 0) {
				docType = make_log_xml();
				docTypeSize = docType.size();

				StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_OK, file.c_str(), CONTENT_TYPE_XML, docTypeSize);
			} else if (file.compare("/STOP") == 0) {
				// KILL
				std::exit(0);
			} else if ((docTypeSize = read_file(path.c_str(), docType))) {
				if (file.find(".xml") != std::string::npos) {
					// check if the request is the SAT>IP description xml then fill in the server version, UUID and tuner string
					if (docType.find("urn:ses-com:device") != std::string::npos) {
						SI_LOG_DEBUG("Client: %s requesed %s", client.getIPAddress().c_str(), file.c_str());

						docTypeSize -= 3 * 2; // minus 3x %s
						docTypeSize += _properties.getDeliverySystemString().size();
						docTypeSize += _properties.getUUID().size();
						docTypeSize += _properties.getSoftwareVersion().size();
						char *doc_desc_xml = new char[docTypeSize + 1];
						snprintf(doc_desc_xml, docTypeSize+1, docType.c_str(), _properties.getSoftwareVersion().c_str(),
						                       _properties.getUUID().c_str(), _properties.getDeliverySystemString().c_str());
						docType = doc_desc_xml;
						delete[] doc_desc_xml;
					}
					StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_OK, file.c_str(), CONTENT_TYPE_XML, docTypeSize);

				} else if (file.find(".html") != std::string::npos) {
					StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_OK, file.c_str(), CONTENT_TYPE_HTML, docTypeSize);
				} else if (file.find(".js") != std::string::npos) {
					StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_OK, file.c_str(), CONTENT_TYPE_JS, docTypeSize);
				} else if (file.find(".css") != std::string::npos) {
					StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_OK, file.c_str(), CONTENT_TYPE_CSS, docTypeSize);
				} else if ((file.find(".png") != std::string::npos) ||
						   (file.find(".ico") != std::string::npos)) {
					StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_OK, file.c_str(), CONTENT_TYPE_PNG, docTypeSize);
				} else {
					StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_OK, file.c_str(), CONTENT_TYPE_HTML, docTypeSize);
				}
			} else if ((docTypeSize = read_file("web/404.html", docType))) {
				StringConverter::addFormattedString(htmlBody, HTML_BODY_CONT, HTML_NOT_FOUND, file.c_str(), CONTENT_TYPE_HTML, docTypeSize);
			}
		}
	}
	// send something?
	if (docType.size() > 0) {
//		SI_LOG_INFO("%s", htmlBody);
		// send 'htmlBody' to client
		if (send(client.getFD(), htmlBody.c_str(), htmlBody.size(), 0) == -1) {
			PERROR("send htmlBody");
			return false;
		}
		// send 'docType' to client
		if (send(client.getFD(), docType.c_str(), docTypeSize, 0) == -1) {
			PERROR("send docType");
			return false;
		}
		return true;
	}
	return false;
}

void HttpServer::make_config_xml(std::string &xml) {
	// make config xml
	xml  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
	xml += "<data>\r\n";
	
	// application data
	xml += "<configdata>\r\n";
	StringConverter::addFormattedString(xml, "<input1><id>"SSDP_INTERVAL_VAR"</id><inputtype>number</inputtype><value>%d</value></input1>", _properties.getSsdpAnnounceTimeSec());
	StringConverter::addFormattedString(xml, "<input2><id>"DVR_BUFFER_VAR"</id><inputtype>number</inputtype><value>%d</value></input2>", _streams.getDVRBufferSize(0));
	xml += "</configdata>\r\n";

	xml += "</data>\r\n";
}

void HttpServer::make_data_xml(std::string &xml) {
	// make data xml
	xml  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
	xml += "<data>\r\n";

	// application data
	xml += "<appdata>";
	StringConverter::addFormattedString(xml, "<uptime>%d</uptime>", time(NULL) - _properties.getApplicationStartTime());
	StringConverter::addFormattedString(xml, "<appversion>%s</appversion>", _properties.getSoftwareVersion().c_str());
	StringConverter::addFormattedString(xml, "<uuid>%s</uuid>", _properties.getUUID().c_str());
	StringConverter::addFormattedString(xml, "<bootID>%d</bootID>", _properties.getBootID());
	StringConverter::addFormattedString(xml, "<deviceID>%d</deviceID>", _properties.getDeviceID());
	xml += "</appdata>";

	xml += "</data>\r\n";
}

void HttpServer::make_streams_xml(std::string &xml) {
	_streams.make_streams_xml(xml);
}
