/* Satpi.cpp

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
#include <Satpi.h>

#include <Log.h>
#include <Utils.h>
#include <StringConverter.h>

#include <chrono>

// =============================================================================
//  -- Constructors and destructor ---------------------------------------------
// =============================================================================
SatPI::SatPI(const SatPI::Params &params) :
	XMLSaveSupport((params.appdataPath.empty() ?
			params.currentPath : params.appdataPath) + "/" + "SatPI.xml"),
	_interface(params.ifaceName),
	_streamManager(),
	_properties(_interface.getUUID(), params.currentPath, params.appdataPath, params.webPath,
		_interface.getIPAddress(), params.httpPort, params.rtspPort),
	_httpServer(*this, _streamManager, _interface.getIPAddress(), _properties),
	_rtspServer(_streamManager, _interface.getIPAddress()),
	_ssdpServer(params.ssdpTTL, _interface.getIPAddress(), _properties) {
	_properties.setFunctionNotifyChanges(std::bind(&XMLSaveSupport::notifyChanges, this));
	_ssdpServer.setFunctionNotifyChanges(std::bind(&XMLSaveSupport::notifyChanges, this));
	//
	_streamManager.enumerateDevices(_interface.getIPAddress(),
		_properties.getAppDataPath(), params.dvbPath, params.numberOfChildPIPE,
		params.enableUnsecureFrontends);
	//
	std::string xml;
	if (restoreXML(xml)) {
		fromXML(xml);
	} else {
		saveXML();
	}

	_httpServer.initialize(_properties.getHttpPort(), true);
	_rtspServer.initialize(_properties.getRtspPort(), true);
	if (params.ssdp) {
		_ssdpServer.startThread();
	}
}

// =============================================================================
// -- base::XMLSupport ---------------------------------------------------------
// =============================================================================

void SatPI::doAddToXML(std::string &xml) const {
	xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
	ADD_XML_BEGIN_ELEMENT(xml, "data");

		// application data
		ADD_XML_BEGIN_ELEMENT(xml, "appdata");
			ADD_XML_ELEMENT(xml, "uptime", std::time(nullptr) - _properties.getApplicationStartTime());
			ADD_XML_ELEMENT(xml, "appversion", _properties.getSoftwareVersion());
			ADD_XML_ELEMENT(xml, "uuid", _properties.getUUID());
		ADD_XML_END_ELEMENT(xml, "appdata");

		ADD_XML_ELEMENT(xml, "streams", _streamManager.toXML());
		ADD_XML_ELEMENT(xml, "configdata", _properties.toXML());
		ADD_XML_ELEMENT(xml, "ssdp", _ssdpServer.toXML());
	ADD_XML_END_ELEMENT(xml, "data");
}

void SatPI::doFromXML(const std::string &xml) {
	std::string element;
	if (findXMLElement(xml, "streams", element)) {
		_streamManager.fromXML(element);
	}
	if (findXMLElement(xml, "configdata", element)) {
		_properties.fromXML(element);
	}
	if (findXMLElement(xml, "ssdp", element)) {
		_ssdpServer.fromXML(element);
	}
	saveXML();
}

// =============================================================================
// -- base::XMLSaveSupport -----------------------------------------------------
// =============================================================================

bool SatPI::saveXML() const {
	std::string xml;
	addToXML(xml);
	return XMLSaveSupport::saveXML(xml);
}

// =============================================================================
// -- Other member functions ---------------------------------------------------
// =============================================================================

bool SatPI::exitApplication() const {
	return _properties.exitApplication();
}
