/* HttpServer.cpp

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
#include "HttpServer.h"
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
#include <iostream>
#include <fstream>

HttpServer::HttpServer(Streams &streams,
                       const InterfaceAttr &interface,
                       Properties &properties,
                       DvbapiClient *dvbapi) :
		ThreadBase("HttpServer"),
		HttpcServer(20, "HTTP", HTTP_PORT, true, streams, interface),
		_properties(properties),
		_dvbapi(dvbapi) {
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

int HttpServer::readFile(const char *file, std::string &data) {
	int file_size = 0;
	int fd_file;
	if ((fd_file = open(file, O_RDONLY | O_NONBLOCK)) < 0) {
		SI_LOG_ERROR("readFile %s", file);
		PERROR("File not found");
		return 0;
	}
	const off_t size = lseek(fd_file, 0, SEEK_END);
	lseek(fd_file, 0, SEEK_SET);
	char *buf = new char[size];
	if (buf != nullptr) {
		file_size = read(fd_file, buf, size);
		data.assign(buf, file_size);
		DELETE_ARRAY(buf);
	}
	CLOSE_FD(fd_file);
//	SI_LOG_DEBUG("GET %s (size %d)", file, file_size);
	return file_size;
}

bool HttpServer::methodPost(const SocketClient &client) {
	std::string content;
	if (StringConverter::getContentFrom(client.getMessage(), content)) {
		std::string file;
		if (StringConverter::getRequestedFile(client.getMessage(), file)) {
			if (file.compare("/data.xml") == 0) {

			} else if (file.compare("/streams.xml") == 0) {
				_streams.fromXML(content);
			} else if (file.compare("/config.xml") == 0) {
				_properties.fromXML(content);
#ifdef LIBDVBCSA
				_dvbapi->fromXML(content);
#endif
			save_config_xml(make_config_xml(content));
			}
		}
	}
	// setup reply
	std::string htmlBody;
	getHtmlBodyWithContent(htmlBody, HTML_NO_RESPONSE, "", CONTENT_TYPE_HTML, 0, 0);

	// send 'htmlBody' to client
	if (send(client.getFD(), htmlBody.c_str(), htmlBody.size(), 0) == -1) {
		PERROR("send htmlBody");
		return false;
	}
	return true;
}

bool HttpServer::methodGet(SocketClient &client) {
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

		getHtmlBodyWithContent(htmlBody, HTML_MOVED_PERMA, "/index.html", CONTENT_TYPE_XML, docTypeSize, 0);
	} else {
		if (StringConverter::hasTransportParameters(client.getMessage())){
			processStreamingRequest(client);
		} else if (StringConverter::getRequestedFile(client.getMessage(), file)) {
			std::string path = _properties.getStartPath() + "/web";
			path += file;
			if (file.compare("/data.xml") == 0) {
				make_data_xml(docType);
				docTypeSize = docType.size();

				getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize, 0);
			} else if (file.compare("/streams.xml") == 0) {
				make_streams_xml(docType);
				docTypeSize = docType.size();

				getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize, 0);
			} else if (file.compare("/config.xml") == 0) {
				parse_config_xml();
				make_config_xml(docType);
				docTypeSize = docType.size();

				getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize, 0);
			} else if (file.compare("/log.xml") == 0) {
				docType = make_log_xml();
				docTypeSize = docType.size();

				getHtmlBodyWithContent(htmlBody,  HTML_OK, file, CONTENT_TYPE_XML, docTypeSize, 0);
			} else if (file.compare("/STOP") == 0) {
				// KILL
				std::exit(0);
			} else if ((docTypeSize = readFile(path.c_str(), docType))) {
				if (file.find(".xml") != std::string::npos) {
					// check if the request is the SAT>IP description xml then fill in the server version, UUID and tuner string
					if (docType.find("urn:ses-com:device") != std::string::npos) {
						SI_LOG_DEBUG("Client: %s requesed %s", client.getIPAddress().c_str(), file.c_str());
						// check did we get our desc.xml (we assume there are some %s in there)
						if (docType.find("%s") != std::string::npos) {
							docTypeSize -= 4 * 2; // minus 4x %s
							docTypeSize += _properties.getDeliverySystemString().size();
							docTypeSize += _properties.getUUID().size();
							docTypeSize += _properties.getSoftwareVersion().size();
							docTypeSize += _properties.getXSatipM3U().size();
							char *doc_desc_xml = new char[docTypeSize + 1];
							if (doc_desc_xml != nullptr) {
								snprintf(doc_desc_xml, docTypeSize+1, docType.c_str(), _properties.getSoftwareVersion().c_str(),
										 _properties.getUUID().c_str(), _properties.getDeliverySystemString().c_str(),
										 _properties.getXSatipM3U().c_str());
								docType = doc_desc_xml;
								DELETE_ARRAY(doc_desc_xml);
							}
						}
					}
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize, 0);

				} else if (file.find(".html") != std::string::npos) {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_HTML, docTypeSize, 0);
				} else if (file.find(".js") != std::string::npos) {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_JS, docTypeSize, 0);
				} else if (file.find(".css") != std::string::npos) {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_CSS, docTypeSize, 0);
				} else if (file.find(".m3u") != std::string::npos) {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_VIDEO, docTypeSize, 0);
				} else if ((file.find(".png") != std::string::npos) ||
						   (file.find(".ico") != std::string::npos)) {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_PNG, docTypeSize, 0);
				} else {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_HTML, docTypeSize, 0);
				}
			} else if ((docTypeSize = readFile("web/404.html", docType))) {
				getHtmlBodyWithContent(htmlBody, HTML_NOT_FOUND, file, CONTENT_TYPE_HTML, docTypeSize, 0);
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

std::string HttpServer::make_config_xml(std::string &xml) {
	// make config xml
	xml  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
	xml += "<data>\r\n";

	// application data
	xml += "<configdata>\r\n";

	_properties.addToXML(xml);
#ifdef LIBDVBCSA
	_dvbapi->addToXML(xml);
#endif
	xml += "</configdata>\r\n";

	xml += "</data>\r\n";
	return xml;
}

// Save Config file
void HttpServer::save_config_xml(std::string xml)
{
	//
	std::string path = _properties.getStartPath() + "/web/config.xml";
	std::ofstream configFile;
	configFile.open(path);
	if (configFile.is_open()){
		configFile << xml;
	}
	configFile.close();
}

void HttpServer::parse_config_xml()
{
	std::string path = _properties.getStartPath() + "/web/config.xml";
	std::ifstream configFile;
	configFile.open(path);
	if (configFile.is_open()){
		std::string cfig((std::istreambuf_iterator<char>(configFile)), std::istreambuf_iterator<char>());
		// Parse config file
		_properties.fromXML(cfig);
	#ifdef LIBDVBCSA
		_dvbapi->fromXML(cfig);
	#endif
	}
	configFile.close();
}

void HttpServer::make_data_xml(std::string &xml) {
	// make data xml
	xml  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
	xml += "<data>\r\n";

	// application data
	xml += "<appdata>";
	StringConverter::addFormattedString(xml, "<uptime>%d</uptime>", time(nullptr) - _properties.getApplicationStartTime());
	StringConverter::addFormattedString(xml, "<appversion>%s</appversion>", _properties.getSoftwareVersion().c_str());
	StringConverter::addFormattedString(xml, "<uuid>%s</uuid>", _properties.getUUID().c_str());
	StringConverter::addFormattedString(xml, "<bootID>%d</bootID>", _properties.getBootID());
	StringConverter::addFormattedString(xml, "<deviceID>%d</deviceID>", _properties.getDeviceID());
	xml += "</appdata>";

	xml += "</data>\r\n";
}

void HttpServer::make_streams_xml(std::string &xml) {
	// make data xml
	xml  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
	xml += "<data>\r\n";

	_streams.addToXML(xml);

	xml += "</data>\r\n";
}
