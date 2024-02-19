/* Frontend_DecryptInterface.cpp

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
#include <input/dvb/Frontend.h>

#include <input/dvb/FrontendData.h>

namespace input::dvb {

void Frontend::startOSCamFilterData(const int pid, const unsigned int demux, const unsigned int filter,
	const unsigned char* filterData, const unsigned char* filterMask) {
	SI_LOG_INFO("Frontend: @#1, Start filter PID: @#2  demux: @#3  filter: @#4 (data @#5 @#6 @#7 mask @#8 @#9 @#10 @#11)",
		_feID, PID(pid), demux, filter,
		HEX2(filterData[0]), HEX2(filterData[1]), HEX2(filterData[2]),
		HEX2(filterMask[0]), HEX2(filterMask[1]), HEX2(filterMask[2]), HEX2(filterMask[3]));
	_dvbapiData.startOSCamFilterData(_feID, pid, demux, filter, filterData, filterMask);
	_frontendData.getFilter().setPID(pid, true);
	// now update frontend, PID list has changed
	updatePIDFilters();
 }

void Frontend::stopOSCamFilterData(const int pid, const unsigned int demux, const unsigned int filter) {
	SI_LOG_INFO("Frontend: @#1, Stop  filter PID: @#2  demux: @#3  filter: @#4",
		_feID, PID(pid), demux, filter);
	_dvbapiData.stopOSCamFilterData(demux, filter);
	// Do not update frontend or remove the PID!
}

}
