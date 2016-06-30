/* DiSEqcEN50494.cpp

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
#include <input/dvb/delivery/DiSEqcEN50494.h>

#include <Log.h>
#include <Utils.h>
#include <StringConverter.h>
#include <input/dvb/FrontendData.h>

#include <cmath>
#include <fcntl.h>
#include <sys/ioctl.h>

namespace input {
namespace dvb {
namespace delivery {

	// =======================================================================
	//  -- Constructors and destructor ---------------------------------------
	// =======================================================================
	DiSEqcEN50494::DiSEqcEN50494() :
		DiSEqc(),
		_diseqcRepeat(true),
		_pin(256),
		_chSlot(0),
		_chFreq(1210) {}

	DiSEqcEN50494::~DiSEqcEN50494() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void DiSEqcEN50494::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		for (std::size_t i = 0u; i < MAX_LNB; ++i) {
			StringConverter::addFormattedString(xml, "<lnb%zu>", i);
			_LNB[i].addToXML(xml);
			StringConverter::addFormattedString(xml, "</lnb%zu>", i);
		}

		ADD_CONFIG_CHECKBOX(xml, "diseqc_repeat", (_diseqcRepeat ? "true" : "false"));
		ADD_CONFIG_NUMBER_INPUT(xml, "chFreq", _chFreq, 0, 2150);
		ADD_CONFIG_NUMBER_INPUT(xml, "chSlot", _chSlot, 0, 7);
		ADD_CONFIG_NUMBER_INPUT(xml, "pin", _pin, 0, 256);
	}

	void DiSEqcEN50494::fromXML(const std::string &xml) {
		base::MutexLock lock(_mutex);
		std::string element;
		if (findXMLElement(xml, "diseqc_repeat.value", element)) {
			_diseqcRepeat = (element == "true") ? true : false;
		}
		if (findXMLElement(xml, "chFreq.value", element)) {
			_chFreq = std::stoi(element);
		}
		if (findXMLElement(xml, "chSlot.value", element)) {
			_chSlot = std::stoi(element);
		}
		if (findXMLElement(xml, "pin.value", element)) {
			_pin = std::stoi(element);
		}
	}

	// =======================================================================
	// -- input::dvb::delivery::DiSEqc ---------------------------------------
	// =======================================================================

	bool DiSEqcEN50494::sendDiseqc(int feFD, int streamID, uint32_t &freq,
                int src, int pol_v) {
		return sendDiseqcUnicable(feFD, streamID, freq, src, pol_v);
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool DiSEqcEN50494::sendDiseqcUnicable(const int feFD, const int streamID, uint32_t &freq,
		const int src, const int pol_v) {
		base::MutexLock lock(_mutex);

		// Digital Satellite Equipment Control, specification is available from http://www.eutelsat.com/
		dvb_diseqc_master_cmd cmd = {
			{0xe0, 0x11, 0x5a, 0x00, 0x00 }, 5
		};

		bool hiband = false;
		_LNB[src].getIntermediateFrequency(freq, hiband, pol_v == POL_V);
		freq /= 1000;
		const uint32_t t = round(((freq + _chFreq + 2) / 4) - 350);
		freq = _chFreq * 1000;

		cmd.msg[3] = ((_chSlot << 5) | ((pol_v == POL_V) ? 0 : 8) | (hiband ? 4 : 0) | ((t >> 8) & 0x03) );
		cmd.msg[4] = (t & 0xff);

		if (ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_13) == -1) {
			PERROR("FE_SET_VOLTAGE failed");
		}
		usleep(50 * 1000);
		if (ioctl(feFD, FE_SET_TONE, SEC_TONE_OFF) == -1) {
			PERROR("FE_SET_TONE failed");
		}
		usleep(50 * 1000);
		for (size_t i = 0; i < (_diseqcRepeat ? 2 : 1); ++i) {
			SI_LOG_INFO("Stream: %d, Sending DiSEqC [%02x] [%02x] [%02x] [%02x] [%02x]", streamID, cmd.msg[0],
					cmd.msg[1], cmd.msg[2], cmd.msg[3], cmd.msg[4]);
			if (ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_18) == -1) {
				PERROR("FE_SET_VOLTAGE failed");
			}
			usleep(15 * 1000);
			if (ioctl(feFD, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1) {
				PERROR("FE_DISEQC_SEND_MASTER_CMD failed");
			}
			usleep(50 * 1000);
			if (ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_13) == -1) {
				PERROR("FE_SET_VOLTAGE failed");
			}
			usleep(150 * 1000);
		}
		return true;
	}

} // namespace delivery
} // namespace dvb
} // namespace input
