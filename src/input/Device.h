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

#include <string>
#include <cstddef>

FW_DECL_NS1(input, DeviceData);
FW_DECL_NS1(mpegts, PacketBuffer);

namespace input {

	/// The class @c Device is an interface to some input device
	/// like for example an frontend with DVB-S2 or File input
	class Device :
		public base::XMLSupport {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
			Device() {}

			virtual ~Device() {}

			/// Add the amount of delivery systems of this device to the
			/// specified parameters
			virtual void addDeliverySystemCount(
				std::size_t &dvbs2,
				std::size_t &dvbt,
				std::size_t &dvbt2,
				std::size_t &dvbc,
				std::size_t &dvbc2) = 0;

			/// Check if there is data to be red from this device
			virtual bool isDataAvailable() = 0;

			/// Read the available data from this device
			/// @param buffer
			virtual bool readFullTSPacket(mpegts::PacketBuffer &buffer) = 0;

			/// Check the capability of this device
			/// @param msys
			virtual bool capableOf(fe_delivery_system_t msys) = 0;

			/// Monitor signal of this device
			virtual void monitorSignal(bool showStatus) = 0;

			///
			virtual bool hasDeviceDataChanged() const = 0;

			///
			virtual void parseStreamString(const std::string &msg, const std::string &method) = 0;

			/// Update the Channel and PID. Will close DVR and reopen it if channel did change
			virtual bool update() = 0;

			/// Teardown/Stop this device
			virtual bool teardown() = 0;

			/// This will read the frontend information for this frontend
			virtual bool setFrontendInfo(const std::string &fe,
				const std::string &dvr,	const std::string &dmx) = 0;

			///
			virtual std::string attributeDescribeString() const = 0;

			/// Check if this frontend is tuned
			virtual bool isTuned() const = 0;
	};

} // namespace input

#endif // INPUT_DEVICE_H_INCLUDE
