/* StreamProperties.cpp

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#include "StreamProperties.h"
#include "StringConverter.h"
#include "Log.h"
#include "dvbfix.h"
#include "Frontend.h"
#include "Configure.h"
#include "Utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

static unsigned int seedp = 0xFEED;

StreamProperties::StreamProperties(int streamID) :
		_streamID(streamID),
		_streamActive(false),
		_ssrc((uint32_t)(rand_r(&seedp) % 0xffff)),
		_timestamp(0),
		_rtp_payload(0.0),
		_dvrBufferSize(40 * 188 * 1024),
		_diseqcRepeat(true),
		_rtcpSignalUpdate(1) {}

StreamProperties::~StreamProperties() {}

void StreamProperties::addRtpData(const uint32_t byte, long timestamp) {
	MutexLock lock(_mutex);

	// inc RTP packet counter
	++_spc;
	_soc += byte;
	_rtp_payload += byte;
	_timestamp = timestamp;
}


////////////////////////////////////
////////////////////////////////////

void StreamProperties::setPMT(int pid, bool set) {
	MutexLock lock(_mutex);
	_channelData._pidTable.setPMT(pid, set);
}

bool StreamProperties::isPMT(int pid) const {
	MutexLock lock(_mutex);
	return _channelData._pidTable.isPMT(pid);
}

void StreamProperties::setECMFilterData(int demux, int filter, int pid, bool set) {
	MutexLock lock(_mutex);
#ifdef LIBDVBCSA
	if(_key[0] == NULL) {
		_key[0] = dvbcsa_bs_key_alloc();
	}
	if(_key[1] == NULL) {
		_key[1] = dvbcsa_bs_key_alloc();
	}
#endif
	const bool isSet = _channelData.isPIDUsed(pid);
	if (!isSet || !set) {
		SI_LOG_INFO("Stream: %d, %s ECM PID: %d  demux: %d  filter: %d", _streamID, set ? "Set" : "Clear", pid, demux, filter);
		_channelData._pidTable.setECMFilterData(demux, filter, pid, set);
	}
}

void StreamProperties::getECMFilterData(int &demux, int &filter, int pid) const {
	_channelData._pidTable.getECMFilterData(demux, filter, pid);
}

bool StreamProperties::getActiveECMFilterData(int &demux, int &filter, int &pid) const {
	return _channelData._pidTable.getActiveECMFilterData(demux, filter, pid);
}

bool StreamProperties::isECM(int pid) const {
	return _channelData._pidTable.isECM(pid);
}

void StreamProperties::setKeyParity(int pid, int parity) {
	_channelData._pidTable.setKeyParity(pid, parity);
}

int StreamProperties::getKeyParity(int pid) const {
	return _channelData._pidTable.getKeyParity(pid);
}
////////////////////////////////////
////////////////////////////////////

void StreamProperties::setFrontendMonitorData(fe_status_t status, uint16_t strength, uint16_t snr,
                                              uint32_t ber, uint32_t ublocks) {
	MutexLock lock(_mutex);

	_status = status;
	_strength = strength;
	_snr = snr;
	_ber = ber;
	_ublocks = ublocks;
}

fe_status_t StreamProperties::getFrontendStatus() const {
	MutexLock lock(_mutex);
	return _status;
}

unsigned long StreamProperties::getDVRBufferSize() const {
	MutexLock lock(_mutex);
	return _dvrBufferSize;
}

bool StreamProperties::diseqcRepeat() const {
	MutexLock lock(_mutex);
	return _diseqcRepeat;
}

unsigned int StreamProperties::getRtcpSignalUpdateFrequency() const {
	MutexLock lock(_mutex);
	return _rtcpSignalUpdate;
}

void StreamProperties::fromXML(const std::string &xml) {
	MutexLock lock(_mutex);

	std::string element;
	if (findXMLElement(xml, "diseqc_repeat.value", element)) {
		_diseqcRepeat = (element == "true") ? true : false;
	}
	if (findXMLElement(xml, "dvrbuffer.value", element)) {
		_dvrBufferSize = atoi(element.c_str());
	}
	if (findXMLElement(xml, "rtcpSignalUpdate.value", element)) {
		_rtcpSignalUpdate = atoi(element.c_str());
	}
}

void StreamProperties::addToXML(std::string &xml) const {
	MutexLock lock(_mutex);

	//
	StringConverter::addFormattedString(xml, "<spc>%d</spc>", _spc);
	StringConverter::addFormattedString(xml, "<payload>%.3f</payload>", (_rtp_payload / (1024.0 * 1024.0)));

	// Channel
	StringConverter::addFormattedString(xml, "<delsys>%s</delsys>", StringConverter::delsys_to_string(_channelData.delsys));
	StringConverter::addFormattedString(xml, "<tunefreq>%d</tunefreq>", _channelData.freq);
	StringConverter::addFormattedString(xml, "<modulation>%s</modulation>", StringConverter::modtype_to_sting(_channelData.modtype));
	StringConverter::addFormattedString(xml, "<fec>%s</fec>", StringConverter::fec_to_string(_channelData.fec));
	StringConverter::addFormattedString(xml, "<tunesymbol>%d</tunesymbol>", _channelData.srate);
	StringConverter::addFormattedString(xml, "<rolloff>%s</rolloff>", StringConverter::rolloff_to_sting(_channelData.rolloff));
	StringConverter::addFormattedString(xml, "<src>%d</src>", _channelData.src);
	StringConverter::addFormattedString(xml, "<pol>%c</pol>", (_channelData.pol_v == POL_V) ? 'V' : 'H');

	// Monitor
	StringConverter::addFormattedString(xml, "<status>%d</status>", _status);
	StringConverter::addFormattedString(xml, "<signal>%d</signal>", _strength);
	StringConverter::addFormattedString(xml, "<snr>%d</snr>", _snr);
	StringConverter::addFormattedString(xml, "<ber>%d</ber>", _ber);
	StringConverter::addFormattedString(xml, "<unc>%d</unc>", _ublocks);

	//
	ADD_CONFIG_CHECKBOX(xml, "diseqc_repeat", (_diseqcRepeat ? "true" : "false"));
	ADD_CONFIG_NUMBER_INPUT(xml, "dvrbuffer", _dvrBufferSize, (10 * 188 * 1024), (80 * 188 * 1024));
	ADD_CONFIG_NUMBER_INPUT(xml, "rtcpSignalUpdate", _rtcpSignalUpdate, 0, 5);
}

std::string StreamProperties::attribute_describe_string(bool &active) const {
	MutexLock lock(_mutex);

	active = _streamActive;
	double freq = _channelData.freq / 1000.0;
	int srate = _channelData.srate / 1000;

	std::string csv = _channelData.getPidCSV();
	std::string desc;
	switch (_channelData.delsys) {
		case SYS_DVBS:
		case SYS_DVBS2:
			// ver=1.0;src=<srcID>;tuner=<feID>,<level>,<lock>,<quality>,<frequency>,<polarisation>
			//             <system>,<type>,<pilots>,<roll_off>,<symbol_rate>,<fec_inner>;pids=<pid0>,…,<pidn>
			StringConverter::addFormattedString(desc, "ver=1.0;src=%d;tuner=%d,%d,%d,%d,%.2lf,%c,%s,%s,%s,%s,%d,%s;pids=%s",
					_channelData.src,
					_streamID+1,
					_strength,
					(_status & FE_HAS_LOCK) ? 1 : 0,
					_snr,
					freq,
					(_channelData.pol_v == POL_V) ? 'v' : 'h',
					StringConverter::delsys_to_string(_channelData.delsys),
					StringConverter::modtype_to_sting(_channelData.modtype),
					StringConverter::pilot_tone_to_string(_channelData.pilot),
					StringConverter::rolloff_to_sting(_channelData.rolloff),
					srate,
					StringConverter::fec_to_string(_channelData.fec),
					csv.c_str());
			break;
		case SYS_DVBT:
		case SYS_DVBT2:
			// ver=1.1;tuner=<feID>,<level>,<lock>,<quality>,<freq>,<bw>,<msys>,<tmode>,<mtype>,<gi>,
			//               <fec>,<plp>,<t2id>,<sm>;pids=<pid0>,…,<pidn>
			StringConverter::addFormattedString(desc, "ver=1.1;tuner=%d,%d,%d,%d,%.2lf,%.3lf,%s,%s,%s,%s,%s,%d,%d,%d;pids=%s",
					_streamID+1,
					_strength,
					(_status & FE_HAS_LOCK) ? 1 : 0,
					_snr,
					freq,
					_channelData.bandwidthHz / 1000000.0,
					StringConverter::delsys_to_string(_channelData.delsys),
					StringConverter::transmode_to_string(_channelData.transmission),
					StringConverter::modtype_to_sting(_channelData.modtype),
					StringConverter::guardinter_to_string(_channelData.guard),
					StringConverter::fec_to_string(_channelData.fec),
					_channelData.plp_id,
					_channelData.t2_system_id,
					_channelData.siso_miso,
					csv.c_str());
			break;
		case SYS_DVBC_ANNEX_B:
#if FULL_DVB_API_VERSION >= 0x0505
		case SYS_DVBC_ANNEX_A:
		case SYS_DVBC_ANNEX_C:
#else
		case SYS_DVBC_ANNEX_AC:
#endif
			// ver=1.2;tuner=<feID>,<level>,<lock>,<quality>,<freq>,<bw>,<msys>,<mtype>,<sr>,<c2tft>,<ds>,
			//               <plp>,<specinv>;pids=<pid0>,…,<pidn>
			StringConverter::addFormattedString(desc, "ver=1.2;tuner=%d,%d,%d,%d,%.2lf,%.3lf,%s,%s,%d,%d,%d,%d,%d;pids=%s",
					_streamID+1,
					_strength,
					(_status & FE_HAS_LOCK) ? 1 : 0,
					_snr,
					freq,
					_channelData.bandwidthHz / 1000000.0,
					StringConverter::delsys_to_string(_channelData.delsys),
					StringConverter::modtype_to_sting(_channelData.modtype),
					srate,
					_channelData.c2tft,
					_channelData.data_slice,
					_channelData.plp_id,
					_channelData.inversion,
					csv.c_str());
			break;
		case SYS_UNDEFINED:
			// Not setup yet
			StringConverter::addFormattedString(desc, "NONE");
			break;
		default:
			// Not supported/
			StringConverter::addFormattedString(desc, "NONE");
			break;
	}
	return desc;
}

void StreamProperties::printChannelInfo() const {
	SI_LOG_INFO("freq:         %d", _channelData.freq);
	SI_LOG_INFO("srate:        %d", _channelData.srate);
	SI_LOG_INFO("MSYS:         %s", StringConverter::delsys_to_string(_channelData.delsys));
	SI_LOG_INFO("MODTYPE:      %s", StringConverter::modtype_to_sting(_channelData.modtype));
	SI_LOG_INFO("pol:          %d", _channelData.pol_v);
	SI_LOG_INFO("src:          %d", _channelData.src);
	SI_LOG_INFO("rolloff:      %s", StringConverter::rolloff_to_sting(_channelData.rolloff));
	SI_LOG_INFO("fec:          %s", StringConverter::fec_to_string(_channelData.fec));
	SI_LOG_INFO("specinv:      %d", _channelData.inversion);
	SI_LOG_INFO("bandwidth:    %d", _channelData.bandwidthHz);
	SI_LOG_INFO("transmission: %d", _channelData.transmission);
	SI_LOG_INFO("guard:        %d", _channelData.guard);
	SI_LOG_INFO("plp_id:       %d", _channelData.plp_id);
	SI_LOG_INFO("t2_id:        %d", _channelData.t2_system_id);
	SI_LOG_INFO("sm:           %d", _channelData.siso_miso);
	SI_LOG_INFO("----------------", _channelData.siso_miso);
	SI_LOG_INFO("pid:          %s", _channelData.getPidCSV().c_str());
	SI_LOG_INFO("----------------", _channelData.siso_miso);
}

