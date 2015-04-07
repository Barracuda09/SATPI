/* Frontend.cpp

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#include "Frontend.h"
#include "Log.h"
#include "Utils.h"
#include "ChannelData.h"
#include "StringConverter.h"
#include "StreamProperties.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <linux/dvb/dmx.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

Frontend::Frontend() :
		_tuned(false),
		_fd_fe(-1),
		_fd_dvr(-1) {
	for (size_t i = 0; i < MAX_LNB; ++i) {
		_diseqc.LNB[i].type        = LNB_UNIVERSAL;
		_diseqc.LNB[i].lofStandard = DEFAULT_LOF_STANDARD;
		_diseqc.LNB[i].switchlof   = DEFAULT_SWITCH_LOF;
		_diseqc.LNB[i].lofLow      = DEFAULT_LOF_LOW_UNIVERSAL;
		_diseqc.LNB[i].lofHigh     = DEFAULT_LOF_HIGH_UNIVERSAL;
	}
	snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Set");
	_path_to_fe  = "Not Set";
	_path_to_dvr = "Not Set";
	_path_to_dmx = "Not Set";
}

Frontend::~Frontend() {;}

int Frontend::open_fe(const std::string &path, bool readonly) const {
	int fd;
	if((fd = open(path.c_str(), (readonly ? O_RDONLY : O_RDWR) | O_NONBLOCK)) < 0){
		PERROR("FRONTEND DEVICE");
	}
	return fd;
}

int Frontend::open_dmx(const std::string &path) {
	int fd;
	if((fd = open(path.c_str(), O_RDWR | O_NONBLOCK)) < 0) {
		PERROR("DMX DEVICE");
	}
	return fd;
}

int Frontend::open_dvr(const std::string &path) {
	int fd;
	if((fd = open(path.c_str(), O_RDONLY | O_NONBLOCK)) < 0) {
		PERROR("DVR DEVICE");
	}
	return fd;
}

bool Frontend::set_demux_filter(int fd, uint16_t pid) {
	struct dmx_pes_filter_params pesFilter;

	pesFilter.pid      = pid;
	pesFilter.input    = DMX_IN_FRONTEND;
	pesFilter.output   = DMX_OUT_TS_TAP;
	pesFilter.pes_type = DMX_PES_OTHER;
	pesFilter.flags    = DMX_IMMEDIATE_START;

	if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilter) != 0) {
		PERROR("DMX_SET_PES_FILTER");
		return false;
	}
	return true;
}

void Frontend::reset_pid(PidData_t &pid) {
	pid.used     = 0;
	pid.cc       = 0x80;
	pid.cc_error = 0;
	pid.count    = 0;

	if (ioctl(pid.fd_dmx, DMX_STOP) != 0) {
		PERROR("DMX_STOP");
	}
	CLOSE_FD(pid.fd_dmx);
}

bool Frontend::capableOf(fe_delivery_system_t msys) {
	for (size_t i = 0; i < MAX_DELSYS; ++i) {
		// we no not like SYS_UNDEFINED
		if (_info_del_sys[i] != SYS_UNDEFINED && msys == _info_del_sys[i]) {
			return true;
		}
	}
	return false;
}

void Frontend::addToXML(std::string &xml) const {
	StringConverter::addFormattedString(xml, "<frontendname>%s</frontendname>", _fe_info.name);
	StringConverter::addFormattedString(xml, "<pathname>%s</pathname>", _path_to_fe.c_str());
	StringConverter::addFormattedString(xml, "<freq>%d Hz to %d Hz</freq>", _fe_info.frequency_min, _fe_info.frequency_max);
	StringConverter::addFormattedString(xml, "<symbol>%d symbols/s to %d symbols/s</symbol>", _fe_info.symbol_rate_min, _fe_info.symbol_rate_max);
}

bool Frontend::setFrontendInfo() {
#if SIMU
	sprintf(_fe_info.name, "Simulation DVB-S2/C/T Card");
	_fe_info.frequency_min = 1000000UL;
	_fe_info.frequency_max = 21000000UL;
	_fe_info.symbol_rate_min = 20000UL;
	_fe_info.symbol_rate_max = 250000UL;
#else
	int fd_fe;
	// open frontend in readonly mode
	if((fd_fe = open_fe(_path_to_fe, true)) < 0){
		snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Found");
		PERROR("open_fe");
		return false;
	}

	if (ioctl(fd_fe, FE_GET_INFO, &_fe_info) != 0){
		snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Set");
		PERROR("FE_GET_INFO");
		CLOSE_FD(fd_fe);
		return false;
	}
#endif
	SI_LOG_INFO("Frontend Name: %s", _fe_info.name);

	struct dtv_property dtvProperty;
#if SIMU
	dtvProperty.u.buffer.len = 4;
	dtvProperty.u.buffer.data[0] = SYS_DVBS;
	dtvProperty.u.buffer.data[1] = SYS_DVBS2;
	dtvProperty.u.buffer.data[2] = SYS_DVBT;
#if FULL_DVB_API_VERSION >= 0x0505
	dtvProperty.u.buffer.data[3] = SYS_DVBC_ANNEX_A;
#else
	dtvProperty.u.buffer.data[3] = SYS_DVBC_ANNEX_AC;
#endif
#else
	dtvProperty.cmd = DTV_ENUM_DELSYS;
	dtvProperty.u.data = DTV_UNDEFINED;

	struct dtv_properties dtvProperties;
	dtvProperties.num = 1; // size
	dtvProperties.props = &dtvProperty;
	if (ioctl(fd_fe, FE_GET_PROPERTY, &dtvProperties ) != 0) {
		// If we are here it can mean we have an DVB-API <= 5.4
		SI_LOG_DEBUG("Unable to enumerate the delivery systems, retrying via old API Call");
		dtvProperty.u.buffer.len = 1;
		switch (_fe_info.type) {
			case FE_QPSK:
				if (_fe_info.caps & FE_CAN_2G_MODULATION) {
					dtvProperty.u.buffer.data[0] = SYS_DVBS2;
				} else {
					dtvProperty.u.buffer.data[0] = SYS_DVBS;
				}
				break;
			case FE_OFDM:
				if (_fe_info.caps & FE_CAN_2G_MODULATION) {
					dtvProperty.u.buffer.data[0] = SYS_DVBT2;
				} else {
					dtvProperty.u.buffer.data[0] = SYS_DVBT;
				}
				break;
			case FE_QAM:
#if FULL_DVB_API_VERSION >= 0x0505
				dtvProperty.u.buffer.data[0] = SYS_DVBC_ANNEX_A;
#else
				dtvProperty.u.buffer.data[0] = SYS_DVBC_ANNEX_AC;
#endif
				break;
			case FE_ATSC:
				if (_fe_info.caps & (FE_CAN_QAM_64 | FE_CAN_QAM_256 | FE_CAN_QAM_AUTO)) {
					dtvProperty.u.buffer.data[0] = SYS_DVBC_ANNEX_B;
					break;
				}
				// Fall-through
			default:
				SI_LOG_ERROR("Frontend does not have any known delivery systems");
				CLOSE_FD(fd_fe);
				return false;
		}
	}
	CLOSE_FD(fd_fe);
#endif
	// clear delsys
	for (size_t i = 0; i < MAX_DELSYS; ++i) {
		_info_del_sys[i] = SYS_UNDEFINED;
	}
	// get capability of fe and save it
	_del_sys_size = dtvProperty.u.buffer.len;
	for (size_t i = 0; i < dtvProperty.u.buffer.len; i++) {
		switch (dtvProperty.u.buffer.data[i]) {
			case SYS_DSS:
				_info_del_sys[i] = SYS_DSS;
				SI_LOG_DEBUG("Frontend Type: DSS");
				break;
			case SYS_DVBS:
				_info_del_sys[i] = SYS_DVBS;
				SI_LOG_DEBUG("Frontend Type: Satellite (DVB-S)");
				break;
			case SYS_DVBS2:
				_info_del_sys[i] = SYS_DVBS2;
				SI_LOG_DEBUG("Frontend Type: Satellite (DVB-S2)");
				break;
			case SYS_DVBT:
				_info_del_sys[i] = SYS_DVBT;
				SI_LOG_DEBUG("Frontend Type: Terrestrial (DVB-T)");
				break;
			case SYS_DVBT2:
				_info_del_sys[i] = SYS_DVBT2;
				SI_LOG_DEBUG("Frontend Type: Terrestrial (DVB-T2)");
				break;
#if FULL_DVB_API_VERSION >= 0x0505
			case SYS_DVBC_ANNEX_A:
				_info_del_sys[i] = SYS_DVBC_ANNEX_A;
				SI_LOG_DEBUG("Frontend Type: Cable (Annex A)");
				break;
			case SYS_DVBC_ANNEX_C:
				_info_del_sys[i] = SYS_DVBC_ANNEX_C;
				SI_LOG_DEBUG("Frontend Type: Cable (Annex C)");
				break;
#else
			case SYS_DVBC_ANNEX_AC:
				_info_del_sys[i] = SYS_DVBC_ANNEX_AC;
				SI_LOG_DEBUG("Frontend Type: Cable (Annex C)");
				break;
#endif
			case SYS_DVBC_ANNEX_B:
				_info_del_sys[i] = SYS_DVBC_ANNEX_B;
				SI_LOG_DEBUG("Frontend Type: Cable (Annex B)");
				break;
			default:
				_info_del_sys[i] = SYS_UNDEFINED;
				SI_LOG_DEBUG("Frontend Type: Unknown %d", dtvProperty.u.buffer.data[i]);
				break;
		}
	}
	SI_LOG_DEBUG("Frontend Freq: %d Hz to %d Hz", _fe_info.frequency_min, _fe_info.frequency_max);
	SI_LOG_DEBUG("Frontend srat: %d symbols/s to %d symbols/s", _fe_info.symbol_rate_min, _fe_info.symbol_rate_max);

	return true;
}

bool Frontend::tune(const ChannelData &channel) {
	struct dtv_property p[15];
	int size = 0;

#define FILL_PROP(CMD, DATA) { p[size].cmd = CMD; p[size].u.data = DATA; ++size; }

	FILL_PROP(DTV_CLEAR, DTV_UNDEFINED);
	switch (channel.delsys) {
		case SYS_DVBS:
		case SYS_DVBS2:
			FILL_PROP(DTV_DELIVERY_SYSTEM,   channel.delsys);
			FILL_PROP(DTV_FREQUENCY,         channel.ifreq);
			FILL_PROP(DTV_MODULATION,        channel.modtype);
			FILL_PROP(DTV_SYMBOL_RATE,       channel.srate);
			FILL_PROP(DTV_INNER_FEC,         channel.fec);
			FILL_PROP(DTV_INVERSION,         INVERSION_AUTO);
			FILL_PROP(DTV_ROLLOFF,           channel.rolloff);
			FILL_PROP(DTV_PILOT,             channel.pilot);
			break;
		case SYS_DVBT2:
			FILL_PROP(DTV_STREAM_ID,         channel.plp_id);
			// Fall-through
		case SYS_DVBT:
			FILL_PROP(DTV_DELIVERY_SYSTEM,   channel.delsys);
			FILL_PROP(DTV_FREQUENCY,         channel.freq * 1000UL);
			FILL_PROP(DTV_MODULATION,        channel.modtype);
			FILL_PROP(DTV_INVERSION,         channel.inversion);
			FILL_PROP(DTV_BANDWIDTH_HZ,      channel.bandwidthHz);
			FILL_PROP(DTV_CODE_RATE_HP,      channel.fec);
			FILL_PROP(DTV_CODE_RATE_LP,      channel.fec);
			FILL_PROP(DTV_TRANSMISSION_MODE, channel.transmission);
			FILL_PROP(DTV_GUARD_INTERVAL,    channel.guard);
			FILL_PROP(DTV_HIERARCHY,         channel.hierarchy);
			break;
#if FULL_DVB_API_VERSION >= 0x0505
		case SYS_DVBC_ANNEX_A:
		case SYS_DVBC_ANNEX_B:
		case SYS_DVBC_ANNEX_C:
#else
		case SYS_DVBC_ANNEX_AC:
		case SYS_DVBC_ANNEX_B:
#endif
			FILL_PROP(DTV_BANDWIDTH_HZ,      channel.bandwidthHz);
			FILL_PROP(DTV_DELIVERY_SYSTEM,   channel.delsys);
			FILL_PROP(DTV_FREQUENCY,         channel.freq * 1000UL);
			FILL_PROP(DTV_INVERSION,         channel.inversion);
			FILL_PROP(DTV_MODULATION,        channel.modtype);
			FILL_PROP(DTV_SYMBOL_RATE,       channel.srate);
			FILL_PROP(DTV_INNER_FEC,         channel.fec);
			break;

		default:
			return false;
	}
	FILL_PROP(DTV_TUNE, DTV_UNDEFINED);
	struct dtv_properties cmdseq;
	cmdseq.num = size;
	cmdseq.props = p;
	// get all pending events to clear the POLLPRI status
	for (;;) {
		struct dvb_frontend_event dfe;
		if (ioctl(_fd_fe, FE_GET_EVENT, &dfe) == -1) {
			break;
		}
	}
	// set the tuning properties
	if ((ioctl(_fd_fe, FE_SET_PROPERTY, &cmdseq)) == -1) {
		PERROR("FE_SET_PROPERTY failed");
		return false;
	}
	return true;
}

struct diseqc_cmd {
	struct dvb_diseqc_master_cmd cmd;
	uint32_t wait;
};

bool Frontend::diseqcSendMsg(fe_sec_voltage_t v, struct diseqc_cmd *cmd,
		fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b, bool repeatDiseqc) {
	if (ioctl(_fd_fe, FE_SET_TONE, SEC_TONE_OFF) == -1) {
		PERROR("FE_SET_TONE failed");
		return false;
	}
	if (ioctl(_fd_fe, FE_SET_VOLTAGE, v) == -1) {
		PERROR("FE_SET_VOLTAGE failed");
		return false;
	}
	usleep(15 * 1000);
	if (ioctl(_fd_fe, FE_DISEQC_SEND_MASTER_CMD, &cmd->cmd) == -1) {
		PERROR("FE_DISEQC_SEND_MASTER_CMD failed");
		return false;
	}
	usleep(cmd->wait * 1000);
	usleep(15 * 1000);
	if (repeatDiseqc) {
		usleep(100 * 1000);
		// Framing 0xe1: Command from Master, No reply required, Repeated transmission
		cmd->cmd.msg[0] = 0xe1;
		if (ioctl(_fd_fe, FE_DISEQC_SEND_MASTER_CMD, &cmd->cmd) == -1) {
			PERROR("Repeated FE_DISEQC_SEND_MASTER_CMD failed");
			return false;
		}
	}
	if (ioctl(_fd_fe, FE_DISEQC_SEND_BURST, b) == -1) {
		PERROR("FE_DISEQC_SEND_BURST failed");
		return false;
	}
	usleep(15 * 1000);
	if (ioctl(_fd_fe, FE_SET_TONE, t) == -1) {
		PERROR("FE_SET_TONE failed");
		return false;
	}
	return true;
}

bool Frontend::sendDiseqc(int streamID, bool repeatDiseqc) {
// Digital Satellite Equipment Control, specification is available from http://www.eutelsat.com/
	struct diseqc_cmd cmd = {
		{ {0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4}, 0
	};

	// Framing 0xe0: Command from Master, No reply required, First transmission(or repeated transmission)
	// Address 0x10: Any LNB, Switcher or SMATV (Master to all...)
	// Command 0x38: Write to Port group 0 (Committed switches)
	// Data 1  0xf0: see below
	// Data 2  0x00: not used
	// Data 3  0x00: not used
	// size    0x04: send x bytes

	// param: high nibble: reset bits, low nibble set bits,
	// bits are: option, position, polarizaion, band
	cmd.cmd.msg[3] =
		0xf0 | (((_diseqc.src << 2) & 0x0f) | ((_diseqc.pol_v == POL_V) ? 0 : 2) | (_diseqc.hiband ? 1 : 0));

	SI_LOG_INFO("Stream: %d, Sending DiSEqC [%02x] [%02x] [%02x] [%02x]", streamID, cmd.cmd.msg[0],
              cmd.cmd.msg[1], cmd.cmd.msg[2], cmd.cmd.msg[3]);

	return diseqcSendMsg((_diseqc.pol_v == POL_V) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18,
		      &cmd, _diseqc.hiband ? SEC_TONE_ON : SEC_TONE_OFF, (_diseqc.src % 2) ? SEC_MINI_B : SEC_MINI_A, repeatDiseqc);
}

bool Frontend::tune_it(StreamProperties &properties) {
	const int streamID = properties.getStreamID();
	ChannelData channel = properties.getChannelData();

	SI_LOG_DEBUG("Stream: %d, Start tuning process...", streamID);

	if (channel.delsys == SYS_DVBS || channel.delsys == SYS_DVBS2) {
		// DiSEqC switch position differs from src
		Lnb_t &lnb = _diseqc.LNB[channel.src - 1];
		_diseqc.src = channel.src - 1;
		_diseqc.pol_v = channel.pol_v;
		_diseqc.hiband = 0;
		if (lnb.lofHigh) {
			if (lnb.switchlof) {
				// Voltage controlled switch
				if (channel.freq >= lnb.switchlof) {
					_diseqc.hiband = 1;
				}

				if (_diseqc.hiband) {
					channel.ifreq = abs(channel.freq - lnb.lofHigh);
				} else {
					channel.ifreq = abs(channel.freq - lnb.lofLow);
				}
			} else {
				// C-Band Multi-point LNB
				channel.ifreq = abs(channel.freq - (_diseqc.pol_v == POL_V ? lnb.lofLow : lnb.lofHigh));
			}
		} else	{
			// Mono-point LNB without switch
			channel.ifreq = abs(channel.freq - lnb.lofLow);
		}
		// send diseqc (repeat = false)
		if (!sendDiseqc(streamID, false)) {
			return false;
		}
	}

	// Now tune
	if (!tune(channel)) {
		return false;
	}
	return true;
}

bool Frontend::update(StreamProperties &properties) {
	// Setup, tune and set PID Filters
	if (properties.getChannelData().changed) {
		_tuned = false;
		properties.getChannelData().changed = false;
		CLOSE_FD(_fd_dvr);
	}
	size_t timeout = 0;
	while (!setupAndTune(properties)) {
		usleep(50000);
		++timeout;
		if (timeout > 3) {
			return false;
		}
	}
	updatePIDFilters(properties.getChannelData(), properties.getStreamID());
	return true;
}

bool Frontend::setupAndTune(StreamProperties &properties) {
	const int streamID = properties.getStreamID();
	if (!_tuned) {
		// Check if we have already opened a FE
		if (_fd_fe == -1) {
			_fd_fe = open_fe(_path_to_fe, false);
			SI_LOG_INFO("Stream: %d, Opened %s fd: %d", streamID, _path_to_fe.c_str(), _fd_fe);
		}
		// try tuning
		size_t timeout = 0;
		while (!tune_it(properties)) {
			usleep(450000);
			++timeout;
			if (timeout > 3) {
				return false;
			}
		}
		SI_LOG_INFO("Stream: %d, Waiting on lock...", streamID);

		// check if frontend is locked, if not try a few times
		timeout = 0;
		while (timeout < 4) {
			fe_status_t status = FE_TIMEDOUT;
			// first read status
			if (ioctl(_fd_fe, FE_READ_STATUS, &status) == 0) {
				if (status & FE_HAS_LOCK) {
					// We are tuned now
					_tuned = true;
					SI_LOG_INFO("Stream: %d, Tuned and locked (FE status 0x%X)", streamID, status);
					break;
				} else {
					SI_LOG_INFO("Stream: %d, Not locked yet   (FE status 0x%X)...", streamID, status);
				}
			}
			usleep(50000);
			++timeout;
		}
	}
	// Check if we have already a DVR open and are tuned
	if (_fd_dvr == -1 && _tuned) {
		// try opening DVR, try again if fails
		size_t timeout = 0;
		while ((_fd_dvr = open_dvr(_path_to_dvr)) == -1) {
			usleep(150000);
			++timeout;
			if (timeout > 3) {
				return false;
			}
		}
		SI_LOG_INFO("Stream: %d, Opened %s fd: %d", streamID, _path_to_dvr.c_str(), _fd_dvr);

		const unsigned long size = properties.getDVRBufferSize();
		if (ioctl(_fd_dvr, DMX_SET_BUFFER_SIZE, size) == -1) {
			PERROR("DMX_SET_BUFFER_SIZE failed");
		}
	}
	return (_fd_dvr != -1) && _tuned;
}

bool Frontend::updatePIDFilters(ChannelData &channel, int streamID) {
	if (channel.pid.changed == 1) {
		SI_LOG_INFO("Stream: %d, Updating PID filters...", streamID);
		channel.pid.changed = 0;
		int i;
		for (i = 0; i < MAX_PIDS; ++i) {
			// check if PID is used or removed
			if (channel.pid.data[i].used == 1) {
				// check if we have no DMX for this PID, then open one
				if (channel.pid.data[i].fd_dmx == -1) {
					channel.pid.data[i].fd_dmx = open_dmx(_path_to_dmx);
					size_t timeout = 0;
					while (set_demux_filter(channel.pid.data[i].fd_dmx, i) != 1) {
						usleep(350000);
						++timeout;
						if (timeout > 3) {
							return false;
						}
					}
					SI_LOG_DEBUG("Stream: %d, Set filter PID: %04d - fd: %03d", streamID, i, channel.pid.data[i].fd_dmx);
				}
			} else if (channel.pid.data[i].fd_dmx != -1) {
				// We have a DMX but no PID anymore, so reset it
				SI_LOG_DEBUG("Stream: %d, Remove filter PID: %04d - fd: %03d - Packet Count: %d", streamID, i, channel.pid.data[i].fd_dmx, channel.pid.data[i].count);
				reset_pid(channel.pid.data[i]);
			}
		}
	}
	return true;
}

bool Frontend::teardown(ChannelData &channel, int streamID) {
	for (size_t i = 0; i < MAX_PIDS; ++i) {
		if (channel.pid.data[i].used) {
			SI_LOG_DEBUG("Stream: %d, Remove filter PID: %04d - fd: %03d - Packet Count: %d", streamID, i, channel.pid.data[i].fd_dmx, channel.pid.data[i].count);
			reset_pid(channel.pid.data[i]);
		} else if (channel.pid.data[i].fd_dmx != -1) {
			SI_LOG_ERROR("Stream: %d, !! No PID %d but still open DMX !!", streamID, i);
			reset_pid(channel.pid.data[i]);
		}
	}
	_tuned = false;
	CLOSE_FD(_fd_fe);
	CLOSE_FD(_fd_dvr);
	return true;
}
