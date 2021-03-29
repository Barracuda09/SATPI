/* PidTable.h

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
		public:

			// ================================================================
			//  -- Constructors and destructor --------------------------------
			// ================================================================
			PidTable();

			virtual ~PidTable();

			// ================================================================
			//  -- Other member functions -------------------------------------
			// ================================================================

		public:

			/// Clear all PID data
			void clear();

			/// Reset that PID has changed
			void resetPIDTableChanged();

			/// Check if the PID has changed
			bool hasPIDTableChanged() const;

			/// Get the amount of packet that were received of this pid
			uint32_t getPacketCounter(int pid) const;

			/// Get the CSV of all the requested PID
			std::string getPidCSV() const;

			/// Set the continuity counter for pid
			void addPIDData(int pid, uint8_t cc);

			/// Set pid used or not
			void setPID(int pid, bool use);

			/// Check if this pid is opened
			bool isPIDOpened(int pid) const;

			/// Check if this pid should be closed
			bool shouldPIDClose(int pid) const;

			/// Set that this pid is closed
			void setPIDClosed(int pid);

			/// Check if PID should be opened
			bool shouldPIDOpen(int pid) const;

			/// Set that this pid is opened
			void setPIDOpened(int pid);

			/// Set all PID
			void setAllPID(bool use);

		protected:

			/// Reset the pid data like counters etc. (Not DMX File Descriptor)
			void resetPidData(int pid);

			// ================================================================
			//  -- Data members -----------------------------------------------
			// ================================================================
		public:

			static constexpr size_t MAX_PIDS = 8193;
			static constexpr size_t ALL_PIDS = 8192;

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

			bool _changed;           /// if something changed to 'pid' array
			PidData _data[MAX_PIDS]; /// used pids

	};

} // namespace mpegts

#endif // MPEGTS_PIDTABLE_H_INCLUDE
