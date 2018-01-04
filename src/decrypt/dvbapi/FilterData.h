/* FilterData.h

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
#ifndef DECRYPT_DVBAPI_FILTERDATA_H_INCLUDE
#define DECRYPT_DVBAPI_FILTERDATA_H_INCLUDE DECRYPT_DVBAPI_FILTERDATA_H_INCLUDE

#include <mpegts/TableData.h>

#include <cstring>
#include <cstdint>

namespace decrypt {
namespace dvbapi {

	/// The class @c FilterData carries all filter data for OSCam
	class FilterData {
		public:

			// =======================================================================
			// -- Constructors and destructor ----------------------------------------
			// =======================================================================

			FilterData() {
				clear();
			}

			virtual ~FilterData() {}

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		public:

			void clear() {
				_filterActive = false;
				_pid = -1;
				std::memset(_data, 0x00, 16);
				std::memset(_mask, 0x00, 16);
				_tableData.clear();
			}

			/// Collect Table data for tableID
			void collectTableData(const int streamID, const int tableID, const unsigned char *data, bool raw) {
				_tableData.collectData(streamID, tableID, data, raw);
			}

			/// Check if Table is collected for tableID
			bool isTableCollected() const {
				return _tableData.isCollected();
			}

			/// Get the collected table data
			void getTableData(size_t secNr, mpegts::TSData &data) const {
				_tableData.getData(secNr, data);
			}

			/// Reset/Clear the collected table data
			void resetTableData() {
				_tableData.clear();
			}

			/// Is the requested pid 'active' in use for filtering
			bool active(int pid) const {
				return (_pid == pid) && _filterActive;
			}

			/// Set the requested filter data and set it active
			void set(int pid,
			         const unsigned char *data,
			         const unsigned char *mask) {
				_pid = pid;
				std::memcpy(_data, data, 16);
				std::memcpy(_mask, mask, 16);
				_filterActive = true;
			}

			/// Check if the requested data matches this filter
			bool match(const unsigned char *data) const {
				std::size_t i, k;
				bool match = true;
				const uint32_t sectionLength = (((data[6] & 0x0F) << 8) | data[7]) + 3; // 3 = tableID + length field
				for (i = 0, k = 5; i < 16 && match; i++, k++) {
					// skip sectionLength bytes
					if (k == 6) {
						k += 2;
					}
					const unsigned char mask = _mask[i];
					if (mask != 0x00) {
						const unsigned char flt = (_data[i] & mask);
						match = (k <= sectionLength) ? (flt == (data[k] & mask)) : false;
					}
				}
				// 0 = data does not match with filter, 1 = data matches with filter
				return (match && i == 16);
			}

			// =======================================================================
			//  -- Data members ------------------------------------------------------
			// =======================================================================

		protected:

			bool _filterActive;
			int _pid;
			unsigned char _data[16];
			unsigned char _mask[16];
			mpegts::TableData _tableData;
	};

} // namespace dvbapi
} // namespace decrypt

#endif // DECRYPT_DVBAPI_FILTERDATA_H_INCLUDE
