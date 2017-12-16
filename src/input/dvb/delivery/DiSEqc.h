/* DiSEqc.h

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE
#define INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE

#include <base/XMLSupport.h>
#include <input/dvb/delivery/Lnb.h>

namespace input {
namespace dvb {
namespace delivery {

	/// The class @c DiSEqc specifies an interface to an connected DiSEqc device
	class DiSEqc :
		public base::XMLSupport {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
			DiSEqc();

			virtual ~DiSEqc();

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================

		public:

			virtual void addToXML(std::string &xml) const override;

			virtual void fromXML(const std::string &xml) override;

			// =======================================================================
			// -- Other member functions ---------------------------------------------
			// =======================================================================

		public:

			///
			virtual bool sendDiseqc(int feFD, int streamID, uint32_t &freq,
				int src, int pol_v) = 0;


			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		protected:

			unsigned int _diseqcRepeat;
			Lnb _lnb[MAX_LNB];    // LNB properties

	};

} // namespace delivery
} // namespace dvb
} // namespace input

#endif // INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE
