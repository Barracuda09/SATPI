/* DiSEqcEN50607.cpp

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
		_chFreq(1210),
		_delayBeforeWrite(8),
		_delayAfterWrite(15) {}

	DiSEqcEN50607::~DiSEqcEN50607() {}

	// =======================================================================
	// -- input::dvb::delivery::DiSEqc ---------------------------------------
	// =======================================================================

	bool DiSEqcEN50607::sendDiseqc(const int feFD, const FeID id, uint32_t &freq,
                int src, Lnb::Polarization pol) {
		return sendDiseqcJess(feFD, id, freq, src, pol);
	}

	void DiSEqcEN50607::doNextAddToXML(std::string &xml) const {
		ADD_XML_NUMBER_INPUT(xml, "chFreq", _chFreq, 0, 2150);
		ADD_XML_NUMBER_INPUT(xml, "chSlot", _chSlot, 0, 31);
		ADD_XML_NUMBER_INPUT(xml, "pin", _pin, 0, 256);
		ADD_XML_NUMBER_INPUT(xml, "delayBeforeWrite", _delayBeforeWrite, 0, 2000);
		ADD_XML_NUMBER_INPUT(xml, "delayAfterWrite", _delayAfterWrite, 0, 2000);
		ADD_XML_N_ELEMENT(xml, "lnb", 0, _lnb.toXML());
	}

	void DiSEqcEN50607::doNextFromXML(const std::string &xml) {
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
		if (findXMLElement(xml, "delayBeforeWrite.value", element)) {
			_delayBeforeWrite = std::stoi(element);
		}
		if (findXMLElement(xml, "delayAfterWrite.value", element)) {
			_delayAfterWrite = std::stoi(element);
		}
		if (findXMLElement(xml, "lnb0", element)) {
			_lnb.fromXML(element);
		}
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool DiSEqcEN50607::sendDiseqcJess(const int feFD, const FeID id,
		uint32_t &freq,	const int src, const Lnb::Polarization pol) {
		bool hiband = false;
		_lnb.getIntermediateFrequency(freq, hiband, pol);

		// Calulate T
		const uint32_t t = (freq / 1000.0) - 100.0;
		// Change tuning frequency to 'Channel Freq'
		freq = _chFreq * 1000;

		// Framing 0x70: Command from Master, Unicable II/Jess tuning command
		// Data 1  0x00: see below
		// Data 2  0x00: see below
		// Data 3  0x00: see below
		dvb_diseqc_master_cmd cmd = {{0x70, 0x00, 0x00, 0x00, 0x00, 0x00}, 4};

		// UB [4:0] (1-32) and T [10:8]
		cmd.msg[1] = _chSlot << 3 | ((t >> 8) & 0x07);
		// T [7:0]
		cmd.msg[2] = (t & 0xff);

		// param: high nibble: "uncommitted switches"
		//        low nibble: "committed switches"
		// bits are: option, position, polarizaion, band
		cmd.msg[3]  = (src << 2) & 0x0f;
		cmd.msg[3] |= pol == Lnb::Polarization::Horizontal ? 0x2 : 0x0;
		cmd.msg[3] |= hiband ? 0x1 : 0x0;

		for (size_t i = 0; i < _diseqcRepeat + 1; ++i) {
			if (ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_18) == -1) {
				PERROR("FE_SET_VOLTAGE failed");
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(_delayBeforeWrite));
			SI_LOG_INFO("Frontend: %d, Sending DiSEqC: [%02x] [%02x] [%02x] [%02x] - DiSEqC Src: %d - UB: %d",
				id.getID(), cmd.msg[0], cmd.msg[1], cmd.msg[2], cmd.msg[3], src, _chSlot);
			if (ioctl(feFD, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1) {
				PERROR("FE_DISEQC_SEND_MASTER_CMD failed");
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(_delayAfterWrite));
			if (ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_13) == -1) {
				PERROR("FE_SET_VOLTAGE failed");
			}
			// Should we repeat message
			if (_diseqcRepeat > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		return true;
	}

} // namespace delivery
} // namespace dvb
} // namespace input
