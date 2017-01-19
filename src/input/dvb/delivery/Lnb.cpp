/* Lnb.cpp

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
#include <input/dvb/delivery/Lnb.h>

#include <Log.h>
#include <StringConverter.h>

namespace input {
namespace dvb {
namespace delivery {

	// slof: switch frequency of LNB
	#define DEFAULT_SWITCH_LOF (11700 * 1000UL)

	// lofLow: local frequency of lower LNB band
	#define DEFAULT_LOF_LOW_UNIVERSAL (9750 * 1000UL)

	// lofHigh: local frequency of upper LNB band
	#define DEFAULT_LOF_HIGH_UNIVERSAL (10600 * 1000UL)

	// Lnb standard Local oscillator frequency
	#define DEFAULT_LOF_STANDARD (10750 * 1000UL)

	// =======================================================================
	//  -- Constructors and destructor ---------------------------------------
	// =======================================================================
	Lnb::Lnb() {
		_type        = LNBType::Universal;
		_switchlof   = DEFAULT_SWITCH_LOF;
		_lofLow      = DEFAULT_LOF_LOW_UNIVERSAL;
		_lofHigh     = DEFAULT_LOF_HIGH_UNIVERSAL;
	}

	Lnb::~Lnb() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void Lnb::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		ADD_CONFIG_NUMBER(xml, "lnbtype", _type);
		ADD_CONFIG_NUMBER_INPUT(xml, "lofSwitch", _switchlof / 1000UL, 0, 20000);
		ADD_CONFIG_NUMBER_INPUT(xml, "lofLow", _lofLow / 1000UL, 0, 20000);
		ADD_CONFIG_NUMBER_INPUT(xml, "lofHigh", _lofHigh / 1000UL, 0, 20000);
	}

	void Lnb::fromXML(const std::string &xml) {
		base::MutexLock lock(_mutex);
		std::string element;
		if (findXMLElement(xml, "lnbtype", element)) {
			;
		}
		if (findXMLElement(xml, "lofSwitch.value", element)) {
			_switchlof = std::stoi(element) * 1000UL;
		}
		if (findXMLElement(xml, "lofLow.value", element)) {
			_lofLow = std::stoi(element) * 1000UL;
		}
		if (findXMLElement(xml, "lofHigh.value", element)) {
			_lofHigh = std::stoi(element) * 1000UL;
		}
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	void Lnb::getIntermediateFrequency(uint32_t &freq,
			bool &hiband, const bool verticalPolarization) const {
		uint32_t ifreq = 0;
		if (_lofHigh > 0) {
			if (_switchlof > 0) {
				// Voltage controlled switch
				if (freq >= _switchlof) {
					hiband = true;
				}
				ifreq = abs(freq - (hiband ? _lofHigh : _lofLow));
			} else {
				// C-Band Multi-point LNB
				ifreq = abs(freq - (verticalPolarization ? _lofLow : _lofHigh));
			}
		} else {
			// Mono-point LNB without switch
			ifreq = abs(freq - _lofLow);
		}
		freq = ifreq;
	}

} // namespace delivery
} // namespace dvb
} // namespace input

