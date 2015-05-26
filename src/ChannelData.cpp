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
#include "StringConverter.h"
#include "Utils.h"
#include "Log.h"

#include <stdio.h>

ChannelData::ChannelData() {
	for (size_t i = 0; i < MAX_PIDS; ++i) {
		_pid.data[i].fd_dmx = -1;
		resetPid(i);
	}
	_pid.changed = false;
	initialize();
}

ChannelData::~ChannelData() {;}

void ChannelData::initialize() {
	delsys = SYS_UNDEFINED;
	freq = 0;
	ifreq = 0;
	modtype = QAM_64;
	srate = 0;
	fec = FEC_AUTO;
	rolloff = ROLLOFF_AUTO;
	inversion = INVERSION_AUTO;

	for (size_t i = 0; i < MAX_PIDS; ++i) {
		resetPid(i);
	}
	_pid.changed = false;

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
	transmission = TRANSMISSION_MODE_AUTO;
	guard = GUARD_INTERVAL_AUTO;
	hierarchy = HIERARCHY_AUTO;
	bandwidthHz = 8000000;
	plp_id = 0;
	t2_system_id = 0;
	siso_miso = 0;
}

void ChannelData::resetPid(int pid) {
	_pid.data[pid].used     = 0;
	_pid.data[pid].cc       = 0x80;
	_pid.data[pid].cc_error = 0;
	_pid.data[pid].count    = 0;
}

uint32_t ChannelData::getPacketCounter(int pid) const {
	return _pid.data[pid].count;
}

void ChannelData::setDMXFileDescriptor(int pid, int fd) {
	_pid.data[pid].fd_dmx = fd;
}

int ChannelData::getDMXFileDescriptor(int pid) const {
	return _pid.data[pid].fd_dmx;
}

void ChannelData::closeDMXFileDescriptor(int pid) {
	CLOSE_FD(_pid.data[pid].fd_dmx);
}

void ChannelData::resetPIDChanged() {
	_pid.changed = false;
}

bool ChannelData::hasPIDChanged() const {
	return _pid.changed;
}

std::string ChannelData::getPidCSV() const {
	std::string csv;
	if (_pid.data[ALL_PIDS].used) {
		csv = "all";
	} else {
		for (size_t i = 0; i < MAX_PIDS; ++i) {
			if (_pid.data[i].used) {
				StringConverter::addFormattedString(csv, "%d,", i);
			}
		}
		if (csv.size() > 1) {
			*(csv.end()-1) = 0;
		}
	}
	return csv;
}

void ChannelData::addPIDData(int pid, uint8_t cc) {
	++_pid.data[pid].count;
	if (_pid.data[pid].cc == 0x80) {
		_pid.data[pid].cc = cc;
	} else {
		++_pid.data[pid].cc;
		_pid.data[pid].cc %= 0x10;
		if (_pid.data[pid].cc != cc) {
			int diff = cc - _pid.data[pid].cc;
			if (diff < 0) {
				diff += 0x10;
			}
			_pid.data[pid].cc = cc;
			_pid.data[pid].cc_error += diff;
		}
	}
}

void ChannelData::setPID(int pid, bool val) {
	_pid.data[pid].used = val;
	_pid.changed = true;
}

bool ChannelData::isPIDUsed(int pid) const {
	return _pid.data[pid].used;
}

void ChannelData::setAllPID(bool val) {
	_pid.data[ALL_PIDS].used = val;
	_pid.changed = true;
}
