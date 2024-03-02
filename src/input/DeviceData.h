/* DeviceData.h

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
#ifndef INPUT_DEVICE_DATA_H_INCLUDE
#define INPUT_DEVICE_DATA_H_INCLUDE INPUT_DEVICE_DATA_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <base/Mutex.h>
#include <base/XMLSupport.h>
#include <input/InputSystem.h>
#include <input/dvb/dvbfix.h>
#include <mpegts/Filter.h>
#include <mpegts/Generator.h>
#include <TransportParamVector.h>

FW_DECL_NS1(mpegts, PacketBuffer);

namespace input {

/// The class @c DeviceData. carries all the data/information for a device
class DeviceData :
	public base::XMLSupport {
		// =========================================================================
		// -- Constructors and destructor ------------------------------------------
		// =========================================================================
	public:

		DeviceData();
		virtual ~DeviceData() = default;

		// =========================================================================
		// -- base::XMLSupport -----------------------------------------------------
		// =========================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		///
		void initialize();

		///
		void parseStreamString(FeID id, const TransportParamVector& params);

		///
		std::string attributeDescribeString(FeID id) const;

	private:

		/// Specialization for @see doAddToXML
		virtual void doNextAddToXML(std::string &UNUSED(xml)) const {}

		/// Specialization for @see doFromXML
		virtual void doNextFromXML(const std::string &UNUSED(xml)) {}

		/// Specialization for @see initialize
		virtual void doInitialize() {}

		/// Specialization for @see parseStreamString
		virtual void doParseStreamString(FeID id, const TransportParamVector& params) = 0;

		/// Specialization for @see attributeDescribeString
		virtual std::string doAttributeDescribeString(FeID id) const = 0;

		/// Indicates if the concrete device supports internal pid filtering
		virtual bool capableOfInternalFiltering() const {
			return false;
		}

	public:

		///
		void setMonitorData(fe_status_t status,
				uint16_t strength,
				uint16_t snr,
				uint32_t ber,
				uint32_t ublocks);

		/// Check the 'Channel Data changed' flag.
		/// If changed we should update frontend
		bool hasDeviceFrequencyChanged() const;

		/// Reset/clear the 'Channel Data changed' flag
		void resetDeviceFrequencyChanged();

		/// Get the current Delivery System
		input::InputSystem getDeliverySystem() const;

		///
		const mpegts::Filter& getFilter() const noexcept {
			return _filter;
		}

		///
		mpegts::Filter& getFilter() noexcept {
			return _filter;
		}

		///
		const mpegts::Generator &getPSIGenerator() const;

		///
		mpegts::Generator &getPSIGenerator();

		///
		fe_delivery_system convertDeliverySystem() const;

		/// General function to parse and update the pid list.
		/// Using the request parameters "pids","addpids","delpids"
		/// @param id
		/// @param params
		void parseAndUpdatePidsTable(FeID id, const TransportParamVector& params);

		int hasLock() const;

		fe_status_t getSignalStatus() const;

		uint16_t getSignalStrength() const;

		uint16_t getSignalToNoiseRatio() const;

		uint32_t getBitErrorRate() const;

		uint32_t getUncorrectedBlocks() const;

		bool isInternalPidFilteringEnabled() const {
			return _internalPidFiltering;
		}

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	protected:

		base::Mutex _mutex;

		bool _frequencyChanged;
		input::InputSystem _delsys;
		mpegts::Filter _filter;
		mpegts::Generator _generator;
		bool _internalPidFiltering;

		// =========================================================================
		// -- Monitor Data members -------------------------------------------------
		// =========================================================================
		fe_status_t _status;
		uint16_t _strength;
		uint16_t _snr;
		uint32_t _ber;
		uint32_t _ublocks;
};

}

#endif // INPUT_DEVICE_DATA_H_INCLUDE
