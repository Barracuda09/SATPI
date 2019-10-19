/* FrontendData.cpp

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
#include <input/dvb/FrontendData.h>

#include <Log.h>
#include <Unused.h>
#include <Utils.h>
#include <StringConverter.h>

namespace input {
namespace dvb {

	using namespace input::dvb::delivery;

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================

	FrontendData::FrontendData() {
		FrontendData::initialize();
	}

	FrontendData::~FrontendData() {;}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void FrontendData::addToXML(std::string &xml) const {
		base::MutexLock lock(_xmlMutex);

		ADD_XML_ELEMENT(xml, "delsys", StringConverter::delsys_to_string(_delsys));
		ADD_XML_ELEMENT(xml, "tunefreq", _freq);
		ADD_XML_ELEMENT(xml, "modulation", StringConverter::modtype_to_sting(_modtype));
		ADD_XML_ELEMENT(xml, "fec", StringConverter::fec_to_string(_fec));
		ADD_XML_ELEMENT(xml, "tunesymbol", _srate);
		ADD_XML_ELEMENT(xml, "pidcsv", _pidTable.getPidCSV());

		switch (_delsys) {
			case input::InputSystem::DVBS:
			case input::InputSystem::DVBS2:
				ADD_XML_ELEMENT(xml, "rolloff", StringConverter::rolloff_to_sting(_rolloff));
				ADD_XML_ELEMENT(xml, "src", _src);
				ADD_XML_ELEMENT(xml, "pol", getPolarizationChar());
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
	// -- input::DeviceData --------------------------------------------------
	// =======================================================================

	void FrontendData::initialize() {
		base::MutexLock lock(_mutex);

		DeviceData::initialize();

		_freq = 0;
		_modtype = QAM_64;
		_srate = 0;
		_fec = FEC_AUTO;
		_rolloff = ROLLOFF_AUTO;
		_inversion = INVERSION_AUTO;

		_pidTable.clear();

		// =======================================================================
		// DVB-S(2) Data members
		// =======================================================================
		_pilot = PILOT_AUTO;
		_src = 1;
		_pol = Lnb::Polarization::Horizontal;

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

	void FrontendData::parseStreamString(
			const int streamID,
			const std::string &msg,
			const std::string &method) {
		base::MutexLock lock(_mutex);

		// Save freq FIRST because of possible initializing of channel data
		const double oldFreq = _freq / 1000.0;
		const double freq = StringConverter::getDoubleParameter(msg, method, "freq=");
		if (freq != -1.0) {
			// New frequency or an SETUP/PLAY, so initialize FrontendData and 'remove' all used PIDS
			if (freq != oldFreq || method == "SETUP" || method == "PLAY") {
				if (freq != oldFreq) {
					SI_LOG_INFO("Stream: %d, New frequency requested, clearing old channel data...", streamID);
				} else {
					SI_LOG_INFO("Stream: %d, %s method with query, clearing old channel data...", streamID, method.c_str());
				}
				initialize();
			}
			_freq = freq * 1000.0;
			_changed = true;
		}
		const int sr = StringConverter::getIntParameter(msg, method, "sr=");
		if (sr != -1) {
			_srate = sr * 1000;
		}
		const input::InputSystem msys = StringConverter::getMSYSParameter(msg, method);
		if (msys != input::InputSystem::UNDEFINED) {
			_delsys = msys;
		}
		const std::string pol = StringConverter::getStringParameter(msg, method, "pol=");
		if (!pol.empty()) {
			if (pol == "h") {
				_pol = Lnb::Polarization::Horizontal;
			} else if (pol == "v") {
				_pol = Lnb::Polarization::Vertical;
			}
		}
		const int src = StringConverter::getIntParameter(msg, method, "src=");
		if (src != -1) {
			_src = src;
		}
		const std::string plts = StringConverter::getStringParameter(msg, method, "plts=");
		if (!plts.empty()) {
			// "on", "off"[, "auto"]
			if (plts == "on") {
				_pilot = PILOT_ON;
			} else if (plts == "off") {
				_pilot = PILOT_OFF;
			} else if (plts == "auto") {
				_pilot = PILOT_AUTO;
			} else {
				SI_LOG_ERROR("Stream: %d, Unknown Pilot Tone [%s]", streamID, plts.c_str());
				_pilot = PILOT_AUTO;
			}
		}
		const std::string ro = StringConverter::getStringParameter(msg, method, "ro=");
		if (!ro.empty()) {
			// "0.35", "0.25", "0.20"[, "auto"]
			if (ro == "0.35") {
				_rolloff = ROLLOFF_35;
			} else if (ro == "0.25") {
				_rolloff = ROLLOFF_25;
			} else if (ro == "0.20") {
				_rolloff = ROLLOFF_20;
			} else if (ro == "auto") {
				_rolloff = ROLLOFF_AUTO;
			} else {
				SI_LOG_ERROR("Stream: %d, Unknown Rolloff [%s]", streamID, ro.c_str());
				_rolloff = ROLLOFF_AUTO;
			}
		}
		const std::string fec = StringConverter::getStringParameter(msg, method, "fec=");
		if (!fec.empty()) {
			// "12", "23", "34", "56", "78", "89", "35", "45", "910"[, "auto"]
			if (fec == "12") {
				_fec = FEC_1_2;
			} else if (fec == "23") {
				_fec = FEC_2_3;
			} else if (fec == "34") {
				_fec = FEC_3_4;
			} else if (fec == "35") {
				_fec = FEC_3_5;
			} else if (fec == "45") {
				_fec = FEC_4_5;
			} else if (fec == "56") {
				_fec = FEC_5_6;
			} else if (fec == "67") {
				_fec = FEC_6_7;
			} else if (fec == "78") {
				_fec = FEC_7_8;
			} else if (fec == "89") {
				_fec = FEC_8_9;
			} else if (fec == "910") {
				_fec = FEC_9_10;
			} else if (fec == "auto") {
				_fec = FEC_AUTO;
			} else {
				SI_LOG_ERROR("Stream: %d, Unknown forward error control [%s]", streamID, fec.c_str());
				_fec = FEC_AUTO;
			}
		}
		const std::string mtype = StringConverter::getStringParameter(msg, method, "mtype=");
		if (!mtype.empty()) {
			if (mtype == "8psk") {
				_modtype = PSK_8;
			} else if (mtype == "qpsk") {
				_modtype = QPSK;
			} else if (mtype == "16qam") {
				_modtype = QAM_16;
			} else if (mtype == "64qam") {
				_modtype = QAM_64;
			} else if (mtype == "256qam") {
				_modtype = QAM_256;
			} else {
				SI_LOG_ERROR("Stream: %d, Unknown modulation type [%s]", streamID, mtype.c_str());
			}
		} else if (msys != input::InputSystem::UNDEFINED) {
			// no 'mtype' set, so guess one according to 'msys'
			switch (msys) {
				case input::InputSystem::DVBS:
					_modtype = QPSK;
					break;
				case input::InputSystem::DVBS2:
					_modtype = PSK_8;
					break;
				case input::InputSystem::DVBT:
				case input::InputSystem::DVBT2:
				case input::InputSystem::DVBC:
					_modtype = QAM_AUTO;
					break;
				default:
					SI_LOG_ERROR("Stream: %d, Not supported delivery system", streamID);
					break;
			}
		}
		const int specinv = StringConverter::getIntParameter(msg, method, "specinv=");
		if (specinv != -1) {
			_inversion = specinv;
		}
		const double bw = StringConverter::getDoubleParameter(msg, method, "bw=");
		if (bw != -1) {
			_bandwidthHz = bw * 1000000.0;
		}
		const std::string tmode = StringConverter::getStringParameter(msg, method, "tmode=");
		if (!tmode.empty()) {
			// "2k", "4k", "8k", "1k", "16k", "32k"[, "auto"]
			if (tmode == "1k") {
				_transmission = TRANSMISSION_MODE_1K;
			} else if (tmode == "2k") {
				_transmission = TRANSMISSION_MODE_2K;
			} else if (tmode == "4k") {
				_transmission = TRANSMISSION_MODE_4K;
			} else if (tmode == "8k") {
				_transmission = TRANSMISSION_MODE_8K;
			} else if (tmode == "16k") {
				_transmission = TRANSMISSION_MODE_16K;
			} else if (tmode == "32k") {
				_transmission = TRANSMISSION_MODE_32K;
			} else if (tmode == "auto") {
				_transmission = TRANSMISSION_MODE_AUTO;
			} else {
				SI_LOG_ERROR("Stream: %d, Unknown transmision mode [%s]", streamID, tmode.c_str());
				_transmission = TRANSMISSION_MODE_AUTO;
			}
		}
		const std::string gi = StringConverter::getStringParameter(msg, method, "gi=");
		if (!gi.empty()) {
			// "14", "18", "116", "132","1128", "19128", "19256"[, "auto"]
			if (gi == "14") {
				_guard = GUARD_INTERVAL_1_4;
			} else if (gi == "18") {
				_guard = GUARD_INTERVAL_1_8;
			} else if (gi == "116") {
				_guard = GUARD_INTERVAL_1_16;
			} else if (gi == "132") {
				_guard = GUARD_INTERVAL_1_32;
			} else if (gi == "1128") {
				_guard = GUARD_INTERVAL_1_128;
			} else if (gi == "19128") {
				_guard = GUARD_INTERVAL_19_128;
			} else if (gi == "19256") {
				_guard = GUARD_INTERVAL_19_256;
			} else if (gi == "auto") {
				_guard = GUARD_INTERVAL_AUTO;
			} else {
				SI_LOG_ERROR("Stream: %d, Unknown Guard interval [%s]", streamID, gi.c_str());
				_guard = GUARD_INTERVAL_AUTO;
			}
		}
		const int plp = StringConverter::getIntParameter(msg, method, "plp=");
		if (plp != -1) {
			_plp_id = plp;
		}
		const int t2id = StringConverter::getIntParameter(msg, method, "t2id=");
		if (t2id != -1) {
			_t2_system_id = t2id;
		}
		const int sm = StringConverter::getIntParameter(msg, method, "sm=");
		if (sm != -1) {
			_siso_miso = sm;
		}
		// Always request PID 0 - Program Association Table (PAT)
		const std::string addPATPid = ",0";
		// Add user defined PIDs
		const std::string addUserPids = ",1,16,17,18";
		const std::string pidsList = StringConverter::getStringParameter(msg, method, "pids=");
		if (!pidsList.empty()) {
			// 'pids=' requested then 'remove' all used PIDS first
			_pidTable.clear();
			const std::string pids = pidsList + addPATPid + addUserPids;
			parsePIDString(pids, true);
		}
		const std::string addpidsList = StringConverter::getStringParameter(msg, method, "addpids=");
		if (!addpidsList.empty()) {
			const std::string pids = addpidsList + addPATPid + addUserPids;
			parsePIDString(pids, true);
		}
		const std::string delpidsList = StringConverter::getStringParameter(msg, method, "delpids=");
		if (!delpidsList.empty()) {
			parsePIDString(delpidsList, false);
		}
	}

	std::string FrontendData::attributeDescribeString(const int streamID) const {
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

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	void FrontendData::parsePIDString(const std::string &pids, const bool add) {
		if (pids == "all" || pids == "none") {
			// all/none pids requested then 'remove' all used PIDS first
			_pidTable.clear();
			if (pids == "all") {
				setAllPID(add);
			}
		} else {
			std::string::size_type begin = 0;
			for (;; ) {
				const std::string::size_type end = pids.find_first_of(",", begin);
				if (end != std::string::npos) {
					const int pid = std::stoi(pids.substr(begin, end - begin));
					setPID(pid, add);
					begin = end + 1;
				} else {
					// Get the last one
					if (begin < pids.size()) {
						const int pid = std::stoi(pids.substr(begin, end - begin));
						setPID(pid, add);
					}
					break;
				}
			}
		}
	}

	void FrontendData::addPIDData(const int pid, const uint8_t cc) {
		base::MutexLock lock(_mutex);
		_pidTable.addPIDData(pid, cc);
	}

	void FrontendData::setDMXFileDescriptor(const int pid, const int fd) {
		base::MutexLock lock(_mutex);
		_pidTable.setDMXFileDescriptor(pid, fd);
	}

	void FrontendData::closeDMXFileDescriptor(const int pid) {
		base::MutexLock lock(_mutex);
		_pidTable.closeDMXFileDescriptor(pid);
	}

	void FrontendData::resetPIDTableChanged() {
		base::MutexLock lock(_mutex);
		_pidTable.resetPIDTableChanged();
	}

	void FrontendData::setPID(const int pid, const bool val) {
		base::MutexLock lock(_mutex);
		_pidTable.setPID(pid, val);
	}

	bool FrontendData::shouldPIDClose(int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.shouldPIDClose(pid);
	}

	void FrontendData::setAllPID(const bool val) {
		base::MutexLock lock(_mutex);
		_pidTable.setAllPID(val);
	}

	bool FrontendData::isPIDUsed(const int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.isPIDUsed(pid);
	}

	uint32_t FrontendData::getPacketCounter(const int pid) const {
		base::MutexLock lock(_mutex);
		return _pidTable.getPacketCounter(pid);
	}

	int FrontendData::getDMXFileDescriptor(const int pid) const {
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

	uint32_t FrontendData::getFrequency() const {
		base::MutexLock lock(_mutex);
		return _freq;
	}

	int FrontendData::getDiSEqcSource() const {
		base::MutexLock lock(_mutex);
		return _src;
	}

	Lnb::Polarization FrontendData::getPolarization() const {
		base::MutexLock lock(_mutex);
		return _pol;
	}

	char FrontendData::getPolarizationChar() const {
		base::MutexLock lock(_mutex);
		return Lnb::translatePolarizationToChar(_pol);
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

} // namespace dvb
} // namespace input
