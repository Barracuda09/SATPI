/* FilterData.h

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
#ifndef DECRYPT_DVBAPI_FILTERDATA_H_INCLUDE
#define DECRYPT_DVBAPI_FILTERDATA_H_INCLUDE DECRYPT_DVBAPI_FILTERDATA_H_INCLUDE

#include <Defs.h>
#include <mpegts/TableData.h>

#include <cstring>
#include <cstdint>

namespace decrypt::dvbapi {

	/// The class @c FilterData carries all filter data for OSCam
	class FilterData {
		public:

			// =======================================================================
			// -- Constructors and destructor ----------------------------------------
			// =======================================================================

			FilterData() {
				clear();
			}

			virtual ~FilterData() = default;

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		public:

			void clear() {
				_filterActive = false;
				_pid = -1;
				_id = -1;
				_collecting = false;
				std::memset(_data, 0x00, 16);
				std::memset(_mask, 0x00, 16);
				_tableData.clear();
			}

			/// Collect Table data for tableID
			void collectRawTableData(const FeID id, const int tableID, const unsigned char* data, bool trace) {
				_tableData.collectRawData(id, tableID, data, trace);
			}

			/// Check if Table is collected for tableID
			bool isTableCollected() const {
				return _tableData.isCollected();
			}

			/// Get the collected table data
			mpegts::TSData getTableData(size_t secNr) const {
				return _tableData.getData(secNr);
			}

			/// Reset/Clear the collected table data
			void resetTableData() {
				_collecting = false;
				_tableData.clear();
			}

			/// Is the requested pid 'active' in use for filtering
			bool activeWith(const FeID id, const int pid) const {
				return (_pid == pid) && (_id == id) && _filterActive;
			}

			/// Is this filtering in use
			bool active() const {
				return _filterActive;
			}

			/// Get the associated PID of this filter
			int getAssociatedPID() const {
				return _pid;
			}

			/// Set the requested filter data and set it active
			void set(const FeID id, int pid, const unsigned char* data, const unsigned char* mask) {
				_pid = pid;
				_id = id;
				_filterActive = true;
				_collecting = false;
				_tableData.clear();
				std::memcpy(_data, data, 16);
				std::memcpy(_mask, mask, 16);
			}

			/// Check if the requested data matches this filter or if we already collecting
			bool matchOrCollecting(const unsigned char* data) const {
				if (!_collecting) {
					bool match = true;
					const uint32_t sectionLength = (((data[6] & 0x0F) << 8) | data[7]) + 3; // 3 = tableID + length field
					uint32_t i, k;
					for (i = 0, k = 5; i < 16 && match; ++i, ++k) {
						// skip sectionLength bytes
						if (k == 6) {
							k += 2;
						}
						const unsigned char mask = _mask[i];
						if (mask != 0x00) {
							const unsigned char filter = (_data[i] & mask);
							match = (k <= sectionLength) ? (filter == (data[k] & mask)) : false;
						}
					}
					_collecting = (match && i == 16);
				}
				return _collecting;
			}

			// =======================================================================
			//  -- Data members ------------------------------------------------------
			// =======================================================================

		protected:

			bool _filterActive;
			FeID _id;
			int _pid;
			unsigned char _data[16];
			unsigned char _mask[16];
			mpegts::TableData _tableData;
			mutable bool _collecting = false;
	};

}

#endif // DECRYPT_DVBAPI_FILTERDATA_H_INCLUDE
