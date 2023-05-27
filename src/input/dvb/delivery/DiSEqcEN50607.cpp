/* DiSEqcEN50607.cpp

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
#include <input/dvb/delivery/DiSEqcEN50607.h>

#include <Log.h>
#include <StringConverter.h>
#include <input/dvb/FrontendData.h>

#include <chrono>
#include <thread>

#include <fcntl.h>
#include <sys/ioctl.h>

namespace input::dvb::delivery {

	// =======================================================================
	//  -- Constructors and destructor ---------------------------------------
	// =======================================================================
	DiSEqcEN50607::DiSEqcEN50607() :
		DiSEqc(),
		_pin(256),
		_chSlot(0),
		_chFreq(1210) {}

	// =======================================================================
	// -- input::dvb::delivery::DiSEqc ---------------------------------------
	// =======================================================================

	bool DiSEqcEN50607::sendDiseqc(const int feFD, const FeID id, uint32_t &freq,
			int src, Lnb::Polarization pol) {

		bool hiband = false;
		_lnb.getIntermediateFrequency(id, freq, hiband, pol);

		// Calulate T
		const uint32_t t = (freq / 1000.0) - 100.0;
		// Change tuning frequency to 'Channel Freq'
		freq = _chFreq * 1000;

		// Framing 0x70: Command from Master, Unicable II/Jess tuning command
		// Data 1  0x00: see below
		// Data 2  0x00: see below
		// Data 3  0x00: see below
		dvb_diseqc_master_cmd cmd = {{0x70, 0x00, 0x00, 0x00}, 4};

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

		SI_LOG_INFO("Frontend: @#1, Sending DiSEqC: [@#2] [@#3] [@#4] [@#5] - DiSEqC Src: @#6 - UB: @#7",
			id, HEX(cmd.msg[0], 2), HEX(cmd.msg[1], 2), HEX(cmd.msg[2], 2), HEX(cmd.msg[3], 2), src, _chSlot);

		return sendDiseqcMasterCommand(feFD, id, cmd, MiniDiSEqCSwitch::DoNotSend, _diseqcRepeat);
	}

	void DiSEqcEN50607::doNextAddToXML(std::string &xml) const {
		ADD_XML_NUMBER_INPUT(xml, "chFreq", _chFreq, 0, 2150);
		ADD_XML_NUMBER_INPUT(xml, "chSlot", _chSlot, 0, 31);
		ADD_XML_NUMBER_INPUT(xml, "pin", _pin, 0, 256);
		ADD_XML_N_ELEMENT(xml, "lnb", 1, _lnb.toXML());
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
		if (findXMLElement(xml, "lnb1", element)) {
			_lnb.fromXML(element);
		}
	}

}
