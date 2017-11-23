/* PMT.cpp

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
#include <mpegts/PMT.h>
#include <Log.h>

namespace mpegts {

	// ========================================================================
	// -- Constructors and destructor -----------------------------------------
	// ========================================================================

	PMT::PMT() :
		_programNumber(0),
		_pcrPID(0),
		_prgLength(0),
		_send(false) {}

	PMT::~PMT() {}

	// =======================================================================
	//  -- mpegts::TableData -------------------------------------------------
	// =======================================================================

	void PMT::clear() {
		_programNumber = 0;
		_pcrPID = 0;
		_prgLength = 0;
		_send = false;
		_progInfo.clear();
		TableData::clear();
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	void PMT::parse(const int streamID) {
		Data tableData;
		if (getDataForSectionNumber(0, tableData)) {
			const unsigned char *data = tableData.data.c_str();
			_programNumber = ((data[ 8u]       ) << 8) | data[ 9u];
			_pcrPID        = ((data[13u] & 0x1F) << 8) | data[14u];
			_prgLength     = ((data[15u] & 0x0F) << 8) | data[16u];

//			SI_LOG_BIN_DEBUG(data, tableData.data.size(), "Stream: %d, PMT data", streamID);

			SI_LOG_INFO("Stream: %d, PMT - Section Length: %d  Prog NR: %05d  Version: %d  secNr: %d  lastSecNr: %d  PCR-PID: %04d  Program Length: %d  CRC: 0x%04X",
						streamID, tableData.sectionLength, _programNumber, tableData.version, tableData.secNr, tableData.lastSecNr, _pcrPID, _prgLength, tableData.crc);

			// To save the Program Info
			if (_prgLength > 0) {
				_progInfo.append(&data[17], _prgLength);
			}

			// 4 = CRC   9 = PMT Header from section length
			const std::size_t len = tableData.sectionLength - 4u - 9u - _prgLength;

			// Skip to ES Table begin and iterate over entries
			const unsigned char *ptr = &data[17u + _prgLength];
			for (std::size_t i = 0u; i < len; ) {
				const int streamType    =   ptr[i + 0u];
				const int elementaryPID = ((ptr[i + 1u] & 0x1F) << 8u) | ptr[i + 2u];
				const std::size_t esInfoLength  = ((ptr[i + 3u] & 0x0F) << 8u) | ptr[i + 4u];

				SI_LOG_INFO("Stream: %d, PMT - Stream Type: %d  ES PID: %04d  ES-Length: %d",
							streamID, streamType, elementaryPID, esInfoLength);
				for (std::size_t j = 0u; j < esInfoLength; ) {
					const std::size_t subLength = ptr[j + i + 6u];
					// Check for Conditional access system and EMM/ECM PID
					if (ptr[j + i + 5u] == 9u) {
						const int caid   =  (ptr[j + i +  7u] << 8u) | ptr[j + i + 8u];
						const int ecmpid = ((ptr[j + i +  9u] & 0x1F) << 8u) | ptr[j + i + 10u];
						const int provid = ((ptr[j + i + 11u] & 0x1F) << 8u) | ptr[j + i + 12u];
						SI_LOG_INFO("Stream: %d, ECM-PID - CAID: 0x%04X  ECM-PID: %04d  PROVID: %05d ES-Length: %d",
									streamID, caid, ecmpid, provid, subLength);

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

} // namespace mpegts
