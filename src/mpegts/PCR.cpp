/* PCR.cpp

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
#include <mpegts/PCR.h>

#include <Unused.h>
#include <Log.h>

namespace mpegts {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

PCR::PCR() :
	_pcrPrev(0),
	_pcrDelta(0) {}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

void PCR::collectData(const FeID UNUSED(id), const unsigned char *data) {
	if (isPCRTableData(data)) {
		//        4           3          2          1          0
		// 76543210 98765432 10987654 32109876 54321098 76543210
		// xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xRRRRRRx xxxxxxxx
		// 21098765 43210987 65432109 87654321 0      8 76543210
		//   3          2          1           0               0
		//  b6       b7       b8       b9       b10      b11
		// PCR Base runs with 90KHz and PCR Ext runs with 27MHz
		#define PCR_BASE(DATA) static_cast<uint64_t>(DATA[6] << 24 | DATA[7] << 16 | DATA[8] << 8 | DATA[9]) << 1
		#define PCR_EXT(DATA)  static_cast<uint64_t>((DATA[10] & 0x01 << 8) | DATA[11])

		const uint64_t pcrCur = PCR_BASE(data);

		_pcrDelta = pcrCur - _pcrPrev;
		_pcrPrev = pcrCur;
		if (_pcrDelta < 0) {
			_pcrDelta = 1;
		}
		if (_pcrDelta > 75000) {
			_pcrDelta = 75000;
		}
		_pcrDelta *= 11;  // 11 -> PCR_Base runs with 90KHz
		#undef PCR_BASE
		#undef PCR_EXT
	}
}

std::int64_t PCR::getPCRDelta() const {
	return _pcrDelta;
}

}
