/* Transformation.cpp

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
#include <input/Transformation.h>

#include <Log.h>
#include <StringConverter.h>
#include <base/Tokenizer.h>
#include <input/dvb/dvbfix.h>

#include <cstdint>

namespace input {

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================

	Transformation::Transformation() :
			_transform(false),
			_transformFileM3U("mapping.m3u") {
		_enabled = _m3u.parse(_transformFileM3U);
		_transformedDeviceData.initialize();
	}

	Transformation::~Transformation() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void Transformation::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		ADD_CONFIG_CHECKBOX(xml, "transformEnable", (_enabled ? "true" : "false"));
		ADD_CONFIG_TEXT_INPUT(xml, "transformM3U", _transformFileM3U.c_str());
	}

	void Transformation::fromXML(const std::string &xml) {
		base::MutexLock lock(_mutex);
		std::string element;
		if (findXMLElement(xml, "transformEnable.value", element)) {
			_enabled = (element == "true") ? true : false;
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
		base::MutexLock lock(_mutex);
		return _enabled;
	}

	void Transformation::resetTransformFlag() {
		base::MutexLock lock(_mutex);
		_transform = false;
	}

	input::InputSystem Transformation::getTransformationSystemFor(
			const double frequency) const {
		base::MutexLock lock(_mutex);
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
		base::MutexLock lock(_mutex);
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
				std::string strVal;
				if (StringConverter::getStringParameter(msg, method, "pids=", strVal) == true) {
					msgTrans += "&pids=";
					msgTrans += strVal;
				}
				if (StringConverter::getStringParameter(msg, method, "addpids=", strVal) == true) {
					msgTrans += "&addpids=";
					msgTrans += strVal;
				}
				if (StringConverter::getStringParameter(msg, method, "delpids=", strVal) == true) {
					msgTrans += "&delpids=";
					msgTrans += strVal;
				}
				msgTrans += " RTSP/1.0";
				SI_LOG_DEBUG("Stream: %d, Request Transformed to %s", streamID, msgTrans.c_str());
			}
		}
		return msgTrans;
	}

	const DeviceData &Transformation::transformDeviceData(
			const DeviceData &deviceData) const {
		base::MutexLock lock(_mutex);
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
