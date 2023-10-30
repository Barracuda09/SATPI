/* Properties.cpp

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
#include <Properties.h>

#include <Log.h>

extern const char* const satpi_version;

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

Properties::Properties(
		const std::string &uuid,
		const std::string &currentPathOpt,
		const std::string &appdataPathOpt,
		const std::string &webPathOpt,
		const std::string &ipAddress,
		const unsigned int httpPortOpt,
		const unsigned int rtspPortOpt) :
	XMLSupport(),
	_uuid(uuid),
	_versionString(satpi_version),
	_xSatipM3U("channellist.m3u"),
	_xmlDeviceDescriptionFile("desc.xml"),
	_appStartTime(std::time(nullptr)),
	_exitApplication(false) {

	_httpPort = httpPortOpt == 0 ? 8875 : httpPortOpt;
	_rtspPort = rtspPortOpt == 0 ? 554 : rtspPortOpt;
	_httpPortOpt = httpPortOpt;
	_rtspPortOpt = rtspPortOpt;
	_ipAddress = ipAddress;

	_webPath = webPathOpt.empty() ? (currentPathOpt + "/" + "web") : webPathOpt;
	_appdataPath = appdataPathOpt.empty() ? currentPathOpt : appdataPathOpt;
	_webPathOpt = webPathOpt;
	_appdataPathOpt = appdataPathOpt;
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void Properties::doFromXML(const std::string &xml) {
	std::string element;
	if (findXMLElement(xml, "xsatipm3u.value", element)) {
		_xSatipM3U = element;
	}
	if (findXMLElement(xml, "xmldesc.value", element)) {
		_xmlDeviceDescriptionFile = element;
	}
	if (findXMLElement(xml, "httpport.value", element)) {
		const unsigned int httpPort = std::stoi(element);
		if (httpPort >= HTTP_PORT_MIN && httpPort <= TCP_PORT_MAX) {
			_httpPort = _httpPortOpt != 0 ? _httpPortOpt : httpPort;
			SI_LOG_INFO("Setting HTTP Port to: @#1", _httpPort);
		}
	}
	if (findXMLElement(xml, "rtspport.value", element)) {
		const unsigned int rtspPort = std::stoi(element);
		if (rtspPort >= RTSP_PORT_MIN && rtspPort <= TCP_PORT_MAX) {
			_rtspPort = _rtspPortOpt != 0 ? _rtspPortOpt : rtspPort;
			SI_LOG_INFO("Setting RTSP Port to: @#1", _rtspPort);
		}
	}
	if (findXMLElement(xml, "webPath.value", element)) {
		_webPath = _webPathOpt.empty() ? element : _webPathOpt;
		SI_LOG_INFO("Setting WEB Path to: @#1", _webPath);
	}
	if (findXMLElement(xml, "appDataPath.value", element)) {
		_appdataPath = _appdataPathOpt.empty() ? element : _appdataPathOpt;
		SI_LOG_INFO("Setting App Data Path to: @#1", _appdataPath);
	}
	if (findXMLElement(xml, "syslog.value", element)) {
		const bool start = (element == "true") ? true : false;
		Log::startSysLog(start);
	}
	if (findXMLElement(xml, "logDebug.value", element)) {
		const bool log = (element == "true") ? true : false;
		Log::logDebug(log);
	}
}

void Properties::doAddToXML(std::string &xml) const {
	ADD_XML_NUMBER_INPUT(xml, "httpport", _httpPort, HTTP_PORT_MIN, TCP_PORT_MAX);
	ADD_XML_NUMBER_INPUT(xml, "rtspport", _rtspPort, RTSP_PORT_MIN, TCP_PORT_MAX);
	ADD_XML_TEXT_INPUT(xml, "ipaddress", _ipAddress);
	ADD_XML_TEXT_INPUT(xml, "xsatipm3u", _xSatipM3U);
	ADD_XML_TEXT_INPUT(xml, "xmldesc", _xmlDeviceDescriptionFile);
	ADD_XML_TEXT_INPUT(xml, "webPath", _webPath);
	ADD_XML_TEXT_INPUT(xml, "appDataPath", _appdataPath);
	ADD_XML_CHECKBOX(xml, "syslog", (Log::getSysLogState() ? "true" : "false"));
	ADD_XML_CHECKBOX(xml, "logDebug", (Log::getLogDebugState() ? "true" : "false"));
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

std::string Properties::getSoftwareVersion() const {
	return _versionString;
}

std::string Properties::getUPnPVersion() const {
	return "SatPI/" + _versionString;
}

std::string Properties::getUUID() const {
	base::MutexLock lock(_mutex);
	return _uuid;
}

std::string Properties::getAppDataPath() const {
	base::MutexLock lock(_mutex);
	return _appdataPath;
}

std::string Properties::getWebPath() const {
	base::MutexLock lock(_mutex);
	return _webPath;
}

std::string Properties::getXSatipM3U() const {
	base::MutexLock lock(_mutex);
	// Should we add a '/'
	if (_xSatipM3U[0] != '/' && _xSatipM3U.find("http://") == std::string::npos) {
		return "/" + _xSatipM3U;
	} else {
		return _xSatipM3U;
	}
}

std::string Properties::getXMLDeviceDescriptionFile() const {
	base::MutexLock lock(_mutex);
	return _xmlDeviceDescriptionFile;
}

void Properties::setHttpPort(const unsigned int httpPort) {
	base::MutexLock lock(_mutex);
	_httpPort = httpPort;
}

unsigned int Properties::getHttpPort() const {
	base::MutexLock lock(_mutex);
	return _httpPort;
}

void Properties::setRtspPort(const unsigned int rtspPort) {
	base::MutexLock lock(_mutex);
	_rtspPort = rtspPort;
}

unsigned int Properties::getRtspPort() const {
	base::MutexLock lock(_mutex);
	return _rtspPort;
}

std::string Properties::getIpAddress() const {
	base::MutexLock lock(_mutex);
	return _ipAddress;
}

std::time_t Properties::getApplicationStartTime() const {
	base::MutexLock lock(_mutex);
	return _appStartTime;
}

bool Properties::exitApplication() const {
	base::MutexLock lock(_mutex);
	return _exitApplication;
}

void Properties::setExitApplication() {
	base::MutexLock lock(_mutex);
	_exitApplication = true;
}
