/* Frontend.cpp

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
#include <input/dvb/Frontend.h>

#include <Log.h>
#include <Utils.h>
#include <Stream.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>
#include <input/dvb/FrontendData.h>
#include <input/dvb/delivery/DVBC.h>
#include <input/dvb/delivery/DVBS.h>
#include <input/dvb/delivery/DVBT.h>
#include <input/dvb/delivery/DiSEqc.h>

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <poll.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/dvb/dmx.h>

namespace input {
namespace dvb {

	Frontend::Frontend(int streamID,
		const std::string &fe,
		const std::string &dvr,
		const std::string &dmx) :
		_streamID(streamID),
		_tuned(false),
		_fd_fe(-1),
		_fd_dvr(-1),
		_path_to_fe(fe),
		_path_to_dvr(dvr),
		_path_to_dmx(dmx),
		_dvbs2(0),
		_dvbt(0),
		_dvbt2(0),
		_dvbc(0),
		_dvbc2(0),
		_dvrBufferSize(40 * 188 * 1024) {
		snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Set");
		setupFrontend();
	}

	Frontend::~Frontend() {
		for (input::dvb::delivery::SystemVector::iterator it = _deliverySystem.begin();
		     it != _deliverySystem.end();
		     ++it) {
			DELETE(*it);
		}
	}

	// =======================================================================
	//  -- Static functions --------------------------------------------------
	// =======================================================================

	static void getAttachedFrontends(StreamVector &streamVector, decrypt::dvbapi::SpClient decrypt,
			const std::string &path) {
#define DMX      "/dev/dvb/adapter%d/demux%d"
#define DVR      "/dev/dvb/adapter%d/dvr%d"
#define FRONTEND "/dev/dvb/adapter%d/frontend%d"

#define FE_PATH_LEN 255
#if SIMU
		// unused var
		(void)path;
		char fe_path[FE_PATH_LEN];
		char dvr_path[FE_PATH_LEN];
		char dmx_path[FE_PATH_LEN];
		snprintf(fe_path,  FE_PATH_LEN, FRONTEND, 0, 0);
		snprintf(dvr_path, FE_PATH_LEN, DVR, 0, 0);
		snprintf(dmx_path, FE_PATH_LEN, DMX, 0, 0);
		input::dvb::SpFrontend frontend0 = std::make_shared<input::dvb::Frontend>(0, fe_path, dvr_path, dmx_path);
		streamVector.push_back(std::make_shared<Stream>(0, frontend0, decrypt));
		snprintf(fe_path,  FE_PATH_LEN, FRONTEND, 1, 0);
		snprintf(dvr_path, FE_PATH_LEN, DVR, 1, 0);
		snprintf(dmx_path, FE_PATH_LEN, DMX, 1, 0);
		input::dvb::SpFrontend frontend1 = std::make_shared<input::dvb::Frontend>(1, fe_path, dvr_path, dmx_path);
		streamVector.push_back(std::make_shared<Stream>(1, frontend1, decrypt));
#else
		struct dirent **file_list;
		const int n = scandir(path.c_str(), &file_list, nullptr, alphasort);
		if (n > 0) {
			for (int i = 0; i < n; ++i) {
				char full_path[FE_PATH_LEN];
				snprintf(full_path, FE_PATH_LEN, "%s/%s", path.c_str(), file_list[i]->d_name);
				struct stat stat_buf;
				if (stat(full_path, &stat_buf) == 0) {
					switch (stat_buf.st_mode & S_IFMT) {
						case S_IFCHR: // character device
							if (strstr(file_list[i]->d_name, "frontend") != nullptr) {
								int fe_nr;
								sscanf(file_list[i]->d_name, "frontend%d", &fe_nr);
								int adapt_nr;
								sscanf(path.c_str(), "/dev/dvb/adapter%d", &adapt_nr);

								// make new paths
								char fe_path[FE_PATH_LEN];
								char dvr_path[FE_PATH_LEN];
								char dmx_path[FE_PATH_LEN];
								snprintf(fe_path,  FE_PATH_LEN, FRONTEND, adapt_nr, fe_nr);
								snprintf(dvr_path, FE_PATH_LEN, DVR, adapt_nr, fe_nr);
								snprintf(dmx_path, FE_PATH_LEN, DMX, adapt_nr, fe_nr);

								const StreamVector::size_type size = streamVector.size();
								input::dvb::SpFrontend frontend = std::make_shared<input::dvb::Frontend>(size, fe_path, dvr_path, dmx_path);
								streamVector.push_back(std::make_shared<Stream>(size, frontend, decrypt));
							}
							break;
						case S_IFDIR:
							// do not use dir '.' an '..'
							if (strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
								getAttachedFrontends(streamVector, decrypt, full_path);
							}
							break;
					}
				}
				free(file_list[i]);
			}
		}
#endif
#undef DMX
#undef DVR
#undef FRONTEND
#undef FE_PATH_LEN
	}

	// =======================================================================
	//  -- Static member functions -------------------------------------------
	// =======================================================================

	void Frontend::enumerate(StreamVector &streamVector, decrypt::dvbapi::SpClient decrypt,
		const std::string &path) {
		const StreamVector::size_type beginSize = streamVector.size();
		SI_LOG_INFO("Detecting frontends in: %s", path.c_str());
		getAttachedFrontends(streamVector, decrypt, path);
		const StreamVector::size_type endSize = streamVector.size();
		SI_LOG_INFO("Frontends found: %zu", endSize - beginSize);
	}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void Frontend::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		StringConverter::addFormattedString(xml, "<frontendname>%s</frontendname>", _fe_info.name);
		StringConverter::addFormattedString(xml, "<pathname>%s</pathname>", _path_to_fe.c_str());
		StringConverter::addFormattedString(xml, "<freq>%d Hz to %d Hz</freq>", _fe_info.frequency_min, _fe_info.frequency_max);
		StringConverter::addFormattedString(xml, "<symbol>%d symbols/s to %d symbols/s</symbol>", _fe_info.symbol_rate_min, _fe_info.symbol_rate_max);

		// Channel
		StringConverter::addFormattedString(xml, "<delsys>%s</delsys>", StringConverter::delsys_to_string(_frontendData.getDeliverySystem()));
		StringConverter::addFormattedString(xml, "<tunefreq>%d</tunefreq>", _frontendData.getFrequency());
		StringConverter::addFormattedString(xml, "<modulation>%s</modulation>", StringConverter::modtype_to_sting(_frontendData.getModulationType()));
		StringConverter::addFormattedString(xml, "<fec>%s</fec>", StringConverter::fec_to_string(_frontendData.getFEC()));
		StringConverter::addFormattedString(xml, "<tunesymbol>%d</tunesymbol>", _frontendData.getSymbolRate());
		StringConverter::addFormattedString(xml, "<rolloff>%s</rolloff>", StringConverter::rolloff_to_sting(_frontendData.getRollOff()));
		StringConverter::addFormattedString(xml, "<src>%d</src>", _frontendData.getDiSEqcSource());
		StringConverter::addFormattedString(xml, "<pol>%c</pol>", (_frontendData.getPolarization() == POL_V) ? 'V' : 'H');

		// Monitor
		StringConverter::addFormattedString(xml, "<status>%d</status>", _status);
		StringConverter::addFormattedString(xml, "<signal>%d</signal>", _strength);
		StringConverter::addFormattedString(xml, "<snr>%d</snr>", _snr);
		StringConverter::addFormattedString(xml, "<ber>%d</ber>", _ber);
		StringConverter::addFormattedString(xml, "<unc>%d</unc>", _ublocks);

		ADD_CONFIG_NUMBER_INPUT(xml, "dvrbuffer", _dvrBufferSize, (10 * 188 * 1024), (80 * 188 * 1024));

		ADD_XML_BEGIN_ELEMENT(xml, "deliverySystem");
		_deliverySystem[0]->addToXML(xml);
		ADD_XML_END_ELEMENT(xml, "deliverySystem");
	}

	void Frontend::fromXML(const std::string &xml) {
		base::MutexLock lock(_mutex);
		std::string element;
		if (findXMLElement(xml, "deliverySystem", element)) {
			_deliverySystem[0]->fromXML(element);
		}
		if (findXMLElement(xml, "dvrbuffer.value", element)) {
			_dvrBufferSize = atoi(element.c_str());
		}
	}

	// ========================================================================
	//  -- input::Device ------------------------------------------------------
	// ========================================================================

	void Frontend::addDeliverySystemCount(
		std::size_t &dvbs2,
		std::size_t &dvbt,
		std::size_t &dvbt2,
		std::size_t &dvbc,
		std::size_t &dvbc2) {
		dvbs2 += _dvbs2;
		dvbt  += _dvbt;
		dvbt2 += _dvbt2;
		dvbc  += _dvbc;
		dvbc2 += _dvbc2;
	}

	bool Frontend::isDataAvailable() {
		struct pollfd pfd[1];
		pfd[0].fd = _fd_dvr;
		pfd[0].events = POLLIN | POLLPRI;
		pfd[0].revents = 0;
		return poll(pfd, 1, 100) > 0;
	}

	bool Frontend::readFullTSPacket(mpegts::PacketBuffer &buffer) {
		// try read maximum amount of bytes from DVR
		const int bytes = read(_fd_dvr, buffer.getWriteBufferPtr(), buffer.getAmountOfBytesToWrite());
		if (bytes > 0) {
			buffer.addAmountOfBytesWritten(bytes);
			const bool full = buffer.full();
			if (full) {
				const std::size_t size = buffer.getNumberOfTSPackets();
				for (std::size_t i = 0; i < size; ++i) {
					const unsigned char *ptr = buffer.getTSPacketPtr(i);
					// sync byte then check cc
					if (ptr[0] == 0x47) {
						// get PID and CC from TS
						const uint16_t pid = ((ptr[1] & 0x1f) << 8) | ptr[2];
						const uint8_t  cc  =   ptr[3] & 0x0f;
						_frontendData.addPIDData(pid, cc);
					}
				}
			}
			return full;
		} else if (bytes < 0) {
			PERROR("Frontend::readFullTSPacket");
		}
		return false;
	}

	bool Frontend::capableOf(input::InputSystem system) const {
		for (input::dvb::delivery::SystemVector::const_iterator it = _deliverySystem.begin();
		     it != _deliverySystem.end();
		     ++it) {
			if ((*it)->isCapableOf(system)) {
				return true;
			}
		}
		return false;
	}

	void Frontend::monitorSignal(bool showStatus) {
		base::MutexLock lock(_mutex);
		// first read status
		if (ioctl(_fd_fe, FE_READ_STATUS, &_status) == 0) {
			// some frontends might not support all these ioctls
			if (ioctl(_fd_fe, FE_READ_SIGNAL_STRENGTH, &_strength) != 0) {
				_strength = 0;
			}
			if (ioctl(_fd_fe, FE_READ_SNR, &_snr) != 0) {
				_snr = 0;
			}
			if (ioctl(_fd_fe, FE_READ_BER, &_ber) != 0) {
				_ber = 0;
			}
			if (ioctl(_fd_fe, FE_READ_UNCORRECTED_BLOCKS, &_ublocks) != 0) {
				_ublocks = 0;
			}
			_strength = (_strength * 240) / 0xffff;
			_snr = (_snr * 15) / 0xffff;

			// Print Status
			if (showStatus) {
				SI_LOG_INFO("status %02x | signal %3u%% | snr %3u%% | ber %d | unc %d | Locked %d",
					_status, _strength, _snr, _ber, _ublocks,
					(_status & FE_HAS_LOCK) ? 1 : 0);
			}
		} else {
			PERROR("FE_READ_STATUS failed");
		}
	}

	bool Frontend::hasDeviceDataChanged() const {
		return _frontendData.hasFrontendDataChanged();
	}

	void Frontend::parseStreamString(const std::string &msg, const std::string &method) {
		double doubleVal = 0.0;
		int intVal = 0;
		std::string strVal;

		SI_LOG_DEBUG("Stream: %d, Parsing transport parameters...", _streamID);

		// Do this AT FIRST because of possible initializing of channel data !! else we will delete it again here !!
		doubleVal = StringConverter::getDoubleParameter(msg, method, "freq=");
		if (doubleVal != -1) {
			// new frequency so initialize FrontendData and 'remove' all used PIDS
			_frontendData.initialize();
			_frontendData.setFrequency(doubleVal * 1000.0);
			SI_LOG_DEBUG("Stream: %d, New frequency requested, clearing channel data...", _streamID);
		}
		// !!!!
		intVal = StringConverter::getIntParameter(msg, method, "sr=");
		if (intVal != -1) {
			_frontendData.setSymbolRate(intVal * 1000);
		}
		const input::InputSystem msys = StringConverter::getMSYSParameter(msg, method);
		if (msys != input::InputSystem::UNDEFINED) {
			_frontendData.setDeliverySystem(msys);
		}
		if (StringConverter::getStringParameter(msg, method, "pol=", strVal) == true) {
			if (strVal.compare("h") == 0) {
				_frontendData.setPolarization(POL_H);
			} else if (strVal.compare("v") == 0) {
				_frontendData.setPolarization(POL_V);
			}
		}
		intVal = StringConverter::getIntParameter(msg, method, "src=");
		if (intVal != -1) {
			_frontendData.setDiSEqcSource(intVal);
		}
		if (StringConverter::getStringParameter(msg, method, "plts=", strVal) == true) {
			// "on", "off"[, "auto"]
			if (strVal.compare("on") == 0) {
				_frontendData.setPilotTones(PILOT_ON);
			} else if (strVal.compare("off") == 0) {
				_frontendData.setPilotTones(PILOT_OFF);
			} else if (strVal.compare("auto") == 0) {
				_frontendData.setPilotTones(PILOT_AUTO);
			} else {
				SI_LOG_ERROR("Unknown Pilot Tone [%s]", strVal.c_str());
				_frontendData.setPilotTones(PILOT_AUTO);
			}
		}
		if (StringConverter::getStringParameter(msg, method, "ro=", strVal) == true) {
			// "0.35", "0.25", "0.20"[, "auto"]
			if (strVal.compare("0.35") == 0) {
				_frontendData.setRollOff(ROLLOFF_35);
			} else if (strVal.compare("0.25") == 0) {
				_frontendData.setRollOff(ROLLOFF_25);
			} else if (strVal.compare("0.20") == 0) {
				_frontendData.setRollOff(ROLLOFF_20);
			} else if (strVal.compare("auto") == 0) {
				_frontendData.setRollOff(ROLLOFF_AUTO);
			} else {
				SI_LOG_ERROR("Unknown Rolloff [%s]", strVal.c_str());
				_frontendData.setRollOff(ROLLOFF_AUTO);
			}
		}
		if (StringConverter::getStringParameter(msg, method, "fec=", strVal) == true) {
			const int fec = atoi(strVal.c_str());
			// "12", "23", "34", "56", "78", "89", "35", "45", "910"
			if (fec == 12) {
				_frontendData.setFEC(FEC_1_2);
			} else if (fec == 23) {
				_frontendData.setFEC(FEC_2_3);
			} else if (fec == 34) {
				_frontendData.setFEC(FEC_3_4);
			} else if (fec == 35) {
				_frontendData.setFEC(FEC_3_5);
			} else if (fec == 45) {
				_frontendData.setFEC(FEC_4_5);
			} else if (fec == 56) {
				_frontendData.setFEC(FEC_5_6);
			} else if (fec == 67) {
				_frontendData.setFEC(FEC_6_7);
			} else if (fec == 78) {
				_frontendData.setFEC(FEC_7_8);
			} else if (fec == 89) {
				_frontendData.setFEC(FEC_8_9);
			} else if (fec == 910) {
				_frontendData.setFEC(FEC_9_10);
			} else if (fec == 999) {
				_frontendData.setFEC(FEC_AUTO);
			} else {
				_frontendData.setFEC(FEC_NONE);
			}
		}
		if (StringConverter::getStringParameter(msg, method, "mtype=", strVal) == true) {
			if (strVal.compare("8psk") == 0) {
				_frontendData.setModulationType(PSK_8);
			} else if (strVal.compare("qpsk") == 0) {
				_frontendData.setModulationType(QPSK);
			} else if (strVal.compare("16qam") == 0) {
				_frontendData.setModulationType(QAM_16);
			} else if (strVal.compare("64qam") == 0) {
				_frontendData.setModulationType(QAM_64);
			} else if (strVal.compare("256qam") == 0) {
				_frontendData.setModulationType(QAM_256);
			}
		} else if (msys != input::InputSystem::UNDEFINED) {
			// no 'mtype' set so guess one according to 'msys'
			switch (msys) {
			case input::InputSystem::DVBS:
				_frontendData.setModulationType(QPSK);
				break;
			case input::InputSystem::DVBS2:
				_frontendData.setModulationType(PSK_8);
				break;
			case input::InputSystem::DVBT:
			case input::InputSystem::DVBT2:
			case input::InputSystem::DVBC:
				_frontendData.setModulationType(QAM_AUTO);
				break;
			default:
				SI_LOG_ERROR("Not supported delivery system");
				break;
			}
		}
		intVal = StringConverter::getIntParameter(msg, method, "specinv=");
		if (intVal != -1) {
			_frontendData.setSpectralInversion(intVal);
		}
		doubleVal = StringConverter::getDoubleParameter(msg, method, "bw=");
		if (doubleVal != -1) {
			_frontendData.setBandwidthHz(doubleVal * 1000000.0);
		}
		if (StringConverter::getStringParameter(msg, method, "tmode=", strVal) == true) {
			// "2k", "4k", "8k", "1k", "16k", "32k"[, "auto"]
			if (strVal.compare("1k") == 0) {
				_frontendData.setTransmissionMode(TRANSMISSION_MODE_1K);
			} else if (strVal.compare("2k") == 0) {
				_frontendData.setTransmissionMode(TRANSMISSION_MODE_2K);
			} else if (strVal.compare("4k") == 0) {
				_frontendData.setTransmissionMode(TRANSMISSION_MODE_4K);
			} else if (strVal.compare("8k") == 0) {
				_frontendData.setTransmissionMode(TRANSMISSION_MODE_8K);
			} else if (strVal.compare("16k") == 0) {
				_frontendData.setTransmissionMode(TRANSMISSION_MODE_16K);
			} else if (strVal.compare("32k") == 0) {
				_frontendData.setTransmissionMode(TRANSMISSION_MODE_32K);
			} else if (strVal.compare("auto") == 0) {
				_frontendData.setTransmissionMode(TRANSMISSION_MODE_AUTO);
			}
		}
		if (StringConverter::getStringParameter(msg, method, "gi=", strVal) == true) {
			// "14", "18", "116", "132","1128", "19128", "19256"
			const int gi = atoi(strVal.c_str());
			if (gi == 14) {
				_frontendData.setGuardInverval(GUARD_INTERVAL_1_4);
			} else if (gi == 18) {
				_frontendData.setGuardInverval(GUARD_INTERVAL_1_8);
			} else if (gi == 116) {
				_frontendData.setGuardInverval(GUARD_INTERVAL_1_16);
			} else if (gi == 132) {
				_frontendData.setGuardInverval(GUARD_INTERVAL_1_32);
			} else if (gi == 1128) {
				_frontendData.setGuardInverval(GUARD_INTERVAL_1_128);
			} else if (gi == 19128) {
				_frontendData.setGuardInverval(GUARD_INTERVAL_19_128);
			} else if (gi == 19256) {
				_frontendData.setGuardInverval(GUARD_INTERVAL_19_256);
			} else {
				_frontendData.setGuardInverval(GUARD_INTERVAL_AUTO);
			}
		}
		intVal = StringConverter::getIntParameter(msg, method, "plp=");
		if (intVal != -1) {
			_frontendData.setUniqueIDPlp(intVal);
		}
		intVal = StringConverter::getIntParameter(msg, method, "t2id=");
		if (intVal != -1) {
			_frontendData.setUniqueIDT2(intVal);
		}
		intVal = StringConverter::getIntParameter(msg, method, "sm=");
		if (intVal != -1) {
			_frontendData.setSISOMISO(intVal);
		}
		if (StringConverter::getStringParameter(msg, method, "pids=", strVal) == true ||
			StringConverter::getStringParameter(msg, method, "addpids=", strVal) == true) {
			processPID_L(strVal, true);
		}
		if (StringConverter::getStringParameter(msg, method, "delpids=", strVal) == true) {
			processPID_L(strVal, false);
		}
		SI_LOG_DEBUG("Stream: %d, Parsing transport parameters (Finished)", _streamID);
	}

	bool Frontend::update() {
		SI_LOG_DEBUG("Stream: %d, Updating frontend...", _streamID);
#ifndef SIMU
		// Setup, tune and set PID Filters
		if (_frontendData.hasFrontendDataChanged()) {
			_frontendData.resetFrontendDataChanged();
			_tuned = false;
			CLOSE_FD(_fd_dvr);
		}

		std::size_t timeout = 0;
		while (!setupAndTune()) {
			usleep(150000);
			++timeout;
			if (timeout > 3) {
				return false;
			}
		}

		updatePIDFilters();
#endif
		SI_LOG_DEBUG("Stream: %d, Updating frontend (Finished)", _streamID);
		return true;
	}

	bool Frontend::teardown() {
		for (std::size_t i = 0; i < MAX_PIDS; ++i) {
			if (_frontendData.isPIDUsed(i)) {
				SI_LOG_DEBUG("Stream: %d, Remove filter PID: %04d - fd: %03d - Packet Count: %d",
						_streamID, i, _frontendData.getDMXFileDescriptor(i), _frontendData.getPacketCounter(i));
				resetPid(i);
			} else if (_frontendData.getDMXFileDescriptor(i) != -1) {
				SI_LOG_ERROR("Stream: %d, !! No PID %d but still open DMX !!", _streamID, i);
				resetPid(i);
			}
		}
		_tuned = false;
		CLOSE_FD(_fd_fe);
		CLOSE_FD(_fd_dvr);
		{
			base::MutexLock lock(_mutex);
			_status = static_cast<fe_status_t>(0);
			_strength = 0;
			_snr = 0;
			_ber = 0;
			_ublocks = 0;
		}
		return true;
	}

	std::string Frontend::attributeDescribeString() const {
		const double freq = _frontendData.getFrequency() / 1000.0;
		const int srate = _frontendData.getSymbolRate() / 1000;
		const int locked = (_status & FE_HAS_LOCK) ? 1 : 0;

		std::string desc;
		std::string csv = _frontendData.getPidCSV();
		switch (_frontendData.getDeliverySystem()) {
			case input::InputSystem::DVBS:
			case input::InputSystem::DVBS2:
				// ver=1.0;src=<srcID>;tuner=<feID>,<level>,<lock>,<quality>,<frequency>,<polarisation>
				//             <system>,<type>,<pilots>,<roll_off>,<symbol_rate>,<fec_inner>;pids=<pid0>,..,<pidn>
				StringConverter::addFormattedString(desc, "ver=1.0;src=%d;tuner=%d,%d,%d,%d,%.2lf,%c,%s,%s,%s,%s,%d,%s;pids=%s",
						_frontendData.getDiSEqcSource(),
						_streamID + 1,
						_strength,
						locked,
						_snr,
						freq,
						(_frontendData.getPolarization() == POL_V) ? 'v' : 'h',
						StringConverter::delsys_to_string(_frontendData.getDeliverySystem()),
						StringConverter::modtype_to_sting(_frontendData.getModulationType()),
						StringConverter::pilot_tone_to_string(_frontendData.getPilotTones()),
						StringConverter::rolloff_to_sting(_frontendData.getRollOff()),
						srate,
						StringConverter::fec_to_string(_frontendData.getFEC()),
						csv.c_str());
				break;
			case input::InputSystem::DVBT:
			case input::InputSystem::DVBT2:
				// ver=1.1;tuner=<feID>,<level>,<lock>,<quality>,<freq>,<bw>,<msys>,<tmode>,<mtype>,<gi>,
				//               <fec>,<plp>,<t2id>,<sm>;pids=<pid0>,..,<pidn>
				StringConverter::addFormattedString(desc, "ver=1.1;tuner=%d,%d,%d,%d,%.2lf,%.3lf,%s,%s,%s,%s,%s,%d,%d,%d;pids=%s",
						_streamID + 1,
						_strength,
						locked,
						_snr,
						freq,
						_frontendData.getBandwidthHz() / 1000000.0,
						StringConverter::delsys_to_string(_frontendData.getDeliverySystem()),
						StringConverter::transmode_to_string(_frontendData.getTransmissionMode()),
						StringConverter::modtype_to_sting(_frontendData.getModulationType()),
						StringConverter::guardinter_to_string(_frontendData.getGuardInverval()),
						StringConverter::fec_to_string(_frontendData.getFEC()),
						_frontendData.getUniqueIDPlp(),
						_frontendData.getUniqueIDT2(),
						_frontendData.getSISOMISO(),
						csv.c_str());
				break;
			case input::InputSystem::DVBC:
				// ver=1.2;tuner=<feID>,<level>,<lock>,<quality>,<freq>,<bw>,<msys>,<mtype>,<sr>,<c2tft>,<ds>,
				//               <plp>,<specinv>;pids=<pid0>,..,<pidn>
				StringConverter::addFormattedString(desc, "ver=1.2;tuner=%d,%d,%d,%d,%.2lf,%.3lf,%s,%s,%d,%d,%d,%d,%d;pids=%s",
						_streamID + 1,
						_strength,
						locked,
						_snr,
						freq,
						_frontendData.getBandwidthHz() / 1000000.0,
						StringConverter::delsys_to_string(_frontendData.getDeliverySystem()),
						StringConverter::modtype_to_sting(_frontendData.getModulationType()),
						srate,
						_frontendData.getC2TuningFrequencyType(),
						_frontendData.getDataSlice(),
						_frontendData.getUniqueIDPlp(),
						_frontendData.getSpectralInversion(),
						csv.c_str());
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

	void Frontend::setupFrontend() {
#if SIMU
		sprintf(_fe_info.name, "Simulation DVB-S2/C/T Card");
		_fe_info.frequency_min = 1000000UL;
		_fe_info.frequency_max = 21000000UL;
		_fe_info.symbol_rate_min = 20000UL;
		_fe_info.symbol_rate_max = 250000UL;
#else
		// open frontend in readonly mode
		int fd_fe = open_fe(_path_to_fe, true);
		if (fd_fe < 0) {
			snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Found");
			PERROR("open_fe");
			return;
		}

		if (ioctl(fd_fe, FE_GET_INFO, &_fe_info) != 0) {
			snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Set");
			PERROR("FE_GET_INFO");
			CLOSE_FD(fd_fe);
			return;
		}
#endif
		SI_LOG_INFO("Frontend Name: %s", _fe_info.name);

		struct dtv_property dtvProperty;
#if SIMU
		dtvProperty.u.buffer.len = 4;
		dtvProperty.u.buffer.data[0] = SYS_DVBS;
		dtvProperty.u.buffer.data[1] = SYS_DVBS2;
		dtvProperty.u.buffer.data[2] = SYS_DVBT;
#  if FULL_DVB_API_VERSION >= 0x0505
		dtvProperty.u.buffer.data[3] = SYS_DVBC_ANNEX_A;
#  else
		dtvProperty.u.buffer.data[3] = SYS_DVBC_ANNEX_AC;
#  endif
#else
		dtvProperty.cmd = DTV_ENUM_DELSYS;
		dtvProperty.u.data = DTV_UNDEFINED;

		struct dtv_properties dtvProperties;
		dtvProperties.num = 1;       // size
		dtvProperties.props = &dtvProperty;
		if (ioctl(fd_fe, FE_GET_PROPERTY, &dtvProperties ) != 0) {
			// If we are here it can mean we have an DVB-API <= 5.4
			SI_LOG_DEBUG("Unable to enumerate the delivery systems, retrying via old API Call");
			auto index = 0;
			switch (_fe_info.type) {
				case FE_QPSK:
					if (_fe_info.caps & FE_CAN_2G_MODULATION) {
						dtvProperty.u.buffer.data[index] = SYS_DVBS2;
						++index;
					}
					dtvProperty.u.buffer.data[index] = SYS_DVBS;
					++index;
					break;
				case FE_OFDM:
					if (_fe_info.caps & FE_CAN_2G_MODULATION) {
						dtvProperty.u.buffer.data[index] = SYS_DVBT2;
						++index;
					}
					dtvProperty.u.buffer.data[index] = SYS_DVBT;
					++index;
					break;
				case FE_QAM:
#  if FULL_DVB_API_VERSION >= 0x0505
					dtvProperty.u.buffer.data[index] = SYS_DVBC_ANNEX_A;
#  else
					dtvProperty.u.buffer.data[index] = SYS_DVBC_ANNEX_AC;
#  endif
					++index;
					break;
				case FE_ATSC:
					if (_fe_info.caps & (FE_CAN_QAM_64 | FE_CAN_QAM_256 | FE_CAN_QAM_AUTO)) {
						dtvProperty.u.buffer.data[index] = SYS_DVBC_ANNEX_B;
						++index;
						break;
					}
				// Fall-through
				default:
					SI_LOG_ERROR("Frontend does not have any known delivery systems");
					CLOSE_FD(fd_fe);
					return;
			}
			dtvProperty.u.buffer.len = index;
		}
		CLOSE_FD(fd_fe);
#endif
		// get capability of this frontend and count the delivery systems
		for (std::size_t i = 0; i < dtvProperty.u.buffer.len; i++) {
			switch (dtvProperty.u.buffer.data[i]) {
				case SYS_DSS:
					SI_LOG_INFO("Frontend Type: DSS");
					break;
				case SYS_DVBS:
					SI_LOG_INFO("Frontend Type: Satellite (DVB-S)");
					break;
				case SYS_DVBS2:
					// we only count DVB-S2
					++_dvbs2;
					SI_LOG_INFO("Frontend Type: Satellite (DVB-S2)");
					break;
				case SYS_DVBT:
					++_dvbt;
					SI_LOG_INFO("Frontend Type: Terrestrial (DVB-T)");
					break;
				case SYS_DVBT2:
					++_dvbt2;
					SI_LOG_INFO("Frontend Type: Terrestrial (DVB-T2)");
					break;
#if FULL_DVB_API_VERSION >= 0x0505
				case SYS_DVBC_ANNEX_A:
					if (_dvbc == 0) {
						++_dvbc;
					}
					SI_LOG_INFO("Frontend Type: Cable (Annex A)");
					break;
				case SYS_DVBC_ANNEX_C:
					if (_dvbc == 0) {
						++_dvbc;
					}
					SI_LOG_INFO("Frontend Type: Cable (Annex C)");
					break;
#else
				case SYS_DVBC_ANNEX_AC:
					if (_dvbc == 0) {
						++_dvbc;
					}
					SI_LOG_INFO("Frontend Type: Cable (Annex C)");
					break;
#endif
				case SYS_DVBC_ANNEX_B:
					if (_dvbc == 0) {
						++_dvbc;
					}
					SI_LOG_INFO("Frontend Type: Cable (Annex B)");
					break;
				default:
					SI_LOG_INFO("Frontend Type: Unknown %d", dtvProperty.u.buffer.data[i]);
					break;
			}
		}
		SI_LOG_INFO("Frontend Freq: %d Hz to %d Hz", _fe_info.frequency_min, _fe_info.frequency_max);
		SI_LOG_INFO("Frontend srat: %d symbols/s to %d symbols/s", _fe_info.symbol_rate_min, _fe_info.symbol_rate_max);

		// Set delivery systems
		if (_dvbs2 > 0) {
			_deliverySystem.push_back(new input::dvb::delivery::DVBS(_streamID));
		}
		if (_dvbt > 0 || _dvbt2 > 0) {
			_deliverySystem.push_back(new input::dvb::delivery::DVBT(_streamID));
		}
		if (_dvbc > 0) {
			_deliverySystem.push_back(new input::dvb::delivery::DVBC(_streamID));
		}
	}

	int Frontend::open_fe(const std::string &path, bool readonly) const {
		const int fd = open(path.c_str(), (readonly ? O_RDONLY : O_RDWR) | O_NONBLOCK);
		if (fd  < 0) {
			PERROR("FRONTEND DEVICE");
		}
		return fd;
	}

	int Frontend::open_dmx(const std::string &path) {
		const int fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
		if (fd < 0) {
			PERROR("DMX DEVICE");
		}
		return fd;
	}

	int Frontend::open_dvr(const std::string &path) {
		const int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
		if (fd < 0) {
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

	bool Frontend::updateInputDevice() {
		//	base::MutexLock lock(_mutex);
		if (_frontendData.hasPIDTableChanged()) {
			if (isTuned()) {
				update();
			} else {
				SI_LOG_INFO("Stream: %d, Updating PID filters requested but frontend not tuned!",
							_streamID);
			}
		}
		return true;
	}

	bool Frontend::tune() {
		const input::InputSystem delsys = _frontendData.getDeliverySystem();
		for (input::dvb::delivery::SystemVector::iterator it = _deliverySystem.begin();
		     it != _deliverySystem.end();
		     ++it) {
			if ((*it)->isCapableOf(delsys)) {
				return (*it)->tune(_fd_fe, _frontendData);
			}
		}
		return false;
	}

	bool Frontend::setupAndTune() {
		if (!_tuned) {
			// Check if we have already opened a FE
			if (_fd_fe == -1) {
				_fd_fe = open_fe(_path_to_fe, false);
				SI_LOG_INFO("Stream: %d, Opened %s fd: %d", _streamID, _path_to_fe.c_str(), _fd_fe);
			}
			// try tuning
			std::size_t timeout = 0;
			while (!tune()) {
				usleep(450000);
				++timeout;
				if (timeout > 3) {
					return false;
				}
			}
			SI_LOG_INFO("Stream: %d, Waiting on lock...", _streamID);

			// check if frontend is locked, if not try a few times
			timeout = 0;
			while (timeout < 4) {
				_status = FE_TIMEDOUT;
				// first read status
				if (ioctl(_fd_fe, FE_READ_STATUS, &_status) == 0) {
					if (_status & FE_HAS_LOCK) {
						// We are tuned now
						_tuned = true;
						SI_LOG_INFO("Stream: %d, Tuned and locked (FE status 0x%X)", _streamID, _status);
						break;
					} else {
						SI_LOG_INFO("Stream: %d, Not locked yet   (FE status 0x%X)...", _streamID, _status);
					}
				}
				usleep(150000);
				++timeout;
			}
		}
		// Check if we have already a DVR open and are tuned
		if (_fd_dvr == -1 && _tuned) {
			// try opening DVR, try again if fails
			std::size_t timeout = 0;
			while ((_fd_dvr = open_dvr(_path_to_dvr)) == -1) {
				usleep(150000);
				++timeout;
				if (timeout > 3) {
					return false;
				}
			}
			SI_LOG_INFO("Stream: %d, Opened %s fd: %d", _streamID, _path_to_dvr.c_str(), _fd_dvr);

			{
				base::MutexLock lock(_mutex);
				if (ioctl(_fd_dvr, DMX_SET_BUFFER_SIZE, _dvrBufferSize) == -1) {
					PERROR("DMX_SET_BUFFER_SIZE failed");
				}
			}
		}
		return (_fd_dvr != -1) && _tuned;
	}

	void Frontend::resetPid(int pid) {
		if (_frontendData.getDMXFileDescriptor(pid) != -1 &&
			ioctl(_frontendData.getDMXFileDescriptor(pid), DMX_STOP) != 0) {
			PERROR("DMX_STOP");
		}
		_frontendData.closeDMXFileDescriptor(pid);
		_frontendData.resetPid(pid);
	}

	bool Frontend::updatePIDFilters() {
		if (_frontendData.hasPIDTableChanged()) {
			_frontendData.resetPIDTableChanged();
			SI_LOG_INFO("Stream: %d, Updating PID filters...", _streamID);
			int i;
			for (i = 0; i < MAX_PIDS; ++i) {
				// check if PID is used or removed
				if (_frontendData.isPIDUsed(i)) {
					// check if we have no DMX for this PID, then open one
					if (_frontendData.getDMXFileDescriptor(i) == -1) {
						_frontendData.setDMXFileDescriptor(i, open_dmx(_path_to_dmx));
						std::size_t timeout = 0;
						while (set_demux_filter(_frontendData.getDMXFileDescriptor(i), i) != 1) {
							usleep(350000);
							++timeout;
							if (timeout > 3) {
								return false;
							}
						}
						SI_LOG_DEBUG("Stream: %d, Set filter PID: %04d - fd: %03d%s",
								_streamID, i, _frontendData.getDMXFileDescriptor(i), _frontendData.isPMT(i) ? " - PMT" : "");
					}
				} else if (_frontendData.getDMXFileDescriptor(i) != -1) {
					// We have a DMX but no PID anymore, so reset it
					SI_LOG_DEBUG("Stream: %d, Remove filter PID: %04d - fd: %03d - Packet Count: %d",
							_streamID, i, _frontendData.getDMXFileDescriptor(i), _frontendData.getPacketCounter(i));
					resetPid(i);
				}
			}
		}
		return true;
	}

	void Frontend::processPID_L(const std::string &pids, bool add) {
		std::string::size_type begin = 0;
		if (pids == "all" ) {
			// All pids requested then 'remove' all used PIDS
			for (std::size_t i = 0; i < MAX_PIDS; ++i) {
				_frontendData.setPID(i, false);
			}
			_frontendData.setAllPID(add);
		} else {
			for (;; ) {
				const std::string::size_type end = pids.find_first_of(",", begin);
				if (end != std::string::npos) {
					const int pid = atoi(pids.substr(begin, end - begin).c_str());
					_frontendData.setPID(pid, add);
					begin = end + 1;
				} else {
					// Get the last one
					if (begin < pids.size()) {
						const int pid = atoi(pids.substr(begin, end - begin).c_str());
						_frontendData.setPID(pid, add);
					}
					break;
				}
			}
			// always request PID 0 - Program Association Table (PAT)
			if (add && !_frontendData.isPIDUsed(0)) {
				_frontendData.setPID(0, true);
			}
		}
	}

} // namespace dvb
} // namespace input
