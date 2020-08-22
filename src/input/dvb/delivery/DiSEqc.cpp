/* DiSEqc.cpp

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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
#include <StringConverter.h>

#include <cmath>
#include <string>

namespace input {
namespace dvb {
namespace delivery {

	// =======================================================================
	//  -- Constructors and destructor ---------------------------------------
	// =======================================================================
	DiSEqc::DiSEqc() :
		_diseqcRepeat(0) {}

	DiSEqc::~DiSEqc() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void DiSEqc::doAddToXML(std::string &xml) const {
		ADD_XML_NUMBER_INPUT(xml, "diseqc_repeat", _diseqcRepeat, 0, 10);
		doNextAddToXML(xml);
	}

	void DiSEqc::doFromXML(const std::string &xml) {
		std::string element;
		if (findXMLElement(xml, "diseqc_repeat.value", element)) {
			_diseqcRepeat = std::stoi(element);
		}
		doNextFromXML(xml);
	}

} // namespace delivery
} // namespace dvb
} // namespace input
