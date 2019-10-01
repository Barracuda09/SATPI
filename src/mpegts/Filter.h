/* Filter.h

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
#ifndef MPEGTS_FILTER_H_INCLUDE
#define MPEGTS_FILTER_H_INCLUDE MPEGTS_FILTER_H_INCLUDE

#include <base/Mutex.h>
#include <mpegts/PAT.h>
#include <mpegts/PMT.h>
#include <mpegts/SDT.h>

namespace mpegts {

	/// The class @c Filter carries the PID Tables
	class Filter {
		public:

			// ================================================================
			//  -- Constructors and destructor --------------------------------
			// ================================================================
			Filter();

			virtual ~Filter();

			// ================================================================
			//  -- Other member functions -------------------------------------
			// ================================================================

		public:

			///
			void addData(int streamID, const unsigned char *ptr);

			///
			void clear(int streamID);

			///
			bool isMarkedAsPMT(int pid) const {
				base::MutexLock lock(_mutex);
				return _pat->isMarkedAsPMT(pid);
			}

			///
			mpegts::SpPMT getPMTData() const {
				base::MutexLock lock(_mutex);
				return _pmt;
			}

			///
			mpegts::SpPAT getPATData() const {
				base::MutexLock lock(_mutex);
				return _pat;
			}

			///
			mpegts::SpSDT getSDTData() const {
				base::MutexLock lock(_mutex);
				return _sdt;
			}

		protected:


			// ================================================================
			//  -- Data members -----------------------------------------------
			// ================================================================

		protected:

		private:
			mutable base::Mutex _mutex;

			mpegts::SpPMT _pmt;
			mpegts::SpSDT _sdt;
			mpegts::SpPAT _pat;

	};

} // namespace mpegts

#endif // MPEGTS_FILTER_H_INCLUDE
