/* Transformation.cpp

   Copyright (C) 2014 - 2019 Marc Postema (mpostema09 -at- gmail.com)

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
			_transform(false),
			_advertiseAsDVBS2(false),
			_transformFileM3U(appDataPath + "/" + "mapping.m3u"),
			_transformedDeviceData(transformedDeviceData) {
		_enabled = _m3u.parse(_transformFileM3U);
		_transformedDeviceData.initialize();
	}

	Transformation::~Transformation() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void Transformation::addToXML(std::string &xml) const {
		base::MutexLock lock(_xmlMutex);
		ADD_XML_CHECKBOX(xml, "transformEnable", (_enabled ? "true" : "false"));
		ADD_XML_CHECKBOX(xml, "advertiseAsDVBS2", (_advertiseAsDVBS2 ? "true" : "false"));
		ADD_XML_TEXT_INPUT(xml, "transformM3U", _transformFileM3U);
	}

	void Transformation::fromXML(const std::string &xml) {
		base::MutexLock lock(_xmlMutex);
		std::string element;
		if (findXMLElement(xml, "transformEnable.value", element)) {
			_enabled = (element == "true") ? true : false;
		}
		if (findXMLElement(xml, "advertiseAsDVBS2.value", element)) {
			_advertiseAsDVBS2 = (element == "true") ? true : false;
		}
		if (findXMLElement(xml, "transformM3U.value", element)) {
			_transformFileM3U = element;
			_enabled = _m3u.parse(_transformFileM3U);
		}
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool Transformation::isEnabled() const {
		base::MutexLock lock(_xmlMutex);
		return _enabled;
	}

	bool Transformation::advertiseAsDVBS2() const {
		base::MutexLock lock(_xmlMutex);
		return _advertiseAsDVBS2;
	}

	void Transformation::resetTransformFlag() {
		base::MutexLock lock(_xmlMutex);
		_transform = false;
	}

	input::InputSystem Transformation::getTransformationSystemFor(
			const double frequency) const {
		base::MutexLock lock(_xmlMutex);
		input::InputSystem system = input::InputSystem::UNDEFINED;
		if (_enabled) {
			std::string uri;
			if (frequency > 0.0 && _m3u.findURIFor(frequency, uri)) {
				system = StringConverter::getMSYSParameter(uri, "");
			}
		}
		return system;
	}

	std::string Transformation::transformStreamString(
			const int streamID,
			const std::string &msg,
			const std::string &method) {
		base::MutexLock lock(_xmlMutex);
		std::string msgTrans = msg;
		if (_enabled) {
			std::string uriMain;
			const double freq = StringConverter::getDoubleParameter(msg, method, "freq=");
			if (freq > 0.0 && _m3u.findURIFor(freq, uriMain)) {
				SI_LOG_INFO("Stream: %d, Request Transformed", streamID);
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
				SI_LOG_DEBUG("Stream: %d, Request Transformed to %s", streamID, msgTrans.c_str());
			}
		}
		return msgTrans;
	}

	const DeviceData &Transformation::transformDeviceData(
			const DeviceData &deviceData) const {
		base::MutexLock lock(_xmlMutex);
		if (_enabled) {
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
		return (_transform && _enabled) ? _transformedDeviceData : deviceData;
	}

} // namespace input
