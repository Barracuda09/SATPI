/* Stream.cpp

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
#include <Stream.h>

#include <Log.h>
#include <StringConverter.h>
#include <Utils.h>
#include <output/StreamClient.h>
#include <input/Device.h>
#include <input/dvb/Frontend.h>
#include <input/dvb/FrontendData.h>
#include <input/dvb/delivery/DVBS.h>
#include <output/StreamClientOutputHttp.h>
#include <output/StreamClientOutputRtp.h>
#include <output/StreamClientOutputRtpTcp.h>
#include <socket/SocketClient.h>

#ifdef LIBDVBCSA
	#include <decrypt/dvbapi/Client.h>
#endif

#include <algorithm>
#include <thread>


// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

Stream::Stream(input::SpDevice device, decrypt::dvbapi::SpClient decrypt) :
	_enabled(true),
	_streamInUse(false),
	_decrypt(decrypt),
	_device(device),
	_rtcpSignalUpdate(1),
	_threadDeviceDataReader(
		StringConverter::stringFormat("Reader@#1", _device->getFeID()),
		std::bind(&Stream::threadExecuteDeviceDataReader, this)),
	_threadDeviceMonitor(
		StringConverter::stringFormat("Monitor@#1", _device->getFeID()),
		std::bind(&Stream::threadExecuteDeviceMonitor, this)),
	_writeIndex(0),
	_readIndex(0),
	_sendInterval(100),
	_signalLock(false) {
	ASSERT(device);
#ifdef LIBDVBCSA
	ASSERT(decrypt);
#endif
	// Initialize all TS packets
	for (mpegts::PacketBuffer& buffer : _tsBuffer) {
		buffer.initialize(0, 0);
	}
	std::array<unsigned char, 188> nullPacked{};
	std::memset(nullPacked.data(), 0xFF, nullPacked.size());
	nullPacked[0] = 0x47;
	nullPacked[1] = 0x1F;
	nullPacked[2] = 0xFF;
	_tsEmpty.initialize(0, 0);
	std::memcpy(_tsEmpty.getWriteBufferPtr(), nullPacked.data(), nullPacked.size());
	_tsEmpty.addAmountOfBytesWritten(188);
}

// ===========================================================================
// -- Static member functions ------------------------------------------------
// ===========================================================================

SpStream Stream::makeSP(input::SpDevice device, decrypt::dvbapi::SpClient decrypt) {
	return std::make_shared<Stream>(device, decrypt);
}

// =======================================================================
//  -- base::XMLSupport --------------------------------------------------
// =======================================================================

void Stream::doAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "streamindex", _device->getFeID().getID());

	ADD_XML_CHECKBOX(xml, "enable", (_enabled ? "true" : "false"));
	ADD_XML_ELEMENT(xml, "attached", _streamInUse ? "yes" : "no");
	ADD_XML_NUMBER_INPUT(xml, "rtcpSignalUpdate", _rtcpSignalUpdate, 1, 5);
	for (const output::SpStreamClient &client : _streamClientVector) {
		client->addToXML(xml);
	}
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

StreamID Stream::getStreamID() const {
	return _device->getStreamID();
}

FeID Stream::getFeID() const {
	return _device->getFeID();
}

int Stream::getFeIndex() const {
	return _device->getFeIndex();
}

#ifdef LIBDVBCSA
input::dvb::SpFrontendDecryptInterface Stream::getFrontendDecryptInterface() {
	return std::dynamic_pointer_cast<input::dvb::FrontendDecryptInterface>(_device);
}
#endif

void Stream::addDeliverySystemCount(
		std::size_t &dvbs2,
		std::size_t &dvbt,
		std::size_t &dvbt2,
		std::size_t &dvbc,
		std::size_t &dvbc2) {
	if (_enabled) {
		_device->addDeliverySystemCount(dvbs2, dvbt, dvbt2, dvbc, dvbc2);
	}
}

void Stream::startStreaming(output::SpStreamClient streamClient) {
	streamClient->startStreaming();

	// set begin timestamp
	_t1 = std::chrono::steady_clock::now();
	_writeIndex = 0;
	_readIndex = 0;
	_tsBuffer[_writeIndex].reset();

	_threadDeviceMonitor.startThread();
	_threadDeviceDataReader.startThread();
	_threadDeviceDataReader.setPriority(base::Thread::Priority::AboveNormal);
	SI_LOG_DEBUG("Frontend: @#1, Start Reader and Monitor Thread", _device->getFeID());
}

void Stream::pauseStreaming(output::SpStreamClient UNUSED(streamClient)) {
	_threadDeviceDataReader.pauseThread();
	_threadDeviceMonitor.pauseThread();
	SI_LOG_DEBUG("Frontend: @#1, Pause Reader and Monitor Thread", _device->getFeID());
#ifdef LIBDVBCSA
	// When LIBDVBCSA is defined _decrypt is created
	_decrypt->stopDecrypt(_device->getFeIndex(), _device->getFeID());
#endif
}

void Stream::restartStreaming(output::SpStreamClient UNUSED(streamClient)) {
	// set begin timestamp
	_t1 = std::chrono::steady_clock::now();
	_writeIndex = 0;
	_readIndex = 0;
	_tsBuffer[_writeIndex].reset();

	_threadDeviceDataReader.restartThread();
	_threadDeviceMonitor.restartThread();
	SI_LOG_DEBUG("Frontend: @#1, Restart Reader and Monitor Thread", _device->getFeID());
}

void Stream::stopStreaming() {
	_threadDeviceDataReader.stopThread();
	_threadDeviceMonitor.stopThread();
	SI_LOG_DEBUG("Frontend: @#1, Stop Reader and Monitor Thread", _device->getFeID());
#ifdef LIBDVBCSA
	// When LIBDVBCSA is defined _decrypt is created
	_decrypt->stopDecrypt(_device->getFeIndex(), _device->getFeID());
#endif
	_device->teardown();
	_streamInUse = false;
}

void Stream::determineAndMakeStreamClientType(FeID feID, const SocketClient &client) {
	// Split message into Headers
	HeaderVector headers = client.getHeaders();
	const TransportParamVector params = client.getTransportParameters();
	const std::string method = client.getMethod();
	if (method == "GET") {
		const std::string multicast = params.getParameter("multicast");
		if (!multicast.empty()) {
			// Format: multicast=IP_ADDR,RTP_PORT,RTCP_PORT,TTL
			const StringVector multiParam = StringConverter::split(multicast, ",");
			if (multiParam.size() == 4) {
				client.spoofHeaderWith(StringConverter::stringFormat(
					"Transport: RTP/AVP;multicast;destination=@#1;port=@#2-@#3;ttl=@#4\r\n",
					multiParam[0], multiParam[1], multiParam[2], multiParam[3]));
				headers = client.getHeaders();
				SI_LOG_INFO("Frontend: @#1, Setup Multicast (@#2) for StreamClient",
					feID, multicast);
				SI_LOG_DEBUG("Frontend: @#1, Found Streaming type: HTTP -> Multicast", feID);
				_streamClientVector.push_back(output::StreamClientOutputRtp::makeSP(feID, true));
			}
		} else {
			SI_LOG_DEBUG("Frontend: @#1, Found Streaming type: HTTP", feID);
			_streamClientVector.push_back(output::StreamClientOutputHttp::makeSP(feID));
		}
	} else {
		const std::string transport = headers.getFieldParameter("Transport");
		if (transport.find("unicast") != std::string::npos) {
			if (transport.find("RTP/AVP") != std::string::npos) {
				SI_LOG_DEBUG("Frontend: @#1, Found Streaming type: RTP/AVP", feID);
				_streamClientVector.push_back(output::StreamClientOutputRtp::makeSP(feID, false));
			}
		} else if (transport.find("multicast") != std::string::npos) {
			SI_LOG_DEBUG("Frontend: @#1, Found Streaming type: RTSP Multicast", feID);
			_streamClientVector.push_back(output::StreamClientOutputRtp::makeSP(feID, true));
		} else if (transport.find("RTP/AVP/TCP") != std::string::npos) {
			SI_LOG_DEBUG("Frontend: @#1, Found Streaming type: RTP/AVP/TCP", feID);
			_streamClientVector.push_back(output::StreamClientOutputRtpTcp::makeSP(feID));
		}
	}
}

output::SpStreamClient Stream::findStreamClientFor(SocketClient &socketClient,
		const bool newSession, const std::string sessionID) {
	base::MutexLock lock(_mutex);
	const FeID id = _device->getFeID();
	const TransportParamVector params = socketClient.getTransportParameters();
	const input::InputSystem msys = params.getMSYSParameter();
	const bool shareable = _device->capableToShare(params);

	// Do we have a new session then check some things
	if (newSession) {
		if (!_enabled) {
			SI_LOG_INFO("Frontend: @#1, New session but this stream is not enabled, skipping...", id);
			return nullptr;
		} else if (_streamInUse && !shareable) {
			SI_LOG_INFO("Frontend: @#1, New session but this stream is in use and not shareable, skipping...", id);
			return nullptr;
		} else if (_device->isLockedByOtherProcess()) {
			SI_LOG_INFO("Frontend: @#1, New session but this stream is in use by an other process, skipping...", id);
			return nullptr;
		} else if (!_device->capableOf(msys)) {
			const double reqFreq = params.getDoubleParameter("freq");
			if (_device->capableToTransform(params)) {
				SI_LOG_INFO("Frontend: @#1, Capable of transforming msys=@#2 with freq=@#3",
					id, StringConverter::delsys_to_string(msys), reqFreq);
			} else {
				SI_LOG_INFO("Frontend: @#1, Not capable of handling msys=@#2 with freq=@#3",
					id, StringConverter::delsys_to_string(msys), reqFreq);
				return nullptr;
			}
		}
	}

	// @TODO[StreamClient] Make here StreamClients etc. (add sharing)
	if (_streamClientVector.empty()) {
		determineAndMakeStreamClientType(id, socketClient);
	}

	// Try to find an empty or requested StreamClients
	for (output::SpStreamClient client : _streamClientVector) {
		// If we have a new session we like to find an empty slot so '-1' or
		// try to find the requested sessionID
		if (client->getSessionID().compare(newSession ? "-1" : sessionID) == 0) {
			if (msys != input::InputSystem::UNDEFINED) {
				SI_LOG_INFO("Frontend: @#1, StreamClient with SessionID @#2 for @#3",
					id, sessionID, StringConverter::delsys_to_string(msys));
			} else {
				SI_LOG_INFO("Frontend: @#1, StreamClient with SessionID @#2",
					id, sessionID);
			}
			client->setSocketClient(socketClient);
			_streamInUse = true;
			return client;
		}
	}

	if (shareable) {
		SI_LOG_INFO("Frontend: @#1, StreamClient with SessionID @#2 is Sharing...", id, sessionID);
		// @TODO: Sharing of devices among StreamClients
	}

	if (msys != input::InputSystem::UNDEFINED) {
		SI_LOG_INFO("Frontend: @#1, No StreamClient with SessionID @#2 for @#3",
			id, sessionID, StringConverter::delsys_to_string(msys));
	} else {
		SI_LOG_INFO("Frontend: @#1, No StreamClient with SessionID @#2", id, sessionID);
	}
	return nullptr;
}

void Stream::checkForSessionTimeout() {
	base::MutexLock lock(_mutex);
	if (!_streamInUse) {
		return;
	}
	for (const output::SpStreamClient &client : _streamClientVector) {
		if (client->sessionTimeout() || !_enabled) {
			if (_enabled) {
				SI_LOG_INFO("Frontend: @#1, Watchdog kicked in for StreamClient with SessionID @#2",
					_device->getFeID(), client->getSessionID());
			} else {
				SI_LOG_INFO("Frontend: @#1, Reclaiming StreamClient with SessionID @#2",
					_device->getFeID(), client->getSessionID());
			}
			teardown(client);
		}
	}
}

bool Stream::update(output::SpStreamClient streamClient) {
	base::MutexLock lock(_mutex);

	// Get frequency changed flag, before device update, because it resets it
	const bool frequencyChanged = _device->hasDeviceFrequencyChanged();

	if (!_device->update()) {
		return false;
	}

	// start or restart streaming again
	const bool threadStopped = _threadDeviceDataReader.isStopped();
	if (threadStopped) {
		startStreaming(streamClient);
	} else if (frequencyChanged) {
		restartStreaming(streamClient);
	}
	return true;
}

bool Stream::teardown(output::SpStreamClient streamClient) {
	base::MutexLock lock(_mutex);

	SI_LOG_INFO("Frontend: @#1, Teardown StreamClient with SessionID @#2",
		_device->getFeID(), streamClient->getSessionID());

	const auto s = std::find(_streamClientVector.begin(), _streamClientVector.end(), streamClient);
	if (s != _streamClientVector.end()) {
		_streamClientVector.erase(s);
	}

	streamClient->teardown();
	if (_streamClientVector.size() == 0) {
		stopStreaming();
	}
	return true;
}

bool Stream::processStreamingRequest(const SocketClient &client, output::SpStreamClient streamClient) {
	base::MutexLock lock(_mutex);

	if (client.hasTransportParameters()) {
		const std::string method = client.getMethod();
		if (method == "SETUP" || method == "PLAY"  || method == "GET") {
			const TransportParamVector params = client.getTransportParameters();
			_device->parseStreamString(params);
		}
	}

	// Frequency changed?.. pause Stream
	if (_device->hasDeviceFrequencyChanged() && _threadDeviceDataReader.isStarted()) {
		pauseStreaming(streamClient);
	}

	streamClient->processStreamingRequest(client);
	return true;
}

std::string Stream::getSDPMediaLevelString() const {
	_device->monitorSignal(false);
	const std::string fmtp = _device->attributeDescribeString();
	std::string mediaLevel;
	if (fmtp.size() > 5) {
		for (const output::SpStreamClient &client : _streamClientVector) {
			mediaLevel += client->getSDPMediaLevelString(_device->getStreamID(), fmtp);
		}
	}
	return mediaLevel;
}

bool Stream::threadExecuteDeviceDataReader() {
	const size_t availableSize = (_writeIndex >= _readIndex) ?
			((_tsBuffer.size() - _writeIndex) + _readIndex) : (_readIndex - _writeIndex);

//	SI_LOG_DEBUG("Frontend: @#1, PacketBuffer MAX @#2 W @#3 R @#4  A @#5", _device->getFeID(), _tsBuffer.size(), write, read, availableSize);
	if (_device->isDataAvailable() && availableSize >= 1) {
		if (_device->readTSPackets(_tsBuffer[_writeIndex])) {
#ifdef LIBDVBCSA
			// When LIBDVBCSA is defined _decrypt is created
			_decrypt->decrypt(_device->getFeIndex(), _device->getFeID(), _tsBuffer[_writeIndex]);
#endif
			// goto next, so inc write index
			++_writeIndex;
			_writeIndex %= _tsBuffer.size();
			// reset next
			_tsBuffer[_writeIndex].reset();
		}
	}
	executeStreamClientWriter();
	return true;
}

void Stream::executeStreamClientWriter() {
	// calculate interval
	_t2 = std::chrono::steady_clock::now();
	const unsigned long interval = std::chrono::duration_cast<std::chrono::microseconds>(_t2 - _t1).count();
	const bool intervalExeeded = interval > _sendInterval;

	const size_t availableSize = (_writeIndex >= _readIndex) ?
			(_writeIndex - _readIndex) : ((_tsBuffer.size() - _readIndex) + _writeIndex);

	if (availableSize > 0 || intervalExeeded) {
		const size_t cnt = (availableSize > 4) ? 4 : 1;
//		SI_LOG_DEBUG("Frontend: @#1, PacketBuffer MAX @#2 W @#3 R @#4 A @#5 C @#6", _device->getFeID(), _tsBuffer.size(), write, read, availableSize, cnt);
		for (size_t i = 0; i < cnt; ++i) {
			const bool readyToSend = _tsBuffer[_readIndex].isReadyToSend();
			if (readyToSend) {
				_t1 = _t2;
				bool incrementReadIndex = false;
				// Send the packet full or not, else send null packet
				for (const output::SpStreamClient &client : _streamClientVector) {
					if (client->writeData(_tsBuffer[_readIndex])) {
						incrementReadIndex = true;
					}
				}
				if (incrementReadIndex) {
					++_readIndex;
					_readIndex %= _tsBuffer.size();
				}
			} else if (intervalExeeded) {
				for (const output::SpStreamClient &client : _streamClientVector) {
					client->writeData(_tsEmpty);
				}
				break;
			} else {
				break;
			}
		}
	}
}

bool Stream::threadExecuteDeviceMonitor() {
	// check do we need to update Device monitor signals
	_signalLock = _device->monitorSignal(false);
	const unsigned long interval = 200 * _rtcpSignalUpdate;

	const std::string desc = _device->attributeDescribeString();
	for (const output::SpStreamClient &client : _streamClientVector) {
		client->writeRTCPData(desc);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(interval));
	return true;
}

