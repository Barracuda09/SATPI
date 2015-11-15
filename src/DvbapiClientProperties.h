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
#define DVB_API_CLIENT_PROPERTIES_H_INCLUDE DVB_API_CLIENT_PROPERTIES_H_INCLUDE

#include "TSTableData.h"
#include "Log.h"
#include "Utils.h"

extern "C" {
	#include <dvbcsa/dvbcsa.h>
}

class DvbapiClientProperties {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		DvbapiClientProperties() {
			_key[0] = NULL;
			_key[1] = NULL;
			_batchSize = dvbcsa_bs_batch_size();
			_batch = new dvbcsa_bs_batch_s[_batchSize + 1];
			_ts = new dvbcsa_bs_batch_s[_batchSize + 1];
			_batchCount = 0;
		}
		virtual ~DvbapiClientProperties() {
			DELETE_ARRAY(_batch);
			DELETE_ARRAY(_ts);
			freeKeys();
		}

		///
		int getMaximumBatchSize() const {
			return _batchSize;
		}

		///
		int getBatchCount() const {
			return _batchCount;
		}

		///
		int getBatchParity() const {
			return _parity;
		}

		/// Set the pointers into the batch
		/// @param ptr specifies the pointer to de data that should be decrypted
		/// @param len specifies the lenght of data
		/// @param originalPtr specifies the original TS packet (so we can clear scramble flag when finished)
		void setBatchData(unsigned char *ptr, int len, int parity, unsigned char *originalPtr) {
			_batch[_batchCount].data = ptr;
			_batch[_batchCount].len  = len;
			_ts[_batchCount].data = originalPtr;
			_parity = parity;
			++_batchCount;
		}

		/// This function will decrypt the batch upon success it will clear scramble flag
		/// on failure it will make a NULL TS Packet and clear scramble flag
		void decryptBatch() {
			if (_key[_parity] != NULL) {
				// terminate batch buffer
				setBatchData(NULL, 0, _parity, NULL);
				// decrypt it
				dvbcsa_bs_decrypt(_key[_parity], _batch, 184);

				// clear scramble flags, so we can send it.
				unsigned int i = 0;
				while (_ts[i].data != NULL) {
					_ts[i].data[3] &= 0x3F;
					++i;
				}
			} else {
				unsigned int i = 0;
				while (_ts[i].data != NULL) {
					// set decrypt failed by setting NULL packet ID..
					_ts[i].data[1] |= 0x1F;
					_ts[i].data[2] |= 0xFF;

					// clear scramble flag, so we can send it.
					_ts[i].data[3] &= 0x3F;
					++i;
				}
			}
			// decrypted this batch reset counter
			_batchCount = 0;
		}

		///
		void freeKeys() {
			dvbcsa_bs_key_free(_key[0]);
			dvbcsa_bs_key_free(_key[1]);
			_key[0] = NULL;
			_key[1] = NULL;

			_batchCount = 0;
		}

		///
		void setKey(const unsigned char *cw, int parity, int /*index*/) {
			dvbcsa_bs_key_set(cw, _key[parity]);
		}

		///
		dvbcsa_bs_key_s *getKey(int parity) const {
			return _key[parity];
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
					SI_LOG_DEBUG("Stream: %d, %s - PID %d: sectionLength: %d  tableDataSize: %d", streamID, getTableTXT(tableID), pid, sectionLength, getTableDataSize(tableID));
					// Check did we finish collecting Table Data
					if (sectionLength <= (188 - 4 - 4)) { // 4 = TS Header  4 = CRC
						setTableCollected(tableID, true);
					}
				} else {
					SI_LOG_ERROR("Stream: %d, %s - PID %d: Unable to add data! Retrying to collect data", streamID, getTableTXT(tableID), pid);
					setTableCollected(tableID, false);
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
					SI_LOG_DEBUG("Stream: %d, %s - PID %d: sectionLength: %d  tableDataSize: %d", streamID, getTableTXT(tableID), pid, sectionLength, tableDataSize);
					// Check did we finish collecting Table Data
					if (sectionLength <= (tableDataSize - 9)) { // 9 = Untill Table Section Length
						setTableCollected(tableID, true);
					}
				} else {
					SI_LOG_ERROR("Stream: %d, %s - PID %d: Unable to add data! Retrying to collect data", streamID, getTableTXT(tableID), pid);
					setTableCollected(tableID, false);
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

	protected:
		struct dvbcsa_bs_key_s   *_key[2];
		struct dvbcsa_bs_batch_s *_batch;
		struct dvbcsa_bs_batch_s *_ts;

		int _batchSize;
		int _batchCount;
		int _parity;

		TSTableData _pat;
		TSTableData _pmt;

}; // class DvbapiClientProperties

#endif // DVB_API_CLIENT_PROPERTIES_H_INCLUDE