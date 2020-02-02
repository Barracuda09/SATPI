/* Transformation.cpp

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================

	Transformation::Transformation(
			const std::string &appDataPath,
			input::DeviceData &transformedDeviceData) :
			_enabled(false),
			_transform(false),
			_advertiseAs(AdvertiseAs::NONE),
			_appDataPath(appDataPath),
			_transformFileM3U("mapping.m3u"),
			_transformedDeviceData(transformedDeviceData) {
		_fileParsed = _m3u.parse(_appDataPath + "/" + _transformFileM3U);
	}

	Transformation::~Transformation() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void Transformation::doAddToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		ADD_XML_CHECKBOX(xml, "transformEnable", (_enabled ? "true" : "false"));
		ADD_XML_TEXT_INPUT(xml, "transformM3U", _transformFileM3U);

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
					SI_LOG_ERROR("Stream: %d, Wrong AdvertiseAs type requested, not changing", 0);
			}
		}
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool Transformation::isEnabled() const {
		base::MutexLock lock(_mutex);
		return _enabled && _fileParsed;
	}

	bool Transformation::advertiseAsDVBS2() const {
		base::MutexLock lock(_mutex);
		return _advertiseAs == AdvertiseAs::DVB_S2;
	}

	bool Transformation::advertiseAsDVBC() const {
		base::MutexLock lock(_mutex);
		return _advertiseAs == AdvertiseAs::DVB_C;
	}

	void Transformation::resetTransformFlag() {
		base::MutexLock lock(_mutex);
		_transform = false;
	}

	input::InputSystem Transformation::getTransformationSystemFor(
			const double frequency) const {
		base::MutexLock lock(_mutex);
		input::InputSystem system = input::InputSystem::UNDEFINED;
		if (_enabled && _fileParsed) {
			if (frequency > 0.0) {
				const std::string uri = _m3u.findURIFor(frequency);
				system = StringConverter::getMSYSParameter(uri, "");
			}
		}
		return system;
	}

	std::string Transformation::transformStreamString(
			const int streamID,
			const std::string &msg,
			const std::string &method) {
		base::MutexLock lock(_mutex);
		std::string msgTrans = msg;
		if (_enabled && _fileParsed) {
			const double freq = StringConverter::getDoubleParameter(msg, method, "freq=");
			if (freq > 0.0) {
				const std::string uriMain = _m3u.findURIFor(freq);
				if (!uriMain.empty()) {
					_transform = true;

					// remove '*pids' from uri
					base::StringTokenizer tokenizer(uriMain, "?& ");
					tokenizer.removeToken("addpids=");
					tokenizer.removeToken("delpids=");
					const std::string uri = tokenizer.removeToken("pids=");

					const input::InputSystem msys = StringConverter::getMSYSParameter(msg, method);
					if (msys != input::InputSystem::UNDEFINED) {
						_transformedDeviceData.setDeliverySystem(msys);
					}

					msgTrans = method;
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
					SI_LOG_INFO("Stream: %d, Request Transformed to %s", streamID, msgTrans.c_str());
				}
			}
		}
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
