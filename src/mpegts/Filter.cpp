/* Filter.cpp

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
#include <mpegts/Filter.h>

#include <Utils.h>
#include <StringConverter.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


namespace mpegts {

	Filter::Filter() {}

	Filter::~Filter() {}

	void Filter::clear(int streamID) {
		SI_LOG_INFO("Stream: %d, Clearing PAT/PMT/SDT Tables...", streamID);
		_pat.clear();
		_pmt.clear();
		_sdt.clear();
	}

	void Filter::addData(const int streamID, const unsigned char *ptr) {
		const uint16_t pid = ((ptr[1] & 0x1f) << 8) | ptr[2];
		if (pid == 0) {
			if (!_pat.isCollected()) {
				// collect PAT data
				_pat.collectData(streamID, PAT_TABLE_ID, ptr, false);

				// Did we finish collecting PAT
				if (_pat.isCollected()) {
					_pat.parse(streamID);
				}
			}
		} else if (_pat.isMarkedAsPMT(pid)) {
			if (!_pmt.isCollected()) {

				{
					char fileFIFO[] = "/tmp/fifo";
					int fd = ::open(fileFIFO, O_WRONLY | O_NONBLOCK);
					if (fd > 0) {
						::write(fd, ptr, 188);
						::close(fd);
					}
				}

				// collect PMT data
				_pmt.collectData(streamID, PMT_TABLE_ID, ptr, false);

				// Did we finish collecting PMT
				if (_pmt.isCollected()) {
					_pmt.parse(streamID);
				}
			}
		} else if (pid == 17) {
			if (!_sdt.isCollected()) {
				// collect SDT data
				_sdt.collectData(streamID, SDT_TABLE_ID, ptr, false);

				// Did we finish collecting SDT
				if (_sdt.isCollected()) {
					_sdt.parse(streamID);
				}
			}
		} else if (pid == 20) {
			{
				char fileFIFO[] = "/tmp/fifo";
				int fd = ::open(fileFIFO, O_WRONLY | O_NONBLOCK);
				if (fd > 0) {
					::write(fd, ptr, 188);
					::close(fd);
				}
			}
			const unsigned int tableID = ptr[5u];
			const unsigned int mjd = (ptr[8u] << 8) | (ptr[9u]);
			const unsigned int y1 = static_cast<unsigned int>((mjd - 15078.2) / 365.25);
			const unsigned int m1 = static_cast<unsigned int>((mjd - 14956.1 - static_cast<unsigned int>(y1 * 365.25)) / 30.6001);
			const unsigned int d = static_cast<unsigned int>(mjd - 14956.0 - static_cast<unsigned int>(y1 * 365.25) - static_cast<unsigned int>(m1 * 30.6001 ));
			const unsigned int k = (m1 == 14 || m1 ==15) ? 1 : 0;
			const unsigned int y = y1 + k + 1900;
			const unsigned int m = m1 - 1 - (k * 12);
			const unsigned int h = ptr[10u];
			const unsigned int mi = ptr[11u];
			const unsigned int s = ptr[12u];

			SI_LOG_INFO("Stream: %d, TDT - Table ID: 0x%02X  Date: %d-%d-%d  Time: %02X:%02X.%02X  MJD: 0x%04X", streamID, tableID, y, m, d, h, mi, s, mjd);
//			SI_LOG_BIN_DEBUG(ptr, 188, "Stream: %d, TDT - ", _streamID);


		}
	}

} // namespace mpegts
