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

#include <input/dvb/delivery/Lnb.h>

namespace input {

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================

	DeviceData::DeviceData() {}

	DeviceData::~DeviceData() {}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	void DeviceData::initialize() {
		base::MutexLock lock(_mutex);
		_delsys = input::InputSystem::UNDEFINED;
		_freq = 0;
		_modtype = QAM_64;
		_srate = 0;
		_fec = FEC_AUTO;
		_rolloff = ROLLOFF_AUTO;
		_inversion = INVERSION_AUTO;

		for (size_t i = 0; i < MAX_PIDS; ++i) {
			_pidTable.resetPidData(i);
		}
		_pidTable.resetPIDTableChanged();

		// =======================================================================
		// DVB-S(2) Data members
		// =======================================================================
		_pilot = PILOT_AUTO;
		_src = 1;
		_pol_v = 0;

		// =======================================================================
		// DVB-C2 Data members
		// =======================================================================
		_c2tft = 0;
		_data_slice = 0;

		// =======================================================================
		// DVB-T(2) Data members
		// =======================================================================
		_transmission = TRANSMISSION_MODE_AUTO;
		_guard = GUARD_INTERVAL_AUTO;
		_hierarchy = HIERARCHY_AUTO;
		_bandwidthHz = 8000000;
		_plp_id = 0;
		_t2_system_id = 0;
		_siso_miso = 0;
	}

	std::string DeviceData::attributeDescribeString(const int streamID) const {
		std::string desc;
		switch (getDeliverySystem()) {
			case input::InputSystem::DVBS:
			case input::InputSystem::DVBS2:
				// ver=1.0;src=<srcID>;tuner=<feID>,<level>,<lock>,<quality>,<frequency>,<polarisation>
				//             <system>,<type>,<pilots>,<roll_off>,<symbol_rate>,<fec_inner>;pids=<pid0>,..,<pidn>
				StringConverter::addFormattedString(desc, "ver=1.0;src=%d;tuner=%d,%d,%d,%d,%.2lf,%c,%s,%s,%s,%s,%d,%s;pids=%s",
						getDiSEqcSource(),
						streamID + 1,
						getSignalStrength(),
						hasLock(),
						getSignalToNoiseRatio(),
						getFrequency() / 1000.0,
						getPolarizationChar(),
						StringConverter::delsys_to_string(getDeliverySystem()),
						StringConverter::modtype_to_sting(getModulationType()),
						StringConverter::pilot_tone_to_string(getPilotTones()),
						StringConverter::rolloff_to_sting(getRollOff()),
						getSymbolRate() / 1000,
						StringConverter::fec_to_string(getFEC()),
						getPidCSV().c_str());
				break;
			case input::InputSystem::DVBT:
			case input::InputSystem::DVBT2:
				// ver=1.1;tuner=<feID>,<level>,<lock>,<quality>,<freq>,<bw>,<msys>,<tmode>,<mtype>,<gi>,
				//               <fec>,<plp>,<t2id>,<sm>;pids=<pid0>,..,<pidn>
				StringConverter::addFormattedString(desc, "ver=1.1;tuner=%d,%d,%d,%d,%.2lf,%.3lf,%s,%s,%s,%s,%s,%d,%d,%d;pids=%s",
						streamID + 1,
						getSignalStrength(),
						hasLock(),
						getSignalToNoiseRatio(),
						getFrequency() / 1000.0,
						getBandwidthHz() / 1000000.0,
						StringConverter::delsys_to_string(getDeliverySystem()),
						StringConverter::transmode_to_string(getTransmissionMode()),
						StringConverter::modtype_to_sting(getModulationType()),
						StringConverter::guardinter_to_string(getGuardInverval()),
						StringConverter::fec_to_string(getFEC()),
						getUniqueIDPlp(),
						getUniqueIDT2(),
						getSISOMISO(),
						getPidCSV().c_str());
				break;
			case input::InputSystem::DVBC:
				// ver=1.2;tuner=<feID>,<level>,<lock>,<quality>,<freq>,<bw>,<msys>,<mtype>,<sr>,<c2tft>,<ds>,
				//               <plp>,<specinv>;pids=<pid0>,..,<pidn>
				StringConverter::addFormattedString(desc, "ver=1.2;tuner=%d,%d,%d,%d,%.2lf,%.3lf,%s,%s,%d,%d,%d,%d,%d;pids=%s",
						streamID + 1,
						getSignalStrength(),
						hasLock(),
						getSignalToNoiseRatio(),
						getFrequency() / 1000.0,
						getBandwidthHz() / 1000000.0,
						StringConverter::delsys_to_string(getDeliverySystem()),
						StringConverter::modtype_to_sting(getModulationType()),
						getSymbolRate() / 1000,
						getC2TuningFrequencyType(),
						getDataSlice(),
						getUniqueIDPlp(),
						getSpectralInversion(),
						getPidCSV().c_str());
				break;
			case input::InputSystem::UNDEFINED:
				// Not setup yet
				StringConverter::addFormattedString(desc, "NONE");
				break;
			default:
				// Not supported/
				StringConverter::addFormattedString(desc, "NONE");
				break;
		}
//		SI_LOG_DEBUG("Stream: %d, %s", _streamID, desc.c_str());
		return desc;
	}

