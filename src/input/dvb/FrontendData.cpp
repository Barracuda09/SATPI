/* FrontendData.cpp

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
#include <input/dvb/FrontendData.h>

#include <Log.h>
#include <Unused.h>
#include <Utils.h>
#include <StringConverter.h>
#include <TransportParamVector.h>

namespace input::dvb {

using namespace input::dvb::delivery;

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

FrontendData::FrontendData() {
	doInitialize();
}

// =============================================================================
// -- input::DeviceData --------------------------------------------------------
// =============================================================================

void FrontendData::doNextAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "tunefreq", _freq);
	ADD_XML_ELEMENT(xml, "delsys", StringConverter::delsys_to_string(_delsys));
	ADD_XML_ELEMENT(xml, "modulation", StringConverter::modtype_to_sting(_modtype));
	ADD_XML_ELEMENT(xml, "fec", StringConverter::fec_to_string(_fec));
	ADD_XML_ELEMENT(xml, "tunesymbol", _srate);

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

void FrontendData::doNextFromXML(const std::string &UNUSED(xml)) {}

void FrontendData::doInitialize() {
	_freq = 0;
	_modtype = QAM_AUTO;
	_srate = 0;
	_fec = FEC_AUTO;
	_rolloff = ROLLOFF_AUTO;
	_inversion = INVERSION_AUTO;

	// ===========================================================================
	// -- DVB-S(2) Data members --------------------------------------------------
	// ===========================================================================
	_pilot = PILOT_AUTO;
	_src = 1;
	_pol = Lnb::Polarization::Horizontal;
	_isId = NO_STREAM_ID;
	_plsMode = PlsMode::Gold;
	_plsCode = DEFAULT_GOLD_CODE;


	// ===========================================================================
	// -- DVB-C2 Data members ----------------------------------------------------
	// ===========================================================================
	_c2tft = 0;
	_data_slice = 0;

	// ===========================================================================
	// -- DVB-T(2) Data members --------------------------------------------------
	// ===========================================================================
	_transmission = TRANSMISSION_MODE_AUTO;
	_guard = GUARD_INTERVAL_AUTO;
	_hierarchy = HIERARCHY_AUTO;
	_bandwidthHz = 8000000;
	_plpId = NO_STREAM_ID;
	_t2SystemId = 0;
	_siso_miso = 0;
}

void FrontendData::doParseStreamString(const FeID id, const TransportParamVector& params) {
	// Save freq FIRST because of possible initializing of channel data
	const double oldFreq = _freq / 1000.0;
	const double reqFreq = params.getDoubleParameter("freq");
	if (reqFreq != -1.0) {
		// TransportParamVector has on first position the Method
		const std::string method = params[0];
		if (reqFreq != oldFreq) {
			// New frequency, so initialize FrontendData and 'remove' all used PIDS
			SI_LOG_INFO("Frontend: @#1, New frequency requested, clearing old channel data...", id);
			initialize();
			_freq = reqFreq * 1000.0;
			_changed = true;
		} else if (method == "SETUP" || method == "PLAY") {
			const std::string list = params.getParameter("pids");
			if (list.empty()) {
				// TvHeadend Bug #4809
				// Channel change within the same frequency and no 'xxxpids=', 'remove' all used PIDS
				SI_LOG_INFO("Frontend: @#1, @#2 method with query and no pids=, clearing old pids...", id, method);
				_filter.clear();
			} else {
				SI_LOG_INFO("Frontend: @#1, @#2 method with query and pids=, clearing old pids...", id, method);
				_filter.clear();
			}
		}
	}
	_plpId = NO_STREAM_ID;
	_isId = NO_STREAM_ID;
	_plsCode = DEFAULT_GOLD_CODE;
	_plsMode = PlsMode::Gold;
	const int isId = params.getIntParameter("isi");
	if (isId != -1) {
		if (_isId != isId) {
			_isId = isId;
			_changed = true;
		}
	}
	const int plsCode = params.getIntParameter("plsc");
	if (plsCode != -1) {
		if (_plsCode != plsCode) {
			_plsCode = plsCode & 0x3FFFF;
			_changed = true;
		}
	}
	const int plpId = params.getIntParameter("plp");
	if (plpId != -1) {
		if (_plpId != plpId) {
			_plpId = plpId;
			_changed = true;
		}
	}
	const int plsMode = params.getIntParameter("plsm");
	if (plsMode != -1) {
		_plsMode = integerToEnum<PlsMode>(plsMode & 0x03);
	}
	const int sr = params.getIntParameter("sr");
	if (sr != -1) {
		_srate = sr * 1000;
	}
	const input::InputSystem msys = params.getMSYSParameter();
	if (msys != input::InputSystem::UNDEFINED) {
		_delsys = msys;
	}
	const std::string pol = params.getParameter("pol");
	if (!pol.empty()) {
		if (pol == "h") {
			_pol = Lnb::Polarization::Horizontal;
		} else if (pol == "v") {
			_pol = Lnb::Polarization::Vertical;
		} else if (pol == "l") {
			_pol = Lnb::Polarization::CircularLeft;
		} else if (pol == "r") {
			_pol = Lnb::Polarization::CircularRight;
		}
	}
	_src = 1; // default value
	const int src = params.getIntParameter("src");
	if (src >= 1 && src <= 255) {
		_src = src;
	}
	const std::string plts = params.getParameter("plts");
	if (!plts.empty()) {
		// "on", "off"[, "auto"]
		if (plts == "on") {
			_pilot = PILOT_ON;
		} else if (plts == "off") {
			_pilot = PILOT_OFF;
		} else if (plts == "auto") {
			_pilot = PILOT_AUTO;
		} else {
			SI_LOG_ERROR("Frontend: @#1, Unknown Pilot Tone [@#2]", id, plts);
			_pilot = PILOT_AUTO;
		}
	}
	const std::string ro = params.getParameter("ro");
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
			SI_LOG_ERROR("Frontend: @#1, Unknown Rolloff [@#2]", id, ro);
			_rolloff = ROLLOFF_AUTO;
		}
	}
	const std::string fec = params.getParameter("fec");
	if (!fec.empty()) {
		// "12", "23", "34", "56", "78", "89", "35", "45", "910"[, "auto"]
		if (fec == "12") {
			_fec = FEC_1_2;
		} else if (fec == "23") {
			_fec = FEC_2_3;
#if FULL_DVB_API_VERSION >= 0x0509
		} else if (fec == "25") {
			_fec = FEC_2_5;
#endif
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
			SI_LOG_ERROR("Frontend: @#1, Unknown forward error control [@#2] using auto", id, fec);
			_fec = FEC_AUTO;
		}
	}
	const std::string mtype = params.getParameter("mtype");
	if (!mtype.empty()) {
		if (mtype == "qpsk") {
			_modtype = QPSK;
		} else if (mtype == "dqpsk") {
			_modtype = DQPSK;
		} else if (mtype == "8psk" || mtype == "psk8") {
			_modtype = PSK_8;
		} else if (mtype == "16apsk" || mtype == "apsk16") {
			_modtype = APSK_16;
		} else if (mtype == "32apsk" || mtype == "apsk32") {
			_modtype = APSK_32;
		} else if (mtype == "16qam" || mtype == "qam16") {
			_modtype = QAM_16;
		} else if (mtype == "32qam" || mtype == "qam32") {
			_modtype = QAM_32;
		} else if (mtype == "64qam" || mtype == "qam64") {
			_modtype = QAM_64;
		} else if (mtype == "128qam" || mtype == "qam128") {
			_modtype = QAM_128;
		} else if (mtype == "256qam" || mtype == "qam256") {
			_modtype = QAM_256;
		} else {
			SI_LOG_ERROR("Frontend: @#1, Unknown modulation type [@#2]", id, mtype);
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
				SI_LOG_ERROR("Frontend: @#1, Not supported delivery system", id);
				break;
		}
	}
	const int specinv = params.getIntParameter("specinv");
	if (specinv != -1) {
		_inversion = specinv;
	}
	const double bw = params.getDoubleParameter("bw");
	if (bw != -1) {
		_bandwidthHz = bw * 1000000.0;
	}
	const std::string tmode = params.getParameter("tmode");
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
			SI_LOG_ERROR("Frontend: @#1, Unknown transmision mode [@#2]", id, tmode);
			_transmission = TRANSMISSION_MODE_AUTO;
		}
	}
	const std::string gi = params.getParameter("gi");
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
			SI_LOG_ERROR("Frontend: @#1, Unknown Guard interval [@#2]", id, gi);
			_guard = GUARD_INTERVAL_AUTO;
		}
	}
	const int t2id = params.getIntParameter("t2id");
	if (t2id != -1) {
		_t2SystemId = t2id;
	}
	const int sm = params.getIntParameter("sm");
	if (sm != -1) {
		_siso_miso = sm;
	}
	parseAndUpdatePidsTable(params);
}

std::string FrontendData::doAttributeDescribeString(const FeID id) const {
	switch (getDeliverySystem()) {
		case input::InputSystem::DVBS:
		case input::InputSystem::DVBS2:
			// ver=1.0;src=<srcID>;tuner=<feID>,<level>,<lock>,<quality>,<frequency>,
			//             <polarisation>,<system>,<type>,<pilots>,<roll_off>,
			//             <symbol_rate>,<fec_inner>;pids=<pid0>,..,<pidn>
			return StringConverter::stringFormat(
					"ver=1.0;src=@#1;tuner=@#2,@#3,@#4,@#5,@#6,@#7,@#8,@#9,@#10,@#11,@#12,@#13;pids=@#14",
					getDiSEqcSource(),
					id,
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
					_filter.getPidCSV());
		case input::InputSystem::DVBT:
		case input::InputSystem::DVBT2:
			// ver=1.1;tuner=<feID>,<level>,<lock>,<quality>,<freq>,<bw>,<msys>,
			//               <tmode>,<mtype>,<gi>,<fec>,<plp>,<t2id>,
			//               <sm>;pids=<pid0>,..,<pidn>
			return StringConverter::stringFormat(
					"ver=1.1;tuner=@#1,@#2,@#3,@#4,@#5,@#6,@#7,@#8,@#9,@#10,@#11,@#12,@#13,@#14;pids=@#15",
					id,
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
					_filter.getPidCSV());
		case input::InputSystem::DVBC:
			// ver=1.2;tuner=<feID>,<level>,<lock>,<quality>,<freq>,<bw>,<msys>,
			//               <mtype>,<sr>,<c2tft>,<ds>,<plp>,
			//               <specinv>;pids=<pid0>,..,<pidn>
			return StringConverter::stringFormat(
					"ver=1.2;tuner=@#1,@#2,@#3,@#4,@#5,@#6,@#7,@#8,@#9,@#10,@#11,@#12,@#13;pids=@#14",
					id,
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
					_filter.getPidCSV());
		case input::InputSystem::UNDEFINED:
			// Not setup yet
			return "NONE";
		default:
			// Not supported
			return "NONE";
	}
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

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
	return _plpId;
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
	return _t2SystemId;
}

int FrontendData::getInputStreamIdentifier() const {
	base::MutexLock lock(_mutex);
	return _isId;
}

FrontendData::PlsMode FrontendData::getPhysicalLayerSignallingMode() const {
	base::MutexLock lock(_mutex);
	return _plsMode;
}

int FrontendData::getPhysicalLayerSignallingCode() const {
	base::MutexLock lock(_mutex);
	return _plsCode;
}

}
