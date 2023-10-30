/* DVBCA.cpp

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
#include <decrypt/dvbca/DVBCA.h>
#include <mpegts/PMT.h>
#include <Utils.h>

#include <cstring>
#include <chrono>
#include <thread>
#include <iostream>

#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/dvb/ca.h>

#define POLL_TIMEOUT 150
#define RECV_TIMEOUT 100
#define RECV_SIZE    4096

static char fileFIFO[] = "/tmp/fifo";

	// ========================================================================
	// -- Constructors and destructor -----------------------------------------
	// ========================================================================

	DVBCA::DVBCA() :
		XMLSupport(),
		ThreadBase("DVB-CA handler"),
		_fd(-1),
		_fdFifo(-1),
		_id(0),
        _timeoutCnt(RECV_TIMEOUT),
		_repeatTime(0),
		_timeDateInterval(0),
        _connected(false),
        _waiting(false),
		apduTimeData(6, 0) {
		for (std::size_t i = 0; i < MAX_SESSIONS; ++i) {
			_sessions[i]._resource = 0;
			_sessions[i]._sessionNB = 0;
			_sessions[i]._sessionStatus = 0;
		}
	}

	DVBCA::~DVBCA() {
		SI_LOG_INFO("Stopping DVB-CA Handler");
		CLOSE_FD(_fdFifo);
		close();
		cancelThread();
		joinThread();
		::unlink(fileFIFO);
	}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void DVBCA::doAddToXML(std::string &/*xml*/) const {}

	void DVBCA::doFromXML(const std::string &/*xml*/) {}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

    bool DVBCA::open(const std::size_t id) {
        char path[100];
#ifdef ENIGMA
        sprintf(path, "/dev/ci%d", id);
#else
        sprintf(path, "/dev/dvb/adapter%d/ca0", id);
#endif
		SI_LOG_INFO("Try to detected CA device: @#1", path);
        _fd = ::open(path, O_RDWR | O_NONBLOCK);
        if (_fd < 0) {
			SI_LOG_ERROR("No CA device detected on adapter @#1", id);
            return false;
        }
        _id = id;
        _timeoutCnt = RECV_TIMEOUT;
		_repeatTime = 0;
		_timeDateInterval = 0;
        _connected = false;
        _waiting = false;
        return true;
    }

    void DVBCA::close() {
		SI_LOG_INFO("Stopping CA@#1: @#2", _id, _fd);
        CLOSE_FD(_fd);
    }

    bool DVBCA::reset() {
		SI_LOG_INFO("Resetting CA@#1, fd @#2", _id, _fd);
#ifdef ENIGMA
        unsigned char buf[256];
        while(::read(_fd, buf, 256)>0);
        if (::ioctl(_fd, 0)) {
			SI_LOG_ERROR("Could not reset CA@#1\n", _id);
            return false;
        }
#else
		// @todo: Do something else here
#endif
		return true;
    }

