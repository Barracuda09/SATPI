/* Filter.h

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
#ifndef DECRYPT_DVBAPI_FILTER_H_INCLUDE
#define DECRYPT_DVBAPI_FILTER_H_INCLUDE DECRYPT_DVBAPI_FILTER_H_INCLUDE

#include <base/Mutex.h>
#include <decrypt/dvbapi/FilterData.h>

#include <string>

namespace decrypt {
namespace dvbapi {

	/// The class @c Filter are all available filters for OSCam
	class Filter {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================

			Filter() :
				_collecting(false),
				_filter(-1),
				_demux(-1),
				_tableID(-1) {}

			virtual ~Filter() {}

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		public:

			void start(int pid, int demux, int filter,
			           const unsigned char *filterData, const unsigned char *filterMask) {
				base::MutexLock lock(_mutex);
				_collecting = false;
				if (demux < DEMUX_SIZE && filter < FILTER_SIZE) {
					_filterData[demux][filter].set(pid, filterData, filterMask);
				}
			}

			bool find(const int streamID, const int pid, const unsigned char *data, int &tableID, int &filter,
				int &demux, mpegts::TSData &filterData) {
				base::MutexLock lock(_mutex);
				if (_collecting) {
					// We are collecting, but is this the correct PID for this filter
					if (_filterData[_demux][_filter].active(pid)) {
						// Collect raw (true)
						_filterData[_demux][_filter].collectTableData(streamID, _tableID, data, true);
						if (_filterData[_demux][_filter].isTableCollected()) {
							filter = _filter;
							demux = _demux;
							tableID = _tableID;
							_collecting = false;
							// Because we collect raw there is only 1
							filterData = _filterData[demux][filter].getTableData(0);
							_filterData[demux][filter].resetTableData();
							return true;
						}
					}
				} else {
					_collecting = false;
					for (demux = 0; demux < DEMUX_SIZE; ++demux) {
						for (filter = 0; filter < FILTER_SIZE; ++filter) {
							if (_filterData[demux][filter].active(pid)) {
								if (_filterData[demux][filter].match(data)) {
									// Collect raw (true)
									_filterData[demux][filter].collectTableData(streamID, tableID, data, true);
									if (!_filterData[demux][filter].isTableCollected()) {
										_collecting = true;
										_filter = filter;
										_demux = demux;
										_tableID = tableID;
									} else {
										// Because we collect raw there is only 1
										filterData = _filterData[demux][filter].getTableData(0);
										_filterData[demux][filter].resetTableData();
										return true;
									}
								}
							}
						}
					}
				}
				// Did not find it yet, reset and try again later
				filter = -1;
				demux = -1;
				return false;
			}

			void stop(int demux, int filter) {
				base::MutexLock lock(_mutex);
				if (demux < DEMUX_SIZE && filter < FILTER_SIZE) {
					_filterData[demux][filter].clear();
				}
				_collecting = false;
			}

			void clear() {
				base::MutexLock lock(_mutex);
				_collecting = false;
				for (int demux = 0; demux < DEMUX_SIZE; ++demux) {
					for (int filter = 0; filter < FILTER_SIZE; ++filter) {
						_filterData[demux][filter].clear();
					}
				}
			}

			// =======================================================================
			//  -- Data members ------------------------------------------------------
			// =======================================================================

		protected:

			static constexpr int DEMUX_SIZE  = 25;
			static constexpr int FILTER_SIZE = 25;

			base::Mutex _mutex;
			FilterData _filterData[DEMUX_SIZE][FILTER_SIZE];
			bool _collecting;
			int _filter;
			int _demux;
			int _tableID;
	};

} // namespace dvbapi
} // namespace decrypt

#endif // DECRYPT_DVBAPI_FILTER_H_INCLUDE
