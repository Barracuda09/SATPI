/* DiSEqcEN50607.cpp

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
#include <input/dvb/delivery/DiSEqcEN50607.h>

#include <Log.h>
#include <StringConverter.h>
#include <input/dvb/FrontendData.h>

#include <chrono>
#include <thread>

#include <fcntl.h>
#include <sys/ioctl.h>

namespace input {
namespace dvb {
namespace delivery {

	// =======================================================================
	//  -- Constructors and destructor ---------------------------------------
	// =======================================================================
	DiSEqcEN50607::DiSEqcEN50607() :
		DiSEqc(),
		_pin(256),
		_chSlot(0),
		_chFreq(1210) {}

	DiSEqcEN50607::~DiSEqcEN50607() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void DiSEqcEN50607::addToXML(std::string &xml) const {
		{
			base::MutexLock lock(_xmlMutex);
			ADD_XML_NUMBER_INPUT(xml, "chFreq", _chFreq, 0, 2150);
			ADD_XML_NUMBER_INPUT(xml, "chSlot", _chSlot, 0, 40);
			ADD_XML_NUMBER_INPUT(xml, "pin", _pin, 0, 256);
		}
		DiSEqc::addToXML(xml);
	}

	void DiSEqcEN50607::fromXML(const std::string &xml) {
		{
			base::MutexLock lock(_xmlMutex);
			std::string element;
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
		DiSEqc::fromXML(xml);
	}

	// =======================================================================
	// -- input::dvb::delivery::DiSEqc ---------------------------------------
	// =======================================================================

	bool DiSEqcEN50607::sendDiseqc(int feFD, int streamID, uint32_t &freq,
                int src, int pol_v) {
		if (ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_13) == -1) {
			PERROR("FE_SET_VOLTAGE failed");
		}
		if (ioctl(feFD, FE_SET_TONE, SEC_TONE_OFF) == -1) {
			PERROR("FE_SET_TONE failed");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(900));

		return sendDiseqcJess(feFD, streamID, freq, src, pol_v);
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool DiSEqcEN50607::sendDiseqcJess(const int feFD, const int streamID, uint32_t &freq,
		const int src, const int pol_v) {
		base::MutexLock lock(_xmlMutex);

		// Digital Satellite Equipment Control, specification is available from http://www.eutelsat.com/
		dvb_diseqc_master_cmd cmd = {
			{0x70, 0x00, 0x00, 0x00, 0x00}, 4
		};

		bool hiband = false;
		_lnb[src % MAX_LNB].getIntermediateFrequency(freq, hiband, pol_v == POL_V);
		freq /= 1000;
		const uint32_t t = freq - 100;
		freq = _chFreq * 1000;

		cmd.msg[1] = _chSlot << 3 | ((t >> 8) & 0x07);
		cmd.msg[2] = (t & 0xff);
		cmd.msg[3] = (((src << 2) & 0x0f) | ((pol_v == POL_V) ? 0 : 2) | (hiband ? 1 : 0));

		for (size_t i = 0; i < _diseqcRepeat; ++i) {
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
			SI_LOG_INFO("Stream: %d, Sending DiSEqC [%02x] [%02x] [%02x] [%02x] [%02x]", streamID, cmd.msg[0],
					cmd.msg[1], cmd.msg[2], cmd.msg[3], cmd.msg[4]);
			if (ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_18) == -1) {
				PERROR("FE_SET_VOLTAGE failed");
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			if (ioctl(feFD, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1) {
				PERROR("FE_DISEQC_SEND_MASTER_CMD failed");
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(70));
			if (ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_13) == -1) {
				PERROR("FE_SET_VOLTAGE failed");
			}
		}
		return true;
	}

} // namespace delivery
} // namespace dvb
} // namespace input
