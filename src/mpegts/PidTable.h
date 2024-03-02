/* PidTable.h

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
#ifndef MPEGTS_PIDTABLE_H_INCLUDE
#define MPEGTS_PIDTABLE_H_INCLUDE MPEGTS_PIDTABLE_H_INCLUDE

#include <cstdint>
#include <string>

namespace mpegts {

/// The class @c PidTable carries all the PID and DMX information
class PidTable {
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		PidTable() noexcept;

		virtual ~PidTable() = default;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		/// Clear all PID data
		void clear() noexcept;

		/// Reset that PID has changed
		void resetPIDTableChanged() noexcept {
			_changed = false;
		}

		/// Check if the PID has changed
		bool hasPIDTableChanged() const noexcept {
			return _changed;
		}

		/// Get the amount of packet that were received of this pid
		uint32_t getPacketCounter(const int pid) const noexcept {
			return _data[pid].count;
		}

		/// Get the amount Continuity Counter Error of this pid
		uint32_t getCCErrors(const int pid) const noexcept {
			return _data[pid].cc_error;
		}

		/// Get the total amount of Continuity Counter Error
		uint32_t getTotalCCErrors() const noexcept {
			return _totalCCErrors - _totalCCErrorsBegin;
		}

		/// Get the CSV of all the requested PID
		std::string getPidCSV() const;

		/// Set the continuity counter for pid
		void addPIDData(const int pid, const uint8_t ccByte) noexcept {
			PidData &data = _data[pid];
			++data.count;
			// Only if it has a Payload
			if ((ccByte & 0x10) == 0x10) {
				const uint8_t cc = ccByte & 0x0F;
				if (data.cc == 0x80) {
					data.cc = cc;
					if (!_totalCCErrorsBeginSet) {
						_totalCCErrorsBegin = _totalCCErrors;
						_totalCCErrorsBeginSet = true;
					}
					return;
				}
				++data.cc;
				data.cc %= 0x10;
				if (data.cc != cc) {
					const uint8_t diff = (cc >= data.cc) ? (cc - data.cc) : ((0x10 - data.cc) + cc);
					data.cc = cc;
					data.cc_error += diff;
					_totalCCErrors += diff;
				}
			}
		}

		/// Set pid used or not
		void setPID(int pid, bool use) noexcept;

		/// Check if this pid is opened
		bool isPIDOpened(const int pid) const noexcept {
			return _data[pid].state == State::Opened;
		}

		/// Check if this pid should be closed
		bool shouldPIDClose(const int pid) const noexcept {
			return _data[pid].state == State::ShouldClose ||
				_data[pid].state == State::ShouldCloseReopen;
		}

		/// Set that this pid is closed
		void setPIDClosed(const int pid) noexcept;

		/// Check if PID should be opened
		bool shouldPIDOpen(const int pid) const noexcept {
			return _data[pid].state == State::ShouldOpen;
		}

		/// Set that this pid is opened
		void setPIDOpened(const int pid) noexcept {
			_data[pid].state = State::Opened;
		}

		/// Set all PID
		void setAllPID(const bool use) noexcept {
			setPID(ALL_PIDS, use);
		}

		/// Check if all PIDs (full Transport Stream) is on
		bool isAllPID() const noexcept {
			return _data[ALL_PIDS].state == State::Opened;
		}

	protected:

		/// Reset the pid data like counters etc.
		void resetPidData(int pid) noexcept;

		// =========================================================================
		//  -- Data members --------------------------------------------------------
		// =========================================================================
	public:

		static constexpr int MAX_PIDS = 8193;
		static constexpr int ALL_PIDS = 8192;

	protected:

	private:

		enum class State {
			ShouldOpen,
			Opened,
			ShouldClose,
			ShouldCloseReopen,
			Closed
		};

		// PID State
		struct PidData {
			State state;
			uint8_t cc;        /// continuity counter (0 - 15) of this PID
			uint32_t cc_error; /// cc error count
			uint32_t count;    /// the number of times this pid occurred
		};
		uint32_t _totalCCErrors;
		uint32_t _totalCCErrorsBegin;
		bool _totalCCErrorsBeginSet;
		bool _changed;
		PidData _data[MAX_PIDS];
};

}

#endif // MPEGTS_PIDTABLE_H_INCLUDE
