/* DiSEqcSwitch.cpp

   Copyright (C) 2014 - 2018 Marc Postema (mpostema09 -at- gmail.com)

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

	bool DiSEqcSwitch::sendDiseqc(const int feFD, const int streamID, uint32_t &freq,
                const int src, const Lnb::Polarization pol) {
		// Digital Satellite Equipment Control, specification is available from http://www.eutelsat.com/
		struct dvb_diseqc_master_cmd cmd = {
			{0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4
		};

		// Framing 0xe0: Command from Master, No reply required, First transmission(or repeated transmission)
		// Address 0x10: Any LNB, Switcher or SMATV (Master to all...)
		// Command 0x38: Write to Port group 0 (Committed switches)
		// Data 1  0xf0: see below
		// Data 2  0x00: not used
		// Data 3  0x00: not used
		// size    0x04: send x bytes

		bool hiband = false;
		_lnb[src % MAX_LNB].getIntermediateFrequency(freq, hiband, pol);

		// param: high nibble: reset bits
		//        low nibble: set bits
		// bits are: option, position, polarizaion, band
		cmd.msg[3] =
		  0xf0 | (((src << 2) & 0x0f) | ((pol == Lnb::Polarization::Vertical) ? 0 : 2) | (hiband ? 1 : 0));

		SI_LOG_INFO("Stream: %d, Sending DiSEqC [%02x] [%02x] [%02x] [%02x]", streamID, cmd.msg[0],
				cmd.msg[1], cmd.msg[2], cmd.msg[3]);

		return diseqcSwitch(feFD, (pol == Lnb::Polarization::Vertical) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18,
				cmd, hiband ? SEC_TONE_ON : SEC_TONE_OFF, (src % 2) ? SEC_MINI_B : SEC_MINI_A);
	}

	void DiSEqcSwitch::doNextAddToXML(std::string &UNUSED(xml)) const {}

	void DiSEqcSwitch::doNextFromXML(const std::string &UNUSED(xml)) {}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool DiSEqcSwitch::diseqcSwitch(int feFD, fe_sec_voltage_t v, dvb_diseqc_master_cmd &cmd,
			fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b) {
		if (ioctl(feFD, FE_SET_TONE, SEC_TONE_OFF) == -1) {
			PERROR("FE_SET_TONE failed");
			return false;
		}
		if (ioctl(feFD, FE_SET_VOLTAGE, v) == -1) {
			PERROR("FE_SET_VOLTAGE failed");
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(15));
		if (ioctl(feFD, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1) {
			PERROR("FE_DISEQC_SEND_MASTER_CMD failed");
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		{
			// Framing 0xe1: Command from Master, No reply required, Repeated transmission
			cmd.msg[0] = 0xe1;
			for (size_t i = 0; i < _diseqcRepeat; ++i) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				if (ioctl(feFD, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1) {
					PERROR("Repeated FE_DISEQC_SEND_MASTER_CMD failed");
					return false;
				}
			}
		}
		if (ioctl(feFD, FE_DISEQC_SEND_BURST, b) == -1) {
			PERROR("FE_DISEQC_SEND_BURST failed");
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(15));
		if (ioctl(feFD, FE_SET_TONE, t) == -1) {
			PERROR("FE_SET_TONE failed");
			return false;
		}
		return true;
	}

} // namespace delivery
} // namespace dvb
} // namespace input
