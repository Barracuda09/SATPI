/* StreamThreadBase.cpp

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
#include <output/StreamThreadBase.h>

#include <base/TimeCounter.h>
#include <StreamInterface.h>
#include <StreamClient.h>
#include <StringConverter.h>
#include <Log.h>
#include <Utils.h>
#include <input/Device.h>
#ifdef LIBDVBCSA
	#include <decrypt/dvbapi/Client.h>
#endif

namespace output {

	StreamThreadBase::StreamThreadBase(const std::string &protocol, StreamInterface &stream,
			decrypt::dvbapi::SpClient decrypt) :
		ThreadBase(StringConverter::getFormattedString("Streaming%d", stream.getStreamID())),
		_stream(stream),
		_protocol(protocol),
		_state(State::Paused),
		_decrypt(decrypt),
		_sendInterval(100) {
		// Initialize all TS packets
		uint32_t ssrc = _stream.getSSRC();
		long timestamp = _stream.getTimestamp();
		for (size_t i = 0; i < MAX_BUF; ++i) {
			_tsBuffer[i].initialize(ssrc, timestamp);
		}
	}

	StreamThreadBase::~StreamThreadBase() {
#ifdef LIBDVBCSA
		// Do not destroy decrypt here
		if (_decrypt != nullptr) {
			_decrypt->stopDecrypt(_stream.getStreamID());
		}
#endif
		const int streamID = _stream.getStreamID();
		SI_LOG_INFO("Stream: %d, Destroy %s stream", streamID, _protocol.c_str());
	}

	bool StreamThreadBase::startStreaming() {
		const int streamID = _stream.getStreamID();
		const int clientID = 0;
		const StreamClient &client = _stream.getStreamClient(clientID);

		_writeIndex        = 0;
		_readIndex         = 0;
		_tsBuffer[_writeIndex].reset();

		if (!startThread()) {
			SI_LOG_ERROR("Stream: %d, Start %s Start stream to %s:%d ERROR", streamID, _protocol.c_str(),
					client.getIPAddress().c_str(), getStreamSocketPort(clientID));
			return false;
		}
		// Set priority above normal for this Thread
		setPriority(Priority::AboveNormal);

		// set begin timestamp
		_t1 = std::chrono::steady_clock::now();

		// Set affinity with CPU
//		setAffinity(3);

		_state = State::Running;
		SI_LOG_INFO("Stream: %d, Start %s stream to %s:%d", streamID, _protocol.c_str(),
				client.getIPAddress().c_str(), getStreamSocketPort(clientID));

		return true;
	}

	bool StreamThreadBase::restartStreaming(int clientID) {
		// Check if thread is running
		if (running()) {
			_writeIndex = 0;
			_readIndex  = 0;
			_tsBuffer[_writeIndex].reset();
			_state = State::Running;
			SI_LOG_INFO("Stream: %d, Restart %s stream to %s:%d", _stream.getStreamID(),
					_protocol.c_str(), _stream.getStreamClient(clientID).getIPAddress().c_str(),
					getStreamSocketPort(clientID));
		}
		return true;
	}

	bool StreamThreadBase::pauseStreaming(int clientID) {
		bool paused = true;
		// Check if thread is running
		if (running()) {
			_state = State::Pause;
			const StreamClient &client = _stream.getStreamClient(clientID);
			const double payload = _stream.getRtpPayload() / (1024.0 * 1024.0);
			// try waiting on pause
			auto timeout = 0;
			while (_state != State::Paused) {
				usleep(50000);
				++timeout;
				if (timeout > 50) {
					SI_LOG_ERROR("Stream: %d, Pause %s stream to %s:%d  TIMEOUT (Streamed %.3f MBytes)",
							_stream.getStreamID(), _protocol.c_str(), client.getIPAddress().c_str(),
							getStreamSocketPort(clientID), payload);
					paused = false;
					break;
				}
			}
			if (paused) {
				SI_LOG_INFO("Stream: %d, Pause %s stream to %s:%d (Streamed %.3f MBytes)",
						_stream.getStreamID(), _protocol.c_str(), client.getIPAddress().c_str(),
						getStreamSocketPort(clientID), payload);
			}
#ifdef LIBDVBCSA
			if (_decrypt != nullptr) {
				_decrypt->stopDecrypt(_stream.getStreamID());
			}
#endif
		}
		return paused;
	}

	void StreamThreadBase::readDataFromInputDevice(StreamClient &client) {

		input::SpDevice inputDevice = _stream.getInputDevice();

		if (inputDevice->isDataAvailable()) {
			if (inputDevice->readFullTSPacket(_tsBuffer[_writeIndex])) {
#ifdef LIBDVBCSA
				if (_decrypt != nullptr) {
					_decrypt->decrypt(_stream.getStreamID(), _tsBuffer[_writeIndex]);
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

		if (interval > _sendInterval && _tsBuffer[_readIndex].isReadyToSend()) {
			//
			_t1 = _t2;

			if (!_tsBuffer[_readIndex].isSynced()) {
				SI_LOG_INFO("Stream: %d, PacketBuffer not in sync", _stream.getStreamID());
			}

			writeDataToOutputDevice(_tsBuffer[_readIndex], client);

			// inc read index
			++_readIndex;
			_readIndex %= MAX_BUF;
		}
	}

} // namespace output
