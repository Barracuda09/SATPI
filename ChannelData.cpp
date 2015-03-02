/* ChannelData.cpp

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#include "ChannelData.h"

#include <stdio.h>

ChannelData::ChannelData() {
	delsys = SYS_UNDEFINED;
	freq = 0;
	ifreq = 0;
	modtype = QAM_64;
	srate = 0;
	fec = FEC_1_2;
	rolloff = ROLLOFF_35;
	inversion = INVERSION_AUTO;
	for (size_t i = 0; i < MAX_PIDS; ++i) {
		pid.data[i].used     = 0;
		pid.data[i].cc       = 0x80;
		pid.data[i].cc_error = 0;
		pid.data[i].count    = 0;
		pid.data[i].fd_dmx   = -1;
	}
	pid.changed = false;
	pid.all = false;

	// =======================================================================
	// DVB-S(2) Data members
	// =======================================================================
	pilot = PILOT_AUTO;
	src = 1;
	pol_v = 0;

	// =======================================================================
	// DVB-C2 Data members
	// =======================================================================
	c2tft = 0;
	data_slice = 0;
	
	// =======================================================================
	// DVB-T(2) Data members
	// =======================================================================
	transmission = TRANSMISSION_MODE_8K;
	guard = GUARD_INTERVAL_1_4;
	hierarchy = HIERARCHY_AUTO;;
	bandwidth = BANDWIDTH_8_MHZ;
	plp_id = 0;
	t2_system_id = 0;
	siso_miso = 0;
}

ChannelData::~ChannelData() {;}
