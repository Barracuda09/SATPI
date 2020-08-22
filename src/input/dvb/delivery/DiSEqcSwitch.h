/* DiSEqcSwitch.h

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef INPUT_DVB_DELIVERY_DISEQCSWITCH_H_INCLUDE
#define INPUT_DVB_DELIVERY_DISEQCSWITCH_H_INCLUDE INPUT_DVB_DELIVERY_DISEQCSWITCH_H_INCLUDE

#include <FwDecl.h>
#include <input/dvb/dvbfix.h>
#include <input/dvb/delivery/DiSEqc.h>
#include <input/dvb/delivery/Lnb.h>

#include <stdint.h>

namespace input {
namespace dvb {
namespace delivery {

	/// The class @c DiSEqcSwitch specifies which type of DiSEqc switch is connected
	class DiSEqcSwitch :
		public DiSEqc {
			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
		public:

			DiSEqcSwitch();
			virtual ~DiSEqcSwitch();

			// =======================================================================
			// -- input::dvb::delivery::DiSEqc ---------------------------------------
			// =======================================================================
		public:

			/// @see DiSEqc
			virtual bool sendDiseqc(int feFD, int streamID, uint32_t &freq,
				int src, Lnb::Polarization pol) final;

		private:

			/// @see DiSEqc
			virtual void doNextAddToXML(std::string &xml) const final;

			/// @see DiSEqc
			virtual void doNextFromXML(const std::string &xml) final;

			// =======================================================================
			// -- Other member functions ---------------------------------------------
			// =======================================================================
		private:

			///
			bool diseqcSwitch(int feFD, int streamID, uint32_t &freq,
				int src, Lnb::Polarization pol);

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:
			Lnb _lnb[MAX_LNB];
	};

} // namespace delivery
} // namespace dvb
} // namespace input

#endif // INPUT_DVB_DELIVERY_DISEQCSWITCH_H_INCLUDE
