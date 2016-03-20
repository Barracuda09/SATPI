/* TableData.cpp

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
#include <mpegts/TableData.h>

#include <Log.h>

#include <assert.h>

namespace mpegts {

	// ========================================================================
	// -- Constructors and destructor -----------------------------------------
	// ========================================================================

	TableData::TableData() :
		_cc(-1),
		_pid(-1),
		_collected(false) {}

	TableData::~TableData() {}

	const char* TableData::getTableTXT() const {
		switch (_tableID) {
			case PAT_TABLE_ID:
				return "PAT";
			case CAT_TABLE_ID:
				return "CAT";
			case PMT_TABLE_ID:
				return "PMT";
			case ECM0_TABLE_ID:
			case ECM1_TABLE_ID:
				return "ECM";
			case EMM1_TABLE_ID:
			case EMM2_TABLE_ID:
			case EMM3_TABLE_ID:
				return "EMM";
			default:
				return "Unknown Table ID";
		}
	}

	void TableData::collectData(const int streamID, const int tableID, const unsigned char *data) {
		const int initialTableSize = getDataSize();
		const unsigned char options = (data[1] & 0xE0);
		if (options == 0x40 && data[4] == 0x00 && data[5] == tableID && initialTableSize == 0) {
			const int pid           = ((data[1] & 0x1f) << 8) | data[2];
			const int cc            =   data[3] & 0x0f;
			const int sectionLength = ((data[6] & 0x0F) << 8) | data[7];
			// Add Table Data
			if (addData(tableID, data, 188, pid, cc)) {
				if (streamID != -1) {
					SI_LOG_INFO("Stream: %d, %s - PID %d: sectionLength: %d  tableDataSize: %d", streamID, getTableTXT(), pid, sectionLength, getDataSize());
				}
				// Check did we finish collecting Table Data
				if (sectionLength <= (188 - 4 - 4)) { // 4 = TS Header  4 = CRC
					setCollected(true);
				}
			} else {
				setCollected(false);
			}
		} else if (initialTableSize > 0) {
			const unsigned char *cData = getData();
			const int sectionLength    = ((cData[6] & 0x0F) << 8) | cData[7];

			// get current PID and next CC
			const int pid = ((data[1] & 0x1f) << 8) | data[2];
			const int cc  =   data[3] & 0x0f;

			// Add Table Data without TS Header
			if (addData(tableID, &data[4], 188 - 4, pid, cc)) { // 4 = TS Header
				const int tableDataSize = getDataSize();
				if (streamID != -1) {
					SI_LOG_DEBUG("Stream: %d, %s - PID %d: sectionLength: %d  tableDataSize: %d", streamID, getTableTXT(), pid, sectionLength, tableDataSize);
				}
				// Check did we finish collecting Table Data
				if (sectionLength <= (tableDataSize - 9)) { // 9 = Untill Table Section Length
					setCollected(true);
				}
			} else {
				SI_LOG_ERROR("Stream: %d, %s - PID %d: Unable to add data! Retrying to collect data", streamID, getTableTXT(), pid);
				setCollected(false);
			}
		} else {
//			SI_LOG_ERROR("Stream: %d, %s - PID %d: Unable to add data! Retrying to collect data", streamID, getTableTXT(), pid);
			setCollected(false);
		}
	}

	bool TableData::addData(int tableID, const unsigned char *data, int length, int pid, int cc) {
		_tableID = tableID;
		if ((_cc  == -1 ||  cc == _cc + 1) &&
			(_pid == -1 || pid == _pid)) {
			if (_cc == -1 && _pid == -1) {
				_data.clear();
			}
			_data.append(reinterpret_cast<const char *>(data), length);
			_cc   = cc;
			_pid  = pid;
			return true;
		}
		return false;
	}

} // namespace mpegts
