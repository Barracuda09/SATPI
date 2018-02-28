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
				return _pat.isMarkedAsPMT(pid);
			}

			///
			mpegts::PMT &getPMTData() {
				return _pmt;
			}

			///
			const mpegts::PMT &getPMTData() const {
				return _pmt;
			}

			///
			mpegts::PAT &getPATData() {
				return _pat;
			}

			///
			mpegts::SDT &getSDTData() {
				return _sdt;
			}

			///
			const mpegts::SDT &getSDTData() const {
				return _sdt;
			}

		protected:


			// ================================================================
			//  -- Data members -----------------------------------------------
			// ================================================================

		protected:

		private:

			mpegts::PMT _pmt;
			mpegts::SDT _sdt;
			mpegts::PAT _pat;

	};

} // namespace mpegts

#endif // MPEGTS_FILTER_H_INCLUDE
