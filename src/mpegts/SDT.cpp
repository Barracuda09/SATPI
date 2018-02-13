/* SDT.cpp

   Copyright (C) 2014 - 2018 Marc Postema (mpostema09 -at- gmail.com)

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
#include <mpegts/SDT.h>
#include <Log.h>

namespace mpegts {

	// ========================================================================
	// -- Constructors and destructor -----------------------------------------
	// ========================================================================

	SDT::SDT() :
		_transportStreamID(0),
		_networkID(0) {}

	SDT::~SDT() {}

	// =======================================================================
	//  -- mpegts::TableData -------------------------------------------------
	// =======================================================================

	void SDT::clear() {
		_transportStreamID = 0;
		_networkID = 0;
		_sdtTable.clear();
		TableData::clear();
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	void SDT::parse(const int streamID) {
		for (std::size_t secNr = 0; secNr < _numberOfSections; ++secNr) {
			TableData::Data tableData;
			if (getDataForSectionNumber(secNr, tableData)) {
				const unsigned char *data = tableData.data.c_str();
				_transportStreamID = (data[ 8u] << 8u) | data[ 9u];
				_networkID         = (data[13u] << 8u) | data[14u];

//				SI_LOG_BIN_DEBUG(data, tableData.data.size(), "Stream: %d, SDT data", streamID);

				SI_LOG_INFO("Stream: %d, SDT - Section Length: %d  Transport Stream ID: %d  Version: %d  secNr: %d  lastSecNr: %d  NetworkID: %04d  CRC: 0x%04X",
							streamID, tableData.sectionLength, _transportStreamID, tableData.version, tableData.secNr, tableData.lastSecNr, _networkID, tableData.crc);

				// 4 = CRC   9 = SDT Header from section length
				const std::size_t len = tableData.sectionLength - 4u - 9u;

				// Skip to Service Description Table begin and iterate over entries
				const unsigned char *ptr = &data[16u];
				for (std::size_t i = 0u; i < len; ) {
					const int serviceID = (ptr[i + 0u] << 8u) | ptr[i + 1u];
					const int eit       =  ptr[i + 2u];
					const std::size_t descLength  = ((ptr[i + 3u] & 0x0F) << 8u) | ptr[i + 4u];
					for (std::size_t j = 5u; j < descLength; ) {
						const int id = ptr[j + i];
						switch (id) {
							case 0x48: {
								Data sdtData;
								std::size_t subLength = 0;
								j += 3;
								subLength = ptr[j + i];
								copyToUTF8(sdtData.networkNameUTF8, &ptr[j + i + 1u], subLength);
								j += subLength + 1;
								subLength = ptr[j + i];
								copyToUTF8(sdtData.channelNameUTF8, &ptr[j + i + 1u], subLength);
								j += subLength + 1;
								SI_LOG_INFO("Stream: %d,  serviceID: 0x%04X - %05d  EIT: 0x%02X  NetworkName: %s  ChannelName: %s",
										streamID, serviceID, serviceID, eit, sdtData.networkNameUTF8.c_str(), sdtData.channelNameUTF8.c_str());
								_sdtTable[serviceID] = sdtData;
								break;
							}
							default:
								// Goto next Description Info
								j += descLength;
							break;
						}
					}
					// Goto next Service Description entry
					i += descLength + 5u;
				}
			}
		}
	}

	// UTF-8 U+0080 U+07FF      yyxx xxxx    yyyyy xxxxxx    110yyyyy 10xxxxxx => UTF-8
	// Ext ASCII - E2       =>  1110 0010 => 00011 100010 => 11000011 10100010 => C3 A2
	void SDT::copyToUTF8(std::string &str, const unsigned char *ptr, const std::size_t len) {
		const std::size_t offset = (ptr[0] < 0x20) ? ((ptr[0] == 0x10) ? 2 : 1) : 0;
		for (std::size_t i = offset; i < len; ++i) {
			if ((ptr[i] & 0x80) == 0x80) {
				const unsigned char b = 0x80 |  (ptr[i] & 0x3F);
				const unsigned char c = 0xC0 | ((ptr[i] & 0xC0) >> 6);
				str.append(1, c);
				str.append(1, b);
			} else {
				str.append(1, ptr[i]);
			}
		}
	}

	bool SDT::getSDTDataFor(const int progID, SDT::Data &data) const {
		auto s = _sdtTable.find(progID);
		if (s != _sdtTable.end()) {
			data = s->second;
			return true;
		}
		data = { "Not Found", "Not Found" };
		return false;
	}

} // namespace mpegts
