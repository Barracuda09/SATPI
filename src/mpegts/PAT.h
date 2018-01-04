/* PAT.h

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
#ifndef MPEGTS_PAT_DATA_H_INCLUDE
#define MPEGTS_PAT_DATA_H_INCLUDE MPEGTS_PAT_DATA_H_INCLUDE

#include <mpegts/TableData.h>

#include <string>
#include <map>

namespace mpegts {

	class PAT :
		public TableData {
		public:

			// ================================================================
			// -- Constructors and destructor ---------------------------------
			// ================================================================

			PAT();

			virtual ~PAT();

			// =======================================================================
			// -- mpegts::TableData --------------------------------------------------
			// =======================================================================

		public:
			virtual void clear() override;

			// ================================================================
			//  -- Other member functions -------------------------------------
			// ================================================================

			void parse(int streamID);

			bool isMarkedAsPMT(int pid);

		public:

			// ================================================================
			//  -- Data members -----------------------------------------------
			// ================================================================

		private:

			uint16_t _programNumber;
			uint16_t _tid;
			std::map<int, bool> _pmtPidTable;
	};

} // namespace mpegts

#endif // MPEGTS_PAT_DATA_H_INCLUDE
