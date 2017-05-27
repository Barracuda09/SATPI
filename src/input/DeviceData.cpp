/* DeviceData.cpp

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
#include <input/DeviceData.h>

#include <Log.h>
#include <Unused.h>

namespace input {

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================

	DeviceData::DeviceData() {
		initialize();
		_changed = false;
		_strength = 0u;
		_snr = 0u;
		_ber = 0u;
		_ublocks = 0u;
	}

	DeviceData::~DeviceData() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void DeviceData::addToXML(std::string &UNUSED(xml)) const {}

	void DeviceData::fromXML(const std::string &UNUSED(xml)) {}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	void DeviceData::initialize() {
		base::MutexLock lock(_mutex);
		_delsys = input::InputSystem::UNDEFINED;
	}

	void DeviceData::setDeliverySystem(input::InputSystem system) {
		base::MutexLock lock(_mutex);
		_delsys = system;
	}

	input::InputSystem DeviceData::getDeliverySystem() const {
		base::MutexLock lock(_mutex);
		return _delsys;
	}

	fe_delivery_system DeviceData::convertDeliverySystem() const {
		base::MutexLock lock(_mutex);
		switch (_delsys) {
			case input::InputSystem::DVBT:
				return SYS_DVBT;
			case input::InputSystem::DVBT2:
				return SYS_DVBT2;
			case input::InputSystem::DVBS:
				return SYS_DVBS;
			case input::InputSystem::DVBS2:
				return SYS_DVBS2;
			case input::InputSystem::DVBC:
#if FULL_DVB_API_VERSION >= 0x0505
				return SYS_DVBC_ANNEX_A;
#else
				return SYS_DVBC_ANNEX_AC;
#endif
			default:
				return SYS_UNDEFINED;
		}
	}

	void DeviceData::resetDeviceDataChanged() {
		base::MutexLock lock(_mutex);
		_changed = false;
	}

	bool DeviceData::hasDeviceDataChanged() const {
		base::MutexLock lock(_mutex);
		return _changed;
	}

	void DeviceData::setMonitorData(
			const fe_status_t status,
			const uint16_t strength,
			const uint16_t snr,
			const uint32_t ber,
			const uint32_t ublocks) {
		base::MutexLock lock(_mutex);
		_status = status;
		_strength = strength;
		_snr = snr;
		_ber = ber;
		_ublocks = ublocks;
	}

	int DeviceData::hasLock() const {
		base::MutexLock lock(_mutex);
		return (_status & FE_HAS_LOCK) ? 1 : 0;
	}

	fe_status_t DeviceData::getSignalStatus() const {
		base::MutexLock lock(_mutex);
		return _status;
	}

	uint16_t DeviceData::getSignalStrength() const {
		base::MutexLock lock(_mutex);
		return _strength;
	}

	uint16_t DeviceData::getSignalToNoiseRatio() const {
		base::MutexLock lock(_mutex);
		return _snr;
	}

	uint32_t DeviceData::getBitErrorRate() const {
		base::MutexLock lock(_mutex);
		return _ber;
	}

	uint32_t DeviceData::getUncorrectedBlocks() const {
		base::MutexLock lock(_mutex);
		return _ublocks;
	}

} // namespace input
