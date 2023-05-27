/* Lnb.h

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
#ifndef INPUT_DVB_DELIVERY_LNB_H_INCLUDE
#define INPUT_DVB_DELIVERY_LNB_H_INCLUDE INPUT_DVB_DELIVERY_LNB_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <base/XMLSupport.h>

#include <cstdint>

namespace input::dvb::delivery {

/// The class @c Lnb specifies which type of LNB is connected
class Lnb :
	public base::XMLSupport {
	public:

		enum class Polarization {
			Horizontal = 0,
			Vertical   = 1,
			CircularLeft  = 2,
			CircularRight = 3
		};

		// =======================================================================
		//  -- Constructors and destructor ---------------------------------------
		// =======================================================================
	public:

		Lnb();
		virtual ~Lnb();

		// =======================================================================
		// -- base::XMLSupport ---------------------------------------------------
		// =======================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =======================================================================
		// -- Static member functions --------------------------------------------
		// =======================================================================
	public:

		static char translatePolarizationToChar(Polarization pol);

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================
	public:

		void getIntermediateFrequency(FeID id, uint32_t &freq,
			bool &hiband, Polarization pol) const;

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================
	private:

		enum class LNBType {
			Universal,
			Standard,
			UserDefined
		};

		LNBType _type;
		uint32_t _switchlof;
		uint32_t _lofLow;
		uint32_t _lofHigh;
};

}

#endif // INPUT_DVB_DELIVERY_LNB_H_INCLUDE
