/* Filter.h

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
#ifndef DECRYPT_DVBAPI_FILTER_H_INCLUDE
#define DECRYPT_DVBAPI_FILTER_H_INCLUDE DECRYPT_DVBAPI_FILTER_H_INCLUDE

#include <Defs.h>
#include <base/Mutex.h>
#include <decrypt/dvbapi/FilterData.h>

#include <string>

namespace decrypt::dvbapi {

	/// The class @c Filter are all available filters for OSCam
	class Filter {
			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
		public:

			Filter() = default;

			virtual ~Filter() = default;

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================
		public:

			/// Start and add the requested filter
			void start(const FeID id, int pid, unsigned int demux, unsigned int filter,
					const unsigned char* filterData, const unsigned char* filterMask) {
				base::MutexLock lock(_mutex);
				if (demux < DEMUX_SIZE && filter < FILTER_SIZE) {
					_filterData[demux][filter].set(id, pid, filterData, filterMask);
				}
			}

			/// Find the correct filter for the 'collected' data or ts packet
			bool find(const FeID id, const int pid, const unsigned char* data, const int tableID,
					unsigned int& filter, unsigned int& demux, mpegts::TSData& filterData) {
				base::MutexLock lock(_mutex);
				for (demux = 0; demux < DEMUX_SIZE; ++demux) {
					for (filter = 0; filter < FILTER_SIZE; ++filter) {
						// Find filter with correct id and PID
						if (!_filterData[demux][filter].activeWith(id, pid)) {
							continue;
						}
						// Does filter matches with 'data' or already collecting
						if (_filterData[demux][filter].matchOrCollecting(data)) {
							// Collect table data
							_filterData[demux][filter].collectRawTableData(id, tableID, data, false);
							if (_filterData[demux][filter].isTableCollected()) {
								// Finished there is only 1 section
								filterData = _filterData[demux][filter].getTableData(0);
								_filterData[demux][filter].resetTableData();
								return true;
							}
						}
					}
				}
				return false;
			}

			/// Stop the requested filter
			void stop(unsigned int demux, unsigned int filter) {
				base::MutexLock lock(_mutex);
				if (demux < DEMUX_SIZE && filter < FILTER_SIZE) {
					_filterData[demux][filter].clear();
				}
			}

			void clear() {
				base::MutexLock lock(_mutex);
				for (unsigned int demux = 0; demux < DEMUX_SIZE; ++demux) {
					for (unsigned int filter = 0; filter < FILTER_SIZE; ++filter) {
						_filterData[demux][filter].clear();
					}
				}
			}

			std::vector<int> getActiveDemuxFilters() const {
				std::vector<int> index;
				for (unsigned int demux = 0; demux < DEMUX_SIZE; ++demux) {
					for (unsigned int filter = 0; filter < FILTER_SIZE; ++filter) {
						if (_filterData[demux][filter].active()) {
							index.emplace_back(demux);
							break;
						}
					}
				}
				return index;
			}

			std::vector<int> getActiveFilterPIDs(unsigned int demux) const {
				std::vector<int> pids;
				for (unsigned int filter = 0; filter < FILTER_SIZE; ++filter) {
					if (_filterData[demux][filter].active()) {
						pids.emplace_back(_filterData[demux][filter].getAssociatedPID());
					}
				}
				return pids;
			}

			// =======================================================================
			//  -- Data members ------------------------------------------------------
			// =======================================================================
		private:

			static constexpr unsigned int DEMUX_SIZE  = 25;
			static constexpr unsigned int FILTER_SIZE = 15;

			base::Mutex _mutex;
			FilterData _filterData[DEMUX_SIZE][FILTER_SIZE];
	};

}

#endif // DECRYPT_DVBAPI_FILTER_H_INCLUDE
