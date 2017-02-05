/* StreamThreadHttp.cpp

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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
#include <output/StreamThreadHttp.h>

#include <InterfaceAttr.h>
#include <Log.h>
#include <StreamInterface.h>
#include <StreamClient.h>
#include <base/TimeCounter.h>

#include <chrono>
#include <thread>

namespace output {

	StreamThreadHttp::StreamThreadHttp(
		StreamInterface &stream,
		decrypt::dvbapi::SpClient decrypt) :
		StreamThreadBase("HTTP", stream, decrypt),
		_clientID(0) {}

	StreamThreadHttp::~StreamThreadHttp() {
		terminateThread();
		const int streamID = _stream.getStreamID();
		StreamClient &client = _stream.getStreamClient(_clientID);
		SI_LOG_INFO("Stream: %d, Destroy %s stream to %s:%d", streamID, _protocol.c_str(),
			client.getIPAddress().c_str(), getStreamSocketPort(_clientID));
	}

	bool StreamThreadHttp::startStreaming() {
		const int streamID = _stream.getStreamID();
		StreamClient &client = _stream.getStreamClient(_clientID);

		// Get default buffer size and set it x times as big
		const int bufferSize = client.getHttpNetworkSendBufferSize() * 20;
		client.setHttpNetworkSendBufferSize(bufferSize);
		SI_LOG_INFO("Stream: %d, %s set network buffer size: %d KBytes", streamID, _protocol.c_str(), bufferSize / 1024);

//		client.setSocketTimeoutInSec(2);

		StreamThreadBase::startStreaming();
		return true;
	}

	int StreamThreadHttp::getStreamSocketPort(int clientID) const {
		return _stream.getStreamClient(clientID).getHttpSocketPort();
	}

	void StreamThreadHttp::threadEntry() {
		StreamClient &client = _stream.getStreamClient(0);
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

	bool StreamThreadHttp::writeDataToOutputDevice(mpegts::PacketBuffer &buffer, StreamClient &client) {
		unsigned char *tsBuffer = buffer.getTSReadBufferPtr();

		const unsigned int size = buffer.getBufferSize();

		const long timestamp = base::TimeCounter::getTicks() * 90;

		// RTP packet octet count (Bytes)
		_stream.addRtpData(size, timestamp);

		iovec iov[1];
		iov[0].iov_base = tsBuffer;
		iov[0].iov_len = size;

		// send the HTTP packet
		if (!client.writeHttpData(iov, 1)) {
			if (!client.isSelfDestructing()) {
				SI_LOG_ERROR("Stream: %d, Error sending HTTP Stream Data to %s", _stream.getStreamID(),
					client.getIPAddress().c_str());
				client.selfDestruct();
			}
		}
		return true;
	}

} // namespace output
