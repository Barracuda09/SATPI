/* DiSEqcEN50494.cpp

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
#include <input/dvb/delivery/DiSEqcEN50494.h>

#include <Log.h>
#include <StringConverter.h>
#include <input/dvb/FrontendData.h>

#include <chrono>
#include <thread>
#include <cmath>

#include <fcntl.h>
#include <sys/ioctl.h>

namespace input::dvb::delivery {

	// =======================================================================
	//  -- Constructors and destructor ---------------------------------------
	// =======================================================================
	DiSEqcEN50494::DiSEqcEN50494() :
		DiSEqc(),
		_pin(256),
		_chSlot(0),
		_chFreq(1210) {}

	// =======================================================================
	// -- input::dvb::delivery::DiSEqc ---------------------------------------
	// =======================================================================

	bool DiSEqcEN50494::sendDiseqc(const int feFD, const FeID id, uint32_t &freq,
			int src, Lnb::Polarization pol) {

		bool hiband = false;
		_lnb.getIntermediateFrequency(id, freq, hiband, pol);

		// Calculate T
		const uint32_t t = round((((freq / 1000.0) + _chFreq + 2.0) / 4.0) - 350.0);
		// Change tuning frequency to 'Channel Freq'
		freq = _chFreq * 1000;

		// Framing 0xe0: Command from Master, No reply required, First transmission
		// Address 0x10: Any LNB, Switcher or SMATV (Master to all...)
		// Command 0x5a: Unicable switch
		// Data 1  0x00: see below
		// Data 2  0x00: see below
		dvb_diseqc_master_cmd cmd = {{0xe0, 0x10, 0x5a, 0x00, 0x00}, 5};
		// Userband-ID (Channel Slot)
		cmd.msg[3]  = _chSlot << 5;
		// Sat. pos => src = 0 (posA)  src >= 1 (posB)
		cmd.msg[3] |= (src >= 1) ? 0x10 : 0x00;
		cmd.msg[3] |= pol == Lnb::Polarization::Horizontal ? 0x8 : 0x0;
		cmd.msg[3] |= hiband ? 0x4 : 0x0;
		cmd.msg[3] |= (t >> 8) & 0x03;
		cmd.msg[4]  = (t & 0xff);

		SI_LOG_INFO("Frontend: @#1, Sending DiSEqC: [@#2] [@#3] [@#4] [@#5] [@#6] - DiSEqC Src: @#7 - UB: @#8",
			id, HEX(cmd.msg[0], 2), HEX(cmd.msg[1], 2), HEX(cmd.msg[2], 2), HEX(cmd.msg[3], 2), HEX(cmd.msg[4], 2), src, _chSlot);

		return sendDiseqcMasterCommand(feFD, id, cmd, MiniDiSEqCSwitch::DoNotSend, _diseqcRepeat);
	}

	void DiSEqcEN50494::doNextAddToXML(std::string &xml) const {
		ADD_XML_NUMBER_INPUT(xml, "chFreq", _chFreq, 0, 2150);
		ADD_XML_NUMBER_INPUT(xml, "chSlot", _chSlot, 0, 7);
		ADD_XML_NUMBER_INPUT(xml, "pin", _pin, 0, 256);
		ADD_XML_N_ELEMENT(xml, "lnb", 1, _lnb.toXML());
	}

	void DiSEqcEN50494::doNextFromXML(const std::string &xml) {
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
