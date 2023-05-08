/* PMT.h

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
#ifndef MPEGTS_PMT_DATA_H_INCLUDE
#define MPEGTS_PMT_DATA_H_INCLUDE MPEGTS_PMT_DATA_H_INCLUDE

#include <FwDecl.h>
#include <base/XMLSupport.h>
#include <mpegts/TableData.h>

#include <string>
#include <vector>

FW_DECL_SP_NS1(mpegts, PMT);

namespace mpegts {

class PMT :
	public TableData,
	public base::XMLSupport {
		// =========================================================================
		// -- Forward declaration --------------------------------------------------
		// =========================================================================
	public:

		struct ECMData;

		// =========================================================================
		// -- Constructors and destructor ------------------------------------------
		// =========================================================================
	public:

		PMT() = default;

		virtual ~PMT() = default;

		// =========================================================================
		// -- Static member functions ----------------------------------------------
		// =========================================================================

	public:

		///
		static void cleanPI(unsigned char *pmt);

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

		mpegts::TSData getProgramInfo() const {
			return _progInfo;
		}

		uint16_t getProgramNumber() const {
			return _programNumber;
		}

		int parsePCRPid();

		int getPCRPid() const {
			return _pcrPID;
		}

		std::vector<ECMData> getECMPIDs() const {
			return _ecmPID;
		}

		bool isReadySend() const {
			if (isCollected() && !_send) {
				_send = true;
				return true;
			}
			return false;
		}

		// =========================================================================
		//  -- Data members --------------------------------------------------------
		// =========================================================================
	public:

		struct ECMData {
			int caid;
			int ecmpid;
			int provid;
		};

	private:

		mpegts::TSData _progInfo;
		uint16_t _programNumber = 0;
		int _pcrPID = 0;
		std::vector<int> _elementaryPID;
		std::vector<ECMData> _ecmPID;
		std::size_t _prgLength = 0;
		mutable bool _send = false;
};

}

#endif // MPEGTS_PMT_DATA_H_INCLUDE