	void DeviceData::setMonitorData(
			fe_status_t status,
			uint16_t strength,
			uint16_t snr,
			uint32_t ber,
			uint32_t ublocks) {
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

	void DeviceData::setPMT(int pid, bool set) {
		base::MutexLock lock(_mutex);
		_pidTable.setPMT(pid, set);
	}

	void DeviceData::setKeyParity(int pid, int parity) {
		base::MutexLock lock(_mutex);
		_pidTable.setKeyParity(pid, parity);
	}

	void DeviceData::addPIDData(int pid, uint8_t cc) {
		base::MutexLock lock(_mutex);
		_pidTable.addPIDData(pid, cc);
	}

	void DeviceData::resetPidData(int pid) {
		base::MutexLock lock(_mutex);
		_pidTable.resetPidData(pid);
	}

	void DeviceData::setDMXFileDescriptor(int pid, int fd) {
		base::MutexLock lock(_mutex);
		_pidTable.setDMXFileDescriptor(pid, fd);
	}

	void DeviceData::closeDMXFileDescriptor(int pid) {
		base::MutexLock lock(_mutex);
		_pidTable.closeDMXFileDescriptor(pid);
	}

	void DeviceData::resetPIDTableChanged() {
		base::MutexLock lock(_mutex);
		_pidTable.resetPIDTableChanged();
	}

	void DeviceData::setPID(int pid, bool val) {
		base::MutexLock lock(_mutex);
		_pidTable.setPID(pid, val);
	}

	void DeviceData::setAllPID(bool val) {
		base::MutexLock lock(_mutex);
		_pidTable.setAllPID(val);
	}

	bool DeviceData::isPMT(int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.isPMT(pid);
	}

	int DeviceData::getKeyParity(int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.getKeyParity(pid);
	}

	bool DeviceData::isPIDUsed(int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.isPIDUsed(pid);
	}

	uint32_t DeviceData::getPacketCounter(int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.getPacketCounter(pid);
	}

	int DeviceData::getDMXFileDescriptor(int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.getDMXFileDescriptor(pid);
	}

	bool DeviceData::hasPIDTableChanged() const {
		base::MutexLock lock(_mutex);
		return _pidTable.hasPIDTableChanged();
	}

	std::string DeviceData::getPidCSV() const {
		base::MutexLock lock(_mutex);
		return _pidTable.getPidCSV();
	}

	uint32_t DeviceData::getFrequency() const {
		base::MutexLock lock(_mutex);
		return _freq;
	}

	void DeviceData::setFrequency(uint32_t freq) {
		base::MutexLock lock(_mutex);
		_freq = freq;
		_changed = true;
	}

	input::InputSystem DeviceData::getDeliverySystem() const {
		base::MutexLock lock(_mutex);
		return _delsys;
	}

	void DeviceData::setDeliverySystem(input::InputSystem system) {
		base::MutexLock lock(_mutex);
		_delsys = system;
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

	int DeviceData::getDiSEqcSource() const {
		base::MutexLock lock(_mutex);
		return _src;
	}

	int DeviceData::getPolarization() const {
		base::MutexLock lock(_mutex);
		return _pol_v;
	}

	char DeviceData::getPolarizationChar() const {
		base::MutexLock lock(_mutex);
		return (_pol_v == POL_V) ? 'v' : 'h';
	}

	void DeviceData::setSymbolRate(int srate) {
		base::MutexLock lock(_mutex);
		_srate = srate;
	}

	void DeviceData::setModulationType(int modtype) {
		base::MutexLock lock(_mutex);
		_modtype = modtype;
	}

	int DeviceData::getModulationType() const {
		base::MutexLock lock(_mutex);
		return _modtype;
	}

	int DeviceData::getSymbolRate() const {
		base::MutexLock lock(_mutex);
		return _srate;
	}

	int DeviceData::getFEC() const {
		base::MutexLock lock(_mutex);
		return _fec;
	}

	void DeviceData::setPolarization(int pol) {
		base::MutexLock lock(_mutex);
		_pol_v = pol;
	}


	int DeviceData::getRollOff() const {
		base::MutexLock lock(_mutex);
		return _rolloff;
	}

	int DeviceData::getPilotTones() const {
		base::MutexLock lock(_mutex);
		return _pilot;
	}

	void DeviceData::setRollOff(int rolloff) {
		base::MutexLock lock(_mutex);
		_rolloff = rolloff;
	}

	void DeviceData::setFEC(int fec) {
		base::MutexLock lock(_mutex);
		_fec = fec;
	}

	void DeviceData::setPilotTones(int pilot) {
		base::MutexLock lock(_mutex);
		_pilot = pilot;
	}

	int DeviceData::getSpectralInversion() const {
		base::MutexLock lock(_mutex);
		return _inversion;
	}

	int DeviceData::getBandwidthHz() const {
		base::MutexLock lock(_mutex);
		return _bandwidthHz;
	}

	int DeviceData::getTransmissionMode() const {
		base::MutexLock lock(_mutex);
		return _transmission;
	}

	int DeviceData::getHierarchy() const {
		base::MutexLock lock(_mutex);
		return _hierarchy;
	}

	int DeviceData::getGuardInverval() const {
		base::MutexLock lock(_mutex);
		return _guard;
	}

	int DeviceData::getUniqueIDPlp() const {
		base::MutexLock lock(_mutex);
		return _plp_id;
	}

	int DeviceData::getSISOMISO() const {
		base::MutexLock lock(_mutex);
		return _siso_miso;
	}

	int DeviceData::getDataSlice() const {
		base::MutexLock lock(_mutex);
		return _data_slice;
	}

	int DeviceData::getC2TuningFrequencyType() const {
		base::MutexLock lock(_mutex);
		return _c2tft;
	}

	int DeviceData::getUniqueIDT2() const {
		base::MutexLock lock(_mutex);
		return _t2_system_id;
	}

	void DeviceData::setDiSEqcSource(int source) {
		base::MutexLock lock(_mutex);
		_src = source;
	}

	void DeviceData::setSpectralInversion(int specinv) {
		base::MutexLock lock(_mutex);
		_inversion = specinv;
	}

	void DeviceData::setHierarchy(int hierarchy) {
		base::MutexLock lock(_mutex);
		_hierarchy = hierarchy;
	}

	void DeviceData::setBandwidthHz(int bandwidth) {
		base::MutexLock lock(_mutex);
		_bandwidthHz = bandwidth;
	}

	void DeviceData::setTransmissionMode(int transmission) {
		base::MutexLock lock(_mutex);
		_transmission = transmission;
	}

	void DeviceData::setGuardInverval(int guard) {
		base::MutexLock lock(_mutex);
		_guard = guard;
	}

	void DeviceData::setUniqueIDPlp(int plp) {
		base::MutexLock lock(_mutex);
		_plp_id = plp;
	}

	void DeviceData::setUniqueIDT2(int id) {
		base::MutexLock lock(_mutex);
		_t2_system_id = id;
	}

	void DeviceData::setSISOMISO(int sm) {
		base::MutexLock lock(_mutex);
		_siso_miso = sm;
	}

	void DeviceData::setDataSlice(int slice) {
		base::MutexLock lock(_mutex);
		_data_slice = slice;
	}

	void DeviceData::setC2TuningFrequencyType(int c2tft) {
		base::MutexLock lock(_mutex);
		_c2tft = c2tft;
	}

} // namespace input
