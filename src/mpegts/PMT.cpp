/* PMT.cpp

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
#include <mpegts/PMT.h>
#include <Log.h>

namespace mpegts {

// =============================================================================
//  -- mpegts::TableData -------------------------------------------------------
// =============================================================================

void PMT::clear() {
	_programNumber = 0;
	_pcrPID = 0;
	_prgLength = 0;
	_send = false;
	_progInfo.clear();
	_elementaryPID.clear();
	_ecmPID.clear();
	TableData::clear();
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void PMT::doAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "serviceID", DIGIT(_programNumber, 6));
	for (const auto &pid : _elementaryPID) {
		ADD_XML_ELEMENT(xml, "elementaryPID", pid);
	}
	for (const auto &data : _ecmPID) {
		std::string name = "caid_" + HEX(data.caid, 4);
		ADD_XML_BEGIN_ELEMENT(xml, name);
			ADD_XML_ELEMENT(xml, "ecmPID", data.ecmpid);
			ADD_XML_ELEMENT(xml, "provid", data.provid);
		ADD_XML_END_ELEMENT(xml, name);
	}
}

void PMT::doFromXML(const std::string &UNUSED(xml)) {}

// =============================================================================
// -- Static member functions --------------------------------------------------
// =============================================================================

void PMT::cleanPI(unsigned char *data) {
	const bool payloadStart = (data[1] & 0x40) == 0x40;
	if (payloadStart && data[5] == TableData::PMT_ID) {
		const int sectionLength = ((data[6] & 0x0F) << 8) | data[7];
		const int prgLength     = ((data[15] & 0x0F) << 8) | data[16];

		mpegts::TSData pmt;
		// Copy first part to new PMT buffer
		pmt.append(&data[0], 17);

		// Clear Section Length
		pmt[6]  &= 0xF0;
		pmt[7]   = 0x00;

		// Clear Program Info Length
		pmt[15] &= 0xF0;
		pmt[16]  = 0x00;

		// 4 = CRC   9 = PMT Header from section length
		const std::size_t len = sectionLength - 4 - 9 - prgLength;

		// skip to ES Table begin and iterate over entries
		const unsigned char *ptr = &data[17 + prgLength];
		for (std::size_t i = 0; i < len; ) {
//				const int streamType    =   ptr[i + 0];
//				const int elementaryPID = ((ptr[i + 1] & 0x1F) << 8) | ptr[i + 2];
			const int esInfoLength  = ((ptr[i + 3] & 0x0F) << 8) | ptr[i + 4];
			// Append
			pmt.append(&ptr[i + 0], esInfoLength + 5);

			// goto next ES entry
			i += esInfoLength + 5;

		}
		// adjust section length --  6 = PMT Header  2 = section Length  4 = CRC
		const std::size_t newSectionLength = pmt.size() - 6 - 2 + 4;
		pmt[6] |= ((newSectionLength >> 8) & 0xFF);
		pmt[7]  =  (newSectionLength & 0xFF);

		// append calculated CRC
		const uint32_t crc = mpegts::TableData::calculateCRC32(pmt.c_str() + 5, pmt.size() - 5);
		pmt += ((crc >> 24) & 0xFF);
		pmt += ((crc >> 16) & 0xFF);
		pmt += ((crc >>  8) & 0xFF);
		pmt += ((crc >>  0) & 0xFF);

		// Stuff the rest of the packet
		for (int i = pmt.size(); i < 188; ++i) {
			pmt += 0xFF;
		}
		// copy new PMT to buffer
		memcpy(data, pmt.c_str(), 188);

		static bool once = true;
		if (once) {
			SI_LOG_BIN_DEBUG(data, 188, "Frontend: @#1, NEW PMT data", 99);
			once = false;
		}

	} else {
		// Clear PID to NULL packet
		data[1] = 0x1F;
		data[2] = 0xFF;
	}
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

int PMT::parsePCRPid() {
	Data tableData;
	if (getDataForSectionNumber(0, tableData)) {
		const unsigned char *const data = tableData.data.c_str();
		_pcrPID = ((data[13u] & 0x1F) << 8) | data[14u];
	}
	return _pcrPID;
}

void PMT::parse(const FeID id) {
	Data tableData;
	if (getDataForSectionNumber(0, tableData)) {
		const unsigned char* const data = tableData.data.c_str();
		_programNumber = ((data[ 8u]       ) << 8) | data[ 9u];
		_pcrPID        = ((data[13u] & 0x1F) << 8) | data[14u];
		_prgLength     = ((data[15u] & 0x0F) << 8) | data[16u];

//		SI_LOG_BIN_DEBUG(data, tableData.data.size(), "Frontend: @#1, PMT data", id);

		SI_LOG_INFO("Frontend: @#1, PMT - PID: @#2 - Section Length: @#3  Prog NR: @#4  Version: @#5  secNr: @#6  lastSecNr: @#7  PCR-PID: @#8  Program Length: @#9  CRC: @#10",
			id, PID(tableData.pid), DIGIT(tableData.sectionLength, 4), DIGIT(_programNumber, 5), tableData.version,
			tableData.secNr, tableData.lastSecNr, PID(_pcrPID), _prgLength, HEX(tableData.crc, 4));

		// To save the Program Info
		if (_prgLength > 0) {
			_progInfo.append(&data[17], _prgLength);
			// Parse program info
			for (std::size_t i = 0u; i < _prgLength; ) {
				const std::size_t subLength = _progInfo[i + 1u];
				// Check for Conditional access system and EMM/ECM PID
				if (_progInfo[i + 0u] == 0x09 && subLength > 0) {
					const int caid   =  (_progInfo[i + 2u] << 8u)         | _progInfo[i + 3u];
					const int ecmpid = ((_progInfo[i + 4u] & 0x1F) << 8u) | _progInfo[i + 5u];
					_ecmPID.emplace_back(ECMData{caid, ecmpid, 0});
					SB_LOG_INFO(MPEGTS_TABLES, "Frontend: @#1, PMT - CAID: @#2  ECM-PID: @#3  ES-Length: @#4",
						id, HEX(caid, 4), PID(ecmpid), DIGIT(subLength, 3));
				}
				i += subLength + 2u;
			}
		}

		// 4 = CRC   9 = PMT Header from section length
		const std::size_t len = tableData.sectionLength - 4u - 9u - _prgLength;

		// Skip to ES Table begin and iterate over entries
		const unsigned char *const ptr = &data[17u + _prgLength];
		for (std::size_t i = 0u; i < len; ) {
			const int streamType    =   ptr[i + 0u];
			const int elementaryPID = ((ptr[i + 1u] & 0x1F) << 8u) | ptr[i + 2u];
			const std::size_t esInfoLength  = ((ptr[i + 3u] & 0x0F) << 8u) | ptr[i + 4u];
			_elementaryPID.emplace_back(elementaryPID);
			SB_LOG_INFO(MPEGTS_TABLES, "Frontend: @#1, PMT - Stream Type: @#2  ES PID: @#3  ES-Length: @#4",
				id, DIGIT(streamType, 2), PID(elementaryPID), DIGIT(esInfoLength, 3));
			for (std::size_t j = 0u; j < esInfoLength; ) {
				const std::size_t subLength = ptr[j + i + 6u];
				// Check for Conditional access system and EMM/ECM PID
				if (ptr[j + i + 5u] == 9u) {
					const int caid   =  (ptr[j + i +  7u] << 8u) | ptr[j + i + 8u];
					const int ecmpid = ((ptr[j + i +  9u] & 0x1F) << 8u) | ptr[j + i + 10u];
					const int provid = ((ptr[j + i + 11u] & 0x1F) << 8u) | ptr[j + i + 12u];
					_ecmPID.emplace_back(ECMData{caid, ecmpid, provid});
					SB_LOG_INFO(MPEGTS_TABLES, "Frontend: @#1, PMT - ECM-PID - CAID: @#2  ECM-PID: @#3  PROVID: @#4 ES-Length: @#5",
						id, HEX(caid, 4), PID(ecmpid), HEX(provid, 6), DIGIT(subLength, 3));

					_progInfo.append(&ptr[j + i + 5u], subLength + 2u);
				}
				// Goto next ES Info
				j += subLength + 2u;
			}
			// Goto next ES entry
			i += esInfoLength + 5u;
		}
	}
}

}
