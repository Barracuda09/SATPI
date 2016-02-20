/* StreamManager.cpp

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
#include <StreamManager.h>

#include <Stream.h>
#include <Log.h>
#include <Utils.h>
#include <StreamClient.h>
#include <socket/SocketClient.h>
#include <StringConverter.h>
#include <input/dvb/Frontend.h>
#include <input/file/TSReader.h>
#ifdef LIBDVBCSA
	#include <decrypt/dvbapi/Client.h>
	#include <input/dvb/FrontendDecryptInterface.h>
#endif

#include <assert.h>

StreamManager::StreamManager(const std::string &xmlFilePath) :
	_xmlFilePath(xmlFilePath),
	_decrypt(nullptr),
	_dummyStream(nullptr) {
#ifdef LIBDVBCSA
	base::Functor1Ret<input::dvb::FrontendDecryptInterface *, int> getFrontendDecryptInterface =
		makeFunctor((base::Functor1Ret<input::dvb::FrontendDecryptInterface *, int>*) 0,
			*this, &StreamManager::getFrontendDecryptInterface);

	_decrypt = new decrypt::dvbapi::Client(xmlFilePath + "/" + "dvbapi.xml", getFrontendDecryptInterface);
#endif
}

StreamManager::~StreamManager() {
	DELETE(_dummyStream);
	for (StreamVector::iterator it = _stream.begin(); it != _stream.end(); ++it) {
		DELETE(*it);
	}
}

#ifdef LIBDVBCSA
	decrypt::dvbapi::Client *StreamManager::getDecrypt() const {
		return _decrypt;
	}

	input::dvb::FrontendDecryptInterface *StreamManager::getFrontendDecryptInterface(int streamID) {
		return _stream[streamID]->getFrontendDecryptInterface();
	}
#endif

void StreamManager::enumerateDevices() {
	base::MutexLock lock(_mutex);

#ifdef NOT_PREFERRED_DVB_API
	SI_LOG_DEBUG("Not the preferred DVB API version, for correct function it should be 5.5 or higher");
#endif
	SI_LOG_INFO("Current DVB_API_VERSION: %d.%d", DVB_API_VERSION, DVB_API_VERSION_MINOR);

	_dummyStream = new Stream(0, nullptr, _decrypt);
	input::dvb::Frontend::enumerate(_stream, _decrypt, "/dev/dvb");
	input::file::TSReader::enumerate(_stream, _xmlFilePath.c_str());


	_nr_dvb_s2 = 0;
	_nr_dvb_t = 0;
	_nr_dvb_t2 = 0;
	_nr_dvb_c = 0;
	_nr_dvb_c2 = 0;
	for (StreamVector::iterator it = _stream.begin(); it != _stream.end(); ++it) {
		(*it)->addDeliverySystemCount(_nr_dvb_s2, _nr_dvb_t, _nr_dvb_t2,
			_nr_dvb_c, _nr_dvb_c2);
	}
	StringConverter::addFormattedString(_del_sys_str, "DVBS2-%zu,DVBT-%zu,DVBT2-%zu,DVBC-%zu,DVBC2-%zu",
			   _nr_dvb_s2, _nr_dvb_t, _nr_dvb_t2, _nr_dvb_c, _nr_dvb_c2);

}

Stream *StreamManager::findStreamAndClientIDFor(SocketClient &socketClient, int &clientID) {
	base::MutexLock lock(_mutex);

	// Here we need to find the correct Stream and StreamClient
	assert(!_stream.empty());
	const std::string &msg = socketClient.getMessage();
	std::string method;
	StringConverter::getMethod(msg, method);

	int streamID = StringConverter::getIntParameter(msg, method, "stream=");
	const int fe_nr = StringConverter::getIntParameter(msg, method, "fe=");
	if (streamID == -1 && fe_nr >= 1 && fe_nr <= static_cast<int>(_stream.size())) {
		streamID = fe_nr - 1;
	}

	std::string sessionID;
	bool foundSessionID = StringConverter::getHeaderFieldParameter(msg, "Session:", sessionID);
	bool newSession = false;
	clientID = 0;

	// if no sessionID then..
	if (!foundSessionID) {
		// Does the SocketClient have an sessionID
		if (socketClient.getSessionID().size() > 2) {
			foundSessionID = true;
			sessionID = socketClient.getSessionID();
			SI_LOG_INFO("Found SessionID %s by SocketClient", sessionID.c_str());
			// Do we need to make a new sessionID (only if there are transport parameters
		} else if (StringConverter::hasTransportParameters(socketClient.getMessage())) {
			static unsigned int seedp = 0xBEEF;
			StringConverter::addFormattedString(sessionID, "%010d", rand_r(&seedp) % 0xffffffff);
			newSession = true;
			// None of the above.. so it is just an outside session give an temporary StreamClient
		} else {
			SI_LOG_INFO("Found message outside session");
			socketClient.setSessionID("-1");
			_dummyStream->copySocketClientAttr(socketClient);
			return _dummyStream;
		}
	}

	// if no streamID we need to find the streamID
	if (streamID == -1) {
		if (foundSessionID) {
			SI_LOG_INFO("Found StreamID x - SessionID: %s", sessionID.c_str());
			for (StreamVector::iterator it = _stream.begin(); it != _stream.end(); ++it) {
				Stream *stream = *it;
				if (stream->findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
					socketClient.setSessionID(sessionID);
					return stream;
				}
			}
		} else {
			SI_LOG_INFO("Found StreamID x - SessionID x - Creating new SessionID: %s", sessionID.c_str());
			for (StreamVector::iterator it = _stream.begin(); it != _stream.end(); ++it) {
				Stream *stream = *it;
				if (!stream->streamInUse()) {
					if (stream->findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
						socketClient.setSessionID(sessionID);
						return stream;
					}
				}
			}
		}
	} else {
// @TODO Check is this the owner of this stream??
		SI_LOG_INFO("Found StreamID %d - SessionID %s", streamID, sessionID.c_str());
		// Did we find the StreamClient? else try to search in other Streams
		if (!_stream[streamID]->findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
			SI_LOG_ERROR("SessionID %s not found int StreamID %d", sessionID.c_str(), streamID);
			for (StreamVector::iterator it = _stream.begin(); it != _stream.end(); ++it) {
				Stream *stream = *it;
				if (stream->findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
					socketClient.setSessionID(sessionID);
					return stream;
				}
			}
		} else {
			socketClient.setSessionID(sessionID);
			return _stream[streamID];
		}
	}
	// Did not find anything
	SI_LOG_ERROR("Found no Stream/Client of interest!");
	return nullptr;
}

void StreamManager::checkStreamClientsWithTimeout() {
	base::MutexLock lock(_mutex);

	assert(!_stream.empty());
	for (StreamVector::iterator it = _stream.begin(); it != _stream.end(); ++it) {
		Stream *stream = *it;
		if (stream->streamInUse()) {
			stream->checkStreamClientsWithTimeout();
		}
	}
}

std::string StreamManager::attributeDescribeString(std::size_t stream, bool &active) const {
	base::MutexLock lock(_mutex);

	assert(!_stream.empty());
	return _stream[stream]->attributeDescribeString(active);
}

void StreamManager::fromXML(const std::string &xml) {
	base::MutexLock lock(_mutex);
	std::size_t i = 0;
	for (StreamVector::iterator it = _stream.begin(); it != _stream.end(); ++it, ++i) {
		Stream *stream = *it;
		std::string find;
		std::string element;
		StringConverter::addFormattedString(find, "data.streams.stream%zu", i);
		if (findXMLElement(xml, find, element)) {
			stream->fromXML(element);
		}
	}
}

void StreamManager::addToXML(std::string &xml) const {
	base::MutexLock lock(_mutex);
	assert(!_stream.empty());
	std::size_t i = 0;
	// application data
	xml += "<streams>";
	for (StreamVector::const_iterator it = _stream.begin(); it != _stream.end(); ++it, ++i) {
		const Stream *stream = *it;
		StringConverter::addFormattedString(xml, "<stream%zu>", i);
		stream->addToXML(xml);
		StringConverter::addFormattedString(xml, "</stream%zu>", i);
	}
	xml += "</streams>";
}
