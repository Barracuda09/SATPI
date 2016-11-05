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
#include <StringConverter.h>
#include <Utils.h>
#include <input/dvb/delivery/DiSEqc.h>

namespace input {
namespace dvb {

	FrontendData::FrontendData() {
		initialize();
	}

	FrontendData::~FrontendData() {;}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void FrontendData::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		StringConverter::addFormattedString(xml, "<delsys>%s</delsys>", StringConverter::delsys_to_string(_delsys));
		StringConverter::addFormattedString(xml, "<tunefreq>%d</tunefreq>", _freq);
		StringConverter::addFormattedString(xml, "<modulation>%s</modulation>", StringConverter::modtype_to_sting(_modtype));
		StringConverter::addFormattedString(xml, "<fec>%s</fec>", StringConverter::fec_to_string(_fec));
		StringConverter::addFormattedString(xml, "<tunesymbol>%d</tunesymbol>", _srate);
		StringConverter::addFormattedString(xml, "<pidcsv>%s</pidcsv>", _pidTable.getPidCSV().c_str());

		switch (_delsys) {
			case input::InputSystem::DVBS:
			case input::InputSystem::DVBS2:
		                StringConverter::addFormattedString(xml, "<rolloff>%s</rolloff>", StringConverter::rolloff_to_sting(_rolloff));
		                StringConverter::addFormattedString(xml, "<src>%d</src>", _src);
		                StringConverter::addFormattedString(xml, "<pol>%c</pol>", (_pol_v == POL_V) ? 'V' : 'H');
				break;
			case input::InputSystem::DVBT:
				// Empty
				break;
			case input::InputSystem::DVBT2:
				// Empty
				break;
			case input::InputSystem::DVBC:
				// Empty
				break;
			default:
				// Do nothing here
				break;
		}
	}

	void FrontendData::fromXML(const std::string &UNUSED(xml)) {}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	void FrontendData::initialize() {
		base::MutexLock lock(_mutex);
		_delsys = input::InputSystem::UNDEFINED;
		_freq = 0;
		_modtype = QAM_64;
		_srate = 0;
		_fec = FEC_AUTO;
		_rolloff = ROLLOFF_AUTO;
		_inversion = INVERSION_AUTO;

		for (size_t i = 0; i < MAX_PIDS; ++i) {
			resetPidData(i);
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
		base::MutexLock lock(_mutex);
		_pidTable.setPMT(pid, set);
	}

	bool FrontendData::isPMT(int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.isPMT(pid);
	}

	void FrontendData::setKeyParity(int pid, int parity) {
		base::MutexLock lock(_mutex);
		_pidTable.setKeyParity(pid, parity);
	}

	int FrontendData::getKeyParity(int pid) const {
		base::MutexLock lock(_mutex);
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
		base::MutexLock lock(_mutex);
		return _changed;
	}

	input::InputSystem FrontendData::getDeliverySystem() const {
		base::MutexLock lock(_mutex);
		return _delsys;
	}

	int FrontendData::getDiSEqcSource() const {
		base::MutexLock lock(_mutex);
		return _src;
	}

	int FrontendData::getPolarization() const {
		base::MutexLock lock(_mutex);
		return _pol_v;
	}

	uint32_t FrontendData::getFrequency() const {
		base::MutexLock lock(_mutex);
		return _freq;
	}

	bool FrontendData::isPIDUsed(int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.isPIDUsed(pid);
	}

	uint32_t FrontendData::getPacketCounter(int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.getPacketCounter(pid);
	}

	int FrontendData::getDMXFileDescriptor(int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.getDMXFileDescriptor(pid);
	}

	bool FrontendData::hasPIDTableChanged() const {
		base::MutexLock lock(_mutex);
		return _pidTable.hasPIDTableChanged();
	}

	std::string FrontendData::getPidCSV() const {
		base::MutexLock lock(_mutex);
		return _pidTable.getPidCSV();
	}

	int FrontendData::getModulationType() const {
		base::MutexLock lock(_mutex);
		return _modtype;
	}

	int FrontendData::getSymbolRate() const {
		base::MutexLock lock(_mutex);
		return _srate;
	}

	int FrontendData::getFEC() const {
		base::MutexLock lock(_mutex);
		return _fec;
	}

	int FrontendData::getRollOff() const {
		base::MutexLock lock(_mutex);
		return _rolloff;
	}

	int FrontendData::getPilotTones() const {
		base::MutexLock lock(_mutex);
		return _pilot;
	}

	int FrontendData::getSpectralInversion() const {
		base::MutexLock lock(_mutex);
		return _inversion;
	}

	int FrontendData::getBandwidthHz() const {
		base::MutexLock lock(_mutex);
		return _bandwidthHz;
	}

	int FrontendData::getTransmissionMode() const {
		base::MutexLock lock(_mutex);
		return _transmission;
	}

	int FrontendData::getHierarchy() const {
		base::MutexLock lock(_mutex);
		return _hierarchy;
	}

	int FrontendData::getGuardInverval() const {
		base::MutexLock lock(_mutex);
		return _guard;
	}

	int FrontendData::getUniqueIDPlp() const {
		base::MutexLock lock(_mutex);
		return _plp_id;
	}

	int FrontendData::getSISOMISO() const {
		base::MutexLock lock(_mutex);
		return _siso_miso;
	}

	int FrontendData::getDataSlice() const {
		base::MutexLock lock(_mutex);
		return _data_slice;
	}

	int FrontendData::getC2TuningFrequencyType() const {
		base::MutexLock lock(_mutex);
		return _c2tft;
	}

	int FrontendData::getUniqueIDT2() const {
		base::MutexLock lock(_mutex);
		return _t2_system_id;
	}

	fe_delivery_system FrontendData::convertDeliverySystem() const {
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

} // namespace dvb
} // namespace input
