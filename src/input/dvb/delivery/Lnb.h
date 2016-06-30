/* Lnb.h

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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

#include <FwDecl.h>
#include <base/Mutex.h>
#include <base/XMLSupport.h>

#include <stdint.h>

namespace input {
namespace dvb {
namespace delivery {

	/// The class @c Lnb specifies which type of LNB is connected
	class Lnb :
		public base::XMLSupport {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
			Lnb();
			virtual ~Lnb();

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

			void getIntermediateFrequency(uint32_t &freq,
				bool &hiband, bool verticalPolarization) const;

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:

			enum class LNBType {
				Universal,
				Standard
			};

			base::Mutex _mutex;
			LNBType _type;
			uint32_t _lofStandard;
			uint32_t _switchlof;
			uint32_t _lofLow;
			uint32_t _lofHigh;
	};

} // namespace delivery
} // namespace dvb
} // namespace input

#endif // INPUT_DVB_DELIVERY_LNB_H_INCLUDE
