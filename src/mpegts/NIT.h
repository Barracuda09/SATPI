/* NIT.h

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
#ifndef MPEGTS_NIT_DATA_H_INCLUDE
#define MPEGTS_NIT_DATA_H_INCLUDE MPEGTS_NIT_DATA_H_INCLUDE

#include <FwDecl.h>
#include <base/M3UParser.h>
#include <base/XMLSupport.h>
#include <mpegts/TableData.h>

#include <string>
#include <vector>

FW_DECL_SP_NS1(mpegts, NIT);

namespace mpegts {

class NIT :
	public TableData,
	public base::XMLSupport {
		// =========================================================================
		// -- Constructors and destructor ------------------------------------------
		// =========================================================================
	public:

		NIT() = default;

		virtual ~NIT() = default;

		// =========================================================================
		// -- mpegts::TableData ----------------------------------------------------
		// =========================================================================
	public:

		virtual void clear() final;

		// =========================================================================
		// -- base::XMLSupport -----------------------------------------------------
		// =========================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		void parse(FeID id);

		TSData generateFrom(
				FeID id, const base::M3UParser::TransformationMap &info);

		// =========================================================================
		//  -- Data members --------------------------------------------------------
		// =========================================================================
	private:

		uint16_t _nid = 0;
		std::string _networkName;
		struct Data {
			uint16_t transportStreamID;
			uint16_t originalNetworkID;
			std::string freq;
			std::string mtype;
			std::string msys;
			std::string srate;
			std::string fec;
			std::string fecOut;
			std::string rolloff;
			std::string inversion;
			std::string pilot;
			std::string pol;
		};
		std::vector<Data> _table;

};

}

#endif // MPEGTS_NIT_DATA_H_INCLUDE
