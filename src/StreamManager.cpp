/* StreamManager.cpp

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
#include <StreamManager.h>

#include <Stream.h>
#include <Log.h>
#include <StreamClient.h>
#include <socket/SocketClient.h>
#include <StringConverter.h>
#include <input/childpipe/TSReader.h>
#include <input/dvb/Frontend.h>
#include <input/file/TSReader.h>
#include <input/stream/Streamer.h>
#ifdef LIBDVBCSA
	#include <decrypt/dvbapi/Client.h>
	#include <input/dvb/FrontendDecryptInterface.h>
#endif

#include <random>
#include <cmath>

#include <assert.h>

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

StreamManager::StreamManager() :
	XMLSupport(),
	_decrypt(nullptr) {
#ifdef LIBDVBCSA
	_decrypt = std::make_shared<decrypt::dvbapi::Client>(*this);
#endif
}

StreamManager::~StreamManager() {}

// =============================================================================
// -- Other member functions ---------------------------------------------------
// =============================================================================

void StreamManager::enumerateDevices(
		const std::string &bindIPAddress,
		const std::string &appDataPath,
		const std::string &dvbPath,
		const int numberOfChildPIPE,
		const bool enableUnsecureFrontends) {
#ifdef NOT_PREFERRED_DVB_API
	SI_LOG_ERROR("Not the preferred DVB API version, for correct function it should be 5.5 or higher");
#endif
	SI_LOG_INFO("Current DVB_API_VERSION: %d.%d", DVB_API_VERSION, DVB_API_VERSION_MINOR);
	SI_LOG_INFO("Enumerating all devices...");

	// enumerate streams (frontends)
	input::dvb::Frontend::enumerate(_stream, appDataPath, _decrypt, dvbPath);
	input::file::TSReader::enumerate(_stream, appDataPath, enableUnsecureFrontends);
	input::stream::Streamer::enumerate(_stream, bindIPAddress, appDataPath);
	for (int i = 0; i < numberOfChildPIPE; ++i) {
		input::childpipe::TSReader::enumerate(_stream, appDataPath, enableUnsecureFrontends);
	}
}

std::string StreamManager::getXMLDeliveryString() const {
	std::size_t dvb_s2 = 0u;
	std::size_t dvb_t = 0u;
	std::size_t dvb_t2 = 0u;
	std::size_t dvb_c = 0u;
	std::size_t dvb_c2 = 0u;
	for (SpStream stream : _stream) {
		stream->addDeliverySystemCount(dvb_s2, dvb_t, dvb_t2, dvb_c, dvb_c2);
	}
	std::vector<std::string> strArry;
	if (dvb_s2 > 0) {
		strArry.push_back(StringConverter::stringFormat("DVBS2-@#1", dvb_s2));
	}
	if (dvb_t > 0) {
		strArry.push_back(StringConverter::stringFormat("DVBT-@#1", dvb_t));
	}
	if (dvb_t2 > 0) {
		strArry.push_back(StringConverter::stringFormat("DVBT2-@#1", dvb_t2));
	}
	if (dvb_c > 0) {
		strArry.push_back(StringConverter::stringFormat("DVBC-@#1", dvb_c));
	}
	if (dvb_c2 > 0) {
		strArry.push_back(StringConverter::stringFormat("DVBC2-@#1", dvb_c2));
	}

	std::string delSysStr;
	const std::size_t size = strArry.size();
	for (std::size_t i = 0; i < size; ++i) {
		delSysStr += strArry[i];
		if (i < (size - 1) ) {
			delSysStr += ',';
		}
	}
	return delSysStr;
}

std::string StreamManager::getRTSPDescribeString() const {
	std::size_t dvb_s2 = 0u;
	std::size_t dvb_t = 0u;
	std::size_t dvb_t2 = 0u;
	std::size_t dvb_c = 0u;
	std::size_t dvb_c2 = 0u;
	for (SpStream stream : _stream) {
		stream->addDeliverySystemCount(dvb_s2, dvb_t, dvb_t2, dvb_c, dvb_c2);
	}
	return StringConverter::stringFormat("@#1,@#2,@#3", dvb_s2, dvb_t + dvb_t2, dvb_c + dvb_c2);
}

FeID StreamManager::findFrontendIDWithStreamID(const StreamID id) const {
	for (SpStream stream : _stream) {
		if (stream->getStreamID() == id) {
			return stream->getFeID();
		}
	}
	return FeID();
}

FeID StreamManager::findFrontendID(const std::string &msg, const std::string &method) const {
	const StreamID streamID = StringConverter::getIntParameter(msg, method, "stream=");
	const FeID feID = StringConverter::getIntParameter(msg, method, "fe=");
	// Did we find StreamID an NO FrondendID
	if (streamID != -1 && feID == -1) {
		return findFrontendIDWithStreamID(streamID);
	}
	// Is the FrondendID in range, then return this
	if (feID >= 1 && feID <= static_cast<int>(_stream.size())) {
		return feID;
	}
	// Out of range, return -1
	return FeID();
}

SpStream StreamManager::findStreamAndClientIDFor(SocketClient &socketClient, int &clientID) {
	// Here we need to find the correct Stream and StreamClient
	assert(!_stream.empty());
	const std::string msg = socketClient.getPercentDecodedMessage();
	const std::string method = StringConverter::getMethod(msg);

	// Now find FrontendID and StreamID of this message
	const StreamID streamID = StringConverter::getIntParameter(msg, method, "stream=");
	const FeID feID = findFrontendID(msg, method);

	std::string sessionID = StringConverter::getHeaderFieldParameter(msg, "Session:");
	bool newSession = false;
	clientID = 0;

	// if no sessionID, then try to find it.
	if (sessionID.empty()) {
		if (StringConverter::hasTransportParameters(socketClient.getMessage())) {
			// Do we need to make a new sessionID (only if there are transport parameters)
			std::random_device rd;
			std::mt19937 gen(rd());
			std::normal_distribution<> dist(0xfffffff, 0xffffff);
			StringConverter::addFormattedString(sessionID, "%010d", std::lround(dist(gen)) % 0xffffffff);
			newSession = true;
		} else {
			// None of the above.. so it is just an outside session
			SI_LOG_DEBUG("Found message outside session");
			return nullptr;
		}
	}

	// if no streamID, then we need to find the streamID
	if (streamID == -1) {
		if (!sessionID.empty()) {
			SI_LOG_INFO("Found FrondtendID x - StreamID x - SessionID: %s", sessionID.c_str());
			for (SpStream stream : _stream) {
				if (stream->findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
					stream->getStreamClient(clientID).setSessionID(sessionID);
					return stream;
				}
			}
		} else {
			SI_LOG_INFO("Found FrondtendID x - StreamID x - SessionID x - Creating new SessionID: %s", sessionID.c_str());
			for (SpStream stream : _stream) {
				if (!stream->streamInUse()) {
					if (stream->findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
						stream->getStreamClient(clientID).setSessionID(sessionID);
						return stream;
					}
				}
			}
		}
	} else if (feID != -1) {
		SI_LOG_INFO("Found FrondtendID %d - StreamID %d - SessionID %s", feID.getID(), streamID.getID(), sessionID.c_str());
		// Did we find the StreamClient? else try to search in other Streams
		if (!_stream[feID.getID()]->findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
			for (SpStream stream : _stream) {
				if (stream->findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
					stream->getStreamClient(clientID).setSessionID(sessionID);
					return stream;
				}
			}
		} else {
			_stream[feID.getID()]->getStreamClient(clientID).setSessionID(sessionID);
			return _stream[feID.getID()];
		}
	}
	// Did not find anything
	SI_LOG_ERROR("Found no Stream/Client of interest!");
	return nullptr;
}

void StreamManager::checkForSessionTimeout() {
	assert(!_stream.empty());
	for (SpStream stream : _stream) {
		if (stream->streamInUse()) {
			stream->checkForSessionTimeout();
		}
	}
}

std::string StreamManager::getDescribeMediaLevelString(const FeID id) const {
	assert(!_stream.empty());
	return _stream[id.getID()]->getDescribeMediaLevelString();
}

#ifdef LIBDVBCSA
input::dvb::SpFrontendDecryptInterface StreamManager::getFrontendDecryptInterface(const FeID id) {
	return _stream[id.getID()]->getFrontendDecryptInterface();
}
#endif

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void StreamManager::doFromXML(const std::string &xml) {
	for (SpStream stream : _stream) {
		std::string element;
		const std::string find = StringConverter::stringFormat("stream@#1", stream->getFeID().getID());
		if (findXMLElement(xml, find, element)) {
			stream->fromXML(element);
		}
	}
#ifdef LIBDVBCSA
	std::string element;
	if (findXMLElement(xml, "decrypt", element)) {
		_decrypt->fromXML(element);
	}
#endif
}

void StreamManager::doAddToXML(std::string &xml) const {
	assert(!_stream.empty());

	for (ScpStream stream : _stream) {
		ADD_XML_N_ELEMENT(xml, "stream", stream->getFeID().getID(), stream->toXML());
	}
#ifdef LIBDVBCSA
	ADD_XML_ELEMENT(xml, "decrypt", _decrypt->toXML());
#endif
}
