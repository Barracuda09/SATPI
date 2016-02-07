/* PidTable.h

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef INPUT_PIDTABLE_H_INCLUDE
#define INPUT_PIDTABLE_H_INCLUDE INPUT_PIDTABLE_H_INCLUDE

#include <stdint.h>
#include <string>

namespace input {

	#define MAX_PIDS 8193

	#define ALL_PIDS 8192

	/// The class @c PidTable carries all the PID and DMX information
	class PidTable {
		public:

			// =======================================================================
			// Constructors and destructor
			// =======================================================================
			PidTable();
			virtual ~PidTable();

			/// Reset the pid
			void resetPid(int pid);

			/// Reset that PID has changed
			void resetPIDTableChanged();

			/// Check if the PID has changed
			bool hasPIDTableChanged() const;

			/// Set DMX file descriptor
			void setDMXFileDescriptor(int pid, int fd);

			/// Get DMX file descriptor
			int getDMXFileDescriptor(int pid) const;

			/// Close DMX file descriptor
			void closeDMXFileDescriptor(int pid);

			/// Get the amount of packet that were received of this pid
			uint32_t getPacketCounter(int pid) const;

			/// Get the CSV of all the requested PID
			std::string getPidCSV() const;

			/// Set the continuity counter for pid
			void addPIDData(int pid, uint8_t cc);

			/// Set pid used or not
			void setPID(int pid, bool val);

			/// Check if PID is used
			bool isPIDUsed(int pid) const;

			/// Set all PID
			void setAllPID(bool val);

			/// Set if pid is an PMT PID
			void setPMT(int pid, bool set);

			/// Check if pid is an PMT PID
			bool isPMT(int pid) const;

			/// Set if pid is an ECM PID
			bool isECM(int pid) const;

			///
			void setKeyParity(int pid, int parity);

			///
			void setECMFilterData(int demux, int filter, int pid, bool set);

			///
			void getECMFilterData(int &demux, int &filter, int pid) const;

			///
			bool getActiveECMFilterData(int &demux, int &filter, int &pid) const;

			///
			int getKeyParity(int pid) const;

			// =======================================================================
			// Data members
			// =======================================================================

		protected:

		private:

			// PID and DMX file descriptor
			typedef struct {
				int fd_dmx;              // used DMX file descriptor for PID
				bool used;               // used pid (0 = not used, 1 = in use)
				uint8_t cc;              // continuity counter (0 - 15) of this PID
				uint32_t cc_error;       // cc error count
				uint32_t count;          // the number of times this pid occurred

				bool pmt;                // show if this is an PMT pid
				bool ecm;                // show if this is an ECM pid
				int demux;               // dvbapi demux
				int filter;              // dvbapi filter
				int parity;              // key parity

			} PidData_t;

			bool _changed;               // if something changed to 'pid' array
			PidData_t _data[MAX_PIDS];   // used pids

	};

} // namespace input

#endif // INPUT_PIDTABLE_H_INCLUDE
