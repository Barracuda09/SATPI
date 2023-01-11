/* DVBT.h

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
#ifndef INPUT_DVB_DELIVERY_DVBT_H_INCLUDE
#define INPUT_DVB_DELIVERY_DVBT_H_INCLUDE INPUT_DVB_DELIVERY_DVBT_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <input/dvb/delivery/System.h>

FW_DECL_NS2(input, dvb, FrontendData);

namespace input {
namespace dvb {
namespace delivery {

/// The class @c DVBT specifies DVB-T/T2 delivery system
class DVBT :
	public input::dvb::delivery::System {
		// =======================================================================
		//  -- Constructors and destructor ---------------------------------------
		// =======================================================================
	public:

		explicit DVBT(FeIndex index, FeID id, const std::string &fePath, unsigned int dvbVersion);
		virtual ~DVBT();

		// =======================================================================
		// -- base::XMLSupport ---------------------------------------------------
		// =======================================================================
	public:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =======================================================================
		// -- input::dvb::delivery::System ---------------------------------------
		// =======================================================================
	public:

		virtual bool tune(
			int feFD,
			const input::dvb::FrontendData &frontendData) final;

		virtual bool isCapableOf(input::InputSystem system) const final {
			return system == input::InputSystem::DVBT2 ||
				   system == input::InputSystem::DVBT;
		}

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================
	private:

		///
		bool setProperties(int feFD, const input::dvb::FrontendData &frontendData);

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================
	private:

		unsigned int _lna;    ///

};

} // namespace delivery
} // namespace dvb
} // namespace input

#endif // INPUT_DVB_DELIVERY_DVBT_H_INCLUDE
