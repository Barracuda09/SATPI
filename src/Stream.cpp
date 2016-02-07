/* Stream.cpp

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
#include <Stream.h>

#include <Log.h>
#include <RtpStreamThread.h>
#include <HttpStreamThread.h>
#include <SocketClient.h>
#include <StringConverter.h>
#include <Utils.h>
#include <input/dvb/Frontend.h>
#include <input/dvb/delivery/DVB_S.h>

#include <stdio.h>
#include <stdlib.h>

const unsigned int Stream::MAX_CLIENTS = 8;

Stream::Stream(int streamID, decrypt::dvbapi::Client *decrypt) :
	_streamingType(NONE),
	_enabled(true),
	_streamInUse(false),
	_client(new StreamClient[MAX_CLIENTS]),
	_streaming(nullptr),
	_decrypt(decrypt),
	_properties(streamID),
	_frontend(new input::dvb::Frontend) {
	for (std::size_t i = 0; i < MAX_CLIENTS; ++i) {
		_client[i].setClientID(i);
	}
}

Stream::~Stream() {
	DELETE_ARRAY(_client);
	DELETE(_frontend);
}

// ===========================================================================
// -- StreamInterface --------------------------------------------------------
// ===========================================================================

int Stream::getStreamID() const {
	return _properties.getStreamID();
}

StreamClient &Stream::getStreamClient(std::size_t clientNr) const {
	return _client[clientNr];
}

input::Device *Stream::getInputDevice() const {
	return _frontend;
}

uint32_t Stream::getSSRC() const {
	return _properties.getSSRC();
}

long Stream::getTimestamp() const {
	return _properties.getTimestamp();
}

uint32_t Stream::getSPC() const {
	return _properties.getSPC();
}

unsigned int Stream::getRtcpSignalUpdateFrequency() const {
	return _properties.getRtcpSignalUpdateFrequency();
}

uint32_t Stream::getSOC() const {
	return _properties.getSOC();
}

void Stream::addRtpData(uint32_t byte, long timestamp) {
	_properties.addRtpData(byte, timestamp);
}

double Stream::getRtpPayload() const {
	return _properties.getRtpPayload();
}

std::string Stream::attributeDescribeString(bool &active) const {
	return _properties.attributeDescribeString(active);
}

void Stream::setFrontendMonitorData(fe_status_t status, uint16_t strength, uint16_t snr,
		uint32_t ber, uint32_t ublocks) {
	_properties.setFrontendMonitorData(status, strength, snr, ber, ublocks);
}

void Stream::addPIDData(int pid, uint8_t cc) {
	_properties.addPIDData(pid, cc);
}

// =======================================================================
//  -- base::XMLSupport --------------------------------------------------
// =======================================================================

void Stream::addToXML(std::string &xml) const {
	base::MutexLock lock(_mutex);

	ADD_CONFIG_NUMBER(xml, "streamindex", _properties.getStreamID());

	ADD_CONFIG_CHECKBOX(xml, "enable", (_enabled ? "true" : "false"));
	ADD_CONFIG_TEXT(xml, "attached", _streamInUse ? "yes" : "no");
	ADD_CONFIG_TEXT(xml, "owner", _client[0].getIPAddress().c_str());
	ADD_CONFIG_TEXT(xml, "ownerSessionID", _client[0].getSessionID().c_str());

	_frontend->addToXML(xml);
	_properties.addToXML(xml);
}

void Stream::fromXML(const std::string &xml) {
	base::MutexLock lock(_mutex);

	std::string element;
	if (findXMLElement(xml, "enable.value", element)) {
		_enabled = (element == "true") ? true : false;;
	}
	_properties.fromXML(xml);
}

// ===========================================================================
// -- Other member functions -------------------------------------------------
// ===========================================================================

bool Stream::findClientIDFor(SocketClient &socketClient,
                             bool newSession,
                             std::string sessionID,
                             const std::string &method,
                             int &clientID) {
	base::MutexLock lock(_mutex);

	// Check if frontend is enabled when we have a new session
	if (newSession && !_enabled) {
		SI_LOG_INFO("Stream: %d, Not enabled...", _properties.getStreamID());
		return false;
	}

	fe_delivery_system_t msys = StringConverter::getMSYSParameter(socketClient.getMessage(), method);
	// Check if frontend is capable if handling 'msys' when we have a new session
	if (newSession && !_frontend->capableOf(msys)) {
		SI_LOG_INFO("Stream: %d, Not capable of handling msys=%s",
		            _properties.getStreamID(), StringConverter::delsys_to_string(msys));
		return false;
	}

	// if we have a session ID try to find it among our StreamClients
	for (std::size_t i = 0; i < MAX_CLIENTS; ++i) {
		// If we have a new session we like to find an empty slot so '-1'
		if (_client[i].getSessionID().compare(newSession ? "-1" : sessionID) == 0) {
			if (msys != SYS_UNDEFINED) {
				SI_LOG_INFO("Stream: %d, StreamClient[%d] with SessionID %s for %s",
				            _properties.getStreamID(), i, sessionID.c_str(), StringConverter::delsys_to_string(msys));
			} else {
				SI_LOG_INFO("Stream: %d, StreamClient[%d] with SessionID %s",
				            _properties.getStreamID(), i, sessionID.c_str());
			}
			_client[i].copySocketClientAttr(socketClient);
			_client[i].setSessionID(sessionID);
			_streamInUse = true;
			clientID = i;
			return true;
		}
	}
	if (msys != SYS_UNDEFINED) {
		SI_LOG_INFO("Stream: %d, No StreamClient with SessionID %s for %s",
		            _properties.getStreamID(), sessionID.c_str(), StringConverter::delsys_to_string(msys));
	} else {
		SI_LOG_INFO("Stream: %d, No StreamClient with SessionID %s",
		            _properties.getStreamID(), sessionID.c_str());
	}
	return false;
}

void Stream::copySocketClientAttr(const SocketClient &socketClient) {
	base::MutexLock lock(_mutex);

	_client[0].copySocketClientAttr(socketClient);
}

void Stream::checkStreamClientsWithTimeout() {
	base::MutexLock lock(_mutex);

	for (std::size_t i = 0; i < MAX_CLIENTS; ++i) {
		if (_client[i].checkWatchDogTimeout()) {
			SI_LOG_INFO("Stream: %d, Watchdog kicked in for StreamClient[%d] with SessionID %s",
			            _properties.getStreamID(), i, _client[i].getSessionID().c_str());
			teardown(i, false);
		}
	}
}

bool Stream::update(int clientID) {
	base::MutexLock lock(_mutex);

//	_properties.printChannelInfo();
	const bool changed = _properties.hasChannelDataChanged();

	// first time streaming?
	if (_streaming == nullptr) {
		switch (_streamingType) {
		case NONE:
			_streaming = nullptr;
			SI_LOG_ERROR("Stream: %d, No streaming type found!!", _properties.getStreamID());
			break;
		case HTTP:
			_streaming = new HttpStreamThread(*this, _decrypt);
			break;
		case RTSP:
			_streaming = new RtpStreamThread(*this, _decrypt);
			break;
		};
		if (_streaming == nullptr) {
			return false;
		}
	}

	// Channel changed?.. stop/pause Stream
	if (changed) {
		_streaming->pauseStreaming(clientID);
	}

	if (!_frontend->update(_properties)) {
		return false;
	}

	// start or restart streaming again
	if (!_properties.getStreamActive()) {
		const bool active = _streaming->startStreaming();
		_properties.setStreamActive(active);
	} else if (changed) {
		_streaming->restartStreaming(clientID);
	}
	return true;
}

void Stream::close(int clientID) {
	base::MutexLock lock(_mutex);

	if (_client[clientID].canClose()) {
		SI_LOG_INFO("Stream: %d, Close StreamClient[%d] with SessionID %s",
		            _properties.getStreamID(), clientID, _client[clientID].getSessionID().c_str());

		processStopStream_L(clientID, false);
	}
}

bool Stream::teardown(int clientID, bool gracefull) {
	base::MutexLock lock(_mutex);

	SI_LOG_INFO("Stream: %d, Teardown StreamClient[%d] with SessionID %s",
	            _properties.getStreamID(), clientID, _client[clientID].getSessionID().c_str());

	processStopStream_L(clientID, gracefull);
	return true;
}

bool Stream::processStream(const std::string &msg, int clientID, const std::string &method) {
	base::MutexLock lock(_mutex);

	if ((method.compare("OPTIONS") == 0 || method.compare("SETUP") == 0 ||
	     method.compare("PLAY") == 0 || method.compare("GET") == 0) &&
	    StringConverter::hasTransportParameters(msg)) {

		_streamingType = (method.compare("GET") == 0) ? HTTP : RTSP;
		parseStreamString_L(msg, method);
	}
	int port = 0;
	if ((port = StringConverter::getIntParameter(msg, "Transport:", "client_port=")) != -1) {
		_client[clientID].setRtpSocketPort(port);
		_client[clientID].setRtcpSocketPort(port+1);
	}

	std::string cseq("0");
	if (StringConverter::getHeaderFieldParameter(msg, "CSeq:", cseq)) {
		_client[clientID].setCSeq(atoi(cseq.c_str()));
	}

	bool canClose = false;
	if (method.compare("SETUP") != 0) {
		std::string sessionID;
		const bool foundSessionID = StringConverter::getHeaderFieldParameter(msg, "Session:", sessionID);
		const bool teardown = method.compare("TEARDOWN") == 0;
		canClose = teardown || !foundSessionID;
	}
	_client[clientID].setCanClose(canClose);

/*
        // Close connection or keep-alive
        std::string connState;
        const bool foundConnection = StringConverter::getHeaderFieldParameter(msg, "Connection:", connState);
        const bool connectionClose = foundConnection && (strncasecmp(connState.c_str(), "Close", 5) == 0);
        _client[clientID].setCanClose(!connectionClose);
 */
	_client[clientID].restartWatchDog();
	return true;
}

