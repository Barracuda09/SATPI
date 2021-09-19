/* Transformation.cpp

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
#include <input/Transformation.h>

#include <Log.h>
#include <Utils.h>
#include <StringConverter.h>
#include <base/Tokenizer.h>
#include <input/DeviceData.h>
#include <input/dvb/dvbfix.h>

#include <cstdint>

namespace input {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

Transformation::Transformation(
		const std::string &appDataPath,
		input::DeviceData &transformedDeviceData) :
		_enabled(false),
		_transform(false),
		_advertiseAs(AdvertiseAs::NONE),
		_appDataPath(appDataPath),
		_transformFileM3U("mapping.m3u"),
		_transformedDeviceData(transformedDeviceData),
		_transformFreq(0) {
	_fileParsed = _m3u.parse(_appDataPath + "/" + _transformFileM3U);
}

Transformation::~Transformation() {}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void Transformation::doAddToXML(std::string &xml) const {
	base::MutexLock lock(_mutex);
	ADD_XML_CHECKBOX(xml, "transformEnable", (_enabled ? "true" : "false"));
	ADD_XML_TEXT_INPUT(xml, "transformM3U", _transformFileM3U);
	ADD_XML_ELEMENT(xml, "transformFreq", _transformFreq);

	ADD_XML_BEGIN_ELEMENT(xml, "advertiseAsType");
		ADD_XML_ELEMENT(xml, "inputtype", "selectionlist");
		ADD_XML_ELEMENT(xml, "value", asInteger(_advertiseAs));
		ADD_XML_BEGIN_ELEMENT(xml, "list");
		ADD_XML_ELEMENT(xml, "option0", "None");
		ADD_XML_ELEMENT(xml, "option1", "DVB-S2");
		ADD_XML_ELEMENT(xml, "option2", "DVB-C");
		ADD_XML_ELEMENT(xml, "option3", "DVB-T");
		ADD_XML_END_ELEMENT(xml, "list");
	ADD_XML_END_ELEMENT(xml, "advertiseAsType");
}

void Transformation::doFromXML(const std::string &xml) {
	base::MutexLock lock(_mutex);
	std::string element;
	if (findXMLElement(xml, "transformEnable.value", element)) {
		_enabled = (element == "true") ? true : false;
	}
	if (findXMLElement(xml, "transformM3U.value", element)) {
		_transformFileM3U = element;
		_fileParsed = _m3u.parse(_appDataPath + "/" + _transformFileM3U);
	}
	if (findXMLElement(xml, "advertiseAsType.value", element)) {
		const AdvertiseAs type = static_cast<AdvertiseAs>(std::stoi(element));
		switch (type) {
			case AdvertiseAs::NONE:
				_advertiseAs = AdvertiseAs::NONE;
				break;
			case AdvertiseAs::DVB_S2:
				_advertiseAs = AdvertiseAs::DVB_S2;
				break;
			case AdvertiseAs::DVB_C:
				_advertiseAs = AdvertiseAs::DVB_C;
				break;
			case AdvertiseAs::DVB_T:
				_advertiseAs = AdvertiseAs::DVB_T;
				break;
			default:
				SI_LOG_ERROR("Frontend: x, Wrong AdvertiseAs type requested, not changing");
		}
	}
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

bool Transformation::isEnabled() const {
	base::MutexLock lock(_mutex);
	return _enabled && _fileParsed;
}

bool Transformation::advertiseAsDVBS2() const {
	base::MutexLock lock(_mutex);
	return _enabled && _advertiseAs == AdvertiseAs::DVB_S2;
}

bool Transformation::advertiseAsDVBC() const {
	base::MutexLock lock(_mutex);
	return _enabled && _advertiseAs == AdvertiseAs::DVB_C;
}

void Transformation::resetTransformFlag() {
	base::MutexLock lock(_mutex);
	_transform = false;
	_transformFreq = 0;
}

input::InputSystem Transformation::getTransformationSystemFor(
		const std::string &msg,
		const std::string &method) const {
	base::MutexLock lock(_mutex);
	if (_enabled && _fileParsed) {
		const double freq = StringConverter::getDoubleParameter(msg, method, "freq=");
		if (freq > 0.0) {
			const int src = StringConverter::getIntParameter(msg, method, "src=");
			const base::M3UParser::TransformationElement element =
				_m3u.findTransformationElementFor(freq);
			if (element.src == -1 || (element.src >= 0 && element.src == src)) {
				return StringConverter::getMSYSParameter(element.uri, "");
			}
		}
	}
	return input::InputSystem::UNDEFINED;
}

bool Transformation::transformStreamPossible(
		const FeID id,
		const std::string &msg,
		const std::string &method,
		std::string &uriTransform) {
	if (_enabled && _fileParsed) {
		const double freq = StringConverter::getDoubleParameter(msg, method, "freq=");
		if (freq > 0.0) {
			const int src = StringConverter::getIntParameter(msg, method, "src=");
			const base::M3UParser::TransformationElement element =
				_m3u.findTransformationElementFor(freq);
			if (!element.uri.empty() && (element.src == -1 || (element.src >= 0 && element.src == src))) {
				SI_LOG_INFO("Frontend: @#1, Request can be Transformed with: @#2", id, uriTransform);
				uriTransform = element.uri;
				_transformFreq = freq * 1000.0;
				return true;
			}
		}
	}
	return false;
}

std::string Transformation::transformStreamString(
		const FeID id,
		const std::string &msg,
		const std::string &method) {
	base::MutexLock lock(_mutex);
	// Transformation possible for this request
	std::string uriTransform;
	if (!transformStreamPossible(id, msg, method, uriTransform)) {
		return msg;
	}
	// Transformation possible
	_transform = true;

	// remove 'xxxpids=' from transform uri if specified
	base::StringTokenizer tokenizer(uriTransform, "?& ");
	tokenizer.removeToken("addpids=");
	tokenizer.removeToken("delpids=");
	const std::string uri = tokenizer.removeToken("pids=");

	const input::InputSystem msys = StringConverter::getMSYSParameter(msg, method);
	if (msys != input::InputSystem::UNDEFINED) {
		_transformedDeviceData.setDeliverySystem(msys);
	}
	// Build new message with transform uri and requested PIDS
	std::string msgTrans = method;
	msgTrans += " ";
	msgTrans += uri;
	const std::string pidsList = StringConverter::getStringParameter(msg, method, "pids=");
	if (!pidsList.empty()) {
		msgTrans += "&pids=";
		msgTrans += pidsList;
	}
	const std::string addpidsList = StringConverter::getStringParameter(msg, method, "addpids=");
	if (!addpidsList.empty()) {
		msgTrans += "&addpids=";
		msgTrans += addpidsList;
	}
	const std::string delpidsList = StringConverter::getStringParameter(msg, method, "delpids=");
	if (!delpidsList.empty()) {
		msgTrans += "&delpids=";
		msgTrans += delpidsList;
	}
	msgTrans += " RTSP/1.0";
	SI_LOG_INFO("Frontend: @#1, Request Transformed to: @#2", id, msgTrans);
	return msgTrans;
}

const DeviceData &Transformation::transformDeviceData(
		const DeviceData &deviceData) const {
	base::MutexLock lock(_mutex);
	if (_enabled && _fileParsed) {
		const fe_status_t status = deviceData.getSignalStatus();
		const uint32_t ber = deviceData.getBitErrorRate();
		const uint32_t ublocks = deviceData.getUncorrectedBlocks();
		uint16_t strength = deviceData.getSignalStrength();
		uint16_t snr = deviceData.getSignalToNoiseRatio();

		// do we need to 'simulate' some signal
		if (_transform && strength == 0 && snr == 0) {
			strength = 240;
			snr = 15;
		}

		_transformedDeviceData.setMonitorData(status, strength, snr, ber, ublocks);
	}
	return (_transform && _enabled && _fileParsed) ? _transformedDeviceData : deviceData;
}

} // namespace input
