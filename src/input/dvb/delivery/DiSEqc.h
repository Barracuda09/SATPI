/* DiSEqc.h

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
#ifndef INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE
#define INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE

#include <base/XMLSupport.h>
#include <Unused.h>
#include <input/dvb/delivery/Lnb.h>

namespace input {
namespace dvb {
namespace delivery {

	/// The class @c DiSEqc specifies an interface to an connected DiSEqc device
	class DiSEqc :
		public base::XMLSupport {
			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
		public:

			DiSEqc();

			virtual ~DiSEqc();

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================
		private:

			/// @see XMLSupport
			virtual void doAddToXML(std::string &xml) const final;

			/// @see XMLSupport
			virtual void doFromXML(const std::string &xml) final;

			// =======================================================================
			// -- Other member functions ---------------------------------------------
			// =======================================================================
		public:

			///
			/// @param feFD
			/// @param streamID
			/// @param freq
			/// @param src specifies the DiSEqc src starting from 0
			/// @param pol
			virtual bool sendDiseqc(int feFD, int streamID, uint32_t &freq,
				int src, Lnb::Polarization pol) = 0;

		private:

			/// Specialization for @see doAddToXML
			virtual void doNextAddToXML(std::string &UNUSED(xml)) const {}

			/// Specialization for @see doFromXML
			virtual void doNextFromXML(const std::string &UNUSED(xml)) {}

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================
		public:

			static constexpr size_t MAX_LNB = 4;

		protected:

			unsigned int _diseqcRepeat;
	};

} // namespace delivery
} // namespace dvb
} // namespace input

#endif // INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE
