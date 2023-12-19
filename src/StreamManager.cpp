/* StreamManager.cpp

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
#include <StreamManager.h>

#include <Stream.h>
#include <Log.h>
#include <output/StreamClient.h>
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
	SI_LOG_INFO("Enumerating all devices...");

	// enumerate streams (frontends)
	input::dvb::Frontend::enumerate(_streamVector, appDataPath, _decrypt, dvbPath);
	input::file::TSReader::enumerate(_streamVector, appDataPath, enableUnsecureFrontends);
	input::stream::Streamer::enumerate(_streamVector, bindIPAddress, appDataPath);
	for (int i = 0; i < numberOfChildPIPE; ++i) {
		input::childpipe::TSReader::enumerate(_streamVector, appDataPath, enableUnsecureFrontends);
	}
}

std::string StreamManager::getXMLDeliveryString() const {
	std::size_t dvb_s2 = 0u;
	std::size_t dvb_t = 0u;
	std::size_t dvb_t2 = 0u;
	std::size_t dvb_c = 0u;
	std::size_t dvb_c2 = 0u;
	for (SpStream stream : _streamVector) {
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

std::tuple<FeIndex, FeID> StreamManager::findFrontendIDWithStreamID(const StreamID id) const {
	for (SpStream stream : _streamVector) {
		if (stream->getStreamID() == id) {
			return { stream->getFeIndex(), stream->getFeID() };
		}
	}
	return { FeIndex(), FeID() };
}

std::tuple<FeIndex, FeID, StreamID> StreamManager::findFrontendID(const TransportParamVector& params) const {
	const StreamID streamID = params.getIntParameter("stream");
	FeID feID = params.getIntParameter("fe");
	// Did we find StreamID an NO FrondendID
	if (streamID != -1 && feID == -1) {
		const auto [index, feIDFound] = findFrontendIDWithStreamID(streamID);
		return { index, feIDFound, streamID };
	}
	// Is the requested FrondendID in range, then return this
	if (feID >= 1 && feID <= static_cast<int>(_streamVector.size())) {
		// The vector starts at 0, therefore we subtract 1 from transport parameter 'fe='/'FeID'
		return { feID - 1, feID, streamID };
	}
	// Out of range, return -1, -1, -1
	return { FeIndex(), FeID(), StreamID() };
}

std::tuple<SpStream, output::SpStreamClient>  StreamManager::findStreamAndClientFor(SocketClient &socketClient) {
	// Here we need to find the correct Stream and StreamClient
	assert(!_streamVector.empty());
	HeaderVector headers = socketClient.getHeaders();
	TransportParamVector params = socketClient.getTransportParameters();

	// Now find index for FrontendID and/or StreamID of this message
	const auto [feIndex, feID, streamID] = findFrontendID(params);

	std::string sessionID = headers.getFieldParameter("Session");
	bool newSession = false;

	// if no sessionID, then make a new one or its just a outside message.
	if (sessionID.empty()) {
		if (socketClient.hasTransportParameters()) {
			// Do we need to make a new sessionID (only if there are transport parameters)
			std::random_device rd;
			std::mt19937 gen(rd());
			std::normal_distribution<> dist(0xfffffff, 0xffffff);
			sessionID = StringConverter::stringFormat("@#1", DIGIT(std::lround(dist(gen)), 10));
			newSession = true;
		} else {
			// None of the above.. so it is just an outside session
			SI_LOG_DEBUG("Found message outside session");
			return { nullptr, nullptr };
		}
	}

	// if no index, then we have to find a suitable one
	if (feIndex == -1) {
		SI_LOG_INFO("Found FrondtendID: x (fe=x)  StreamID: x  SessionID: @#1  New Session: @#2",
			sessionID, newSession ? "true" : "false");
		for (SpStream stream : _streamVector) {
			output::SpStreamClient streamClient = stream->findStreamClientFor(socketClient, newSession, sessionID);
			if (streamClient) {
				streamClient->setSessionID(sessionID);
				return { stream, streamClient };
			}
		}
	} else {
		SI_LOG_INFO("Found FrondtendID: @#1 (fe=@#2)  StreamID: @#3  SessionID: @#4", feID, feID, streamID, sessionID);
		// Did we find the StreamClient?
		output::SpStreamClient streamClient = _streamVector[feIndex]->findStreamClientFor(socketClient, newSession, sessionID);
		if (streamClient) {
			streamClient->setSessionID(sessionID);
			return { _streamVector[feIndex], streamClient };
		}
		// No, Then try to search in other Streams
		for (SpStream stream : _streamVector) {
			streamClient = stream->findStreamClientFor(socketClient, newSession, sessionID);
			if (streamClient) {
				streamClient->setSessionID(sessionID);
				return { stream, streamClient };
			}
		}
	}
	// Did not find anything
	SI_LOG_ERROR("Found no Stream/Client of interest!");
	return { nullptr, nullptr };
}

void StreamManager::checkForSessionTimeout() {
	assert(!_streamVector.empty());
	for (SpStream stream : _streamVector) {
		stream->checkForSessionTimeout();
	}
}

std::string StreamManager::getSDPSessionLevelString(
		const std::string& bindIPAddress,
		const std::string& sessionID) const {
	assert(!_streamVector.empty());
	std::size_t dvb_s2 = 0u;
	std::size_t dvb_t = 0u;
	std::size_t dvb_t2 = 0u;
	std::size_t dvb_c = 0u;
	std::size_t dvb_c2 = 0u;
	for (SpStream stream : _streamVector) {
		stream->addDeliverySystemCount(dvb_s2, dvb_t, dvb_t2, dvb_c, dvb_c2);
	}
	std::string sessionNameString = StringConverter::stringFormat("@#1,@#2,@#3",
			dvb_s2, dvb_t + dvb_t2, dvb_c + dvb_c2);

	static const char* SDP_SESSION_LEVEL =
		"v=0\r\n" \
		"o=- @#1 @#2 IN IP4 @#3\r\n" \
		"s=SatIPServer:1 @#4\r\n" \
		"t=0 0\r\n";

	// Describe streams
	return StringConverter::stringFormat(SDP_SESSION_LEVEL,
		(sessionID.size() > 2) ? sessionID : "0",
		(sessionID.size() > 2) ? sessionID : "0",
		bindIPAddress,
		sessionNameString);
}

std::string StreamManager::getSDPMediaLevelString(const FeIndex feIndex) const {
	assert(!_streamVector.empty());
	return _streamVector[feIndex.getID()]->getSDPMediaLevelString();
}

#ifdef LIBDVBCSA
input::dvb::SpFrontendDecryptInterface StreamManager::getFrontendDecryptInterface(const FeIndex feIndex) {
	return _streamVector[feIndex.getID()]->getFrontendDecryptInterface();
}
#endif

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void StreamManager::doFromXML(const std::string &xml) {
	for (SpStream stream : _streamVector) {
		std::string element;
		const std::string find = StringConverter::stringFormat("stream@#1", stream->getFeID());
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
	assert(!_streamVector.empty());

	for (ScpStream stream : _streamVector) {
		ADD_XML_N_ELEMENT(xml, "stream", stream->getFeID(), stream->toXML());
	}
#ifdef LIBDVBCSA
	ADD_XML_ELEMENT(xml, "decrypt", _decrypt->toXML());
#endif
}
