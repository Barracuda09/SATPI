/* Generator.h

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
#ifndef MPEGTS_GENERATOR_H_INCLUDE
#define MPEGTS_GENERATOR_H_INCLUDE MPEGTS_GENERATOR_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <Log.h>
#include <base/Mutex.h>
#include <base/M3UParser.h>
#include <mpegts/NIT.h>
#include <mpegts/PAT.h>
#include <mpegts/PCR.h>
#include <mpegts/PMT.h>
#include <mpegts/SDT.h>

FW_DECL_NS1(mpegts, PacketBuffer);

namespace mpegts {

/// The class @c Generator will generate PSI metadata for a given TransformationMap
class Generator {
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		Generator();

		virtual ~Generator() = default;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		/// This will generate PSI metadata for the given TransformationMap
		/// @param feID specifies the frontend ID
		/// @param info specifies the info for generating the PSI metadata
		mpegts::PacketBuffer generatePSIFrom(
				FeID id, const base::M3UParser::TransformationMap &info); 

		// =========================================================================
		//  -- Data members --------------------------------------------------------
		// =========================================================================
	private:

		mutable base::Mutex _mutex;

		mutable mpegts::SpNIT _nit;
		mutable mpegts::SpPAT _pat;
		mutable mpegts::SpPCR _pcr;
		mutable mpegts::SpPMT _pmt;
		mutable mpegts::SpSDT _sdt;
};

}

#endif // MPEGTS_GENERATOR_H_INCLUDE
