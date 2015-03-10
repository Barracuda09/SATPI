/* Stream.cpp

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
#include "Stream.h"
#include "StringConverter.h"
#include "SocketClient.h"

#include <stdio.h>
#include <stdlib.h>

#include "Log.h"

const unsigned int Stream::MAX_CLIENTS = 8;

Stream::Stream() :
		_streamInUse(false),
		_client(new StreamClient[MAX_CLIENTS]),
		_rtpThread(_client, _properties),
		_rtcpThread(_client, _properties) {
	for (size_t i = 0; i < MAX_CLIENTS; ++i) {
		_client[i]._clientID = i;
	}
}

Stream::~Stream() {
	delete[] _client;
}

bool Stream::findClientIDFor(SocketClient &socketClient,
                             bool newSession,
                             std::string sessionID,
                             const std::string &method,
                             int &clientID) {
	fe_delivery_system_t msys = StringConverter::getMSYSParameter(socketClient.getMessage(), method);
	// Check if frontend is capable if handling 'msys' when we have a new session
	if (newSession && !_frontend.capableOf(msys)) {
		SI_LOG_INFO("Stream: %d, Not capable of handling msys=%s",
		            _properties.getStreamID(), StringConverter::delsys_to_string(msys));
		return false;
	}

	// if we have a session ID try to find it among our StreamClients
	for (size_t i = 0; i < MAX_CLIENTS; ++i) {
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
	_client[0].copySocketClientAttr(socketClient);
}

void Stream::checkStreamClientsWithTimeout() {
	for (size_t i = 0; i < MAX_CLIENTS; ++i) {
		if (_client[i].checkWatchDogTimeout()) {
			SI_LOG_INFO("Stream: %d, Watchdog kicked in for StreamClient[%d] with SessionID %s",
			             _properties.getStreamID(), i, _client[i].getSessionID().c_str());
			teardown(i, false);
		}
	}
}

void Stream::startStreaming() {
	if (!_properties.getStreamActive()) {
		_properties.setStreamActive(_rtpThread.startStreaming(_frontend.get_dvr_fd()));
		_rtcpThread.startStreaming(_frontend.get_fe_fd());
	}
}

void Stream::close(int clientID) {
	if (_client[clientID].canClose()) {
		SI_LOG_INFO("Stream: %d, Close StreamClient[%d] with SessionID %s",
		            _properties.getStreamID(), clientID, _client[clientID].getSessionID().c_str());

		processStopStream(clientID, false);
/*
		_client[clientID].close();
		if (clientID == 0) {
// @TODO Stop all other StreamClients here??
			_streamInUse = false;
		}
*/
	}
}

bool Stream::teardown(int clientID, bool gracefull) {
	SI_LOG_INFO("Stream: %d, Teardown StreamClient[%d] with SessionID %s",
	            _properties.getStreamID(), clientID, _client[clientID].getSessionID().c_str());

	processStopStream(clientID, gracefull);
	return true;
}

void Stream::processStopStream(int clientID, bool gracefull) {
	_rtpThread.stopStreaming(clientID);
	_rtcpThread.stopStreaming(clientID);
	_frontend.teardown(_properties.getChannelData(), _properties.getStreamID());
	
	// as last, else sessionID and IP is reset
	_client[clientID].teardown(gracefull);

	if (clientID == 0) {
// @TODO Stop all other StreamClients here??
		_properties.setStreamActive(false);
		_streamInUse = false;
	}
}

bool Stream::updateFrontend() {
//	_properties.printChannelInfo();
	return _frontend.updateFrontend(_properties.getChannelData(), _properties.getStreamID());
}

void Stream::addToXML(std::string &xml) const {
	StringConverter::addFormattedString(xml, "<streamindex>%d</streamindex>", _properties.getStreamID());
	StringConverter::addFormattedString(xml, "<attached>%s</attached>", _streamInUse ? "yes" : "no");
	StringConverter::addFormattedString(xml, "<owner>%s</owner>", _client[0].getIPAddress().c_str());
	StringConverter::addFormattedString(xml, "<ownerSessionID>%s</ownerSessionID>", _client[0].getSessionID().c_str());
	_frontend.addToXML(xml);
	_properties.addToXML(xml);
}