/// private NO Locking
void Stream::processStopStream_L(int clientID, bool gracefull) {

	// Stop streaming by deleting object
	DELETE(_streaming);

	_frontend->teardown(_properties);

	// as last, else sessionID and IP is reset
	_client[clientID].teardown(gracefull);

	// @TODO Are all other StreamClients stopped??
	if (clientID == 0) {
		for (std::size_t i = 1; i < MAX_CLIENTS; ++i) {
			// Not gracefully teardown for other clients
			_client[i].teardown(false);
		}
		_properties.setStreamActive(false);
		_streamInUse = false;
	}
}

void Stream::parseStreamString_L(const std::string &msg, const std::string &method) {
	double doubleVal = 0.0;
	int intVal = 0;
	std::string strVal;
	fe_delivery_system_t msys;

	SI_LOG_DEBUG("Stream: %d, Parsing transport parameters...", _properties.getStreamID());

	// Do this AT FIRST because of possible initializing of channel data !! else we will delete it again here !!
	if ((doubleVal = StringConverter::getDoubleParameter(msg, method, "freq=")) != -1) {
		// new frequency so initialize ChannelData and 'remove' all used PIDS
		_properties.initializeChannelData();
		_properties.setFrequency(doubleVal * 1000.0);
		SI_LOG_DEBUG("Stream: %d, New frequency requested, clearing channel data...", _properties.getStreamID());
	}
	// !!!!
	if ((intVal = StringConverter::getIntParameter(msg, method, "sr=")) != -1) {
		_properties.setSymbolRate(intVal * 1000);
	}
	if ((msys = StringConverter::getMSYSParameter(msg, method)) != SYS_UNDEFINED) {
		_properties.setDeliverySystem(msys);
	}
	if (StringConverter::getStringParameter(msg, method, "pol=", strVal) == true) {
		if (strVal.compare("h") == 0) {
			_properties.setPolarization(POL_H);
		} else if (strVal.compare("v") == 0) {
			_properties.setPolarization(POL_V);
		}
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "src=")) != -1) {
		_properties.setDiSEqcSource(intVal);
	}
	if (StringConverter::getStringParameter(msg, method, "plts=", strVal) == true) {
		// "on", "off"[, "auto"]
		if (strVal.compare("on") == 0) {
			_properties.setPilotTones(PILOT_ON);
		} else if (strVal.compare("off") == 0) {
			_properties.setPilotTones(PILOT_OFF);
		} else if (strVal.compare("auto") == 0) {
			_properties.setPilotTones(PILOT_AUTO);
		} else {
			SI_LOG_ERROR("Unknown Pilot Tone [%s]", strVal.c_str());
			_properties.setPilotTones(PILOT_AUTO);
		}
	}
	if (StringConverter::getStringParameter(msg, method, "ro=", strVal) == true) {
		// "0.35", "0.25", "0.20"[, "auto"]
		if (strVal.compare("0.35") == 0) {
			_properties.setRollOff(ROLLOFF_35);
		} else if (strVal.compare("0.25") == 0) {
			_properties.setRollOff(ROLLOFF_25);
		} else if (strVal.compare("0.20") == 0) {
			_properties.setRollOff(ROLLOFF_20);
		} else if (strVal.compare("auto") == 0) {
			_properties.setRollOff(ROLLOFF_AUTO);
		} else {
			SI_LOG_ERROR("Unknown Rolloff [%s]", strVal.c_str());
			_properties.setRollOff(ROLLOFF_AUTO);
		}
	}
	if (StringConverter::getStringParameter(msg, method, "fec=", strVal) == true) {
		const int fec = atoi(strVal.c_str());
		// "12", "23", "34", "56", "78", "89", "35", "45", "910"
		if (fec == 12) {
			_properties.setFEC(FEC_1_2);
		} else if (fec == 23) {
			_properties.setFEC(FEC_2_3);
		} else if (fec == 34) {
			_properties.setFEC(FEC_3_4);
		} else if (fec == 35) {
			_properties.setFEC(FEC_3_5);
		} else if (fec == 45) {
			_properties.setFEC(FEC_4_5);
		} else if (fec == 56) {
			_properties.setFEC(FEC_5_6);
		} else if (fec == 67) {
			_properties.setFEC(FEC_6_7);
		} else if (fec == 78) {
			_properties.setFEC(FEC_7_8);
		} else if (fec == 89) {
			_properties.setFEC(FEC_8_9);
		} else if (fec == 910) {
			_properties.setFEC(FEC_9_10);
		} else if (fec == 999) {
			_properties.setFEC(FEC_AUTO);
		} else {
			_properties.setFEC(FEC_NONE);
		}
	}
	if (StringConverter::getStringParameter(msg, method, "mtype=", strVal) == true) {
		if (strVal.compare("8psk") == 0) {
			_properties.setModulationType(PSK_8);
		} else if (strVal.compare("qpsk") == 0) {
			_properties.setModulationType(QPSK);
		} else if (strVal.compare("16qam") == 0) {
			_properties.setModulationType(QAM_16);
		} else if (strVal.compare("64qam") == 0) {
			_properties.setModulationType(QAM_64);
		} else if (strVal.compare("256qam") == 0) {
			_properties.setModulationType(QAM_256);
		}
	} else if (msys != SYS_UNDEFINED) {
		// no 'mtype' set so guess one according to 'msys'
		switch (msys) {
		case SYS_DVBS:
			_properties.setModulationType(QPSK);
			break;
		case SYS_DVBS2:
			_properties.setModulationType(PSK_8);
			break;
		case SYS_DVBT:
		case SYS_DVBT2:
#if FULL_DVB_API_VERSION >= 0x0505
		case SYS_DVBC_ANNEX_A:
		case SYS_DVBC_ANNEX_B:
		case SYS_DVBC_ANNEX_C:
#else
		case SYS_DVBC_ANNEX_AC:
		case SYS_DVBC_ANNEX_B:
#endif
			_properties.setModulationType(QAM_AUTO);
			break;
		default:
			SI_LOG_ERROR("Not supported delivery system");
			break;
		}
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "specinv=")) != -1) {
		_properties.setSpectralInversion(intVal);
	}
	if ((doubleVal = StringConverter::getDoubleParameter(msg, method, "bw=")) != -1) {
		_properties.setBandwidthHz(doubleVal * 1000000.0);
	}
	if (StringConverter::getStringParameter(msg, method, "tmode=", strVal) == true) {
		// "2k", "4k", "8k", "1k", "16k", "32k"[, "auto"]
		if (strVal.compare("1k") == 0) {
			_properties.setTransmissionMode(TRANSMISSION_MODE_1K);
		} else if (strVal.compare("2k") == 0) {
			_properties.setTransmissionMode(TRANSMISSION_MODE_2K);
		} else if (strVal.compare("4k") == 0) {
			_properties.setTransmissionMode(TRANSMISSION_MODE_4K);
		} else if (strVal.compare("8k") == 0) {
			_properties.setTransmissionMode(TRANSMISSION_MODE_8K);
		} else if (strVal.compare("16k") == 0) {
			_properties.setTransmissionMode(TRANSMISSION_MODE_16K);
		} else if (strVal.compare("32k") == 0) {
			_properties.setTransmissionMode(TRANSMISSION_MODE_32K);
		} else if (strVal.compare("auto") == 0) {
			_properties.setTransmissionMode(TRANSMISSION_MODE_AUTO);
		}
	}
	if (StringConverter::getStringParameter(msg, method, "gi=", strVal) == true) {
		// "14", "18", "116", "132","1128", "19128", "19256"
		const int gi = atoi(strVal.c_str());
		if (gi == 14) {
			_properties.setGuardInverval(GUARD_INTERVAL_1_4);
		} else if (gi == 18) {
			_properties.setGuardInverval(GUARD_INTERVAL_1_8);
		} else if (gi == 116) {
			_properties.setGuardInverval(GUARD_INTERVAL_1_16);
		} else if (gi == 132) {
			_properties.setGuardInverval(GUARD_INTERVAL_1_32);
		} else if (gi == 1128) {
			_properties.setGuardInverval(GUARD_INTERVAL_1_128);
		} else if (gi == 19128) {
			_properties.setGuardInverval(GUARD_INTERVAL_19_128);
		} else if (gi == 19256) {
			_properties.setGuardInverval(GUARD_INTERVAL_19_256);
		} else {
			_properties.setGuardInverval(GUARD_INTERVAL_AUTO);
		}
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "plp=")) != -1) {
		_properties.setUniqueIDPlp(intVal);
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "t2id=")) != -1) {
		_properties.setUniqueIDT2(intVal);
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "sm=")) != -1) {
		_properties.setSISOMISO(intVal);
	}
	if (StringConverter::getStringParameter(msg, method, "pids=", strVal) == true ||
	    StringConverter::getStringParameter(msg, method, "addpids=", strVal) == true) {
		processPID_L(strVal, true);
	}
	if (StringConverter::getStringParameter(msg, method, "delpids=", strVal) == true) {
		processPID_L(strVal, false);
	}
	SI_LOG_DEBUG("Stream: %d, Parsing transport parameters (Finished)", _properties.getStreamID());
}

/// private NO Locking
void Stream::processPID_L(const std::string &pids, bool add) {
	std::string::size_type begin = 0;
	if (pids == "all" ) {
		// All pids requested then 'remove' all used PIDS
		for (std::size_t i = 0; i < MAX_PIDS; ++i) {
			_properties.setPID(i, false);
		}
		_properties.setAllPID(add);
	} else {
		for (;; ) {
			std::string::size_type end = pids.find_first_of(",", begin);
			if (end != std::string::npos) {
				const int pid = atoi(pids.substr(begin, end - begin).c_str());
				_properties.setPID(pid, add);
				begin = end + 1;
			} else {
				// Get the last one
				if (begin < pids.size()) {
					const int pid = atoi(pids.substr(begin, end - begin).c_str());
					_properties.setPID(pid, add);
				}
				break;
			}
		}
	}
}
