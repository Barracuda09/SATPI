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
#include "Log.h"

#include <stdio.h>

ChannelData::ChannelData() {
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
	resetPIDTableChanged();

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
	_pidTable.resetPid(pid);
}

uint32_t ChannelData::getPacketCounter(int pid) const {
	return _pidTable.getPacketCounter(pid);
}

void ChannelData::setDMXFileDescriptor(int pid, int fd) {
	_pidTable.setDMXFileDescriptor(pid, fd);
}

int ChannelData::getDMXFileDescriptor(int pid) const {
	return _pidTable.getDMXFileDescriptor(pid);
}

void ChannelData::closeDMXFileDescriptor(int pid) {
	_pidTable.closeDMXFileDescriptor(pid);
}

void ChannelData::resetPIDTableChanged() {
	_pidTable.resetPIDTableChanged();
}

bool ChannelData::hasPIDTableChanged() const {
	return _pidTable.hasPIDTableChanged();
}

std::string ChannelData::getPidCSV() const {
	return _pidTable.getPidCSV();
}

void ChannelData::addPIDData(int pid, uint8_t cc) {
	_pidTable.addPIDData(pid, cc);
}

void ChannelData::setPID(int pid, bool val) {
	_pidTable.setPID(pid, val);
}

bool ChannelData::isPIDUsed(int pid) const {
	return _pidTable.isPIDUsed(pid);
}

void ChannelData::setAllPID(bool val) {
	_pidTable.setAllPID(val);
}
