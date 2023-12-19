/* HttpServer.cpp

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
#include <HttpServer.h>

#include <base/XMLSupport.h>
#include <Log.h>
#include <Utils.h>
#include <Properties.h>
#include <Stream.h>
#include <StreamManager.h>
#include <socket/SocketClient.h>
#include <StringConverter.h>

#include <iostream>
#include <fstream>
#include <sstream>

HttpServer::HttpServer(
	base::XMLSupport &xml,
	StreamManager &streamManager,
	const std::string &bindIPAddress,
	Properties &properties) :
	ThreadBase("HttpServer"),
	HttpcServer(20, "HTTP", streamManager, bindIPAddress),
	_properties(properties),
	_xml(xml) {}

HttpServer::~HttpServer() {
	cancelThread();
	joinThread();
}

void HttpServer::initialize(
		int port,
		bool nonblock) {
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
	try {
		std::ifstream file;
		file.open(filePath);
		if (file.is_open()) {
			data = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();
			return data.size();
		}
	} catch (...) {
		// Eat this exception
	}
	SI_LOG_ERROR("Unable to read from File: @#1", filePath);
	return 0;
}

bool HttpServer::methodPost(SocketClient &client) {
	const std::string content = client.getContentFrom();
	if (!content.empty()) {
		const std::string file = client.getRequestedFile();
		if (file == "/SatPI.xml") {
			_xml.fromXML(content);
		}
	}
	// setup reply
	std::string htmlBody;
	getHtmlBodyWithContent(htmlBody, HTML_NO_RESPONSE, "", CONTENT_TYPE_HTML, 0, 0);

	// send 'htmlBody' to client
	if (!client.sendData(htmlBody.data(), htmlBody.size(), 0)) {
		SI_LOG_ERROR("Send htmlBody failed");
		return false;
	}
	return true;
}

bool HttpServer::methodGet(SocketClient &client, bool headOnly) {
	std::string htmlBody;
	std::string docType;
	int docTypeSize = 0;
	bool exitRequest = false;

	// Parse what to get
	if (client.isRootFile()) {
		static const char* HTML_MOVED = "<html>\r\n"                           \
		                               "<head>\r\n"                            \
		                               "<title>Page is moved</title>\r\n"      \
		                               "</head>\r\n"                           \
		                               "<body>\r\n"                            \
		                               "<h1>Moved</h1>\r\n"                    \
		                               "<p>This page is moved:\r\n"            \
		                               "<a href=\"@#1:@#2/@#3\">link</a>.</p>\r\n"\
		                               "</body>\r\n"                           \
		                               "</html>";
		docType = StringConverter::stringFormat(HTML_MOVED, _bindIPAddress, _properties.getHttpPort(), "index.html");
		docTypeSize = docType.size();

		getHtmlBodyWithContent(htmlBody, HTML_MOVED_PERMA, "/index.html", CONTENT_TYPE_XML, docTypeSize, 0);
	} else {
		std::string file = client.getRequestedFile();
		if (!file.empty()) {
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
			if (file == "SatPI.xml") {
				_xml.addToXML(docType);
				docTypeSize = docType.size();
				getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize, 0);
			} else if (file == "log.json") {
				docType = Log::makeJSON();
				docTypeSize = docType.size();
				getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_JSON, docTypeSize, 0);
			} else if (file == "STOP") {
				exitRequest = true;
				getHtmlBodyWithContent(htmlBody, HTML_NO_RESPONSE, "", CONTENT_TYPE_HTML, 0, 0);
			} else if ((docTypeSize = readFile(filePath.data(), docType))) {
				if (file.find(".xml") != std::string::npos) {
					// check if the request is the SAT>IP description xml then fill in the server version, UUID,
					// XSatipM3U, presentationURL and tuner string
					if (docType.find("urn:ses-com:device") != std::string::npos) {
						SI_LOG_DEBUG("Client: @#1 requested @#2", client.getIPAddressOfSocket(), file);
						// check did we get our desc.xml (we assume there are some @#1 in there)
						if (docType.find("@#1") != std::string::npos) {
							// @todo 'presentationURL' change this later
							const std::string presentationURL = StringConverter::stringFormat("http://@#1:@#2/",
									_bindIPAddress,
									std::to_string(_properties.getHttpPort()));
							const std::string modelName = StringConverter::stringFormat("SatPI Server (@#1)", _bindIPAddress);
							const std::string newDocType = StringConverter::stringFormat(docType.data(),
								modelName, _properties.getUPnPVersion(), _properties.getUUID(), presentationURL,
								_streamManager.getXMLDeliveryString(), _properties.getXSatipM3U());
							docType = newDocType;
							docTypeSize = docType.size();
						}
					}
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize, 0, _properties.getRtspPort());
				} else if (file.find(".html") != std::string::npos) {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_HTML, docTypeSize, 0);
				} else if (file.find(".json") != std::string::npos) {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_JSON, docTypeSize, 0);
				} else if (file.find(".js") != std::string::npos) {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_JS, docTypeSize, 0);
				} else if (file.find(".css") != std::string::npos) {
					getHtmlBodyWithContent(htmlBody, HTML_OK, file, CONTENT_TYPE_CSS, docTypeSize, 0);
				} else if (file.find(".m3u") != std::string::npos) {
					SI_LOG_DEBUG("Client: @#1 requested @#2", client.getIPAddressOfSocket(), file);
					// did we read our *.m3u, we assume there are some @#1
					if (docType.find("@#1") != std::string::npos) {
						const std::string rtsp = StringConverter::stringFormat("@#1:@#2",
								_bindIPAddress,	std::to_string(_properties.getRtspPort()));
						const std::string http = StringConverter::stringFormat("@#1:@#2",
								_bindIPAddress,	std::to_string(_properties.getHttpPort()));
						std::stringstream docTypeStream(docType);
						docType.clear();
						for (std::string line; std::getline(docTypeStream, line); ) {
							line += "\n";
							if (line.find("@#1") == std::string::npos) {
								docType += line;
								continue;
							}
							if (line.find("rtsp://") != std::string::npos) {
								docType += StringConverter::stringFormat(line.data(), rtsp);
							} else if (line.find("http://") != std::string::npos) {
								docType += StringConverter::stringFormat(line.data(), http);
							}
						}
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
				docTypeSize = readFile(file.data(), docType);
				getHtmlBodyWithContent(htmlBody, HTML_NOT_FOUND, file, CONTENT_TYPE_HTML, docTypeSize, 0);
			}
		}
	}
	// send something?
	if (htmlBody.size() > 0) {
		// send 'htmlBody' to client
		if (!client.sendData(htmlBody.data(), htmlBody.size(), 0)) {
			SI_LOG_ERROR("Send htmlBody failed");
			return false;
		}
		// send 'docType' to client if needed
		if (!headOnly && docTypeSize > 0) {
			if (!client.sendData(docType.data(), docTypeSize, 0)) {
				SI_LOG_ERROR("Send docType failed");
				return false;
			}
		}
		// check did we have stop request
		if (exitRequest) {
			_properties.setExitApplication();
		}
		return true;
	}
	return false;
}
