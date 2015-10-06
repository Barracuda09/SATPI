/* DvbapiClientProperties.h

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef DVB_API_CLIENT_PROPERTIES_H_INCLUDE
#define DVB_API_CLIENT_PROPERTIES_H_INCLUDE

#include "TSTableData.h"
#include "Log.h"

#ifdef LIBDVBCSA
extern "C" {
	#include <dvbcsa/dvbcsa.h>
}
#endif

class DvbapiClientProperties {
#define MAX_PIDS 8193
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		DvbapiClientProperties() {
			_key[0] = NULL;
			_key[1] = NULL;
			for (size_t i = 0; i < MAX_PIDS; ++i) {
				_pidTable[i].pmt = false;
				_pidTable[i].ecm = false;
				_pidTable[i].demux = -1;
				_pidTable[i].filter = -1;
				_pidTable[i].parity = -1;
			}
			_batchSize = dvbcsa_bs_batch_size();
		}
		virtual ~DvbapiClientProperties() {
			freeKeys();
		}

		// PID and DMX file descriptor
		typedef struct {
			bool     pmt;            // show if this is an PMT pid
			bool     ecm;            // show if this is an ECM pid
			int      demux;          // dvbapi demux
			int      filter;         // dvbapi filter
			int      parity;         // key parity
		} PidTable_t;

		///
		int getBatchSize() const {
			return _batchSize;
		}

		///
		void freeKeys() {
			dvbcsa_bs_key_free(_key[0]);
			dvbcsa_bs_key_free(_key[1]);
			_key[0] = NULL;
			_key[1] = NULL;
		}

		///
		void setKey(const unsigned char *cw, int parity, int /*index*/) {
			dvbcsa_bs_key_set(cw, _key[parity]);
		}

		///
		dvbcsa_bs_key_s *getKey(int parity) const {
			return _key[parity];
		}

		///
		void setPMT(int pid, bool set) {
			_pidTable[pid].pmt = set;
		}

		///
		bool isPMT(int pid) const {
			return _pidTable[pid].pmt;
		}

		///
		void setECMFilterData(int demux, int filter, int pid, bool set) {
			if(!_key[0]) {
				_key[0] = dvbcsa_bs_key_alloc();
			}
			if(!_key[1]) {
				_key[1] = dvbcsa_bs_key_alloc();
			}
			_pidTable[pid].ecm    = set;
			_pidTable[pid].demux  = demux;
			_pidTable[pid].filter = filter;
		}

		///
		void getECMFilterData(int &demux, int &filter, int pid) const {
			demux  = _pidTable[pid].demux;
			filter = _pidTable[pid].filter;
		}

		///
		bool isECM(int pid) const {
			return _pidTable[pid].ecm;
		}

		///
		void setKeyParity(int pid, int parity) {
			_pidTable[pid].parity = parity;
		}

		///
		int getKeyParity(int pid) const {
			return _pidTable[pid].parity;
		}

		const char* getTableTXT(int tableID) {
			switch (tableID) {
				case PAT_TABLE_ID:
					return "PAT";
				case PMT_TABLE_ID:
					return "PMT";
				default:
					return "Unknown Table";
			}
		}

		/// Collect Table data for tableID
		void collectTableData(int streamID, int tableID, const unsigned char *data) {
			const unsigned char options = (data[1] & 0xE0);
			if (options == 0x40 && data[5] == tableID) {
				const int pid           = ((data[1] & 0x1f) << 8) | data[2];
				const int cc            =   data[3] & 0x0f;
				const int sectionLength = ((data[6] & 0x0F) << 8) | data[7];
				// Add Table Data
				if (addTableData(tableID, data, 188, pid, cc)) {
					SI_LOG_DEBUG("Stream: %d, %s: sectionLength: %d  tableDataSize: %d", streamID, getTableTXT(tableID), sectionLength, getTableDataSize(tableID));
					// Check did we finish collecting Table Data
					if (sectionLength <= 180) {
						setTableCollected(tableID, true);
					}
				} else {
					SI_LOG_ERROR("Stream: %d, %s: Unable to add data!", streamID, getTableTXT(tableID));
				}
			} else {
				const unsigned char *cData = getTableData(tableID);
				const int sectionLength    = ((cData[6] & 0x0F) << 8) | cData[7];

				// get current PID and next CC
				const int pid = ((data[1] & 0x1f) << 8) | data[2];
				const int cc  =   data[3] & 0x0f;

				// Add Table Data without TS Header
				if (addTableData(tableID, &data[4], 188 - 4, pid, cc)) { // 4 = TS Header
					const int tableDataSize = getTableDataSize(tableID);
					SI_LOG_DEBUG("Stream: %d, %s: sectionLength: %d  tableDataSize: %d", streamID, getTableTXT(tableID), sectionLength, tableDataSize);
					// Check did we finish collecting Table Data
					if (sectionLength <= (tableDataSize - 9)) { // 9 = Untill Table Section Length
						setTableCollected(tableID, true);
					}
				} else {
					SI_LOG_ERROR("Stream: %d, %s: Unable to add data!", streamID, getTableTXT(tableID));
				}
			}
		}

		/// Add Table data for tableID that was collected
		bool addTableData(int tableID, const unsigned char *data, int length, int pid, int cc) {
			switch (tableID) {
				case PAT_TABLE_ID:
					return _pat.addData(tableID, data, length, pid, cc);
				case PMT_TABLE_ID:
					return _pmt.addData(tableID, data, length, pid, cc);
				default:
					return false;
			}
		}

		/// Get the collected Table Data for tableID
		const unsigned char *getTableData(int tableID) const {
			switch (tableID) {
				case PAT_TABLE_ID:
					return _pat.getData();
				case PMT_TABLE_ID:
					return _pmt.getData();
				default:
					return reinterpret_cast<const unsigned char *>("");
			}
		}

		/// Get the current size of the Table data for tableID !!Whitout the CRC (4Bytes)!!
		int getTableDataSize(int tableID) const {
			switch (tableID) {
				case PAT_TABLE_ID:
					return _pat.getDataSize();
				case PMT_TABLE_ID:
					return _pmt.getDataSize();
				default:
					return 0;
			}
		}

		/// Set if Table has been collected (or not) for tableID
		void setTableCollected(int tableID, bool collected) {
			switch (tableID) {
				case PAT_TABLE_ID:
					_pat.setCollected(collected);
					break;
				case PMT_TABLE_ID:
					_pmt.setCollected(collected);
					break;
				default:
					break;
			}
		}

		/// Check if Table is collected for tableID
		bool isTableCollected(int tableID) const {
			switch (tableID) {
				case PAT_TABLE_ID:
					return _pat.isCollected();
				case PMT_TABLE_ID:
					return _pmt.isCollected();
				default:
					return false;
			}
		}

	private:
		struct dvbcsa_bs_key_s *_key[2];
		int _batchSize;
		PidTable_t _pidTable[MAX_PIDS];
		int _parity;

		TSTableData _pat;
		TSTableData _pmt;

}; // class DvbapiClientProperties

#endif // DVB_API_CLIENT_PROPERTIES_H_INCLUDE