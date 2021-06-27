/* DiSEqcLnb.cpp

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
#include <input/dvb/delivery/DiSEqcLnb.h>

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
	DiSEqcLnb::DiSEqcLnb() :
		DiSEqc() {}

	DiSEqcLnb::~DiSEqcLnb() {}

	// =======================================================================
	// -- input::dvb::delivery::DiSEqc ---------------------------------------
	// =======================================================================

	bool DiSEqcLnb::sendDiseqc(const int feFD, const FeID id, uint32_t &freq,
			const int src, const Lnb::Polarization pol) {
		return diseqcLnb(feFD, id, freq, src, pol);
	}

	void DiSEqcLnb::doNextAddToXML(std::string &xml) const {
		ADD_XML_N_ELEMENT(xml, "lnb", 0, _lnb.toXML());
	}

	void DiSEqcLnb::doNextFromXML(const std::string &xml) {
		std::string element;
		if (findXMLElement(xml, "lnb0", element)) {
			_lnb.fromXML(element);
		}
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool DiSEqcLnb::diseqcLnb(const int feFD, const FeID id, uint32_t &freq,
				int src, Lnb::Polarization pol) {
		bool hiband = false;
		_lnb.getIntermediateFrequency(freq, hiband, pol);

		SI_LOG_INFO("Frontend: %d, Sending LNB: Mini-Switch Src: %d", id.getID(), src);

		if (ioctl(feFD, FE_SET_TONE, SEC_TONE_OFF) == -1) {
			PERROR("FE_SET_TONE failed");
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));

		const fe_sec_voltage_t v = (pol == Lnb::Polarization::Vertical) ?
			SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
		if (ioctl(feFD, FE_SET_VOLTAGE, v) == -1) {
			PERROR("FE_SET_VOLTAGE failed");
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));

		const fe_sec_mini_cmd_t b = (src % 2) ? SEC_MINI_B : SEC_MINI_A;
		if (ioctl(feFD, FE_DISEQC_SEND_BURST, b) == -1) {
			PERROR("FE_DISEQC_SEND_BURST failed");
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));

		const fe_sec_tone_mode_t tone = hiband ? SEC_TONE_ON : SEC_TONE_OFF;
		if (ioctl(feFD, FE_SET_TONE, tone) == -1) {
			PERROR("FE_SET_TONE failed");
			return false;
		}
		return true;
	}

} // namespace delivery
} // namespace dvb
} // namespace input
