/* System.h

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
#ifndef INPUT_DVB_DELIVERY_SYSTEM_H_INCLUDE
#define INPUT_DVB_DELIVERY_SYSTEM_H_INCLUDE INPUT_DVB_DELIVERY_SYSTEM_H_INCLUDE

#include <FwDecl.h>
#include <base/XMLSupport.h>
#include <input/InputSystem.h>

#include <string>

FW_DECL_NS2(input, dvb, FrontendData);

FW_DECL_VECTOR_OF_UP_NS3(input, dvb, delivery, System);

namespace input {
namespace dvb {
namespace delivery {

/// The class @c System specifies the interface to an specific delivery system
class System :
	public base::XMLSupport {
		// =======================================================================
		//  -- Constructors and destructor ---------------------------------------
		// =======================================================================
	public:

		explicit System(int streamID) : _streamID(streamID) {}

		virtual ~System() {}

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================
	public:

		///
		virtual bool tune(
			int feFD,
			const input::dvb::FrontendData &frontendData) = 0;

		///
		virtual bool isCapableOf(input::InputSystem system) const = 0;

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================
	protected:

		int _streamID;

};

} // namespace delivery
} // namespace dvb
} // namespace input

#endif // INPUT_DVB_DELIVERY_SYSTEM_H_INCLUDE