bool Stream::processStream(const std::string &msg, int clientID, const std::string &method) {
	int port;
	if ((port = StringConverter::getIntParameter(msg, "Transport:", "client_port=")) != -1) {
		_client[clientID].setRtpSocketPort(port);
		_client[clientID].setRtcpSocketPort(port+1);
	}
	
	std::string cseq;
	if (StringConverter::getHeaderFieldParameter(msg, "CSeq:", cseq)) {
		_client[clientID].setCSeq(atoi(cseq.c_str()));
	}

	if (method.compare("OPTIONS") == 0 || method.compare("SETUP") == 0 || method.compare("PLAY") == 0) {
		parseStreamString(msg, method);
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

void Stream::parseStreamString(const std::string &msg, const std::string &method) {
	double doubleVal = 0.0;
	int    intVal = 0;
	std::string strVal;
	fe_delivery_system_t msys;
	
	SI_LOG_DEBUG("Stream: %d, Parsing transport parameters...", _properties.getStreamID());

	// Do this before [pids] [addpids] !! else we will delete it again here !!
	if ((doubleVal = StringConverter::getDoubleParameter(msg, method, "freq=")) != -1) {
		setFrequency(doubleVal * 1000.0);
		// new frequency so delete all used PIDS
		for (size_t i = 0; i < MAX_PIDS; ++i) {
			setPID(i, false);
		}
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "sr=")) != -1) {
		setSymbolRate(intVal * 1000);
	}
	if ((msys = StringConverter::getMSYSParameter(msg, method)) != SYS_UNDEFINED) {
		setDeliverySystem(msys);
	}
	if (StringConverter::getStringParameter(msg, method, "pol=", strVal) == true) {
		if (strVal.compare("h") == 0) {
			setLNBPolarization(POL_H);
		} else if (strVal.compare("v") == 0) {
			setLNBPolarization(POL_V);
		}
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "src=")) != -1) {
		setLNBSource(intVal);
	}
	if (StringConverter::getStringParameter(msg, method, "plts=", strVal) == true) {
		// "on", "off"[, "auto"]
		if (strVal.compare("on") == 0) {
			setPilotTones(PILOT_ON);
		} else if (strVal.compare("off") == 0) {
			setPilotTones(PILOT_OFF);
		} else if (strVal.compare("auto") == 0) {
			setPilotTones(PILOT_AUTO);
		} else {
			SI_LOG_ERROR("Unknown Pilot Tone [%s]", strVal.c_str());
			setPilotTones(PILOT_AUTO);
		}
	}
	if (StringConverter::getStringParameter(msg, method, "ro=", strVal) == true) {
		// "0.35", "0.25", "0.20"[, "auto"]
		if (strVal.compare("0.35") == 0) {
			setRollOff(ROLLOFF_35);
		} else if (strVal.compare("0.25") == 0) {
			setRollOff(ROLLOFF_25);
		} else if (strVal.compare("0.20") == 0) {
			setRollOff(ROLLOFF_20);
		} else if (strVal.compare("auto") == 0) {
			setRollOff(ROLLOFF_AUTO);
		} else {
			SI_LOG_ERROR("Unknown Rolloff [%s]", strVal.c_str());
			setRollOff(ROLLOFF_AUTO);
		}
	}
	if (StringConverter::getStringParameter(msg, method, "fec=", strVal) == true) {
		const int fec = atoi(strVal.c_str());
		// "12", "23", "34", "56", "78", "89", "35", "45", "910"
		if (fec == 12) {
			setFEC(FEC_1_2);
		} else if (fec == 23) {
			setFEC(FEC_2_3);
		} else if (fec == 34) {
			setFEC(FEC_3_4);
		} else if (fec == 35) {
			setFEC(FEC_3_5);
		} else if (fec == 45) {
			setFEC(FEC_4_5);
		} else if (fec == 56) {
			setFEC(FEC_5_6);
		} else if (fec == 67) {
			setFEC(FEC_6_7);
		} else if (fec == 78) {
			setFEC(FEC_7_8);
		} else if (fec == 89) {
			setFEC(FEC_8_9);
		} else if (fec == 910) {
			setFEC(FEC_9_10);
		} else if (fec == 999) {
			setFEC(FEC_AUTO);
		} else {
			setFEC(FEC_NONE);
		}
	}
	if (StringConverter::getStringParameter(msg, method, "mtype=", strVal) == true) {
		if (strVal.compare("8psk") == 0) {
			setModulationType(PSK_8);
		} else if (strVal.compare("qpsk") == 0) {
			setModulationType(QPSK);
		} else if (strVal.compare("16qam") == 0) {
			setModulationType(QAM_16);
		} else if (strVal.compare("64qam") == 0) {
			setModulationType(QAM_64);
		} else if (strVal.compare("256qam") == 0) {
			setModulationType(QAM_256);
		}
	} else {
		// no 'mtype' set so guess one according to 'msys'
		switch (msys) {
			case SYS_DVBS:
				setModulationType(QPSK);
				break;
			case SYS_DVBS2:
				setModulationType(PSK_8);
				break;
			case SYS_DVBT:
			case SYS_DVBT2:
			case SYS_DVBC_ANNEX_A:
			case SYS_DVBC_ANNEX_B:
#if FULL_DVB_API_VERSION >= 0x0505
			case SYS_DVBC_ANNEX_C:
#endif
				setModulationType(QAM_AUTO);
				break;
			default:
				SI_LOG_ERROR("Not supported delivery system");
				break;
		}
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "specinv=")) != -1) {
		setSpectralInversion(intVal);
	}
	if (StringConverter::getStringParameter(msg, method, "bw=", strVal) == true) {
		//	"5", "6", "7", "8", "10", "1.712"
		if (strVal.compare("5") == 0) {
			setBandwidth(BANDWIDTH_5_MHZ);
		} else if (strVal.compare("6") == 0) {
			setBandwidth(BANDWIDTH_6_MHZ);
		} else if (strVal.compare("7") == 0) {
			setBandwidth(BANDWIDTH_7_MHZ);
		} else if (strVal.compare("8") == 0) {
			setBandwidth(BANDWIDTH_8_MHZ);
		} else if (strVal.compare("10") == 0) {
			setBandwidth(BANDWIDTH_10_MHZ);
		} else if (strVal.compare("1.712") == 0) {
			setBandwidth(BANDWIDTH_1_712_MHZ);
		}
	}
	if (StringConverter::getStringParameter(msg, method, "tmode=", strVal) == true) {
		// "2k", "4k", "8k", "1k", "16k", "32k"[, "auto"]
		if (strVal.compare("1k") == 0) {
			setTransmissionMode(TRANSMISSION_MODE_1K);
		} else if (strVal.compare("2k") == 0) {
			setTransmissionMode(TRANSMISSION_MODE_2K);
		} else if (strVal.compare("4k") == 0) {
			setTransmissionMode(TRANSMISSION_MODE_4K);
		} else if (strVal.compare("8k") == 0) {
			setTransmissionMode(TRANSMISSION_MODE_8K);
		} else if (strVal.compare("16k") == 0) {
			setTransmissionMode(TRANSMISSION_MODE_16K);
		} else if (strVal.compare("32k") == 0) {
			setTransmissionMode(TRANSMISSION_MODE_32K);
		} else if (strVal.compare("auto") == 0) {
			setTransmissionMode(TRANSMISSION_MODE_AUTO);
		}
	}
	if (StringConverter::getStringParameter(msg, method, "gi=", strVal) == true) {
		// "14", "18", "116", "132","1128", "19128", "19256"
		const int gi = atoi(strVal.c_str());
		if (gi == 14) {
			setGuardInverval(GUARD_INTERVAL_1_4);
		} else if (gi == 18) {
			setGuardInverval(GUARD_INTERVAL_1_8);
		} else if (gi == 116) {
			setGuardInverval(GUARD_INTERVAL_1_16);
		} else if (gi == 132) {
			setGuardInverval(GUARD_INTERVAL_1_32);
		} else if (gi == 1128) {
			setGuardInverval(GUARD_INTERVAL_1_128);
		} else if (gi == 19128) {
			setGuardInverval(GUARD_INTERVAL_19_128);
		} else if (gi == 19256) {
			setGuardInverval(GUARD_INTERVAL_19_256);
		} else {
			setGuardInverval(GUARD_INTERVAL_AUTO);
		}
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "plp=")) != -1) {
		setUniqueIDPlp(intVal);
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "t2id=")) != -1) {
		setUniqueIDT2(intVal);
	}
	if ((intVal = StringConverter::getIntParameter(msg, method, "sm=")) != -1) {
		setSISOMISO(intVal);
	}
	if (StringConverter::getStringParameter(msg, method, "pids=", strVal) == true ||
		StringConverter::getStringParameter(msg, method, "addpids=", strVal) == true) {
		processPID(strVal, true);
	}
	if (StringConverter::getStringParameter(msg, method, "delpids=", strVal) == true) {
		processPID(strVal, false);
	}
}

void Stream::processPID(const std::string &pids, bool add) {
	std::string::size_type begin = 0;
	for (;;) {
		std::string::size_type end = pids.find_first_of(",", begin);
		if (end != std::string::npos) {
			const int pid = atoi(pids.substr(begin, end - begin).c_str());
			setPID(pid, add);
			begin = end + 1;
		} else {
			// Get the last one
			if (begin < pids.size()) {
				const int pid = atoi(pids.substr(begin, end - begin).c_str());
				setPID(pid, add);
			}
			break;
		}
	}
}
