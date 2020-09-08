/* Frontend.cpp

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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

namespace input {
namespace dvb {

	const unsigned int Frontend::DEFAULT_DVR_BUFFER_SIZE = 18;
	const unsigned int Frontend::MAX_DVR_BUFFER_SIZE     = 18 * 10;

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================

	Frontend::Frontend(
			int streamID,
			const std::string &appDataPath,
			const std::string &fe,
			const std::string &dvr,
			const std::string &dmx) :
		Device(streamID),
		_tuned(false),
		_fd_fe(-1),
		_fd_dmx(-1),
		_path_to_fe(fe),
		_path_to_dvr(dvr),
		_path_to_dmx(dmx),
		_transform(appDataPath, _transformFrontendData),
		_dvbs2(0),
		_dvbt(0),
		_dvbt2(0),
		_dvbc(0),
		_dvbc2(0),
		_dvrBufferSizeMB(DEFAULT_DVR_BUFFER_SIZE) {
		snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Set");
		setupFrontend();
#if FULL_DVB_API_VERSION >= 0x050A
		_oldApiCallStats = false;
#else
		_oldApiCallStats = true;
#endif
	}

	Frontend::~Frontend() {}

	// =======================================================================
	//  -- Static functions --------------------------------------------------
	// =======================================================================
	// Called recursive
	static void getAttachedFrontends(
			StreamSpVector &streamVector,
			const std::string &appDataPath,
			decrypt::dvbapi::SpClient decrypt,
			const std::string &path,
			const std::string &startPath) {
		const std::string ADAPTER = startPath + "/adapter%1";
		const std::string DMX = ADAPTER + "/demux%2";
		const std::string DVR = ADAPTER + "/dvr%2";
		const std::string FRONTEND = ADAPTER + "/frontend%2";
#if SIMU
		// unused var
		(void)path;

		const std::string fe0 = StringConverter::stringFormat(FRONTEND.c_str(), 0, 0);
		const std::string dvr0 = StringConverter::stringFormat(DVR.c_str(), 0, 0);
		const std::string dmx0 = StringConverter::stringFormat(DMX.c_str(), 0, 0);
		input::dvb::SpFrontend frontend0 = std::make_shared<input::dvb::Frontend>(0, appDataPath, fe0, dvr0, dmx0);
		streamVector.push_back(std::make_shared<Stream>(0, frontend0, decrypt));

		const std::string fe1 = StringConverter::stringFormat(FRONTEND.c_str(), 1, 0);
		const std::string dvr1 = StringConverter::stringFormat(DVR.c_str(), 1, 0);
		const std::string dmx1 = StringConverter::stringFormat(DMX.c_str(), 1, 0);
		input::dvb::SpFrontend frontend1 = std::make_shared<input::dvb::Frontend>(1, appDataPath, fe1, dvr1, dmx1);
		streamVector.push_back(std::make_shared<Stream>(1, frontend1, decrypt));
#else
		dirent **file_list;
		const int n = scandir(path.c_str(), &file_list, nullptr, alphasort);
		if (n > 0) {
			for (int i = 0; i < n; ++i) {
				const std::string full_path = StringConverter::stringFormat("%1/%2", path, file_list[i]->d_name);
				struct stat stat_buf;
				if (stat(full_path.c_str(), &stat_buf) == 0) {
					switch (stat_buf.st_mode & S_IFMT) {
						case S_IFCHR: // character device
							if (strstr(file_list[i]->d_name, "frontend") != nullptr) {
								int fe_nr;
								sscanf(file_list[i]->d_name, "frontend%d", &fe_nr);
								int adapt_nr;
								const std::string ADAPTER_TMP = startPath + "/adapter%d";
								sscanf(path.c_str(), ADAPTER_TMP.c_str(), &adapt_nr);

								// Make new paths
								const std::string fe = StringConverter::stringFormat(FRONTEND.c_str(), adapt_nr, fe_nr);
								const std::string dvr = StringConverter::stringFormat(DVR.c_str(), adapt_nr, fe_nr);
								const std::string dmx = StringConverter::stringFormat(DMX.c_str(), adapt_nr, fe_nr);

								// Make new frontend here
								const StreamSpVector::size_type size = streamVector.size();
								const input::dvb::SpFrontend frontend = std::make_shared<input::dvb::Frontend>(size, appDataPath, fe, dvr, dmx);
								streamVector.push_back(std::make_shared<Stream>(size, frontend, decrypt));
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

	// =======================================================================
	//  -- Static member functions -------------------------------------------
	// =======================================================================

	void Frontend::enumerate(
			StreamSpVector &streamVector,
			const std::string &appDataPath,
			decrypt::dvbapi::SpClient decrypt,
			const std::string &dvbAdapterPath) {
		const StreamSpVector::size_type beginSize = streamVector.size();
		SI_LOG_INFO("Detecting frontends in: %s", dvbAdapterPath.c_str());
		getAttachedFrontends(streamVector, appDataPath, decrypt, dvbAdapterPath, dvbAdapterPath);
		const StreamSpVector::size_type endSize = streamVector.size();
		SI_LOG_INFO("Frontends found: %u", endSize - beginSize);
	}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void Frontend::doAddToXML(std::string &xml) const {
		ADD_XML_ELEMENT(xml, "frontendname", _fe_info.name);
		ADD_XML_ELEMENT(xml, "pathname", _path_to_fe);
		ADD_XML_ELEMENT(xml, "freq", StringConverter::stringFormat("%1 Hz to %2 Hz", _fe_info.frequency_min, _fe_info.frequency_max));
		ADD_XML_ELEMENT(xml, "symbol", StringConverter::stringFormat("%1 symbols/s to %2 symbols/s", _fe_info.symbol_rate_min, _fe_info.symbol_rate_max));

		ADD_XML_NUMBER_INPUT(xml, "dvrbuffer", _dvrBufferSizeMB, 0, MAX_DVR_BUFFER_SIZE);

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
		for (std::size_t i = 0; i < _deliverySystem.size(); ++i) {
			const std::string deliverySystem = StringConverter::stringFormat("deliverySystem%1", i);
			if (findXMLElement(xml, deliverySystem, element)) {
				_deliverySystem[i]->fromXML(element);
			}
		}
		if (findXMLElement(xml, "transformation", element)) {
			_transform.fromXML(element);
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
		dvbs2 += _transform.advertiseAsDVBS2() ? _dvbc : _dvbs2;
		dvbt  += _dvbt;
		dvbt2 += _dvbt2;
		dvbc  +=  _transform.advertiseAsDVBC() ? _dvbs2 : _dvbc;
		dvbc2 += _dvbc2;
	}

	bool Frontend::isDataAvailable() {
		pollfd pfd;
		pfd.fd = _fd_dmx;
		pfd.events = POLLIN;
		pfd.revents = 0;
		const int pollRet = ::poll(&pfd, 1, 180);
		if (pollRet > 0) {
			return (pfd.revents & POLLIN) == POLLIN;
		} else if (pollRet < 0) {
			PERROR("Error during polling frontend for data");
			return false;
		}
//		SI_LOG_DEBUG("Stream: %d, Timeout during polling frontend for data", _streamID);
		return false;
	}

	bool Frontend::readFullTSPacket(mpegts::PacketBuffer &buffer) {
		// try read maximum amount of bytes from DMX
		const int bytes = ::read(_fd_dmx, buffer.getWriteBufferPtr(), buffer.getAmountOfBytesToWrite());
		if (bytes > 0) {
			buffer.addAmountOfBytesWritten(bytes);
			if (buffer.full()) {
				// Add data to Filter
				_frontendData.addFilterData(_streamID, buffer);
				return true;
			}
		} else if (bytes < 0) {
			PERROR("Frontend::readFullTSPacket");
		}
		return false;
	}

	bool Frontend::capableOf(const input::InputSystem system) const {
		for (const input::dvb::delivery::UpSystem &deliverySystem : _deliverySystem) {
			if (deliverySystem->isCapableOf(system)) {
				return true;
			}
		}
		return false;
	}

	bool Frontend::capableToTransform(const std::string &msg,
			const std::string &method) const {
		const double freq = StringConverter::getDoubleParameter(msg, method, "freq=");
		const input::InputSystem system = _transform.getTransformationSystemFor(freq);
		return capableOf(system);
	}

	void Frontend::monitorSignal(const bool showStatus) {
#if SIMU
		(void)showStatus;
		_frontendData.setMonitorData(FE_HAS_LOCK, 214, 15, 0, 0);
#else
		fe_status_t status;

		// first read status
		if (::ioctl(_fd_fe, FE_READ_STATUS, &status) == 0) {
			uint16_t strength = 0;
			uint16_t snr = 0;
			uint32_t ber = 0;
			uint32_t ublocks = 0;

#	if FULL_DVB_API_VERSION >= 0x050A
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
					PERROR("FE_GET_PROPERTY failed");
				}

				const auto strengthScale = cmdseq.props[0].u.st.stat[0].scale;
				switch (strengthScale) {
					case FE_SCALE_DECIBEL:
						strength = cmdseq.props[0].u.st.stat[0].svalue * 0.0001;
						break;
					case FE_SCALE_RELATIVE:
						strength = cmdseq.props[0].u.st.stat[0].uvalue;
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
						snr = cmdseq.props[1].u.st.stat[0].uvalue;
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
						_oldApiCallStats = true;
						break;
				}
			}
#	endif
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
				SI_LOG_INFO("status %02x | signal %3u%% | snr %3u%% | ber %d | unc %d | Locked %d",
					status, strength, snr, ber, ublocks,
					(status & FE_HAS_LOCK) ? 1 : 0);
			}
			_frontendData.setMonitorData(status, strength, snr, ber, ublocks);
		} else {
			PERROR("FE_READ_STATUS failed");
		}
#endif
	}

	bool Frontend::hasDeviceDataChanged() const {
		return _frontendData.hasDeviceDataChanged();
	}

	void Frontend::parseStreamString(const std::string &msg1, const std::string &method) {
		SI_LOG_INFO("Stream: %d, Parsing transport parameters...", _streamID);

		// Do we need to transform this request?
		const std::string msg = _transform.transformStreamString(_streamID, msg1, method);

		_frontendData.parseStreamString(_streamID, msg, method);

		SI_LOG_DEBUG("Stream: %d, Parsing transport parameters (Finished)", _streamID);
	}

	bool Frontend::update() {
		SI_LOG_INFO("Stream: %d, Updating frontend...", _streamID);
#ifndef SIMU
		// Setup, tune and set PID Filters
		if (_frontendData.hasDeviceDataChanged()) {
			_frontendData.resetDeviceDataChanged();
			_tuned = false;
			// Close active PIDs
			for (std::size_t i = 0; i < mpegts::PidTable::MAX_PIDS; ++i) {
				_frontendData.getFilterData().setPID(i, false);
				closePid(i);
			}
			closeFE();
			closeDMX();
			// After close wait a moment before opening it again
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
		}

		std::size_t timeout = 0;
		while (!setupAndTune()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
			++timeout;
			if (timeout > 1) {
				SI_LOG_INFO("Stream: %d, Updating frontend (Failed)", _streamID);
				return false;
			}
		}
		updatePIDFilters();
#endif
		SI_LOG_DEBUG("Stream: %d, Updating frontend (Finished)", _streamID);
		return true;
	}

	bool Frontend::teardown() {
		// Close active PIDs
		for (std::size_t i = 0; i < mpegts::PidTable::MAX_PIDS; ++i) {
			_frontendData.getFilterData().setPID(i, false);
			closePid(i);
		}
		_tuned = false;
		closeFE();
		closeDMX();
		_frontendData.initialize();
		_transform.resetTransformFlag();
		return true;
	}

	std::string Frontend::attributeDescribeString() const {
		const DeviceData &data = _transform.transformDeviceData(_frontendData);
		return data.attributeDescribeString(_streamID);
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
		int fd_fe = openFE(_path_to_fe, true);
		if (fd_fe < 0) {
			snprintf(_fe_info.name, sizeof(_fe_info.name), "Not Found");
			PERROR("openFE");
			return;
		}

		if (::ioctl(fd_fe, FE_GET_INFO, &_fe_info) != 0) {
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
		if (::ioctl(fd_fe, FE_GET_PROPERTY, &dtvProperties ) != 0) {
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
		for (std::size_t i = 0; i < dtvProperty.u.buffer.len; ++i) {
			switch (dtvProperty.u.buffer.data[i]) {
				case SYS_DSS:
					SI_LOG_INFO("Frontend Type: DSS");
					break;
				case SYS_DVBS:
					++_dvbs2;
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
					SI_LOG_INFO("Frontend Type: Unknown %d", dtvProperty.u.buffer.data[i]);
					break;
			}
		}
		SI_LOG_INFO("Frontend Freq: %d Hz to %d Hz", _fe_info.frequency_min, _fe_info.frequency_max);
		SI_LOG_INFO("Frontend srat: %d symbols/s to %d symbols/s", _fe_info.symbol_rate_min, _fe_info.symbol_rate_max);

		// Do we run on an Set-Top Box with Enigma2
		const std::string infoVersionPath = StringConverter::stringFormat(
			"/proc/stb/info/version");
		std::ifstream infoVersionFile(infoVersionPath);
		if (infoVersionFile.is_open()) {
			int offset = 0;
			const std::string offsetFilePath = StringConverter::stringFormat(
				"/proc/stb/frontend/dvr_source_offset");
			std::ifstream offsetFile(offsetFilePath);
			if (offsetFile.is_open()) {
				offsetFile >> offset;
			}
			const int fdDMX = openDMX(_path_to_dmx);
			int n = _streamID;
			if (::ioctl(fdDMX, DMX_SET_SOURCE, &n) != 0) {
				PERROR("DMX_SET_SOURCE");
			}
			SI_LOG_INFO("Set DMX_SET_SOURCE for frontend %d (Offset: %d)", _streamID, offset);
			::close(fdDMX);
		}

		// Set delivery systems
		if (_dvbs2 > 0) {
			_deliverySystem.push_back(input::dvb::delivery::UpSystem(new input::dvb::delivery::DVBS(_streamID)));
		}
		if (_dvbt > 0 || _dvbt2 > 0) {
			_deliverySystem.push_back(input::dvb::delivery::UpSystem(new input::dvb::delivery::DVBT(_streamID)));
		}
		if (_dvbc > 0) {
			_deliverySystem.push_back(input::dvb::delivery::UpSystem(new input::dvb::delivery::DVBC(_streamID)));
		}
	}

	int Frontend::openFE(const std::string &path, const bool readonly) const {
		const int fd = ::open(path.c_str(), (readonly ? O_RDONLY : O_RDWR) | O_NONBLOCK);
		if (fd  < 0) {
			PERROR("FRONTEND DEVICE");
		}
		return fd;
	}

	void Frontend::closeFE() {
		if (_fd_fe != -1) {
			SI_LOG_INFO("Stream: %d, Closing %s fd: %d", _streamID, _path_to_fe.c_str(), _fd_fe);
			CLOSE_FD(_fd_fe);
		}
	}

	int Frontend::openDMX(const std::string &path) const {
		const int fd = ::open(path.c_str(), O_RDWR | O_NONBLOCK);
		if (fd < 0) {
			PERROR("DMX DEVICE");
		}
		return fd;
	}

	void Frontend::closeDMX() {
		if (_fd_dmx != -1) {
			SI_LOG_INFO("Stream: %d, Closing %s fd: %d", _streamID, _path_to_dmx.c_str(), _fd_dmx);
			if (::ioctl(_fd_dmx, DMX_STOP) != 0) {
				PERROR("DMX_STOP");
			}
			CLOSE_FD(_fd_dmx);
		}
	}

	bool Frontend::tune() {
		const input::InputSystem delsys = _frontendData.getDeliverySystem();
		for (input::dvb::delivery::UpSystem &system : _deliverySystem) {
			if (system->isCapableOf(delsys)) {
				return system->tune(_fd_fe, _frontendData);
			}
		}
		return false;
	}

	bool Frontend::setupAndTune() {
		if (!_tuned) {
			// Check if we have already opened a FE
			if (_fd_fe == -1) {
				_fd_fe = openFE(_path_to_fe, false);
				SI_LOG_INFO("Stream: %d, Opened %s fd: %d", _streamID, _path_to_fe.c_str(), _fd_fe);
			}
			// try tuning
			std::size_t timeout = 0;
			while (!tune()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(450));
				++timeout;
				if (timeout > 2) {
					return false;
				}
			}
			SI_LOG_INFO("Stream: %d, Waiting on lock...", _streamID);

			// check if frontend is locked, if not try a few times
			timeout = 0;
			while (timeout < 4) {
				fe_status_t status = FE_TIMEDOUT;
				// first read status
				if (::ioctl(_fd_fe, FE_READ_STATUS, &status) == 0) {
					if (status & FE_HAS_LOCK) {
						// We are tuned now
						_tuned = true;
						SI_LOG_INFO("Stream: %d, Tuned and locked (FE status 0x%X)", _streamID, status);
						break;
					} else {
						SI_LOG_INFO("Stream: %d, Not locked yet   (FE status 0x%X)...", _streamID, status);
					}
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(150));
				++timeout;
			}
		}
		return _tuned;
	}

	void Frontend::openPid(const int pid) {
		if (!_frontendData.getFilterData().shouldPIDOpen(pid)) {
			return;
		}
		// Check if we have already a DMX open
		if (_fd_dmx == -1) {
			// try opening DMX, try again if fails
			std::size_t timeout = 0;
			while ((_fd_dmx = openDMX(_path_to_dmx)) == -1) {
				std::this_thread::sleep_for(std::chrono::milliseconds(150));
				++timeout;
				if (timeout > 3) {
					return;
				}
			}
			{
				base::MutexLock lock(_mutex);
				if (_dvrBufferSizeMB > 0) {
					const unsigned int size = _dvrBufferSizeMB * 1024 * 1024;
					if (::ioctl(_fd_dmx, DMX_SET_BUFFER_SIZE, size) != 0) {
						PERROR("DMX - DMX_SET_BUFFER_SIZE failed");
					} else {
						SI_LOG_INFO("Stream: %d, Set DMX buffer size to %d Bytes", _streamID, size);
					}
				}
			}
			struct dmx_pes_filter_params pesFilter;
			pesFilter.pid      = pid;
			pesFilter.input    = DMX_IN_FRONTEND;
			pesFilter.output   = DMX_OUT_TSDEMUX_TAP;
			pesFilter.pes_type = DMX_PES_OTHER;
			pesFilter.flags    = DMX_IMMEDIATE_START;
			if (::ioctl(_fd_dmx, DMX_SET_PES_FILTER, &pesFilter) != 0) {
				PERROR("Stream: %d, DMX_SET_PES_FILTER (PID %04d)", _streamID, 0);
				return;
			}
			SI_LOG_INFO("Stream: %d, Opened %s fd: %d", _streamID, _path_to_dmx.c_str(), _fd_dmx);
		} else if (::ioctl(_fd_dmx, DMX_ADD_PID, &pid) != 0) {
			PERROR("Stream: %d, DMX_ADD_PID: PID %04d", _streamID, pid);
			return;
		}
		_frontendData.getFilterData().setPIDOpened(pid);
		SI_LOG_DEBUG("Stream: %d, Set filter PID: %04d%s", _streamID, pid,
				_frontendData.getFilterData().isMarkedAsPMT(pid) ? " - PMT" : "");
	}

	void Frontend::closePid(const int pid) {
		if (!_frontendData.getFilterData().shouldPIDClose(pid)) {
			return;
		}
		if (::ioctl(_fd_dmx, DMX_REMOVE_PID, &pid) != 0) {
			PERROR("Stream: %d, DMX_REMOVE_PID: PID %04d", _streamID, pid);
			return;
		}
		_frontendData.getFilterData().setPIDClosed(pid);
		SI_LOG_DEBUG("Stream: %d, Remove filter PID: %04d - Packet Count: %d%s",
				_streamID, pid, _frontendData.getFilterData().getPacketCounter(pid),
				_frontendData.getFilterData().isMarkedAsPMT(pid) ? " - PMT" : "");
	}

	void Frontend::updatePIDFilters() {
		base::MutexLock lock(_mutex);
		if (!_frontendData.getFilterData().hasPIDTableChanged()) {
			return;
		}
		if (!_tuned) {
			SI_LOG_INFO("Stream: %d, Update PID filters requested, but frontend not tuned!",
						_streamID);
			return;
		}
		_frontendData.getFilterData().resetPIDTableChanged();
		SI_LOG_INFO("Stream: %d, Updating PID filters...", _streamID);
		for (std::size_t i = 0; i < mpegts::PidTable::MAX_PIDS; ++i) {
			// Check should we close PIDs first then open again
			closePid(i);
			openPid(i);
		}
	}

} // namespace dvb
} // namespace input
