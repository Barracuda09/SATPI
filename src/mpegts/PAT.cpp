/* PAT.cpp

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
#include <mpegts/PAT.h>
#include <Log.h>
#include <Unused.h>

#include <array>

namespace mpegts {

// =============================================================================
//  -- mpegts::TableData -------------------------------------------------------
// =============================================================================

void PAT::clear() noexcept {
	_tid = 0;
	_pmtPidTable.clear();
	TableData::clear();
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void PAT::doAddToXML(std::string &xml) const {
	for (const auto& [pid, _] : _pmtPidTable) {
		ADD_XML_ELEMENT(xml, "pmt", pid);
	}
}

void PAT::doFromXML(const std::string &UNUSED(xml)) {}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

void PAT::parse(const FeID id) {
	Data tableData;
	if (getDataForSectionNumber(0, tableData)) {
		const unsigned char* data = tableData.data.data();
		_tid =  (data[8u] << 8) | data[9u];

//		SI_LOG_BIN_DEBUG(data, tableData.data.size(), "Frontend: @#1, PAT data", id);

		SI_LOG_INFO("Frontend: @#1, PAT - Section Length: @#2  TID: @#3  Version: @#4  secNr: @#5 lastSecNr: @#6  CRC: @#7",
			id, DIGIT(tableData.sectionLength, 4), _tid, tableData.version, tableData.secNr, tableData.lastSecNr, HEX(tableData.crc, 4));

		// 4 = CRC  5 = PAT Table begin from section length
		const size_t len = tableData.sectionLength - 4u - 5u;

		// skip to Table begin and iterate over entries
		const unsigned char* ptr = &data[13u];
		for (size_t i = 0u; i < len; i += 4u) {
			// Get PAT entry
			const int prognr =  (ptr[i + 0u] << 8) | ptr[i + 1u];
			const int pid    = ((ptr[i + 2u] & 0x1F) << 8) | ptr[i + 3u];
			if (prognr == 0u) {
				SB_LOG_INFO(MPEGTS_TABLES, "Frontend: @#1, PAT: Prog NR: @#2 - @#3  NIT PID: @#4",
					id, HEX(prognr, 4), DIGIT(prognr, 5), DIGIT(pid, 4));
			} else {
				SB_LOG_INFO(MPEGTS_TABLES, "Frontend: @#1, PAT: Prog NR: @#2 - @#3  PMT PID: @#4",
					id, HEX(prognr, 4), DIGIT(prognr, 5), DIGIT(pid, 4));
				_pmtPidTable[pid] = true;
			}
		}
	}
}

mpegts::TSData PAT::generateFrom(
		FeID UNUSED(id), const base::M3UParser::TransformationMap& UNUSED(info)) {
	static int cc = 0;

	int version = 5;
	int currIndicator = 1;
	int pid = 0; // for PAT
//	int payloadStart = 1;
	int payloadOnly = 1;
	int scrambled = 0;
	int transportStreamID = 0;

	std::array<unsigned char, 188> tmp;
	tmp[0]  = 0x47;
	tmp[1]  = 0x40 | (pid & 0x1F) >> 8;
	tmp[2]  = (pid & 0xFF);
	tmp[3]  = (scrambled & 0x3) << 6 | (payloadOnly & 0x3) << 4 | (cc & 0xF);
	tmp[4]  = 0x00; // P1
	tmp[5]  = TableData::PAT_ID;
	tmp[6]  = 0x00; // Length
	tmp[7]  = 0x00; // Length
	tmp[8]  = (transportStreamID & 0xFF00) >> 8; // TID
	tmp[9]  = (transportStreamID & 0x00FF);      // TID
	tmp[10] = (0xC0 | ((version & 0x1F) << 1) | (currIndicator & 0x01));
	tmp[11] = 0x00; // section number
	tmp[12] = 0x00; // last section number

	++cc;
	cc %= 0x10;

	int index = 13;

	// First program is NIT
	tmp[index + 0] = 0x00; //
	tmp[index + 1] = 0x00; //
	tmp[index + 2] = 0x00; //
	tmp[index + 3] = 0x10; //
	index += 4;

/*
	int prognr = 0x4000;
	int pmtPid = 0x0100;
	for (const auto& [freq, element] : info) {
//		SI_LOG_DEBUG("Frontend: @#1, Generating PAT: TID: @#2  Prog NR: @#3 - @#4  PMT PID: @#5  CC: @#6",
//			id, transportStreamID, HEX(prognr, 4), DIGIT(prognr, 5), element.freq, cc);
		tmp[index + 0] = (prognr & 0xFF00) >> 8;
		tmp[index + 1] = (prognr & 0x00FF);
		tmp[index + 2] = (pmtPid & 0x1F00) >> 8; //
		tmp[index + 3] = (pmtPid & 0x00FF); //
		index += 4;

		// Increment program and pid
		++prognr;
		pmtPid += 0x10;
	}
*/
	// Adjust lenght
	int len = index - 8 + 4;
	tmp[6]  = (len & 0xFF00) >> 8;
	tmp[7]  = len & 0xFF;

	// append calculated CRC
	const uint32_t crc = mpegts::TableData::calculateCRC32(&tmp[5], len - 4 + 3);
	tmp[index + 0] = ((crc >> 24) & 0xFF);
	tmp[index + 1] = ((crc >> 16) & 0xFF);
	tmp[index + 2] = ((crc >>  8) & 0xFF);
	tmp[index + 3] = ((crc >>  0) & 0xFF);
	index += 4;

	// Stuffing
	std::memset(&tmp[index], 0xFF, tmp.size() - index);

	return TSData(tmp.data(), tmp.size());
}

}
