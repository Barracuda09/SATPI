/* DiSEqc.cpp

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
#include <input/dvb/delivery/DiSEqc.h>

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
	DiSEqc::DiSEqc() :
		_diseqcRepeat(2) {}

	DiSEqc::~DiSEqc() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void DiSEqc::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		for (std::size_t i = 0u; i < MAX_LNB; ++i) {
			StringConverter::addFormattedString(xml, "<lnb%zu>", i);
			_LNB[i].addToXML(xml);
			StringConverter::addFormattedString(xml, "</lnb%zu>", i);
		}

		ADD_CONFIG_NUMBER_INPUT(xml, "diseqc_repeat", _diseqcRepeat, 1, 10);
	}

	void DiSEqc::fromXML(const std::string &xml) {
		base::MutexLock lock(_mutex);
		std::string element;
		for (std::size_t i = 0u; i < MAX_LNB; ++i) {
			std::string lnb;
			StringConverter::addFormattedString(lnb, "lnb%zu", i);
			if (findXMLElement(xml, lnb, element)) {
				_LNB[i].fromXML(element);
			}
		}

		if (findXMLElement(xml, "diseqc_repeat.value", element)) {
			_diseqcRepeat = std::stoi(element);
		}
	}

} // namespace delivery
} // namespace dvb
} // namespace input
