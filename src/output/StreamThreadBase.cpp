/* StreamThreadBase.cpp

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
#include <output/StreamThreadBase.h>

#include <base/TimeCounter.h>
#include <StreamInterface.h>
#include <StreamClient.h>
#include <StringConverter.h>
#include <Log.h>
#include <input/Device.h>
#ifdef LIBDVBCSA
	#include <decrypt/dvbapi/Client.h>
#endif

#include <array>
#include <chrono>
#include <thread>

namespace output {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

StreamThreadBase::StreamThreadBase(const std::string &protocol, StreamInterface &stream) :
	ThreadBase(StringConverter::stringFormat("Streaming@#1", stream.getFeID())),
	_stream(stream),
	_protocol(protocol),
	_state(State::Paused),
	_signalLock(false),
	_clientID(0),
	_cseq(0),
	_threadDeviceMonitor(
		StringConverter::stringFormat("Monitor@#1", stream.getFeID()),
		std::bind(&StreamThreadBase::threadExecuteDeviceMonitor, this)),
	_writeIndex(0),
	_readIndex(0),
	_sendInterval(100) {
	// Initialize all TS packets
	uint32_t ssrc = _stream.getSSRC();
	long timestamp = _stream.getTimestamp();
	for (size_t i = 0; i < MAX_BUF; ++i) {
		_tsBuffer[i].initialize(ssrc, timestamp);
	}
	const std::array<unsigned char, 188> nullPacked{0x47, 0x1F, 0xFF};
	_tsEmpty.initialize(ssrc, timestamp);
	std::memcpy(_tsEmpty.getWriteBufferPtr(), nullPacked.data(), 188);
	_tsEmpty.addAmountOfBytesWritten(188);
}

StreamThreadBase::~StreamThreadBase() {
	_threadDeviceMonitor.terminateThread();
#ifdef LIBDVBCSA
	decrypt::dvbapi::SpClient decrypt = _stream.getDecryptDevice();
	if (decrypt != nullptr) {
		decrypt->stopDecrypt(_stream.getFeIndex(), _stream.getFeID());
	}
#endif
}

// =============================================================================
//  -- base::ThreadBase --------------------------------------------------------
// =============================================================================

void StreamThreadBase::threadEntry() {
	StreamClient &client = _stream.getStreamClient(_clientID);
	while (running()) {
		switch (_state) {
			case State::Pause:
				_state = State::Paused;
				break;
			case State::Paused:
				// Do nothing here, just wait
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				break;
			case State::Running:
				readDataFromInputDevice(client);
				break;
			default:
				SI_LOG_PERROR("Wrong State");
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				break;
		}
	}
}

// =========================================================================
//  -- Other member functions ----------------------------------------------
// =========================================================================

bool StreamThreadBase::startStreaming(const int clientID) {
	_clientID = clientID;
	const FeID id = _stream.getFeID();
	const StreamClient &client = _stream.getStreamClient(clientID);

	if (!_threadDeviceMonitor.startThread()) {
		SI_LOG_ERROR("Frontend: @#1, Error Starting device monitor", id);
		return false;
	}

	doStartStreaming(clientID);

	_cseq = 0x0000;
	_writeIndex = 0;
	_readIndex = 0;
	_tsBuffer[_writeIndex].reset();

	if (!startThread()) {
		SI_LOG_ERROR("Frontend: @#1, Start @#2 Start stream to @#3:@#4 ERROR", id, _protocol,
			client.getIPAddressOfStream(), getStreamSocketPort(clientID));
		return false;
	}
	// Set priority above normal for this Thread
	setPriority(Priority::AboveNormal);

	// set begin timestamp
	_t1 = std::chrono::steady_clock::now();

	_state = State::Running;
	SI_LOG_INFO("Frontend: @#1, Start @#2 stream to @#3:@#4", id, _protocol,
		client.getIPAddressOfStream(), getStreamSocketPort(clientID));

	return true;
}

bool StreamThreadBase::pauseStreaming(const int clientID) {
	bool paused = true;
	// Check if thread is running
	if (running()) {
		_threadDeviceMonitor.pauseThread();
		doPauseStreaming(clientID);

		_state = State::Pause;
		const StreamClient &client = _stream.getStreamClient(clientID);
		const double payload = _stream.getRtpPayload() / (1024.0 * 1024.0);
		// try waiting on pause
		auto timeout = 0;
		while (_state != State::Paused) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			++timeout;
			if (timeout > 50) {
				SI_LOG_ERROR("Frontend: @#1, Pause @#2 stream to @#3:@#4  TIMEOUT (Streamed @#5 MBytes)",
					_stream.getFeID(), _protocol, client.getIPAddressOfStream(),
					getStreamSocketPort(clientID), payload);
				paused = false;
				break;
			}
		}
		if (paused) {
			SI_LOG_INFO("Frontend: @#1, Pause @#2 stream to @#3:@#4 (Streamed @#5 MBytes)",
					_stream.getFeID(), _protocol, client.getIPAddressOfStream(),
					getStreamSocketPort(clientID), payload);
		}
#ifdef LIBDVBCSA
		decrypt::dvbapi::SpClient decrypt = _stream.getDecryptDevice();
		if (decrypt != nullptr) {
			decrypt->stopDecrypt(_stream.getFeIndex(), _stream.getFeID());
		}
#endif
	}
	return paused;
}

bool StreamThreadBase::restartStreaming(const int clientID) {
	// Check if thread is running
	if (running()) {
		_threadDeviceMonitor.restartThread();
		doRestartStreaming(clientID);
		_writeIndex = 0;
		_readIndex  = 0;
		_tsBuffer[_writeIndex].reset();
		_state = State::Running;
		SI_LOG_INFO("Frontend: @#1, Restart @#2 stream to @#3:@#4", _stream.getFeID(),
			_protocol, _stream.getStreamClient(clientID).getIPAddressOfStream(),
			getStreamSocketPort(clientID));
	}
	return true;
}

void StreamThreadBase::readDataFromInputDevice(StreamClient &client) {
	const input::SpDevice inputDevice = _stream.getInputDevice();

	// calculate interval
	_t2 = std::chrono::steady_clock::now();
	const unsigned long interval = std::chrono::duration_cast<std::chrono::microseconds>(_t2 - _t1).count();
	const bool intervalExeeded = interval > _sendInterval;

	size_t availableSize = (MAX_BUF - (_writeIndex - _readIndex));
	if (availableSize > MAX_BUF) {
		availableSize %= MAX_BUF;
	}
//	SI_LOG_DEBUG("Frontend: @#1, PacketBuffer MAX @#2 W @#3 R @#4  S @#5", _stream.getFeID(), MAX_BUF, _writeIndex, _readIndex, availableSize);
	if (inputDevice->isDataAvailable() && availableSize > 1) {
		if (inputDevice->readTSPackets(_tsBuffer[_writeIndex], intervalExeeded)) {
#ifdef LIBDVBCSA
			decrypt::dvbapi::SpClient decrypt = _stream.getDecryptDevice();
			if (decrypt != nullptr) {
				decrypt->decrypt(_stream.getFeIndex(), _stream.getFeID(), _tsBuffer[_writeIndex]);
			}
#endif
			// goto next, so inc write index
			++_writeIndex;
			_writeIndex %= MAX_BUF;
			// reset next
			_tsBuffer[_writeIndex].reset();
		}
	}
	if (intervalExeeded) {
		_t1 = _t2;
		// Send the packet full or not, else send null packet
		if (_tsBuffer[_readIndex].isReadyToSend()) {
			if (writeDataToOutputDevice(_tsBuffer[_readIndex], client)) {
				// inc read index only when send is successful
				++_readIndex;
				_readIndex %= MAX_BUF;
			}
		} else if (_signalLock) {
			writeDataToOutputDevice(_tsEmpty, client);
		}
	}
}

bool StreamThreadBase::threadExecuteDeviceMonitor() {
	// check do we need to update Device monitor signals
	_signalLock = _stream.getInputDevice()->monitorSignal(false);
	const unsigned long interval = 200 * _stream.getRtcpSignalUpdateFrequency();
	std::this_thread::sleep_for(std::chrono::milliseconds(interval));
	return true;
}

} // namespace output
