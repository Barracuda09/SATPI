/* Translation.cpp

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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
#include <input/Translation.h>

#include <Log.h>
#include <StringConverter.h>
#include <input/dvb/dvbfix.h>

#include <cstdint>

namespace input {

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================

	Translation::Translation() :
			_translate(false) {
 		_m3u.parse("./satpi-maps-v1.m3u");
 		_translatedDeviceData.initialize();
	}

	Translation::~Translation() {}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	void Translation::resetTranslationFlag() {
		_translate = false;
	}

	input::InputSystem Translation::getTranslationSystemFor(
			const double frequency) const {
		input::InputSystem system = input::InputSystem::UNDEFINED;
		std::string uri;
		if (frequency > 0.0 && _m3u.findURIFor(frequency, uri)) {
			std::string msg = "SETUP ";
			msg += uri;
			system = StringConverter::getMSYSParameter(msg, "SETUP");
		}
		return system;
	}

	std::string Translation::translateStreamString(
			const int streamID,
			const std::string &msg,
			const std::string &method) {
		std::string msgTrans = msg;
		std::string uri;
		const double freq = StringConverter::getDoubleParameter(msg, method, "freq=");
		if (freq > 0.0 && _m3u.findURIFor(freq, uri)) {
			SI_LOG_INFO("Stream: %d, Request Translated", streamID);
			_translate = true;

			const input::InputSystem msys = StringConverter::getMSYSParameter(msg, method);
			if (msys != input::InputSystem::UNDEFINED) {
				_translatedDeviceData.setDeliverySystem(msys);
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
			SI_LOG_DEBUG("Stream: %d, Request Translated to %s", streamID, msgTrans.c_str());
		}
		return msgTrans;
	}

	const DeviceData &Translation::translateDeviceData(
			const DeviceData &deviceData) const {
		const fe_status_t status = deviceData.getSignalStatus();
		const uint32_t ber = deviceData.getBitErrorRate();
		const uint32_t ublocks = deviceData.getUncorrectedBlocks();
		uint16_t strength = deviceData.getSignalStrength();
		uint16_t snr = deviceData.getSignalToNoiseRatio();

		// do we need to 'simulate' some signal
		if (_translate && strength == 0 && snr == 0) {
			strength = 240;
			snr = 15;
		}

		_translatedDeviceData.setMonitorData(status, strength, snr, ber, ublocks);

		return _translate ? _translatedDeviceData : deviceData;
	}

} // namespace input
