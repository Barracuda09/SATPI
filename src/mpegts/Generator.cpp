/* Generator.cpp

   Copyright (C) 2014 - 2026 Marc Postema (mpostema09 -at- gmail.com)

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
#include <mpegts/Generator.h>

#include <Utils.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace mpegts {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

Generator::Generator() {
	_pat = std::make_shared<PAT>();
	_pcr = std::make_shared<PCR>();
	_pmt = std::make_shared<PMT>();
	_sdt = std::make_shared<SDT>();
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

TSData Generator::generatePSIFrom(
		FeID id, const base::M3UParser::TransformationMap &info) {
	TSData pat = _pat->generateFrom(id, info);


	return pat;
}

}
