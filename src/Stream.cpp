/* Stream.cpp

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
#include <Stream.h>

#include <Log.h>
#include <StringConverter.h>
#include <Utils.h>
#include <input/dvb/Frontend.h>
#include <input/dvb/FrontendData.h>
#include <input/dvb/delivery/DVBS.h>
#include <output/StreamThreadHttp.h>
#include <output/StreamThreadRtp.h>
#include <output/StreamThreadRtpTcp.h>
#include <output/StreamThreadTSWriter.h>
#include <socket/SocketClient.h>

#include <stdio.h>
#include <stdlib.h>

static unsigned int seedp = 0xFEED;
const unsigned int Stream::MAX_CLIENTS = 8;

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

Stream::Stream(input::SpDevice device, decrypt::dvbapi::SpClient decrypt) :
	_streamingType(StreamingType::NONE),
	_enabled(true),
	_streamInUse(false),
	_streamActive(false),
	_client(new StreamClient[MAX_CLIENTS]),
	_streaming(nullptr),
	_decrypt(decrypt),
	_device(device),
	_ssrc((uint32_t)(rand_r(&seedp) % 0xffff)),
	_spc(0),
	_soc(0),
	_timestamp(0),
	_rtp_payload(0.0),
	_rtcpSignalUpdate(1) {
	ASSERT(device);
}

Stream::~Stream() {
	DELETE_ARRAY(_client);
}

// ===========================================================================
// -- StreamInterface --------------------------------------------------------
// ===========================================================================

StreamID Stream::getStreamID() const {
	return _device->getStreamID();
}

FeID Stream::getFeID() const {
	return _device->getFeID();
}

int Stream::getFeIndex() const {
	return _device->getFeIndex();
}

StreamClient &Stream::getStreamClient(int clientID) const {
	return _client[clientID];
}

input::SpDevice Stream::getInputDevice() const {
	return _device;
}

#ifdef LIBDVBCSA
decrypt::dvbapi::SpClient Stream::getDecryptDevice() const {
	return _decrypt;
}
#endif

uint32_t Stream::getSSRC() const {
	return _ssrc;
}

long Stream::getTimestamp() const {
	return _timestamp;
}

uint32_t Stream::getSPC() const {
	return _spc;
}

unsigned int Stream::getRtcpSignalUpdateFrequency() const {
	return _rtcpSignalUpdate;
}

uint32_t Stream::getSOC() const {
	return _soc;
}

void Stream::addRtpData(uint32_t byte, long timestamp) {
	// inc RTP packet counter
	++_spc;
	_soc += byte;
	_rtp_payload = _rtp_payload + byte;
	_timestamp = timestamp;
}

double Stream::getRtpPayload() const {
	return _rtp_payload;
}

std::string Stream::attributeDescribeString() const {
	return _device->attributeDescribeString();
}

// =======================================================================
//  -- base::XMLSupport --------------------------------------------------
// =======================================================================

void Stream::doAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "streamindex", _device->getFeID().getID());

	ADD_XML_CHECKBOX(xml, "enable", (_enabled ? "true" : "false"));
	ADD_XML_ELEMENT(xml, "attached", _streamInUse ? "yes" : "no");
	ADD_XML_NUMBER_INPUT(xml, "rtcpSignalUpdate", _rtcpSignalUpdate, 0, 5);
	ADD_XML_ELEMENT(xml, "spc", _spc.load());
	ADD_XML_ELEMENT(xml, "payload", _rtp_payload.load() / (1024.0 * 1024.0));

	_client[0].addToXML(xml);
	_device->addToXML(xml);
}

void Stream::doFromXML(const std::string &xml) {
	std::string element;
	if (findXMLElement(xml, "enable.value", element)) {
		_enabled = (element == "true") ? true : false;
	}
	if (findXMLElement(xml, "rtcpSignalUpdate.value", element)) {
		_rtcpSignalUpdate = std::stoi(element);
	}
	_device->fromXML(xml);
}

// ===========================================================================
// -- Other member functions -------------------------------------------------
// ===========================================================================
#ifdef LIBDVBCSA
input::dvb::SpFrontendDecryptInterface Stream::getFrontendDecryptInterface() {
	return std::dynamic_pointer_cast<input::dvb::FrontendDecryptInterface>(_device);
}
#endif

bool Stream::findClientIDFor(SocketClient &socketClient,
		const bool newSession, const std::string sessionID, int &clientID) {
	base::MutexLock lock(_mutex);
	const FeID id = _device->getFeID();
	const TransportParamVector params = socketClient.getTransportParameters();
	const input::InputSystem msys = params.getMSYSParameter();

	// Check if the input device is set, else this stream is not usable
	if (!_device) {
		SI_LOG_ERROR("Frontend: @#1, Input Device not set?...", id);
		return false;
	}

	// Do we have a new session then check some things
	if (newSession) {
		if (!_enabled) {
			SI_LOG_INFO("Frontend: @#1, New session but this stream is not enabled, skipping...", id);
			return false;
		} else if (_streamInUse) {
			SI_LOG_INFO("Frontend: @#1, New session but this stream is in use, skipping...", id);
			return false;
		} else if (!_device->capableOf(msys)) {
			if (_device->capableToTransform(params)) {
				SI_LOG_INFO("Frontend: @#1, Capable of transforming msys=@#2",
					id, StringConverter::delsys_to_string(msys));
			} else {
				SI_LOG_INFO("Frontend: @#1, Not capable of handling msys=@#2",
					id, StringConverter::delsys_to_string(msys));
				return false;
			}
		}
	}

	// if we have a session ID try to find it among our StreamClients
	for (std::size_t i = 0; i < MAX_CLIENTS; ++i) {
		// If we have a new session we like to find an empty slot so '-1'
		if (_client[i].getSessionID().compare(newSession ? "-1" : sessionID) == 0) {
			if (msys != input::InputSystem::UNDEFINED) {
				SI_LOG_INFO("Frontend: @#1, StreamClient[@#2] with SessionID @#3 for @#4",
					id, i, sessionID, StringConverter::delsys_to_string(msys));
			} else {
				SI_LOG_INFO("Frontend: @#1, StreamClient[@#2] with SessionID @#3",
					id, i, sessionID);
			}
			_client[i].setSocketClient(socketClient);
			_streamInUse = true;
			clientID = i;
			return true;
		}
	}
	if (msys != input::InputSystem::UNDEFINED) {
		SI_LOG_INFO("Frontend: @#1, No StreamClient with SessionID @#2 for @#3",
			id, sessionID, StringConverter::delsys_to_string(msys));
	} else {
		SI_LOG_INFO("Frontend: @#1, No StreamClient with SessionID @#2", id, sessionID);
	}
	return false;
}

void Stream::checkForSessionTimeout() {
	base::MutexLock lock(_mutex);

	for (std::size_t i = 0; i < MAX_CLIENTS; ++i) {
		if (_client[i].sessionTimeout() || !_enabled) {
			if (_enabled) {
				SI_LOG_INFO("Frontend: @#1, Watchdog kicked in for StreamClient[@#2] with SessionID @#3",
					_device->getFeID(), i, _client[i].getSessionID());
			} else {
				SI_LOG_INFO("Frontend: @#1, Reclaiming StreamClient[@#2] with SessionID @#3",
					_device->getFeID(), i, _client[i].getSessionID());
			}
			teardown(i);
		}
	}
}

bool Stream::update(int clientID) {
	base::MutexLock lock(_mutex);
	const FeID id = _device->getFeID();
	// first time streaming?
	if (!_streaming) {
		switch (_streamingType) {
			case StreamingType::NONE:
				_streaming.reset(nullptr);
				SI_LOG_ERROR("Frontend: @#1, No streaming type found!!", id);
				break;
			case StreamingType::HTTP:
				SI_LOG_DEBUG("Frontend: @#1, Found Streaming type: HTTP", id);
				_streaming.reset(new output::StreamThreadHttp(*this));
				break;
			case StreamingType::RTSP_UNICAST:
				SI_LOG_DEBUG("Frontend: @#1, Found Streaming type: RTSP Unicast", id);
				_streaming.reset(new output::StreamThreadRtp(*this));
				break;
			case StreamingType::RTSP_MULTICAST:
				SI_LOG_DEBUG("Frontend: @#1, Found Streaming type: RTSP Multicast", id);
				_streaming.reset(new output::StreamThreadRtp(*this));
				break;
			case StreamingType::RTP_TCP:
				SI_LOG_DEBUG("Frontend: @#1, Found Streaming type: RTP/TCP", id);
				_streaming.reset(new output::StreamThreadRtpTcp(*this));
				break;
			case StreamingType::FILE:
				SI_LOG_DEBUG("Frontend: @#1, Found Streaming type: FILE", id);
				_streaming.reset(new output::StreamThreadTSWriter(*this, "test.ts"));
				break;
			default:
				_streaming.reset(nullptr);
				SI_LOG_ERROR("Frontend: @#1, Unknown streaming type!", id);
		};
		if (!_streaming) {
			return false;
		}
	}
	// Get changed flag, before device update, because it resets it
	const bool changed = _device->hasDeviceDataChanged();

	if (!_device->update()) {
		return false;
	}

	// start or restart streaming again
	if (_streaming) {
		if (!_streamActive) {
			_streamActive = _streaming->startStreaming(clientID);
		} else if (changed) {
			_streaming->restartStreaming(clientID);
		}
	}
	return true;
}

bool Stream::teardown(int clientID) {
	base::MutexLock lock(_mutex);

	SI_LOG_INFO("Frontend: @#1, Teardown StreamClient[@#2] with SessionID @#3",
		_device->getFeID(), clientID, _client[clientID].getSessionID());

	// Stop streaming by deleting object
	if (_streaming) {
		_streaming.reset(nullptr);
	}

	_device->teardown();

	// as last, else sessionID and IP is reset
	_client[clientID].teardown();

	// @TODO Are all other StreamClients stopped??
	if (clientID == 0) {
		for (std::size_t i = 1; i < MAX_CLIENTS; ++i) {
			_client[i].teardown();
		}
		_streamActive = false;
		_streamInUse = false;
		_streamingType = StreamingType::NONE;
	}
	return true;
}

bool Stream::processStreamingRequest(const SocketClient &client, const int clientID) {
	base::MutexLock lock(_mutex);

	// Split message into Headers
	HeaderVector headers = client.getHeaders();

	// Save clients seq number
	const std::string cseq = headers.getFieldParameter("CSeq");
	if (!cseq.empty()) {
		_client[clientID].setCSeq(std::stoi(cseq));
	}

	// Save clients User-Agent
	const std::string userAgent = headers.getFieldParameter("User-Agent");
	if (!userAgent.empty()) {
		_client[clientID].setUserAgent(userAgent);
	}

	const std::string method = client.getMethod();
	if ((method == "SETUP" || method == "PLAY"  || method == "GET") &&
			client.hasTransportParameters()) {
		TransportParamVector params = client.getTransportParameters();
		_device->parseStreamString(params);
	}

	// Channel changed?.. stop/pause Stream
	if (_device->hasDeviceDataChanged()) {
		if (_streaming) {
			_streaming->pauseStreaming(clientID);
		}
	}

	// Get transport type from request, and maybe ports
	if (_streamingType == StreamingType::NONE) {
		if (method == "GET") {
			_streamingType = StreamingType::HTTP;
			_client[clientID].setSessionTimeoutCheck(StreamClient::SessionTimeoutCheck::FILE_DESCRIPTOR);
			_client[clientID].setIPAddressOfStream(_client[clientID].getIPAddressOfSocket());
		} else {
			const std::string transport = headers.getFieldParameter("Transport");
			if (transport.find("unicast") != std::string::npos) {
				if (transport.find("RTP/AVP") != std::string::npos) {
					_streamingType = StreamingType::RTSP_UNICAST;
				}
				_client[clientID].setSessionTimeoutCheck(StreamClient::SessionTimeoutCheck::WATCHDOG);
			} else if (transport.find("multicast") != std::string::npos) {
				_streamingType = StreamingType::RTSP_MULTICAST;
				_client[clientID].setSessionTimeoutCheck(StreamClient::SessionTimeoutCheck::TEARDOWN);
			} else if (transport.find("RTP/AVP/TCP") != std::string::npos) {
				_streamingType = StreamingType::RTP_TCP;
				_client[clientID].setSessionTimeoutCheck(StreamClient::SessionTimeoutCheck::WATCHDOG);
			}
		}
	}

	switch (_streamingType) {
		case StreamingType::RTP_TCP: {
				const int interleaved = headers.getIntFieldParameter("Transport", "interleaved");
				if (interleaved != -1) {
					_client[clientID].setIPAddressOfStream(_client[clientID].getIPAddressOfSocket());
				}
			}
			break;
		case StreamingType::RTSP_UNICAST: {
				const int port = headers.getIntFieldParameter("Transport", "client_port");
				if (port != -1) {
					_client[clientID].setIPAddressOfStream(_client[clientID].getIPAddressOfSocket());
					_client[clientID].getRtpSocketAttr().setupSocketStructure(_client[clientID].getIPAddressOfStream(), port);
					_client[clientID].getRtcpSocketAttr().setupSocketStructure(_client[clientID].getIPAddressOfStream(), port + 1);
				}
			}
			break;
		case StreamingType::RTSP_MULTICAST: {
				const std::string dest = headers.getStringFieldParameter("Transport", "destination");
				if (!dest.empty()) {
					_client[clientID].setIPAddressOfStream(dest);
				}
				const int port = headers.getIntFieldParameter("Transport", "port");
				if (port != -1) {
					_client[clientID].getRtpSocketAttr().setupSocketStructure(dest, port);
					_client[clientID].getRtcpSocketAttr().setupSocketStructure(dest, port + 1);
				}
			}
			break;
		default:
			// Do nothing here
			break;
	};

	_client[clientID].restartWatchDog();
	return true;
}

std::string Stream::getDescribeMediaLevelString() const {
	static const char *RTSP_DESCRIBE_MEDIA_LEVEL =
		"m=video @#1 RTP/AVP 33\r\n" \
		"c=IN IP4 @#2\r\n" \
		"a=control:stream=@#3\r\n" \
		"a=fmtp:33 @#4\r\n" \
		"a=@#5\r\n";
	_device->monitorSignal(false);
	const std::string desc_attr = _device->attributeDescribeString();
	if (desc_attr.size() > 5) {
		if (_streamingType == StreamingType::RTSP_MULTICAST) {
			return StringConverter::stringFormat(RTSP_DESCRIBE_MEDIA_LEVEL,
				_client[0].getRtpSocketAttr().getSocketPort(),
				_client[0].getIPAddressOfStream() + "/0",
				_device->getStreamID().getID(), desc_attr,
				(_streamActive) ? "sendonly" : "inactive");
		} else {
			return StringConverter::stringFormat(RTSP_DESCRIBE_MEDIA_LEVEL,
				0, "0.0.0.0", _device->getStreamID().getID(), desc_attr,
				(_streamActive) ? "sendonly" : "inactive");
		}
	}
	return "";
}
