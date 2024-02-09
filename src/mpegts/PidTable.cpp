/* PidTable.cpp

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
#include <mpegts/PidTable.h>

#include <Utils.h>

#include <StringConverter.h>

namespace mpegts {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================
PidTable::PidTable() noexcept {
	for (size_t i = 0; i < MAX_PIDS; ++i) {
		resetPidData(i);
	}
	_changed = false;
	_totalCCErrors = 0;
	_totalCCErrorsBegin = 0;
	_totalCCErrorsBeginSet = false;
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================
void PidTable::clear() noexcept {
	_changed = false;
	_totalCCErrorsBegin = 0;
	_totalCCErrorsBeginSet = false;
	for (size_t i = 0; i < MAX_PIDS; ++i) {
		// Check PID still open.
		// Then set PID not used, to handle and close them later
		if (_data[i].state != State::Closed && _data[i].state != State::ShouldOpen) {
			setPID(i, false);
		} else {
			resetPidData(i);
		}
	}
}

void PidTable::resetPidData(const int pid) noexcept {
	_data[pid].state    = State::Closed;
	_data[pid].cc       = 0x80;
	_data[pid].cc_error = 0;
	_data[pid].count    = 0;
}

std::string PidTable::getPidCSV() const {
	if (_data[ALL_PIDS].state == State::Opened) {
		return "all";
	}
	std::string csv;
	for (size_t i = 0; i < MAX_PIDS; ++i) {
		if (_data[i].state == State::Opened) {
			csv += StringConverter::stringFormat("@#1,", i);
		}
	}
	if (csv.size() > 1) {
		csv.erase(csv.end() - 1);
		return csv;
	}
	return "";
}

void PidTable::setPID(const int pid, const bool use) noexcept {
	switch (_data[pid].state) {
		case State::Closed:
			if (use) {
				_data[pid].state = State::ShouldOpen;
				_changed = true;
			}
			break;
		case State::ShouldClose:
			if (use) {
				_data[pid].state = State::ShouldCloseReopen;
				_changed = true;
			}
			break;
		case State::Opened:
			if (!use) {
				_data[pid].state = State::ShouldClose;
				_changed = true;
			}
			break;
		default:
			// Nothing to do here
			break;
	}
}

void PidTable::setPIDClosed(const int pid) noexcept {
	switch (_data[pid].state) {
		case State::ShouldCloseReopen:
			_data[pid].state = State::ShouldOpen;
			_changed = true;
			break;
		default:
			_data[pid].state = State::Closed;
			break;
	}
	_data[pid].cc       = 0x80;
	_data[pid].cc_error = 0;
	_data[pid].count    = 0;
}

}
