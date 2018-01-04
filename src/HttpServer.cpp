/* HttpServer.cpp

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
#include <HttpServer.h>

#include <base/XMLSupport.h>
#include <InterfaceAttr.h>
#include <Log.h>
#include <Utils.h>
#include <Properties.h>
#include <Stream.h>
#include <StreamManager.h>
#include <socket/SocketClient.h>
#include <StringConverter.h>

#include <iostream>
#include <fstream>

HttpServer::HttpServer(
	base::XMLSupport &xml,
	StreamManager &streamManager,
	const InterfaceAttr &interface,
	Properties &properties) :
	ThreadBase("HttpServer"),
	HttpcServer(20, "HTTP", streamManager, interface),
	_properties(properties),
	_xml(xml) {}

HttpServer::~HttpServer() {
	cancelThread();
	joinThread();
}

void HttpServer::initialize(int port, bool nonblock) {
	HttpcServer::initialize(port, nonblock);
	startThread();
}

void HttpServer::threadEntry() {
	SI_LOG_INFO("Setting up HTTP server");

	for (;; ) {
		// call poll with a timeout of 500 ms
		poll(500);
	}
}

std::size_t HttpServer::readFile(const char *filePath, std::string &data) const {
	std::ifstream file;
	file.open(filePath);
	if (file.is_open()) {
		data = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		return data.size();
	}
	SI_LOG_ERROR("Unable to read from File: %s", filePath);
	return 0;
}

bool HttpServer::methodPost(SocketClient &client) {
	std::string content;
	if (StringConverter::getContentFrom(client.getMessage(), content)) {
		std::string file;
		if (StringConverter::getRequestedFile(client.getMessage(), file)) {
			if (file.compare("/SatPI.xml") == 0) {
				_xml.fromXML(content);
			}
		}
	}
	// setup reply
	std::string htmlBody;
	getHtmlBodyWithContent(htmlBody, HTML_NO_RESPONSE, "", CONTENT_TYPE_HTML, 0, 0);

	// send 'htmlBody' to client
	if (!client.sendData(htmlBody.c_str(), htmlBody.size(), 0)) {
		SI_LOG_ERROR("Send htmlBody failed");
		return false;
	}
	return true;
}

bool HttpServer::methodGet(SocketClient &client) {
	std::string htmlBody;
	std::string docType;
	int docTypeSize = 0;
	bool exitRequest = false;

	// Parse what to get
	if (StringConverter::isRootFile(client.getMessage())) {
		const std::string HTML_MOVED = "<html>\r\n"                            \
		                               "<head>\r\n"                            \
		                               "<title>Page is moved</title>\r\n"      \
		                               "</head>\r\n"                           \
		                               "<body>\r\n"                            \
		                               "<h1>Moved</h1>\r\n"                    \
		                               "<p>This page is moved:\r\n"            \
		                               "<a href=\"%s:%d%s\">link</a>.</p>\r\n" \
		                               "</body>\r\n"                           \
		                               "</html>";
		StringConverter::addFormattedString(docType, HTML_MOVED.c_str(), _interface.getIPAddress().c_str(), _properties.getHttpPort(), "/index.html");
		docTypeSize = docType.size();

		getHtmlBodyWithContent(htmlBody, HTML_MOVED_PERMA, "/index.html", CONTENT_TYPE_XML, docTypeSize, 0);
	} else {
		std::string file;
		if (StringConverter::hasTransportParameters(client.getMessage())) {
			processStreamingRequest(client);
		} else if (StringConverter::getRequestedFile(client.getMessage(), file)) {
			// remove first '/'?
			if (file[0] == '/') {
				file.erase(0, 1);
			}

			// Check if there is an '.eot?' in the URI as part of an IE fix?
			const std::string::size_type found = file.find(".eot?");
			if (found != std::string::npos) {
				file.erase(found + 4, 1);
			}

			const std::string filePath = _properties.getWebPath() + "/" + file;
			if (file.compare("SatPI.xml") == 0) {
				_xml.addToXML(docType);
				docTypeSize = docType.size();

				getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize, 0);
			} else if (file.compare("log.xml") == 0) {
				docType = Log::makeXml();
				docTypeSize = docType.size();

				getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize, 0);
			} else if (file.compare("STOP") == 0) {
				exitRequest = true;

				getHtmlBodyWithContent(htmlBody, HTML_NO_RESPONSE, "", CONTENT_TYPE_HTML, 0, 0);
			} else if ((docTypeSize = readFile(filePath.c_str(), docType))) {
				if (file.find(".xml") != std::string::npos) {
					// check if the request is the SAT>IP description xml then fill in the server version, UUID,
					// XSatipM3U, presentationURL and tuner string
					if (docType.find("urn:ses-com:device") != std::string::npos) {
						SI_LOG_DEBUG("Client: %s requesed %s", client.getIPAddress().c_str(), file.c_str());
						// check did we get our desc.xml (we assume there are some %1 in there)
						if (docType.find("%1") != std::string::npos) {
							// @todo 'presentationURL' change this later
							const std::string presentationURL = StringConverter::stringFormat("http://%1:%2/",
									_interface.getIPAddress().c_str(),
									std::to_string(_properties.getHttpPort()));

							const std::string newDocType = StringConverter::stringFormat(docType.c_str(),
								_properties.getSoftwareVersion(), _properties.getUUID(), presentationURL,
								_streamManager.getXMLDeliveryString(), _properties.getXSatipM3U());
							docType = newDocType;
							docTypeSize = docType.size();
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
					// check did we get our *.m3u (we assume there are some %1 in there, we should fill in IP Address)
					if (docType.find("%1") != std::string::npos) {
						SI_LOG_DEBUG("Client: %s requesed %s", client.getIPAddress().c_str(), file.c_str());
						const std::string newDocType = StringConverter::stringFormat(
								docType.c_str(), _interface.getIPAddress());
						docType = newDocType;
						docTypeSize = docType.size();
					}
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_VIDEO, docTypeSize, 0);
				} else if ((file.find(".png") != std::string::npos) ||
				           (file.find(".ico") != std::string::npos)) {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_PNG, docTypeSize, 0);
				} else {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_HTML, docTypeSize, 0);
				}
			} else {
				file = _properties.getWebPath() + "/" + "404.html";
				docTypeSize = readFile(file.c_str(), docType);
				getHtmlBodyWithContent(htmlBody, HTML_NOT_FOUND, file, CONTENT_TYPE_HTML, docTypeSize, 0);
			}
		}
	}
	// send something?
	if (docType.size() > 0) {
//		SI_LOG_INFO("%s", htmlBody);
		// send 'htmlBody' to client
		if (!client.sendData(htmlBody.c_str(), htmlBody.size(), 0)) {
			SI_LOG_ERROR("Send htmlBody failed");
			return false;
		}
		// send 'docType' to client
		if (!client.sendData(docType.c_str(), docTypeSize, 0)) {
			SI_LOG_ERROR("Send docType failed");
			return false;
		}
		return true;
	}
	// check did we have stop request
	if (exitRequest) {
		_properties.setExitApplication();
	}
	return false;
}
