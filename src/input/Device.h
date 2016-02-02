/* Device.h

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
#ifndef INPUT_DEVICE_H_INCLUDE
#define INPUT_DEVICE_H_INCLUDE INPUT_DEVICE_H_INCLUDE

#include <FwDecl.h>
#include <dvbfix.h>
#include <base/XMLSupport.h>

#include <stdint.h>
#include <string>

FW_DECL_NS0(StreamProperties);
FW_DECL_NS1(mpegts, PacketBuffer);

namespace input {

	/// The class @c Device is an interface to some input device
	/// like for example an DVB-S2 frontend
	class Device :
		public base::XMLSupport {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
			Device() {}

			virtual ~Device() {}

			///
			virtual bool isDataAvailable() = 0;

			///
			virtual bool readFullTSPacket(mpegts::PacketBuffer &buffer) = 0;

			///
			virtual bool capableOf(fe_delivery_system_t msys) = 0;

			///
			virtual void monitorFrontend(fe_status_t &status, uint16_t &strength, uint16_t &snr, uint32_t &ber,
				uint32_t &ublocks, bool showStatus) = 0;

			/// Update the Channel and PID. Will close DVR and reopen it if channel did change
			virtual bool update(StreamProperties &properties) = 0;

			///
			virtual bool teardown(StreamProperties &properties) = 0;
			
			
/////////////

			///
			virtual void addFrontendPaths(const std::string &fe,
				const std::string &dvr,
				const std::string &dmx) = 0;

			///
			virtual bool setFrontendInfo() = 0;

			/// Check if this frontend is tuned
			virtual bool isTuned() const = 0;

			///
			virtual std::size_t getDeliverySystemSize() const = 0;

			///
			virtual const fe_delivery_system_t *getDeliverySystem() const = 0;

	};

} // namespace input

#endif // INPUT_DEVICE_H_INCLUDE
