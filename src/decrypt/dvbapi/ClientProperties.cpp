/* ClientProperties.cpp

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
#include <decrypt/dvbapi/ClientProperties.h>

#include <Utils.h>

extern "C" {
	#include <dvbcsa/dvbcsa.h>
}

namespace decrypt {
namespace dvbapi {

	// ===========================================================================
	// -- Constructors and destructor --------------------------------------------
	// ===========================================================================

	ClientProperties::ClientProperties() {
		_batchSize = dvbcsa_bs_batch_size();
		_batch = new dvbcsa_bs_batch_s[_batchSize + 1];
		_ts = new dvbcsa_bs_batch_s[_batchSize + 1];
		_batchCount = 0;
	}

	ClientProperties::~ClientProperties() {
		DELETE_ARRAY(_batch);
		DELETE_ARRAY(_ts);
		freeKeys();
	}

	// ===========================================================================
	// -- Other member functions -------------------------------------------------
	// ===========================================================================

	void ClientProperties::setBatchData(unsigned char *ptr, int len,
		int parity, unsigned char *originalPtr) {
		_batch[_batchCount].data = ptr;
		_batch[_batchCount].len  = len;
		_ts[_batchCount].data = originalPtr;
		_parity = parity;
		++_batchCount;
	}

	void ClientProperties::decryptBatch(bool final) {
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

	void ClientProperties::collectTableData(const int streamID, const int tableID, const unsigned char *data) {
		switch (tableID) {
			case PMT_TABLE_ID:
				_pmt.collectData(streamID, tableID, data);
				break;
			case PAT_TABLE_ID:
				_pat.collectData(streamID, tableID, data);
				break;
			default:
				// Do nothing here
				break;
		}
	}

	const unsigned char *ClientProperties::getTableData(const int tableID) const {
		switch (tableID) {
			case PMT_TABLE_ID:
				return _pmt.getData();
			case PAT_TABLE_ID:
				return _pat.getData();
			default:
				return reinterpret_cast<const unsigned char *>("");
		}
	}

	int ClientProperties::getTableDataSize(const int tableID) const {
		switch (tableID) {
			case PMT_TABLE_ID:
				return _pmt.getDataSize();
			case PAT_TABLE_ID:
				return _pat.getDataSize();
			default:
				return 0;
		}
	}

	void ClientProperties::setTableCollected(const int tableID, const bool collected) {
		switch (tableID) {
			case PMT_TABLE_ID:
				_pmt.setCollected(collected);
				break;
			case PAT_TABLE_ID:
				_pat.setCollected(collected);
				break;
			default:
				// Do nothing here
				break;
		}
	}

	bool ClientProperties::isTableCollected(const int tableID) const {
		switch (tableID) {
			case PMT_TABLE_ID:
				return _pmt.isCollected();
			case PAT_TABLE_ID:
				return _pat.isCollected();
			default:
				return false;
		}
	}

} // namespace dvbapi
} // namespace decrypt
