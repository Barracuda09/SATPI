/* ClientProperties.h

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
#ifndef DECRYPT_DVBAPI_CLIENT_PROPERTIES_H_INCLUDE
#define DECRYPT_DVBAPI_CLIENT_PROPERTIES_H_INCLUDE DECRYPT_DVBAPI_CLIENT_PROPERTIES_H_INCLUDE

#include <Log.h>
#include <Utils.h>
#include <mpegts/TableData.h>
#include <base/TimeCounter.h>

#include <utility>
#include <queue>

extern "C" {
	#include <dvbcsa/dvbcsa.h>
}

namespace decrypt {
namespace dvbapi {

///
class Keys {
	public:
		typedef std::pair<long, dvbcsa_bs_key_s *> KeyPair;
		typedef std::queue<KeyPair> KeyQueue;

		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		Keys() {}

		virtual ~Keys() {}

		void set(const unsigned char *cw, int parity, int /*index*/) {
			dvbcsa_bs_key_s *k = dvbcsa_bs_key_alloc();
			dvbcsa_bs_key_set(cw, k);
			_key[parity].push(std::make_pair(base::TimeCounter::getTicks(), k));
		}

		const dvbcsa_bs_key_s *get(int parity) const {
			if (!_key[parity].empty()) {
				const KeyPair pair = _key[parity].front();
				return pair.second;
			} else {
				return nullptr;
			}
		}

		void remove(int parity) {
			const KeyPair pair = _key[parity].front();
			dvbcsa_bs_key_free(pair.second);
			_key[parity].pop();
		}

		void freeKeys() {
			while (!_key[0].empty()) {
				remove(0);
			}
			while (!_key[1].empty()) {
				remove(1);
			}
		}

	private:
		KeyQueue _key[2];
};

///
class ClientProperties {
	public:

		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		ClientProperties() {
			_batchSize = dvbcsa_bs_batch_size();
			_batch = new dvbcsa_bs_batch_s[_batchSize + 1];
			_ts = new dvbcsa_bs_batch_s[_batchSize + 1];
			_batchCount = 0;
		}
		virtual ~ClientProperties() {
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
		void decryptBatch(bool final) {
			if (_keys.get(_parity) != nullptr) {
				// terminate batch buffer
				setBatchData(nullptr, 0, _parity, nullptr);
				// decrypt it
				dvbcsa_bs_decrypt(_keys.get(_parity), _batch, 184);

				// Final, then remove this key
				if (final) {
					_keys.remove(_parity);
				}

				// clear scramble flags, so we can send it.
				unsigned int i = 0;
				while (_ts[i].data != nullptr) {
					_ts[i].data[3] &= 0x3F;
					++i;
				}
			} else {
				unsigned int i = 0;
				while (_ts[i].data != nullptr) {
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
			_keys.freeKeys();
			_batchCount = 0;
		}

		///
		void setKey(const unsigned char *cw, int parity, int index) {
			_keys.set(cw, parity, index);
		}

		///
		const dvbcsa_bs_key_s *getKey(int parity) const {
			return _keys.get(parity);
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
			} else if (getTableDataSize(tableID) > 0) {
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

	private:
		struct dvbcsa_bs_batch_s *_batch;
		struct dvbcsa_bs_batch_s *_ts;

		int         _batchSize;
		int         _batchCount;
		int         _parity;
		Keys        _keys;
		mpegts::TableData _pat;
		mpegts::TableData _pmt;

}; // class ClientProperties

} // namespace dvbapi
} // namespace decrypt

#endif // DECRYPT_DVBAPI_CLIENT_PROPERTIES_H_INCLUDE
