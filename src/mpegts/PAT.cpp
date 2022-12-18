/* PAT.cpp

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
#include <mpegts/PAT.h>
#include <Log.h>

namespace mpegts {

// =============================================================================
//  -- mpegts::TableData -------------------------------------------------------
// =============================================================================

void PAT::clear() {
	_tid = 0;
	_pmtPidTable.clear();
	TableData::clear();
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

void PAT::parse(const FeID id) {
	Data tableData;
	if (getDataForSectionNumber(0, tableData)) {
		const unsigned char *data = tableData.data.c_str();
		_tid =  (data[8u] << 8) | data[9u];

//		SI_LOG_BIN_DEBUG(data, tableData.data.size(), "Frontend: @#1, PAT data", id);

		SI_LOG_INFO("Frontend: @#1, PAT: Section Length: @#2  TID: @#3  Version: @#4  secNr: @#5 lastSecNr: @#6  CRC: @#7",
			id, tableData.sectionLength, _tid, tableData.version, tableData.secNr, tableData.lastSecNr, HEX(tableData.crc, 4));

		// 4 = CRC  5 = PAT Table begin from section length
		const size_t len = tableData.sectionLength - 4u - 5u;

		// skip to Table begin and iterate over entries
		const unsigned char *ptr = &data[13u];
		for (size_t i = 0u; i < len; i += 4u) {
			// Get PAT entry
			const int prognr =  (ptr[i + 0u] << 8) | ptr[i + 1u];
			const int pid    = ((ptr[i + 2u] & 0x1F) << 8) | ptr[i + 3u];
			if (prognr == 0u) {
				SI_LOG_INFO("Frontend: @#1, PAT: Prog NR: @#2 - @#3  NIT PID: @#4",
					id, HEX(prognr, 4), DIGIT(prognr, 5), DIGIT(pid, 4));
			} else {
				SI_LOG_INFO("Frontend: @#1, PAT: Prog NR: @#2 - @#3  PMT PID: @#4",
					id, HEX(prognr, 4), DIGIT(prognr, 5), DIGIT(pid, 4));
				_pmtPidTable[pid] = true;
			}
		}
	}
}

bool PAT::isMarkedAsPMT(int pid) const {
	const auto s = _pmtPidTable.find(pid);
	if (s != _pmtPidTable.end() && s->second) {
		return true;
	}
	return false;
}

mpegts::TSData PAT::generateFrom(
		FeID id, const base::M3UParser::TransformationMap &info) {
	mpegts::TSData data;

	int transportStreamID = 0;
	int prognr = 0;
#ifndef NEED_BACKPORT
	for (auto [freq, element] : info) {
#else
	for (auto entry : info) {
		auto freq    = entry.first;
		auto element = entry.second;
		(void) freq; /* unused */
#endif
		SI_LOG_DEBUG("Frontend: @#1, Generating PAT: TID: @#2  Prog NR: @#3 - @#4  PMT PID: @#5",
			id, transportStreamID, HEX(prognr, 4), DIGIT(prognr, 5), element.freq);
	}
	return data;
}

} // namespace
