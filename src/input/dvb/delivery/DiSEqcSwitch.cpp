/* DiSEqcSwitch.cpp

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
#include <input/dvb/delivery/DiSEqcSwitch.h>

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
	DiSEqcSwitch::DiSEqcSwitch() :
		DiSEqc() {}

	DiSEqcSwitch::~DiSEqcSwitch() {}

	// =======================================================================
	// -- input::dvb::delivery::DiSEqc ---------------------------------------
	// =======================================================================

	bool DiSEqcSwitch::sendDiseqc(const int feFD, const FeID id, uint32_t &freq,
			const int src, const Lnb::Polarization pol) {
		return diseqcSwitch(feFD, id, freq, src, pol);
	}

	void DiSEqcSwitch::doNextAddToXML(std::string &xml) const {
		for (std::size_t i = 0; i < MAX_LNB; ++i) {
			ADD_XML_N_ELEMENT(xml, "lnb", i, _lnb[i].toXML());
		}
	}

	void DiSEqcSwitch::doNextFromXML(const std::string &xml) {
		std::string element;
		for (std::size_t i = 0; i < MAX_LNB; ++i) {
			const std::string lnb = StringConverter::stringFormat("lnb@#1", i);
			if (findXMLElement(xml, lnb, element)) {
				_lnb[i].fromXML(element);
			}
		}
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool DiSEqcSwitch::diseqcSwitch(const int feFD, const FeID id, uint32_t &freq,
				int src, Lnb::Polarization pol) {
		bool hiband = false;
		_lnb[src].getIntermediateFrequency(freq, hiband, pol);

		// Framing 0xe0: Command from Master, No reply required, First transmission
		// Address 0x10: Any LNB, Switcher or SMATV (Master to all...)
		// Command 0x38: Write to Port group 0 (Committed switches)
		// Data 1  0xf0: see below
		// Data 2  0x00: not used
		// Data 3  0x00: not used
		// size    0x04: send x bytes
		struct dvb_diseqc_master_cmd cmd = {{0xe0, 0x10, 0x38, 0x00}, 4};

		// param: high nibble: reset bits
		//        low nibble: set bits
		// bits are: option, position, polarizaion, band
		cmd.msg[3]  = 0xf0;
		cmd.msg[3] |= (src << 2) & 0x0f;
		cmd.msg[3] |= pol == Lnb::Polarization::Horizontal ? 0x2 : 0x0;
		cmd.msg[3] |= hiband ? 0x1 : 0x0;

		for (size_t i = 0; i < _diseqcRepeat + 1; ++i) {
			if (ioctl(feFD, FE_SET_TONE, SEC_TONE_OFF) == -1) {
				SI_LOG_PERROR("FE_SET_TONE failed");
				return false;
			}
			const fe_sec_voltage_t v = (pol == Lnb::Polarization::Vertical) ?
				SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
			if (ioctl(feFD, FE_SET_VOLTAGE, v) == -1) {
				SI_LOG_PERROR("FE_SET_VOLTAGE failed");
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			SI_LOG_INFO("Frontend: @#1, Sending DiSEqC: [@#2] [@#3] [@#4] [@#5] - DiSEqC Src: @#6",
				id, HEX(cmd.msg[0], 2), HEX(cmd.msg[1], 2), HEX(cmd.msg[2], 2), HEX(cmd.msg[3], 2), src);
			if (ioctl(feFD, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1) {
				SI_LOG_PERROR("FE_DISEQC_SEND_MASTER_CMD failed");
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			const fe_sec_mini_cmd_t b = (src % 2) ? SEC_MINI_B : SEC_MINI_A;
			if (ioctl(feFD, FE_DISEQC_SEND_BURST, b) == -1) {
				SI_LOG_PERROR("FE_DISEQC_SEND_BURST failed");
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			const fe_sec_tone_mode_t tone = hiband ? SEC_TONE_ON : SEC_TONE_OFF;
			if (ioctl(feFD, FE_SET_TONE, tone) == -1) {
				SI_LOG_PERROR("FE_SET_TONE failed");
				return false;
			}
			// Should we repeat message
			if (_diseqcRepeat > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				// Framing 0xe1: Command from master, no reply required, repeated transmission
				cmd.msg[0] = 0xe1;
			}
		}
		return true;
	}

} // namespace delivery
} // namespace dvb
} // namespace input
