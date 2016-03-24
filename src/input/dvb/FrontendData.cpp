/* FrontendData.cpp

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
#include <input/dvb/FrontendData.h>

#include <Log.h>
#include <Utils.h>

namespace input {
namespace dvb {

	FrontendData::FrontendData() {
		initialize();
	}

	FrontendData::~FrontendData() {;}

	void FrontendData::initialize() {
		_delsys = input::InputSystem::UNDEFINED;
		_freq = 0;
		_modtype = QAM_64;
		_srate = 0;
		_fec = FEC_AUTO;
		_rolloff = ROLLOFF_AUTO;
		_inversion = INVERSION_AUTO;

		for (size_t i = 0; i < MAX_PIDS; ++i) {
			resetPid(i);
		}
		resetPIDTableChanged();

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

////////////////////////////////////
////////////////////////////////////

	void FrontendData::setPMT(int pid, bool set) {
		_pidTable.setPMT(pid, set);
	}

	bool FrontendData::isPMT(int pid) const {
		return _pidTable.isPMT(pid);
	}

	void FrontendData::setKeyParity(int pid, int parity) {
		_pidTable.setKeyParity(pid, parity);
	}

	int FrontendData::getKeyParity(int pid) const {
		return _pidTable.getKeyParity(pid);
	}

	void FrontendData::setECMInfo(
		int UNUSED(pid),
		int UNUSED(serviceID),
		int UNUSED(caID),
		int UNUSED(provID),
		int UNUSED(emcTime),
		const std::string &UNUSED(cardSystem),
		const std::string &UNUSED(readerName),
		const std::string &UNUSED(sourceName),
		const std::string &UNUSED(protocolName),
		int UNUSED(hops)) {
	}

////////////////////////////////////
////////////////////////////////////

	bool FrontendData::hasFrontendDataChanged() const {
		return _changed;
	}

	input::InputSystem FrontendData::getDeliverySystem() const {
		return _delsys;
	}

	int FrontendData::getDiSEqcSource() const {
		return _src;
	}

	int FrontendData::getPolarization() const {
		return _pol_v;
	}

	uint32_t FrontendData::getFrequency() const {
		return _freq;
	}

	bool FrontendData::isPIDUsed(int pid) const {
		return _pidTable.isPIDUsed(pid);
	}

	uint32_t FrontendData::getPacketCounter(int pid) const {
		return _pidTable.getPacketCounter(pid);
	}

	int FrontendData::getDMXFileDescriptor(int pid) const {
		return _pidTable.getDMXFileDescriptor(pid);
	}

	bool FrontendData::hasPIDTableChanged() const {
		return _pidTable.hasPIDTableChanged();
	}

	std::string FrontendData::getPidCSV() const {
		return _pidTable.getPidCSV();
	}

	int FrontendData::getModulationType() const {
		return _modtype;
	}

	int FrontendData::getSymbolRate() const {
		return _srate;
	}

	int FrontendData::getFEC() const {
		return _fec;
	}

	int FrontendData::getRollOff() const {
		return _rolloff;
	}

	int FrontendData::getPilotTones() const {
		return _pilot;
	}

	int FrontendData::getSpectralInversion() const {
		return _inversion;
	}

	int FrontendData::getBandwidthHz() const {
		return _bandwidthHz;
	}

	int FrontendData::getTransmissionMode() const {
		return _transmission;
	}

	int FrontendData::getHierarchy() const {
		return _hierarchy;
	}

	int FrontendData::getGuardInverval() const {
		return _guard;
	}

	int FrontendData::getUniqueIDPlp() const {
		return _plp_id;
	}

	int FrontendData::getSISOMISO() const {
		return _siso_miso;
	}

	int FrontendData::getDataSlice() const {
		return _data_slice;
	}

	int FrontendData::getC2TuningFrequencyType() const {
		return _c2tft;
	}

	int FrontendData::getUniqueIDT2() const {
		return _t2_system_id;
	}

	fe_delivery_system FrontendData::convertDeliverySystem() const {
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

} // namespace dvb
} // namespace input
