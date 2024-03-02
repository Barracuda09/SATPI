/* Frontend.cpp

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
#include <input/dvb/Frontend.h>

#include <base/StopWatch.h>
#include <Log.h>
#include <Utils.h>
#include <Stream.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>
#include <input/dvb/dvbfix.h>
#include <input/dvb/FrontendData.h>
#include <input/dvb/delivery/DVBC.h>
#include <input/dvb/delivery/DVBS.h>
#include <input/dvb/delivery/DVBT.h>
#include <input/dvb/delivery/DiSEqc.h>

#include <chrono>
#include <thread>

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <poll.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <base/StopWatch.h>

namespace input::dvb {

// =============================================================================
// -- Static const data --------------------------------------------------------
// =============================================================================

static constexpr unsigned int DEFAULT_DVR_BUFFER_SIZE       = 3;
static constexpr unsigned int MAX_DVR_BUFFER_SIZE           = 3 * 10;
static constexpr unsigned long MAX_WAIT_ON_LOCK_TIMEOUT     = 3500;
static constexpr unsigned long DEFAULT_WAIT_ON_LOCK_TIMEOUT = 1000;

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

Frontend::Frontend(
		FeIndex index,
		const std::string &appDataPath,
		const std::string &fe,
		const std::string &dvr,
		const std::string &dmx) :
	Device(index),
	_tuned(false),
	_fd_fe(-1),
	_fd_dmx(-1),
	_path_to_fe(fe),
	_path_to_dvr(dvr),
	_path_to_dmx(dmx),
	_dvbVersion(0),
	_transform(appDataPath),
	_dvbs(0),
	_dvbs2(0),
	_dvbt(0),
	_dvbt2(0),
	_dvbc(0),
	_dvbc2(0),
	_dvrBufferSizeMB(DEFAULT_DVR_BUFFER_SIZE),
	_waitOnLockTimeout(DEFAULT_WAIT_ON_LOCK_TIMEOUT) {
	snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Set");
	setupFrontend();
#if FULL_DVB_API_VERSION >= 0x050A
	_oldApiCallStats = false;
#else
	_oldApiCallStats = true;
#endif
}

// =============================================================================
//  -- Static functions --------------------------------------------------------
// =============================================================================
namespace {
	// Called recursive
	void getAttachedFrontends(
			StreamSpVector &streamVector,
			const std::string &appDataPath,
			decrypt::dvbapi::SpClient decrypt,
			const std::string &path,
			const std::string &startPath) {
		const std::string ADAPTER = startPath + "/adapter@#1";
		const std::string DMX = ADAPTER + "/demux@#2";
		const std::string DVR = ADAPTER + "/dvr@#2";
		const std::string FRONTEND = ADAPTER + "/frontend@#2";
	#if SIMU
		// unused var
		(void)path;

		const std::string fe0 = StringConverter::stringFormat(FRONTEND.data(), 0, 0);
		const std::string dvr0 = StringConverter::stringFormat(DVR.data(), 0, 0);
		const std::string dmx0 = StringConverter::stringFormat(DMX.data(), 0, 0);
		input::dvb::SpFrontend frontend0 = std::make_shared<input::dvb::Frontend>(0, appDataPath, fe0, dvr0, dmx0);
		streamVector.push_back(Stream::makeSP(frontend0, decrypt));

		const std::string fe1 = StringConverter::stringFormat(FRONTEND.data(), 1, 0);
		const std::string dvr1 = StringConverter::stringFormat(DVR.data(), 1, 0);
		const std::string dmx1 = StringConverter::stringFormat(DMX.data(), 1, 0);
		input::dvb::SpFrontend frontend1 = std::make_shared<input::dvb::Frontend>(1, appDataPath, fe1, dvr1, dmx1);
		streamVector.push_back(Stream::makeSP(frontend1, decrypt));
	#else
		dirent **file_list;
		const int n = scandir(path.data(), &file_list, nullptr, versionsort);
		if (n > 0) {
			for (int i = 0; i < n; ++i) {
				const std::string full_path = StringConverter::stringFormat("@#1/@#2", path, file_list[i]->d_name);
				struct stat stat_buf;
				if (stat(full_path.data(), &stat_buf) == 0) {
					switch (stat_buf.st_mode & S_IFMT) {
						case S_IFCHR: // character device
							if (strstr(file_list[i]->d_name, "frontend") != nullptr) {
								int fe_nr;
								sscanf(file_list[i]->d_name, "frontend%d", &fe_nr);
								int adapt_nr;
								const std::string ADAPTER_TMP = startPath + "/adapter%d";
								sscanf(path.data(), ADAPTER_TMP.data(), &adapt_nr);

								// Make new paths
								const std::string fe = StringConverter::stringFormat(FRONTEND.data(), adapt_nr, fe_nr);
								const std::string dvr = StringConverter::stringFormat(DVR.data(), adapt_nr, fe_nr);
								const std::string dmx = StringConverter::stringFormat(DMX.data(), adapt_nr, fe_nr);

								// Make new frontend here
								const StreamSpVector::size_type size = streamVector.size();
								const input::dvb::SpFrontend frontend = std::make_shared<input::dvb::Frontend>(size, appDataPath, fe, dvr, dmx);
								streamVector.push_back(Stream::makeSP(frontend, decrypt));
							}
							break;
						case S_IFDIR:
							// do not use dir '.' an '..'
							if (strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
								getAttachedFrontends(streamVector, appDataPath, decrypt, full_path, startPath);
							}
							break;
						default:
							// Do nothing here, just find next
							break;
					}
				}
				free(file_list[i]);
			}
		}
	#endif
	}
}

// =============================================================================
//  -- Static member functions -------------------------------------------------
// =============================================================================

void Frontend::enumerate(
		StreamSpVector &streamVector,
		const std::string &appDataPath,
		decrypt::dvbapi::SpClient decrypt,
		const std::string &dvbAdapterPath) {
	const StreamSpVector::size_type beginSize = streamVector.size();
	SI_LOG_INFO("Detecting frontends in: @#1", dvbAdapterPath);
	getAttachedFrontends(streamVector, appDataPath, decrypt, dvbAdapterPath, dvbAdapterPath);
	const StreamSpVector::size_type endSize = streamVector.size();
	SI_LOG_INFO("Frontends found: @#1", endSize - beginSize);
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void Frontend::doAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "frontendname", _fe_info.name);
	ADD_XML_ELEMENT(xml, "pathname", _path_to_fe);
	ADD_XML_ELEMENT(xml, "freq", StringConverter::stringFormat("@#1 Hz to @#2 Hz", _fe_info.frequency_min, _fe_info.frequency_max));
	ADD_XML_ELEMENT(xml, "symbol", StringConverter::stringFormat("@#1 symbols/s to @#2 symbols/s", _fe_info.symbol_rate_min, _fe_info.symbol_rate_max));
	ADD_XML_ELEMENT(xml, "dvbversion", HEX(_dvbVersion, 4));

	ADD_XML_NUMBER_INPUT(xml, "dvrbuffer", _dvrBufferSizeMB, 0, MAX_DVR_BUFFER_SIZE);
	ADD_XML_NUMBER_INPUT(xml, "waitOnLockTimeout", _waitOnLockTimeout, 0, MAX_WAIT_ON_LOCK_TIMEOUT);
	ADD_XML_CHECKBOX(xml, "forceOldStyleStatus", (_oldApiCallStats ? "true" : "false"));

#ifdef LIBDVBCSA
	_dvbapiData.addToXML(xml);
#endif

	// Channel
	_frontendData.addToXML(xml);

	ADD_XML_ELEMENT(xml, "transformation", _transform.toXML());

	for (std::size_t i = 0; i < _deliverySystem.size(); ++i) {
		ADD_XML_N_ELEMENT(xml, "deliverySystem", i, _deliverySystem[i]->toXML());
	}
}

void Frontend::doFromXML(const std::string &xml) {
	std::string element;
	if (findXMLElement(xml, "dvrbuffer.value", element)) {
		const unsigned int newSize = std::stoi(element);
		_dvrBufferSizeMB = (newSize < MAX_DVR_BUFFER_SIZE) ?
			newSize : DEFAULT_DVR_BUFFER_SIZE;
	}
	if (findXMLElement(xml, "waitOnLockTimeout.value", element)) {
		const unsigned int c = std::stoi(element);
		_waitOnLockTimeout = (c < MAX_WAIT_ON_LOCK_TIMEOUT) ? c : MAX_WAIT_ON_LOCK_TIMEOUT;
	}
	if (findXMLElement(xml, "forceOldStyleStatus.value", element)) {
		_oldApiCallStats = (element == "true") ? true : false;
	}
	for (std::size_t i = 0; i < _deliverySystem.size(); ++i) {
		const std::string deliverySystem = StringConverter::stringFormat("deliverySystem@#1", i);
		if (findXMLElement(xml, deliverySystem, element)) {
			_deliverySystem[i]->fromXML(element);
		}
	}
	if (findXMLElement(xml, "transformation", element)) {
		_transform.fromXML(element);
	}
	_frontendData.fromXML(xml);
}

// =============================================================================
//  -- input::Device -----------------------------------------------------------
// =============================================================================

void Frontend::addDeliverySystemCount(
		std::size_t &dvbs2,
		std::size_t &dvbt,
		std::size_t &dvbt2,
		std::size_t &dvbc,
		std::size_t &dvbc2) {
	dvbs2 += _transform.advertiseAsDVBS2() ? _dvbc : ((_dvbs2 > 0) ? _dvbs2 : _dvbs);
	dvbt  += _dvbt;
	dvbt2 += _dvbt2;
	dvbc  +=  _transform.advertiseAsDVBC() ? ((_dvbs2 > 0) ? _dvbs2 : _dvbs) : _dvbc;
	dvbc2 += _dvbc2;
}

bool Frontend::isDataAvailable() {
	thread_local pollfd pfd;
	pfd.fd = _fd_dmx;
	pfd.events = POLLIN;
	pfd.revents = 0;
	const int pollRet = ::poll(&pfd, 1, 100);
	if (pollRet > 0) {
		return (pfd.revents & POLLIN) == POLLIN;
	} else if (pollRet < 0) {
		SI_LOG_PERROR("Frontend: @#1, Error during polling frontend for data", _feID);
		return false;
	}
	// Time-out return false
	return false;
}

bool Frontend::readTSPackets(mpegts::PacketBuffer& buffer) {
	// try read maximum amount of bytes from DMX
	const auto readSize = ::read(_fd_dmx, buffer.getWriteBufferPtr(), buffer.getAmountOfBytesToWrite());
	if (readSize > 0) {
		buffer.addAmountOfBytesWritten(readSize);
		if (buffer.full()) {
			_frontendData.getFilter().filterData(_feID, buffer, false);

			return true;
		}
	} else if (readSize < 0) {
		SI_LOG_PERROR("Frontend: @#1, Error reading data..", _feID);
	} else {
		SI_LOG_ERROR("Frontend: @#1, Error reading data: 0 Bytes available..", _feID);
	}
	return false;
}

bool Frontend::capableOf(const input::InputSystem system) const {
	for (const input::dvb::delivery::UpSystem& deliverySystem : _deliverySystem) {
		if (deliverySystem->isCapableOf(system)) {
			return true;
		}
	}
	return false;
}

bool Frontend::capableToShare(const TransportParamVector& UNUSED(params)) const {
	// @TODO: Check if the same "channel" is requested for sharing this frontend
	//        maybe check if sharing is allowed
	return false;
}

bool Frontend::capableToTransform(const TransportParamVector& params) const {
	const input::InputSystem system = _transform.getTransformationSystemFor(params);
	return capableOf(system);
}

bool Frontend::isLockedByOtherProcess() const {
	int fd = ::open(_path_to_fe.data(), O_RDWR);
	if (fd  < 0) {
		return true;
	}
	CLOSE_FD(fd);
	return false;
}

bool Frontend::monitorSignal(const bool showStatus) {
#if SIMU
	(void)showStatus;
	_frontendData.setMonitorData(FE_HAS_LOCK, 214, 15, 0, 0);
	return true;
#else
	if (_fd_fe == -1) {
		return false;
	}
	// first read status
	fe_status_t status{};
	if (::ioctl(_fd_fe, FE_READ_STATUS, &status) == 0) {
		uint16_t strength = 0;
		uint16_t snr = 0;
		uint32_t ber = 0;
		uint32_t ublocks = 0;

#  if FULL_DVB_API_VERSION >= 0x050A
		if (!_oldApiCallStats) {
			struct dtv_property p[3];
			int size = 0;
			#define FILL_PROP(CMD, DATA) { p[size].cmd = CMD; p[size].u.data = DATA; ++size; }

			FILL_PROP(DTV_STAT_SIGNAL_STRENGTH, DTV_UNDEFINED);
			FILL_PROP(DTV_STAT_CNR, DTV_UNDEFINED);
			FILL_PROP(DTV_STAT_ERROR_BLOCK_COUNT, DTV_UNDEFINED);

			#undef FILL_PROP

			struct dtv_properties cmdseq;
			cmdseq.num = size;
			cmdseq.props = p;

			if (ioctl(_fd_fe, FE_GET_PROPERTY, &cmdseq) == -1) {
				SI_LOG_PERROR("Frontend: @#1, FE_GET_PROPERTY failed", _feID);
			}

			const auto strengthScale = cmdseq.props[0].u.st.stat[0].scale;
			switch (strengthScale) {
				case FE_SCALE_DECIBEL:
					strength = cmdseq.props[0].u.st.stat[0].svalue * 0.0001;
					break;
				case FE_SCALE_RELATIVE:
					strength = (static_cast<uint32_t>(cmdseq.props[0].u.st.stat[0].uvalue) * 100) >> 16;
					break;
				case FE_SCALE_NOT_AVAILABLE:
				default:
					_oldApiCallStats = true;
					break;
			}
			const auto cnrScale = cmdseq.props[1].u.st.stat[0].scale;
			switch (cnrScale) {
				case FE_SCALE_DECIBEL:
					snr = cmdseq.props[1].u.st.stat[0].svalue * 0.0001;
					break;
				case FE_SCALE_RELATIVE:
					snr = (static_cast<uint32_t>(cmdseq.props[1].u.st.stat[0].uvalue) * 100) >> 16;
					break;
				case FE_SCALE_NOT_AVAILABLE:
				default:
					_oldApiCallStats = true;
					break;
			}
			const auto berScale = cmdseq.props[2].u.st.stat[0].scale;
			switch (berScale) {
				case FE_SCALE_DECIBEL:
				case FE_SCALE_RELATIVE:
				case FE_SCALE_COUNTER:
					ber = cmdseq.props[2].u.st.stat[0].uvalue & 0x7FFF;
					break;
				case FE_SCALE_NOT_AVAILABLE:
				default:
					ber = 0;
					break;
			}
		}
#  endif
		if (_oldApiCallStats) {
			// some frontends might not support all these ioctls
			if (::ioctl(_fd_fe, FE_READ_SIGNAL_STRENGTH, &strength) != 0) {
				strength = 0;
			}
			if (::ioctl(_fd_fe, FE_READ_SNR, &snr) != 0) {
				snr = 0;
			}
			if (::ioctl(_fd_fe, FE_READ_BER, &ber) != 0) {
				ber = 0;
			}
			if (::ioctl(_fd_fe, FE_READ_UNCORRECTED_BLOCKS, &ublocks) != 0) {
				ublocks = 0;
			}
			strength = (strength * 240) / 0xffff;
			snr = (snr * 15) / 0xffff;
		}

		// Print Status
		if (showStatus) {
			SI_LOG_INFO("status @#1 | signal @#2% | snr @#3% | ber @#4 | unc @#5 | Locked @#6",
				HEX(status, 2), DIGIT(strength, 3), DIGIT(snr, 3), ber, ublocks,
				(status & FE_HAS_LOCK) ? 1 : 0);
		}
		_frontendData.setMonitorData(status, strength, snr, ber, ublocks);
	} else {
		SI_LOG_PERROR("Frontend: @#1, FE_READ_STATUS failed", _feID);
	}
	return (status & FE_HAS_LOCK) == FE_HAS_LOCK;
#endif
}

bool Frontend::hasDeviceFrequencyChanged() const {
	return _frontendData.hasDeviceFrequencyChanged();
}

void Frontend::parseStreamString(const TransportParamVector& params) {
	SI_LOG_INFO("Frontend: @#1, Parsing transport parameters...", _feID);

	// Do we need to transform this request?
	const TransportParamVector transParams = _transform.transformStreamString(_feID, params);

	_frontendData.parseStreamString(_feID, transParams);

	SI_LOG_DEBUG("Frontend: @#1, Parsing transport parameters (Finished)", _feID);
}

bool Frontend::update() {
	SI_LOG_INFO("Frontend: @#1, Updating frontend...", _feID);
	base::StopWatch sw;
	sw.start();
	// Setup, tune and set PID Filters
	if (_frontendData.hasDeviceFrequencyChanged()) {
		_frontendData.resetDeviceFrequencyChanged();
		_tuned = false;
		// Close active PIDs
		closeActivePIDFilters();
		closeDMX();
		closeFE();
		// After close wait a moment before opening it again
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	if (!setupAndTune()) {
		SI_LOG_INFO("Frontend: @#1, Updating frontend (Failed)", _feID);
		return false;
	}
	updatePIDFilters();
	const unsigned long time = sw.getIntervalUS();
	SI_LOG_INFO("Frontend: @#1, Updating frontend (Finished in @#2 us)", _feID, time);
	return true;
}

bool Frontend::teardown() {
	// Close active PIDs
	closeActivePIDFilters();
	_tuned = false;
	// Do teardown of frontends before closing FE
	for (const input::dvb::delivery::UpSystem& deliverySystem : _deliverySystem) {
		deliverySystem->teardown(_fd_fe);
	}
	closeDMX();
	closeFE();
	_frontendData.initialize();
	_transform.resetTransformFlag();
	return true;
}

std::string Frontend::attributeDescribeString() const {
	const DeviceData &data = _transform.transformDeviceData(_frontendData);
	return data.attributeDescribeString(_feID);
}

void Frontend::closeActivePIDFilters() {
	_frontendData.getFilter().closeActivePIDFilters(_feID,
		// closePid lambda function
		[&](const int pid) {
			uint16_t p = pid;
			if (::ioctl(_fd_dmx, DMX_REMOVE_PID, &p) != 0) {
				SI_LOG_PERROR("Frontend: @#1, DMX_REMOVE_PID: PID @#2", _feID, PID(p));
				return false;
			}
			return true;
		});
}

void Frontend::updatePIDFilters() {
	if (!_tuned) {
		SI_LOG_INFO("Frontend: @#1, Update PID filters requested, but frontend not tuned!", _feID);
		return;
	}
	_frontendData.getFilter().updatePIDFilters(_feID,
		// openPid lambda function
		[&](const int pid) {
			uint16_t p = pid;
			// Check if we have already a DMX open
			if (_fd_dmx == -1) {
				// try opening DMX, try again if fails
				std::size_t timeout = 0;
				while ((_fd_dmx = openDMX(_path_to_dmx)) == -1) {
					std::this_thread::sleep_for(std::chrono::milliseconds(20));
					++timeout;
					if (timeout > 3) {
						return false;
					}
				}
				SI_LOG_INFO("Frontend: @#1, Opened @#2 using fd: @#3", _feID, _path_to_dmx, _fd_dmx);
				if (_dvrBufferSizeMB > 0) {
					const unsigned int size = _dvrBufferSizeMB * 1024 * 1024;
					if (::ioctl(_fd_dmx, DMX_SET_BUFFER_SIZE, size) != 0) {
						SI_LOG_PERROR("Frontend: @#1, Failed to set DMX_SET_BUFFER_SIZE", _feID);
					} else {
						SI_LOG_INFO("Frontend: @#1, Set DMX buffer size to @#2 Bytes", _feID, size);
					}
				}
				// Do we run on an Set-Top Box with Enigma2, then we need to set DMX_SET_SOURCE
				std::ifstream infoVersionFile("/proc/stb/info/version");
				if (infoVersionFile.is_open()) {
					int offset = 0;
					std::ifstream offsetFile("/proc/stb/frontend/dvr_source_offset");
					if (offsetFile.is_open()) {
						offsetFile >> offset;
					}
					int n = DMX_SOURCE_FRONT0 + _index.getID();
					if (::ioctl(_fd_dmx, DMX_SET_SOURCE, &n) != 0) {
						SI_LOG_PERROR("Frontend: @#1, Failed to set DMX_SET_SOURCE with (Src: @#2 - Offset: @#3)", _feID, n, offset);
						return false;
					}
					SI_LOG_INFO("Frontend: @#1, Set DMX_SET_SOURCE with (Src: @#2 - Offset: @#3)", _feID, n, offset);
				}
				struct dmx_pes_filter_params pesFilter{};
				pesFilter.pid      = p;
				pesFilter.input    = DMX_IN_FRONTEND;
				pesFilter.output   = DMX_OUT_TSDEMUX_TAP;
				pesFilter.pes_type = DMX_PES_OTHER;
				pesFilter.flags    = DMX_IMMEDIATE_START;
				if (::ioctl(_fd_dmx, DMX_SET_PES_FILTER, &pesFilter) != 0) {
					SI_LOG_PERROR("Frontend: @#1, Failed to set DMX_SET_PES_FILTER for PID: @#2", _feID, PID(p));
					return false;
				}
			} else if (::ioctl(_fd_dmx, DMX_ADD_PID, &p) != 0) {
				SI_LOG_PERROR("Frontend: @#1, Failed to set DMX_ADD_PID for PID: @#2", _feID, PID(p));
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return true;
		},
		// closePid lambda function
		[&](const int pid) {
			uint16_t p = pid;
			if (::ioctl(_fd_dmx, DMX_REMOVE_PID, &p) != 0) {
				SI_LOG_PERROR("Frontend: @#1, DMX_REMOVE_PID: PID @#2", _feID, PID(p));
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return true;
		});
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

void Frontend::setupFrontend() {
#if SIMU
	sprintf(_fe_info.name, "Simulation DVB-S2/C/T Card");
	_fe_info.frequency_min = 1000000UL;
	_fe_info.frequency_max = 21000000UL;
	_fe_info.symbol_rate_min = 20000UL;
	_fe_info.symbol_rate_max = 250000UL;
#else
	// open frontend in readonly mode
	int fd_fe = openFE(_path_to_fe, true);
	if (fd_fe < 0) {
		snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Found");
		SI_LOG_PERROR("openFE");
		return;
	}

	if (::ioctl(fd_fe, FE_GET_INFO, &_fe_info) != 0) {
		snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Set");
		SI_LOG_PERROR("FE_GET_INFO");
		CLOSE_FD(fd_fe);
		return;
	}
#endif
	SI_LOG_INFO("Frontend Name: @#1", _fe_info.name);
	if (_oldApiCallStats) {
		SI_LOG_INFO("Frontend Stat: Use legacy signal stats");
	} else {
		SI_LOG_INFO("Frontend Stat: Use advanced signal stats");
	}
	struct dtv_property dtvProperty[2];
#if SIMU
	dtvProperty[0].u.buffer.len = 4;
	dtvProperty[0].u.buffer.data[0] = SYS_DVBS;
	dtvProperty[0].u.buffer.data[1] = SYS_DVBS2;
	dtvProperty[0].u.buffer.data[2] = SYS_DVBT;
#  if FULL_DVB_API_VERSION >= 0x0505
	dtvProperty[0].u.buffer.data[3] = SYS_DVBC_ANNEX_A;
#  else
	dtvProperty[0].u.buffer.data[3] = SYS_DVBC_ANNEX_AC;
#  endif
#else
	dtvProperty[0].cmd    = DTV_ENUM_DELSYS;
	dtvProperty[0].u.data = DTV_UNDEFINED;
	dtvProperty[1].cmd    = DTV_API_VERSION;
	dtvProperty[1].u.data = DTV_UNDEFINED;

	struct dtv_properties dtvProperties;
	dtvProperties.num = 2;
	dtvProperties.props = dtvProperty;
	if (::ioctl(fd_fe, FE_GET_PROPERTY, &dtvProperties ) != 0) {
		// If we are here it can mean we have an DVB-API <= 5.4
		SI_LOG_DEBUG("Unable to enumerate the delivery systems, retrying via old API Call");
		auto index = 0;
		switch (_fe_info.type) {
			case FE_QPSK:
				if (_fe_info.caps & FE_CAN_2G_MODULATION) {
					dtvProperty[0].u.buffer.data[index] = SYS_DVBS2;
					++index;
				}
				dtvProperty[0].u.buffer.data[index] = SYS_DVBS;
				++index;
				break;
			case FE_OFDM:
				if (_fe_info.caps & FE_CAN_2G_MODULATION) {
					dtvProperty[0].u.buffer.data[index] = SYS_DVBT2;
					++index;
				}
				dtvProperty[0].u.buffer.data[index] = SYS_DVBT;
				++index;
				break;
			case FE_QAM:
#  if FULL_DVB_API_VERSION >= 0x0505
				dtvProperty[0].u.buffer.data[index] = SYS_DVBC_ANNEX_A;
#  else
				dtvProperty[0].u.buffer.data[index] = SYS_DVBC_ANNEX_AC;
#  endif
				++index;
				break;
			case FE_ATSC:
				if (_fe_info.caps & (FE_CAN_QAM_64 | FE_CAN_QAM_256 | FE_CAN_QAM_AUTO)) {
					dtvProperty[0].u.buffer.data[index] = SYS_DVBC_ANNEX_B;
					++index;
					break;
				}
			// Fall-through
			default:
				SI_LOG_ERROR("Frontend does not have any known delivery systems");
				CLOSE_FD(fd_fe);
				return;
		}
		dtvProperty[0].u.buffer.len = index;
	}
	CLOSE_FD(fd_fe);
#endif
	// get capability of this frontend and count the delivery systems
	for (std::size_t i = 0; i < dtvProperty[0].u.buffer.len; ++i) {
		const int deliveryType = dtvProperty[0].u.buffer.data[i];
		switch (deliveryType) {
			case SYS_DSS:
				SI_LOG_INFO("Frontend Type: DSS");
				break;
			case SYS_DVBS:
				++_dvbs;
				SI_LOG_INFO("Frontend Type: Satellite (DVB-S)");
				break;
			case SYS_DVBS2:
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
				SI_LOG_INFO("Frontend Type: Cable (Annex AC)");
				break;
#endif
			case SYS_DVBC_ANNEX_B:
				if (_dvbc == 0) {
					++_dvbc;
				}
				SI_LOG_INFO("Frontend Type: Cable (Annex B)");
				break;
			default:
				SI_LOG_INFO("Frontend Type: Unknown @#1", deliveryType);
				break;
		}
	}
	SI_LOG_INFO("Frontend Freq: @#1 Hz to @#2 Hz", _fe_info.frequency_min, _fe_info.frequency_max);
	SI_LOG_INFO("Frontend srat: @#1 symbols/s to @#2 symbols/s", _fe_info.symbol_rate_min, _fe_info.symbol_rate_max);

	_dvbVersion = FULL_DVB_API_VERSION;
	if (dtvProperty[1].u.data > 5) {
		const unsigned int minor = dtvProperty[1].u.buffer.data[0];
		const unsigned int major = dtvProperty[1].u.buffer.data[1];
		_dvbVersion = dtvProperty[1].u.data;
		SI_LOG_INFO("Frontend  API: @#1.@#2 (@#3)", major, minor, HEX(_dvbVersion, 4));
	}

	// Set delivery systems
	if ((_dvbs + _dvbs2) > 0) {
		_deliverySystem.push_back(input::dvb::delivery::UpSystem(
			new input::dvb::delivery::DVBS(_index, _feID, _path_to_fe, _dvbVersion)));
	}
	if ((_dvbt + _dvbt2) > 0) {
		_deliverySystem.push_back(input::dvb::delivery::UpSystem(
			new input::dvb::delivery::DVBT(_index, _feID, _path_to_fe, _dvbVersion)));
	}
	if (_dvbc > 0) {
		_deliverySystem.push_back(input::dvb::delivery::UpSystem(
			new input::dvb::delivery::DVBC(_index, _feID, _path_to_fe, _dvbVersion)));
	}
}

int Frontend::openFE(const std::string &path, const bool readonly) const {
	const int fd = ::open(path.data(), (readonly ? O_RDONLY : O_RDWR) | O_NONBLOCK);
	if (fd  < 0) {
		SI_LOG_PERROR("Frontend: @#1, Failed to open @#2", _feID, path);
	}
	return fd;
}

void Frontend::closeFE() {
	if (_fd_fe != -1) {
		SI_LOG_INFO("Frontend: @#1, Closing @#2 fd: @#3", _feID, _path_to_fe, _fd_fe);
		CLOSE_FD(_fd_fe);
	}
}

int Frontend::openDMX(const std::string &path) const {
	const int fd = ::open(path.data(), O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		SI_LOG_PERROR("Frontend: @#1, Failed to open @#2", _feID, path);
	}
	return fd;
}

void Frontend::closeDMX() {
	if (_fd_dmx != -1) {
		SI_LOG_INFO("Frontend: @#1, Closing @#2 fd: @#3", _feID, _path_to_dmx, _fd_dmx);
		CLOSE_FD(_fd_dmx);
	}
}

bool Frontend::tune() {
	const input::InputSystem delsys = _frontendData.getDeliverySystem();
	for (input::dvb::delivery::UpSystem& system : _deliverySystem) {
		if (system->isCapableOf(delsys)) {
			return system->tune(_fd_fe, _frontendData);
		}
	}
	return false;
}

bool Frontend::setupAndTune() {
	if (!_tuned) {
		base::StopWatch sw;
		// Check if we have already opened a FE
		if (_fd_fe == -1) {
			sw.start();
			_fd_fe = openFE(_path_to_fe, false);
			if (_fd_fe < 0) {
				const unsigned long openFETime = sw.getIntervalMS();
				SI_LOG_INFO("Frontend: @#1, Fail to open @#2 for Read/Write with fd: @#3 (@#4 ms)", _feID, _path_to_fe, _fd_fe, openFETime);
				_fd_fe = -1;
				return false;
			}
			const unsigned long openFETime = sw.getIntervalMS();
			SI_LOG_INFO("Frontend: @#1, Opened @#2 for Read/Write with fd: @#3 (@#4 ms)", _feID, _path_to_fe, _fd_fe, openFETime);
		}
		// try tuning
		if (!tune()) {
			return false;
		}
		_tuned = true;
		SI_LOG_INFO("Frontend: @#1, Tuned, waiting on lock...", _feID);
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		if (sw.getIntervalMS() < _waitOnLockTimeout) {
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
			// check if frontend is locked, if not try a few times (Untill TIMEOUT)
			for (int i = 1;; ++i) {
				fe_status_t status = FE_TIMEDOUT;
				// first read status
				if (::ioctl(_fd_fe, FE_READ_STATUS, &status) == 0) {
					if (status & FE_HAS_LOCK) {
						// We are tuned now, add some tuning stats
						_frontendData.setMonitorData(FE_HAS_LOCK, 100, 8, 0, 0);
						SI_LOG_INFO("Frontend: @#1, Tuned and locked (FE status @#2)", _feID, HEX(status, 2));
						break;
					}
					if (i == 1) {
						SI_LOG_INFO("Frontend: @#1, Not locked yet   (FE status @#2)...", _feID, HEX(status, 2));
					}
				} else {
					SI_LOG_PERROR("Frontend: @#1, FE_READ_STATUS", _feID);
				}
				const unsigned long waitTime = sw.getIntervalMS();
				if (waitTime > _waitOnLockTimeout) {
					SI_LOG_INFO("Frontend: @#1, Not locked yet   (Timeout @#2 ms)...", _feID, waitTime);
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(150));
			}
		} else {
			SI_LOG_INFO("Frontend: @#1, Not locked yet   (Timeout @#2 ms)...", _feID, sw.getIntervalMS());
		}
	}
	return _tuned;
}

}
