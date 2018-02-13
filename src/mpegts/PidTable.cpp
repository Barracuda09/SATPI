/* PidTable.cpp

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
#include <mpegts/PidTable.h>

#include <Utils.h>

#include <StringConverter.h>

namespace mpegts {

	PidTable::PidTable() {
		for (size_t i = 0; i < MAX_PIDS; ++i) {
			_data[i].fd_dmx = -1;
			resetPidData(i);
		}
		_changed = false;
	}

	PidTable::~PidTable() {}

	void PidTable::clear() {
		_changed = false;
		for (size_t i = 0; i < MAX_PIDS; ++i) {
			// Check still open File Descriptor.
			// Then set PID not used, to handle and close them later
			if (getDMXFileDescriptor(i) != -1) {
				setPID(i, false);
			} else {
				resetPidData(i);
			}
		}
	}

	void PidTable::resetPidData(int pid) {
		_data[pid].used        = false;
		_data[pid].shouldClose = false;
		_data[pid].cc          = 0x80;
		_data[pid].cc_error    = 0;
		_data[pid].count       = 0;
		_data[pid].pmt         = false;
		_data[pid].parity      = -1;
	}

	uint32_t PidTable::getPacketCounter(int pid) const {
		return _data[pid].count;
	}

	void PidTable::setDMXFileDescriptor(int pid, int fd) {
		_data[pid].fd_dmx = fd;
	}

	int PidTable::getDMXFileDescriptor(int pid) const {
		return _data[pid].fd_dmx;
	}

	void PidTable::closeDMXFileDescriptor(int pid) {
		CLOSE_FD(_data[pid].fd_dmx);
		const bool used = _data[pid].used;
		resetPidData(pid);
		_data[pid].used = used;
	}

	void PidTable::resetPIDTableChanged() {
		_changed = false;
	}

	bool PidTable::hasPIDTableChanged() const {
		return _changed;
	}

	std::string PidTable::getPidCSV() const {
		std::string csv;
		if (_data[ALL_PIDS].used) {
			csv = "all";
		} else {
			for (size_t i = 0; i < MAX_PIDS; ++i) {
				if (_data[i].used) {
					csv += StringConverter::stringFormat("%1,", i);
				}
			}
			if (csv.size() > 1) {
				csv.erase(csv.end() - 1);
			} else {
				csv = " ";
			}
		}
		return csv;
	}

	void PidTable::addPIDData(int pid, uint8_t cc) {
		++_data[pid].count;
		if (_data[pid].cc == 0x80) {
			_data[pid].cc = cc;
		} else {
			++_data[pid].cc;
			_data[pid].cc %= 0x10;
			if (_data[pid].cc != cc) {
				int diff = cc - _data[pid].cc;
				if (diff < 0) {
					diff += 0x10;
				}
				_data[pid].cc = cc;
				_data[pid].cc_error += diff;
			}
		}
	}

	void PidTable::setPID(int pid, bool use) {
		_data[pid].used = use;
		// Check PID not used anymore, so set "shouldClose" flag
		if (!use && getDMXFileDescriptor(pid) != -1) {
			_data[pid].shouldClose = true;
		}
		_changed = true;
	}

	bool PidTable::shouldPIDClose(int pid) const {
		return _data[pid].shouldClose;
	}

	bool PidTable::isPIDUsed(int pid) const {
		return _data[pid].used;
	}

	void PidTable::setAllPID(bool use) {
		setPID(ALL_PIDS, use);
	}

	void PidTable::markAsPMT(int pid, bool set) {
		_data[pid].pmt = set;
	}

	bool PidTable::isMarkedAsPMT(int pid) const {
		return _data[pid].pmt;
	}

	void PidTable::setKeyParity(int pid, int parity) {
		_data[pid].parity = parity;
	}

	int PidTable::getKeyParity(int pid) const {
		return _data[pid].parity;
	}

} // namespace mpegts
