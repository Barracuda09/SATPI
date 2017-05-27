/* DeviceData.h

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
#ifndef INPUT_DEVICE_DATA_H_INCLUDE
#define INPUT_DEVICE_DATA_H_INCLUDE INPUT_DEVICE_DATA_H_INCLUDE

#include <base/Mutex.h>
#include <base/XMLSupport.h>
#include <input/InputSystem.h>
#include <input/dvb/dvbfix.h>
#include <mpegts/PidTable.h>

namespace input {

	/// The class @c DeviceData. carries all the data/information for a device
	class DeviceData :
		public base::XMLSupport {
		public:
			// =======================================================================
			// -- Constructors and destructor ----------------------------------------
			// =======================================================================
			DeviceData();
			virtual ~DeviceData();

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================

		public:
			///
			virtual void addToXML(std::string &xml) const override;

			///
			virtual void fromXML(const std::string &xml) override;

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		public:

			///
			virtual void initialize();

			///
			virtual void parseStreamString(int streamID, const std::string &msg, const std::string &method) = 0;

			///
			virtual std::string attributeDescribeString(int streamID) const = 0;

			///
			void setMonitorData(fe_status_t status,
					uint16_t strength,
					uint16_t snr,
					uint32_t ber,
					uint32_t ublocks);

			/// Check the 'Channel Data changed' flag.
			/// If changed we should update frontend
			bool hasDeviceDataChanged() const;

			/// Reset/clear the 'Channel Data changed' flag
			void resetDeviceDataChanged();

			/// Set the current Delivery System
			void setDeliverySystem(input::InputSystem system);

			/// Get the current Delivery System
			input::InputSystem getDeliverySystem() const;

			///
			fe_delivery_system convertDeliverySystem() const;

			int hasLock() const;

			fe_status_t getSignalStatus() const;

			uint16_t getSignalStrength() const;

			uint16_t getSignalToNoiseRatio() const;

			uint32_t getBitErrorRate() const;

			uint32_t getUncorrectedBlocks() const;

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		protected:
			base::Mutex _mutex;

			bool _changed;             ///
			input::InputSystem _delsys;/// modulation system i.e. (DVBS/DVBS2)

			// =======================================================================
			// -- Monitor Data members -----------------------------------------------
			// =======================================================================
			fe_status_t _status;
			uint16_t _strength;
			uint16_t _snr;
			uint32_t _ber;
			uint32_t _ublocks;
	};

} // namespace input

#endif // INPUT_DEVICE_DATA_H_INCLUDE
