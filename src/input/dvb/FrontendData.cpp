/* FrontendData.cpp

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
#include <input/dvb/FrontendData.h>

#include <Log.h>
#include <StringConverter.h>
#include <Utils.h>

namespace input {
namespace dvb {

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================

	FrontendData::FrontendData() {
		initialize();
	}

	FrontendData::~FrontendData() {;}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void FrontendData::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		StringConverter::addFormattedString(xml, "<delsys>%s</delsys>", StringConverter::delsys_to_string(_delsys));
		StringConverter::addFormattedString(xml, "<tunefreq>%d</tunefreq>", _freq);
		StringConverter::addFormattedString(xml, "<modulation>%s</modulation>", StringConverter::modtype_to_sting(_modtype));
		StringConverter::addFormattedString(xml, "<fec>%s</fec>", StringConverter::fec_to_string(_fec));
		StringConverter::addFormattedString(xml, "<tunesymbol>%d</tunesymbol>", _srate);
		StringConverter::addFormattedString(xml, "<pidcsv>%s</pidcsv>", _pidTable.getPidCSV().c_str());

		switch (_delsys) {
			case input::InputSystem::DVBS:
			case input::InputSystem::DVBS2:
		                StringConverter::addFormattedString(xml, "<rolloff>%s</rolloff>", StringConverter::rolloff_to_sting(_rolloff));
		                StringConverter::addFormattedString(xml, "<src>%d</src>", _src);
		                StringConverter::addFormattedString(xml, "<pol>%c</pol>", getPolarizationChar());
				break;
			case input::InputSystem::DVBT:
				// Empty
				break;
			case input::InputSystem::DVBT2:
				// Empty
				break;
			case input::InputSystem::DVBC:
				// Empty
				break;
			default:
				// Do nothing here
				break;
		}
	}

	void FrontendData::fromXML(const std::string &UNUSED(xml)) {}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	void FrontendData::initialize() {
		base::MutexLock lock(_mutex);
		DeviceData::initialize();
	}

	void FrontendData::setECMInfo(
		int UNUSED(pid),
		int UNUSED(serviceID),
		int UNUSED(caID),
		int UNUSED(provID),
		int UNUSED(emcTime),
		const std::string &UNUSED(cardSystem),
		const std::string &UNUSED(readerName),
		const std::string &UNUSED(sourceName),
		const std::string &UNUSED(protocolName),
		int UNUSED(hops)) {
	}

} // namespace dvb
} // namespace input
