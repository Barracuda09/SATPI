/* Properties.cpp

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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
#include <Properties.h>
#include <Log.h>

extern const char *satpi_version;

Properties::Properties(const std::string &uuid,
                       const std::string &appdataPath, const std::string &webPath,
					   const unsigned int httpPort, const unsigned int rtspPort) :
	XMLSupport(),
	_uuid(uuid),
	_versionString(satpi_version),
	_appdataPath(appdataPath),
	_webPath(webPath),
	_xSatipM3U("channellist.m3u"),
	_xmlDeviceDescriptionFile("desc.xml"),
	_httpPort(8875),
	_httpPortExt(httpPort),
	_rtspPort(554),
	_rtspPortExt(rtspPort),
	_appStartTime(std::time(nullptr)),
	_exitApplication(false) {}

Properties::~Properties() {}

void Properties::fromXML(const std::string &xml) {
	base::MutexLock lock(_xmlMutex);

	std::string element;
	if (findXMLElement(xml, "xsatipm3u.value", element)) {
		_xSatipM3U = element;
	}
	if (findXMLElement(xml, "xmldesc.value", element)) {
		_xmlDeviceDescriptionFile = element;
	}
	if (findXMLElement(xml, "httpport.value", element)) {
		const unsigned int httpPort = atoi(element.c_str());
		if (_httpPortExt != 0) {
			_httpPort = _httpPortExt;
		} else {
			_httpPort = httpPort;
		}
		SI_LOG_INFO("Setting HTTP Port to: %d", _httpPort);
	}
	if (findXMLElement(xml, "rtspport.value", element)) {
		const unsigned int rtspPort = atoi(element.c_str());
		if (_rtspPortExt != 0) {
			_rtspPort = _rtspPortExt;
		} else {
			_rtspPort = rtspPort;
		}
		SI_LOG_INFO("Setting RTSP Port to: %d", _rtspPort);
	}
	if (findXMLElement(xml, "webPath.value", element)) {
		_webPath = element;
		SI_LOG_INFO("Setting WEB Path to: %s", _webPath.c_str());
	}
	if (findXMLElement(xml, "appDataPath.value", element)) {
		_appdataPath = element;
		SI_LOG_INFO("Setting App Data Path to: %s", _appdataPath.c_str());
	}
}

void Properties::addToXML(std::string &xml) const {
	base::MutexLock lock(_xmlMutex);

	ADD_XML_NUMBER_INPUT(xml, "httpport", _httpPort, 0, 65535);
	ADD_XML_NUMBER_INPUT(xml, "rtspport", _rtspPort, 0, 65535);
	ADD_XML_TEXT_INPUT(xml, "xsatipm3u", _xSatipM3U);
	ADD_XML_TEXT_INPUT(xml, "xmldesc", _xmlDeviceDescriptionFile);
	ADD_XML_TEXT_INPUT(xml, "webPath", _webPath);
	ADD_XML_TEXT_INPUT(xml, "appDataPath", _appdataPath);
}

std::string Properties::getSoftwareVersion() const {
	base::MutexLock lock(_xmlMutex);
	return _versionString;
}

std::string Properties::getUUID() const {
	base::MutexLock lock(_xmlMutex);
	return _uuid;
}

std::string Properties::getAppDataPath() const {
	base::MutexLock lock(_xmlMutex);
	return _appdataPath;
}

std::string Properties::getWebPath() const {
	base::MutexLock lock(_xmlMutex);
	return _webPath;
}

std::string Properties::getXSatipM3U() const {
	base::MutexLock lock(_xmlMutex);
	// Should we add a '/'
	if (_xSatipM3U[0] != '/' && _xSatipM3U.find("http://") == std::string::npos) {
		return "/" + _xSatipM3U;
	} else {
		return _xSatipM3U;
	}
}

std::string Properties::getXMLDeviceDescriptionFile() const {
	base::MutexLock lock(_xmlMutex);
	return _xmlDeviceDescriptionFile;
}

void Properties::setHttpPort(const unsigned int httpPort) {
	base::MutexLock lock(_xmlMutex);
	_httpPort = httpPort;
}

unsigned int Properties::getHttpPort() const {
	base::MutexLock lock(_xmlMutex);
	return _httpPort;
}

void Properties::setRtspPort(const unsigned int rtspPort) {
	base::MutexLock lock(_xmlMutex);
	_rtspPort = rtspPort;
}

unsigned int Properties::getRtspPort() const {
	base::MutexLock lock(_xmlMutex);
	return _rtspPort;
}

std::time_t Properties::getApplicationStartTime() const {
	base::MutexLock lock(_xmlMutex);
	return _appStartTime;
}

bool Properties::exitApplication() const {
	base::MutexLock lock(_xmlMutex);
	return _exitApplication;
}

void Properties::setExitApplication() {
	base::MutexLock lock(_xmlMutex);
	_exitApplication = true;
}
