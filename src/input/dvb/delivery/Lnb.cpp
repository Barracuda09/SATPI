/* Lnb.cpp

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
#include <input/dvb/delivery/Lnb.h>

#include <Log.h>
#include <StringConverter.h>
#include <Utils.h>

#include <cmath>

namespace input::dvb::delivery {

	// slof: switch frequency of LNB
	#define DEFAULT_SWITCH_LOF (11700 * 1000UL)

	// lofLow: local frequency of lower LNB band
	#define DEFAULT_LOF_LOW_UNIVERSAL (9750 * 1000UL)

	// lofHigh: local frequency of upper LNB band
	#define DEFAULT_LOF_HIGH_UNIVERSAL (10600 * 1000UL)

	// Lnb standard Local oscillator frequency
	#define DEFAULT_LOF_STANDARD (10750 * 1000UL)

	// =========================================================================
	//  -- Constructors and destructor -----------------------------------------
	// =========================================================================
	Lnb::Lnb() {
		_type        = LNBType::Universal;
		_switchlof   = DEFAULT_SWITCH_LOF;
		_lofLow      = DEFAULT_LOF_LOW_UNIVERSAL;
		_lofHigh     = DEFAULT_LOF_HIGH_UNIVERSAL;
	}

	Lnb::~Lnb() {}

	// =========================================================================
	//  -- base::XMLSupport ----------------------------------------------------
	// =========================================================================

	void Lnb::doAddToXML(std::string &xml) const {
		ADD_XML_ELEMENT(xml, "lnbtype", asInteger(_type));
		ADD_XML_NUMBER_INPUT(xml, "lofSwitch", _switchlof / 1000UL, 0, 20000);
		ADD_XML_NUMBER_INPUT(xml, "lofLow", _lofLow / 1000UL, 0, 20000);
		ADD_XML_NUMBER_INPUT(xml, "lofHigh", _lofHigh / 1000UL, 0, 20000);
	}

	void Lnb::doFromXML(const std::string &xml) {
		std::string element;
		if (findXMLElement(xml, "lnbtype", element)) {
			_type = static_cast<LNBType>(std::stoi(element));
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

	// =========================================================================
	//  -- Other member functions ----------------------------------------------
	// =========================================================================

	void Lnb::getIntermediateFrequency(
			const FeID id,
			uint32_t &freq,
			bool &hiband,
			const Polarization pol) const {
		uint32_t ifreq = 0;
		if (_lofHigh > 0 && _lofHigh != _lofLow) {
			if (_switchlof > 0) {
				// Voltage controlled switch
				if (freq >= _switchlof) {
					hiband = true;
				}
				ifreq = std::abs(static_cast<long>(freq) - static_cast<long>(hiband ? _lofHigh : _lofLow));
			} else {
				// C-Band Multi-point LNB
				ifreq = std::abs(static_cast<long>(freq) - static_cast<long>(pol == Polarization::Vertical ? _lofLow : _lofHigh));
			}
		} else {
			// Mono-point LNB without switch
			ifreq = std::abs(static_cast<long>(freq) - static_cast<long>(_lofLow));
		}
		SI_LOG_DEBUG("Frontend: @#1, Using LNB with: Lof High=@#2 MHz  Lof Low=@#3 MHz  Lof Switch=@#4 MHz",
			id, _lofHigh / 1000, _lofLow / 1000, _switchlof / 1000);
		freq = ifreq;
	}

	// =========================================================================
	// -- Static member functions ----------------------------------------------
	// =========================================================================

	char Lnb::translatePolarizationToChar(Polarization pol) {
		switch (pol) {
			case Polarization::Horizontal:
				return 'h';
			case Polarization::Vertical:
				return 'v';
			case Polarization::CircularLeft:
				return 'l';
			case Polarization::CircularRight:
				return 'r';
			default:
				return 'E';
		};
	}

}