//[                           src/mpegts/PMT.cpp:062] Frontend: 0, PMT data
//47 47 D0 19 00 02 B0 57 4B 15 CF 00 00 E7 D1 F0  GG.....WK.......
//24 09 04 18 01 E0 33 09 04 18 50 E0 97 09 04 06  $.....3...P.....
//02 E0 C9 09 04 06 04 E1 2D 09 04 06 06 E1 91 09  ........-.......
//04 18 68 E1 C3 1B E7 D1 F0 00 03 E7 DB F0 06 0A  ..h.............
//04 64 75 74 00 06 E7 DC F0 11 6A 03 C0 42 06 05  .dut......j..B..
//04 41 43 2D 33 0A 04 64 75 74 00 B0 E0 91 45 FF  .AC-3..dut....E.
//FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
//FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
//FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
//FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
//FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF  ................
//FF FF FF FF FF FF FF FF FF FF FF FF              ............
	bool DVBCA::sendCAPMT(const std::size_t sessionNB, const mpegts::PMT &pmt) {
/*
        unsigned char data[] = {
			0x03,        // a_pmt_list_management
			0x4B, 0x15,  // program_number
			0xC5,        // 2b-res 5b-version 1b-current_next_indicator
			0xF0, 0x07,  // 4b-res 12b-prog_info_len (len 7)
			0x01,        // ca_pmt_cmd_id
			0x09,        // CA_descriptor()
			0x04,        // Len
			0x06, 0x04,  // CA_Id
			0xE1, 0x2D,  // 3b-res 13b-EMM_PID (7DC)
			0x1B,        // stream_type (1B)
			0xE7, 0xD1,  // 3b-res 13b-elementary_PID (7D1)
			0xF0, 0x00,  // 4b-res 12b-ES_info_length (00)
			0x03,        // stream_type (03)
			0xE7, 0xDB,  // 3b-res 13b-elementary_PID (7DB)
			0xF0, 0x00,  // 4b-res 12b-ES_info_length (00)
			0x06,        // stream_type (06)
			0xE7, 0xDC,  // 3b-res 13b-elementary_PID (7DC)
			0xF0, 0x00   // 4b-res 12b-ES_info_length (00)
		};
*/
		mpegts::PMT::Data tableData;
		pmt.getDataForSectionNumber(0, tableData);
		const int programNumber = pmt.getProgramNumber();
		const unsigned char* data = tableData.data.data();
		const std::size_t tableSize = (data[6] & 0x0F) | data[7];
		const mpegts::TSData progInfo = pmt.getProgramInfo();
		const std::size_t progSize = progInfo.size();

		// Get Only CA IDs we can handle, from progInfo
		mpegts::TSData caDescriptor;
		for (std::size_t i = 0u; i < progSize; ) {
			const std::size_t size = progInfo[i + 1] + 2u;
			if (progInfo[i] == 0x09 && size >= 0x06) {
				std::size_t caID  = (progInfo[i + 2u] << 8) | progInfo[i + 3u];
				if (caID == 0x0604) {
					caDescriptor = progInfo.substr(i, size);
				}
			}
			i += size;
		}
		// Assemble caPMT
		CAData caPMT;
		caPMT += 0x03;                   // a_pmt_list_management (only)
//		caPMT += 0x05;                   // a_pmt_list_management (update)
		caPMT += (programNumber >> 8) & 0xFF;
		caPMT +=  programNumber & 0xFF;
		caPMT += data[10];               // 2b-res 5b-version 1b-current_next_indicator

		std::size_t caDescriptorSize = caDescriptor.size() + 1;
		caPMT += (caDescriptorSize >> 8) & 0xFF; // 4b-res 12b-prog_info_len (len 7)
		caPMT +=  caDescriptorSize & 0xFF;       //
//		caPMT += 0x01;                           // ca_pmt_cmd_id (ok_descrambling)
		caPMT += 0x02;                           // ca_pmt_cmd_id (query)
		caPMT += caDescriptor;

		// 4 = CRC   9 = PMT Header from section length
		const std::size_t len = tableSize - 4u - 9u - progSize;
		const unsigned char* ptr = &data[17u + progSize];
		for (std::size_t i = 0u; i < len; ) {
			caPMT += ptr[i + 0u];
			caPMT += ptr[i + 1u];
			caPMT += ptr[i + 2u];
			// Make Length zero
			caPMT += ptr[i + 3u] & 0xF0;
			caPMT += static_cast<unsigned char>(ptr[i + 4u] & 0x00);
			const std::size_t esInfoLength  = ((ptr[i + 3u] & 0x0F) << 8u) | ptr[i + 4u];
			// Goto next ES entry
			i += esInfoLength + 5u;
		}
		createAndSendAPDUTag(sessionNB, APDU_CA_PMT, caPMT);
		return true;
	}

	void DVBCA::createSessionFor(const std::size_t resource,
			std::size_t &sessionStatus, std::size_t &sessionNB) {
		for (std::size_t i = 0; i < MAX_SESSIONS; ++i) {
			SI_LOG_DEBUG("sessionNB: @#1 - @#2", i + 1, HEX(_sessions[i]._resource, 8));
			if (_sessions[i]._resource == 0) {
				sessionStatus = SESSION_RESOURCE_OPEN;
				sessionNB = i + 1;

				_sessions[i]._resource = resource;
				_sessions[i]._sessionNB = sessionNB;
				_sessions[i]._sessionStatus = sessionStatus;
				return;
			}
		}
		// Out of resources
		sessionStatus = SESSION_RESOURCE_RESOURCE_BUSY;
		sessionNB = 0x0000;
	}

	std::size_t DVBCA::findSessionNumberForRecource(const std::size_t resource) const {
		for (std::size_t i = 0; i < MAX_SESSIONS; ++i) {
			if (_sessions[i]._resource == resource) {
				SI_LOG_DEBUG("Found sessionNB: @#1 for Resource: @#2",
					_sessions[i]._sessionNB, HEX(_sessions[i]._resource, 8));
				return _sessions[i]._sessionNB;
			}
		}
		return 0;
	}

	void DVBCA::clearSessionFor(std::size_t sessionNB) {
		_sessions[sessionNB]._resource = 0;
		_sessions[sessionNB]._sessionNB = 0;
		_sessions[sessionNB]._sessionStatus = 0;
	}

	void DVBCA::closeSessionRequest(const CAData &spduTag) {
		if (spduTag[1u] == 0x02) {
			const std::size_t sessionNB = spduTag[2u] << 8 | spduTag[3u];
			SI_LOG_DEBUG("Close Session Request @#1", HEX(sessionNB, 4));
			clearSessionFor(sessionNB);
			CAData buf(4, 0);
			buf[0] = SPDU_CLOSE_SESSION_RESPONSE;
			buf[1] = 0x02;
			buf[2] = spduTag[2u];
			buf[3] = spduTag[3u];
			sendPDU(buf);
		}
	}

	void DVBCA::openSessionRequest(const CAData &spduTag) {
		const int resource = spduTag[2] << 24 |
		                     spduTag[3] << 16 |
		                     spduTag[4] << 8 |
		                     spduTag[5];

		std::size_t sessionStatus = SESSION_RESOURCE_OPEN;
		std::size_t sessionNB = 0x0001;
		switch (resource) {
			case RESOURCE_MANAGER:
				SI_LOG_DEBUG("Create Session Response @#1 (Resource Manager)", HEX(resource, 8));
				createSessionFor(resource, sessionStatus, sessionNB);
				sendOpenSessionResponse(resource, sessionStatus, sessionNB);
				if (sessionStatus == SESSION_RESOURCE_OPEN) {
					createAndSendAPDUTag(sessionNB, APDU_PROFILE_ENQUIRY);
					_waiting = true;
				}
				break;
			case APPLICATION_MANAGER:
				SI_LOG_DEBUG("Create Session Response @#1 (Application Manager)", HEX(resource, 8));
				createSessionFor(resource, sessionStatus, sessionNB);
				sendOpenSessionResponse(resource, sessionStatus, sessionNB);
				if (sessionStatus == SESSION_RESOURCE_OPEN) {
					createAndSendAPDUTag(sessionNB, APDU_APPLICATION_INFO_ENQUIRY);
					_waiting = true;
				}
				break;
			case CA_MANAGER:
				SI_LOG_DEBUG("Create Session Response @#1 (CA Manager)", HEX(resource, 8));
				createSessionFor(resource, sessionStatus, sessionNB);
				sendOpenSessionResponse(resource, sessionStatus, sessionNB);
				if (sessionStatus == SESSION_RESOURCE_OPEN) {
					createAndSendAPDUTag(sessionNB, APDU_CA_INFO_ENQUIRY);
					_waiting = true;
				}
				break;
			case AUTHENTICATION:
				SI_LOG_DEBUG("Create Session Response @#1 (Authentication Manager)", HEX(resource, 8));
				break;
			case HOST_CONTROLER:
				SI_LOG_DEBUG("Create Session Response @#1 (Host Controler Manager)", HEX(resource, 8));
				break;
			case DATE_TIME:
				SI_LOG_DEBUG("Create Session Response @#1 (Data-Time Manager)", HEX(resource, 8));
				createSessionFor(resource, sessionStatus, sessionNB);
				sendOpenSessionResponse(resource, sessionStatus, sessionNB);
				break;
			case MMI_MANAGER:
				SI_LOG_DEBUG("Create Session Response @#1 (MMI Manager)", HEX(resource, 8));
				createSessionFor(resource, sessionStatus, sessionNB);
				sendOpenSessionResponse(resource, sessionStatus, sessionNB);
				break;
			default:
				SI_LOG_ERROR("Unknown resource @#1", HEX(resource, 8));
				break;
		}
	}

	void DVBCA::sessionNumber(const CAData &spduTag) {
		if (spduTag[1u] == 0x02) {
			const std::size_t sessionNB = spduTag[2u] << 8 | spduTag[3u];
			// Do we have an APDU tag
			if (spduTag.size() >= 8u && spduTag[4u] == APDU_BEGIN_PRIMITIVE_TAG) {
				// Found APDU tag and go parse it
				CAData apduData(&spduTag[4u], spduTag[7u] + 4);
				parseAPDUTag(sessionNB, apduData);
			} else {
				SI_LOG_ERROR("Other request with tag: @#1", HEX(spduTag[4u], 2));
			}
		} else {
			SI_LOG_ERROR("Invalid len: @#1", HEX(spduTag[1], 2));
		}
	}

	void DVBCA::parseApplicationInfo(const std::size_t sessionNB, const CAData &apduData) {
		SI_LOG_INFO("Application info (@#1):", HEX(sessionNB, 4));
		const std::size_t size = apduData.size();
		if (size > 4) {
			const std::size_t len = apduData[3u];
			if (size == len + 4u) {
				SI_LOG_INFO("  Application Type: @#1", HEX(apduData[4], 2));
				SI_LOG_INFO("  Application Manufacturer: @#1", HEX((apduData[5] << 8 | apduData[6]), 4));
				SI_LOG_INFO("  Manufacturer Code: @#1", HEX(((apduData[7] << 8) | apduData[8]), 4));
				SI_LOG_INFO("  Menu String: @#1", apduData.substr(10, apduData[9]));
			}
		}
	}

	void DVBCA::parseMMIMenu(const std::size_t sessionNB, const CAData &apduData) {
		SI_LOG_INFO("MMI Menu Strings (@#1):", HEX(sessionNB, 4));
		std::size_t strCnt = 0;
		const std::size_t size = apduData.size();
		if (size > 4) {
			const std::size_t len = apduData[3u];
			if (size == len + 4u) {
				for (std::size_t i = 5u; i < size;) {
					const std::size_t apduTag = apduData[i] << 16 | apduData[i + 1u] << 8 | apduData[i + 2u];
					switch (apduTag) {
						case APDU_MMI_TEXT_LAST: {
							const std::size_t tagLen = apduData[i + 3u];
							const std::string mmi(reinterpret_cast<const char *>(&apduData[i + 4u]), tagLen);
							SI_LOG_INFO("  Menu Strings @#1: @#2", strCnt, mmi);
							// Goto next tag entry
							i += tagLen + 4u;
							++strCnt;
							break;
						}
						default:
							SI_LOG_ERROR("Unknown APDU Tag: @#1 for Session NB: @#2", HEX(apduTag, 8), HEX(sessionNB, 4));
							++i;
							break;
					}
				}
			}
		}
	}

	void DVBCA::parseCAInfo(const std::size_t sessionNB, const CAData &apduData) {
		SI_LOG_INFO("Ca info (@#1):", HEX(sessionNB, 4));
		const std::size_t size = apduData.size();
		if (size > 4u) {
			const std::size_t len = apduData[3u];
			if (size == len + 4u) {
				for (std::size_t i = 4u; i < size; i += 2u) {
					const std::size_t id = apduData[i] << 8u | apduData[i + 1];
					SI_LOG_INFO("  CA System ID: (@#1)", HEX(id, 4));
				}
			}
		}
	}

    std::size_t DVBCA::read(CAData &data) const {
        ssize_t recvLen;
		do {
			unsigned char buf[RECV_SIZE];
			recvLen = ::read(_fd, buf, RECV_SIZE);
			data.append(buf, recvLen);
		} while (recvLen < 0 && errno == EINTR);

		if (recvLen < 0) {
			SI_LOG_ERROR("device read failed @#1", recvLen);
		} else {
			if (recvLen > 0) {
				SI_LOG_BIN_DEBUG(data.data(), recvLen, "read CA@#1:", _id);
			}
		}
		return data.size();
    }

	bool DVBCA::sendPDU(const CAData &data) {
		ssize_t len;
		do {
			len = ::write(_fd, data.data(), data.size());
		} while (len < 0 && errno == EINTR);

		if (len < 0) {
			SI_LOG_ERROR("Unable to write");
			return false;
		} else if (data.size() > static_cast<std::size_t>(len)) {
			SI_LOG_ERROR("Partial write: @#1", len);
			return false;
		} else {
			SI_LOG_BIN_DEBUG(data.data(), data.size(), "write CA@#1:", _id);
			return true;
		}
	}

    bool DVBCA::sendAPDU(const std::size_t sessionNB, const CAData &data) {
		CAData buf(4, 0);
		buf[0] = SPDU_SESSION_NUMBER;
		buf[1] = 0x02;
		buf[2] = (sessionNB >> 8) & 0xFF;
		buf[3] =  sessionNB       & 0xFF;
		buf += data;
		return sendPDU(buf);
    }

	void DVBCA::sendOpenSessionResponse(const std::size_t resource,
			const std::size_t sessionStatus, const std::size_t sessionNB) {
		SI_LOG_DEBUG("Send Session Response Status: @#1 Resource: @#2 SessionNB: @#3",
			HEX(sessionStatus, 2), HEX(resource, 8), HEX(sessionNB, 4));
		CAData buf(9, 0);
		buf[0] = SPDU_OPEN_SESSION_RESPONSE;
		buf[1] = 0x07; // len
		buf[2] = sessionStatus;
		buf[3] = (resource >> 24) & 0xFF;
		buf[4] = (resource >> 16) & 0xFF;
		buf[5] = (resource >>  8) & 0xFF;
		buf[6] =  resource        & 0xFF;
		buf[7] = (sessionNB >> 8) & 0xFF;
		buf[8] =  sessionNB       & 0xFF;
		sendPDU(buf);
	}

	void DVBCA::createAndSendAPDUTag(const std::size_t sessionNB,
			const std::size_t apduTag) {
		const CAData dummy;
		createAndSendAPDUTag(sessionNB, apduTag, dummy);
	}

	void DVBCA::createAndSendAPDUTag(const std::size_t sessionNB,
			const std::size_t apduTag, const CAData &apduData) {
		CAData buf(4, 0);
		std::size_t len = apduData.size();
		buf[0] = (apduTag >> 16) & 0xFF;
		buf[1] = (apduTag >>  8) & 0xFF;
		buf[2] =  apduTag        & 0xFF;
		buf[3] =  len            & 0xFF;
		if (len > 0) {
			buf += apduData;
		}
		sendAPDU(sessionNB, buf);
	}

	DVBCA::CAData DVBCA::addResourceID(const std::size_t resource) {
		CAData buf(4, 0);
		buf[0] = (resource >> 24) & 0xFF;
		buf[1] = (resource >> 16) & 0xFF;
		buf[2] = (resource >>  8) & 0xFF;
		buf[3] =  resource        & 0xFF;
		return buf;
	}

	void DVBCA::parseAPDUTag(const std::size_t sessionNB, const CAData &apduData) {
		const size_t apduTag = apduData[0] << 16 |
							   apduData[1] << 8 |
							   apduData[2];
		switch (apduTag) {
			case APDU_PROFILE:
				createAndSendAPDUTag(sessionNB, APDU_PROFILE_CHANGE);
				break;
			case APDU_PROFILE_ENQUIRY: {
				CAData apduReplyData;
				apduReplyData = addResourceID(RESOURCE_MANAGER);
				apduReplyData += addResourceID(APPLICATION_MANAGER);
				apduReplyData += addResourceID(CA_MANAGER);
//				apduReplyData += addResourceID(AUTHENTICATION);
//				apduReplyData += addResourceID(HOST_CONTROLER);
				apduReplyData += addResourceID(DATE_TIME);
				apduReplyData += addResourceID(MMI_MANAGER);
				createAndSendAPDUTag(sessionNB, APDU_PROFILE, apduReplyData);
				break;
			}
			case APDU_APPLICATION_INFO:
				parseApplicationInfo(sessionNB, apduData);
				break;
			case APDU_CA_INFO:
				parseCAInfo(sessionNB, apduData);
				break;
			case APDU_CA_REPLY:
/*
90 02 00 03 9F 80 33 0A 54 4F 11 81 0A 29 00 0A  ......3.TO...)..
33 00                                            3.
--- LINE END ---
END
[                          src/dvbca/DVBCA.cpp:538] Unknown APDU Tag: 0x009F8033 for Session NB: 0x0003
*/
				break;
			case APDU_DATE_TIME_ENQUIRY: {
				_timeDateInterval = apduData[4];
				SI_LOG_INFO("Data-Time Enquiry (@#1):", HEX(sessionNB, 4));
				SI_LOG_INFO("  Response Interval: @#1", HEX(_timeDateInterval, 2));

				_connected = true;
				break;
			}
			case APDU_MMI_DISPLAY_CONTROL: {
				SI_LOG_INFO("MMI Display Control (@#1):", HEX(sessionNB, 4));
				SI_LOG_INFO("  Display Control Cmd: @#1", HEX(apduData[4], 2));
				SI_LOG_INFO("  MMI Mode: @#1", HEX(apduData[5], 2));
				CAData apduReplyData;
				apduReplyData = apduData.substr(4, apduData[3]);
				createAndSendAPDUTag(sessionNB, APDU_MMI_DISPLAY_REPLY, apduReplyData);
				break;
			}
			case APDU_MMI_MENU_LAST:
				parseMMIMenu(sessionNB, apduData);
				break;
			default:
				SI_LOG_ERROR("Unknown APDU Tag: @#1 for Session NB: @#2",
					HEX(apduTag, 8), HEX(sessionNB, 4));
				break;
		}
	}

	// =======================================================================
	//  -- base::ThreadBase --------------------------------------------------
	// =======================================================================
	void DVBCA::threadEntry() {
		SI_LOG_INFO("Setting up DVB-CA Handler");
		{
			if (mkfifo(fileFIFO, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH) < 0) {
				SI_LOG_PERROR("Error creating FIFO-PIPE");
			}

			_fdFifo = ::open(fileFIFO, O_RDONLY | O_NONBLOCK);
			if (_fdFifo < 0) {
				SI_LOG_ERROR("Error openinge FIFO");
			}
		}

//		path << "/proc/stb/tsmux/input" << tuner_no << "_choices";
//		if(::access(path.str().data(), R_OK) < 0)

//		static const char *proc_ci_choices = "/proc/stb/tsmux/ci0_input_choices";
//		if (CFile::contains_word(proc_ci_choices, "PVR"))	// lowest prio = PVR
//		if (CFile::contains_word(proc_ci_choices, "DVR"))	// low prio = DVR
//		if (CFile::contains_word(proc_ci_choices, "DVR0"))	// high prio = DVR0
//		if (CFile::contains_word(proc_ci_choices, "PVR_NONE"))	// low prio = PVR_NONE
//		if (CFile::contains_word(proc_ci_choices, "NONE"))		// high prio = NONE
//		if (m_stream_finish_mode == finish_none)				// fallback = "tuner"

//setInputSource(int tuner_no, const std::string &source)	'source' Tuner Letter
//		snprintf(buf, sizeof(buf), "/proc/stb/tsmux/input%d", tuner_no);
//		if (CFile::write(buf, source.data()) == -1)

//int eDVBCISlot::setSource(const std::string &source)	'source' Tuner Letter
//	snprintf(buf, sizeof(buf), "/proc/stb/tsmux/ci%d_input", slotid);
//	if(CFile::write(buf, source.data()) == -1)

//int eDVBCISlot::setClockRate(int rate)
//	snprintf(buf, sizeof(buf), "/proc/stb/tsmux/ci%d_tsclk", slotid);
//	if(CFile::write(buf, rate ? "high" : "normal") == -1)

		mpegts::PMT pmt;
		int id = 0;
		open(id);

		std::size_t pollSize = 2;
		struct pollfd pfd[pollSize];
		pfd[0].fd = _fd;
		pfd[0].events = POLLIN | POLLPRI | POLLERR;
		pfd[0].revents = 0;

		pfd[1].fd = _fdFifo;
		pfd[1].events = POLLIN | POLLPRI | POLLERR;
		pfd[1].revents = 0;

		for (;; ) {
			const int pollRet = ::poll(pfd, pollSize, POLL_TIMEOUT);
			if (pollRet > 0) {
				if (pfd[0].revents != 0) {
					CAData data;
					const size_t recvLen = read(data);
					if (recvLen > 0) {
						switch (data[0]) {
							case SPDU_OPEN_SESSION_REQUEST:
								openSessionRequest(data);
								break;
							case SPDU_SESSION_NUMBER:
								sessionNumber(data);
								break;
							case SPDU_CLOSE_SESSION_REQUEST:
								closeSessionRequest(data);
								break;
							default:
								SI_LOG_ERROR("Found: Other request with tag: @#1", HEX(data[0], 2));
								break;
						}
					} else if (recvLen == 0) {
						SI_LOG_INFO("DVB-CA@#1 active", id);
//						_connected = true;
					}
				}
				if (pfd[1].revents != 0) {
					unsigned char buf[1024];
					ssize_t recvLen  = ::read(_fdFifo, buf, sizeof(buf));
					if (recvLen > 0) {
//						SI_LOG_BIN_DEBUG(buf, recvLen, "FIFO Data (@#1):", _id);

						const unsigned int tableID = buf[5u];
						if (_connected) {
							if (!pmt.isCollected()) {
								pmt.collectData(0, PMT_TABLE_ID, buf, false);
								// Did we finish collecting PMT
								if (pmt.isCollected()) {
									pmt.parse(0);
									const std::size_t sessionNB = findSessionNumberForRecource(CA_MANAGER);
									sendCAPMT(sessionNB, pmt);
									// Clear for next PMT we receive
									pmt.clear();
								}
							}
							if (tableID == 0x70 || tableID == 0x73) {
								apduTimeData[0] = 0x05; // Length of UTC-Time
								apduTimeData[1] = buf[8u]; // UTC-Time
								apduTimeData[2] = buf[9u]; // UTC-Time
								apduTimeData[3] = buf[10u]; // UTC-Time
								apduTimeData[4] = buf[11u]; // UTC-Time
								apduTimeData[5] = buf[12u]; // UTC-Time
								const std::size_t sessionNB = findSessionNumberForRecource(DATE_TIME);
								if (sessionNB > 0) {
									createAndSendAPDUTag(sessionNB, APDU_DATE_TIME, apduTimeData);
								}
							}
						}

					}
				}
			} else if (_connected) {
/*
				if (_waiting) {
					if (_timeoutCnt > 0) {
						--_timeoutCnt;
					} else {
						SI_LOG_ERROR("Timeout");
						_timeoutCnt = RECV_TIMEOUT;
						_connected = false;
						_waiting = false;
						//
						reset();
						close();
						_fd = open(_id);
				   }
				}
*/
			}
			if (_timeDateInterval != 0) {
				const std::time_t currentTime = std::time(nullptr);
				if (currentTime > _repeatTime) {
					if (_repeatTime == 0) {
						// Send CA_INFO
						const std::size_t sessionNB = findSessionNumberForRecource(CA_MANAGER);
						if (sessionNB > 0) {
							createAndSendAPDUTag(sessionNB, APDU_CA_INFO_ENQUIRY);
						}
					}
					_repeatTime = currentTime + _timeDateInterval;
/*
					// Send Data-Time
					const std::size_t sessionNB = findSessionNumberForRecource(DATE_TIME);
					if (sessionNB > 0) {
						if ((apduTimeData[5] & 0x0F) > 0x09) {
							apduTimeData[5] += 6;
						}
						if (apduTimeData[5] >= 0x60) {
							apduTimeData[5] = 0;
							apduTimeData[4] += 1;
							if ((apduTimeData[4] & 0x0F) > 9) {
								apduTimeData[4] += 6;
							}
						}
						createAndSendAPDUTag(sessionNB, APDU_DATE_TIME, apduTimeData);
					}
*/
				}
			}
		}
	}

