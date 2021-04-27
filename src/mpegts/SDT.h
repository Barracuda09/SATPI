/* SDT.h

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
#ifndef MPEGTS_SDT_DATA_H_INCLUDE
#define MPEGTS_SDT_DATA_H_INCLUDE MPEGTS_SDT_DATA_H_INCLUDE

#include <FwDecl.h>
#include <mpegts/TableData.h>

#include <map>
#include <string>

FW_DECL_SP_NS1(mpegts, SDT);

namespace mpegts {

class SDT :
	public TableData {
		// =====================================================================
		// -- Forward declaration ----------------------------------------------
		// =====================================================================
	public:

		struct Data;

		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:

		SDT();

		virtual ~SDT();

		// =====================================================================
		// -- mpegts::TableData ------------------------------------------------
		// =====================================================================
	public:

		virtual void clear() final;

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	public:

		void parse(FeID id);

		int getTransportStreamID() const {
			return _transportStreamID;
		}

		SDT::Data getSDTDataFor(int progID) const;

	protected:

		void copyToUTF8(std::string &str, const unsigned char *ptr, std::size_t len);

		// =====================================================================
		//  -- Data members ----------------------------------------------------
		// =====================================================================
	public:

		struct Data {
			std::string networkNameUTF8;
			std::string channelNameUTF8;
		};

	private:

		int _transportStreamID;
		int _networkID;
		std::map<int, Data> _sdtTable;

};

} // namespace mpegts

#endif // MPEGTS_SDT_DATA_H_INCLUDE
