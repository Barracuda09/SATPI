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

#include <chrono>
#include <thread>

namespace output {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

StreamThreadBase::StreamThreadBase(const std::string &protocol, StreamInterface &stream) :
	ThreadBase(StringConverter::getFormattedString("Streaming%d", stream.getFeID())),
	_stream(stream),
	_protocol(protocol),
	_state(State::Paused),
	_clientID(0),
	_cseq(0),
	_threadDeviceMonitor(
		StringConverter::getFormattedString("Monitor%d", stream.getFeID()),
		std::bind(&StreamThreadBase::threadExecuteDeviceMonitor, this)),
	_writeIndex(0),
	_readIndex(0),
	_sendInterval(100) {
	// Initialize all TS packets
	uint32_t ssrc = _stream.getSSRC();
	long timestamp = _stream.getTimestamp();
	_tsEmpty.initialize(ssrc, timestamp);
	for (size_t i = 0; i < MAX_BUF; ++i) {
		_tsBuffer[i].initialize(ssrc, timestamp);
	}
}

StreamThreadBase::~StreamThreadBase() {
	_threadDeviceMonitor.terminateThread();
#ifdef LIBDVBCSA
	decrypt::dvbapi::SpClient decrypt = _stream.getDecryptDevice();
	if (decrypt != nullptr) {
		decrypt->stopDecrypt(_stream.getFeID());
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
				PERROR("Wrong State");
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
		SI_LOG_ERROR("Frontend: %d, Error Starting device monitor", _stream.getFeID());
		return false;
	}

	doStartStreaming(clientID);

	_cseq = 0x0000;
	_writeIndex = 0;
	_readIndex = 0;
	_tsBuffer[_writeIndex].reset();

	if (!startThread()) {
		SI_LOG_ERROR("Frontend: %d, Start %s Start stream to %s:%d ERROR", id, _protocol.c_str(),
			client.getIPAddressOfStream().c_str(), getStreamSocketPort(clientID));
		return false;
	}
	// Set priority above normal for this Thread
	setPriority(Priority::AboveNormal);

	// set begin timestamp
	_t1 = std::chrono::steady_clock::now();
	_tm1 = std::chrono::steady_clock::now();
	_updateMonInterval = 1;

	_state = State::Running;
	SI_LOG_INFO("Frontend: %d, Start %s stream to %s:%d", id, _protocol.c_str(),
		client.getIPAddressOfStream().c_str(), getStreamSocketPort(clientID));

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
				SI_LOG_ERROR("Frontend: %d, Pause %s stream to %s:%d  TIMEOUT (Streamed %.3f MBytes)",
					_stream.getFeID(), _protocol.c_str(), client.getIPAddressOfStream().c_str(),
					getStreamSocketPort(clientID), payload);
				paused = false;
				break;
			}
		}
		if (paused) {
			SI_LOG_INFO("Frontend: %d, Pause %s stream to %s:%d (Streamed %.3f MBytes)",
					_stream.getFeID(), _protocol.c_str(), client.getIPAddressOfStream().c_str(),
					getStreamSocketPort(clientID), payload);
		}
#ifdef LIBDVBCSA
		decrypt::dvbapi::SpClient decrypt = _stream.getDecryptDevice();
		if (decrypt != nullptr) {
			decrypt->stopDecrypt(_stream.getFeID());
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
		SI_LOG_INFO("Frontend: %d, Restart %s stream to %s:%d", _stream.getFeID(),
				_protocol.c_str(), _stream.getStreamClient(clientID).getIPAddressOfStream().c_str(),
				getStreamSocketPort(clientID));
	}
	return true;
}

void StreamThreadBase::readDataFromInputDevice(StreamClient &client) {
	const input::SpDevice inputDevice = _stream.getInputDevice();

	size_t availableSize = (MAX_BUF - (_writeIndex - _readIndex));
	if (availableSize > MAX_BUF) {
		availableSize %= MAX_BUF;
	}
//		SI_LOG_DEBUG("Frontend: %d, PacketBuffer MAX %d W %d R %d  S %d", _stream.getFeID(), MAX_BUF, _writeIndex, _readIndex, availableSize);
	if (inputDevice->isDataAvailable() && availableSize > 1) {
		if (inputDevice->readFullTSPacket(_tsBuffer[_writeIndex])) {
#ifdef LIBDVBCSA
			decrypt::dvbapi::SpClient decrypt = _stream.getDecryptDevice();
			if (decrypt != nullptr) {
				decrypt->decrypt(_stream.getFeID(), _tsBuffer[_writeIndex]);
			}
#endif
			// goto next, so inc write index
			++_writeIndex;
			_writeIndex %= MAX_BUF;

			// reset next
			_tsBuffer[_writeIndex].reset();
		}
	}
	// calculate interval
	_t2 = std::chrono::steady_clock::now();
	const unsigned long interval = std::chrono::duration_cast<std::chrono::microseconds>(_t2 - _t1).count();
	if (interval > _sendInterval) {
		_t1 = _t2;
		if (_tsBuffer[_readIndex].isReadyToSend()) {
			if (writeDataToOutputDevice(_tsBuffer[_readIndex], client)) {
				// inc read index only when send is successful
				++_readIndex;
				_readIndex %= MAX_BUF;
			}
		} else {
			writeDataToOutputDevice(_tsEmpty, client);
		}
	}
}

bool StreamThreadBase::threadExecuteDeviceMonitor() {
	// check do we need to update Device monitor signals
	_tm2 = std::chrono::steady_clock::now();
	const unsigned long intervalMon = std::chrono::duration_cast<std::chrono::milliseconds>(_tm2 - _tm1).count();
	if (intervalMon > _updateMonInterval) {
		_tm1 = _tm2;
		_stream.getInputDevice()->monitorSignal(false);
		_updateMonInterval = 200 * _stream.getRtcpSignalUpdateFrequency();
	}
	return true;
}

} // namespace output
