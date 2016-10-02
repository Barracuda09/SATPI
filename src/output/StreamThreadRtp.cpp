/* StreamThreadRtp.cpp

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
#include <output/StreamThreadRtp.h>

#include <StreamClient.h>
#include <Log.h>
#include <StreamInterface.h>
#include <InterfaceAttr.h>
#include <Utils.h>
#include <base/TimeCounter.h>

namespace output {

	StreamThreadRtp::StreamThreadRtp(StreamInterface &stream, decrypt::dvbapi::SpClient decrypt) :
		StreamThreadBase("RTP/UDP", stream, decrypt),
		_clientID(0),
		_cseq(0),
		_rtcp(stream, false) {
	}

	StreamThreadRtp::~StreamThreadRtp() {
		terminateThread();
		StreamClient &client = _stream.getStreamClient(_clientID);
		client.getRtpSocketAttr().closeFD();
	}

	bool StreamThreadRtp::startStreaming() {
		const int streamID = _stream.getStreamID();
		SocketAttr &rtp = _stream.getStreamClient(_clientID).getRtpSocketAttr();

		// RTP
		if (!rtp.setupSocketHandle(SOCK_DGRAM, IPPROTO_UDP)) {
			SI_LOG_ERROR("Stream: %d, Get RTP handle failed", streamID);
		}

		// Get default buffer size and set it x times as big
		const int bufferSize = rtp.getNetworkBufferSize() * 20;
		rtp.setNetworkBufferSize(bufferSize);
		SI_LOG_INFO("Stream: %d, %s set network buffer size: %d KBytes", streamID, _protocol.c_str(), bufferSize / 1024);

		// RTCP
		_rtcp.startStreaming();

		_cseq = 0x0000;
		StreamThreadBase::startStreaming();
		return true;
	}

	int StreamThreadRtp::getStreamSocketPort(int clientID) const {
		return  _stream.getStreamClient(clientID).getRtpSocketAttr().getSocketPort();
	}

	void StreamThreadRtp::threadEntry() {
		StreamClient &client = _stream.getStreamClient(0);
		while (running()) {
			switch (_state) {
			case State::Pause:
				_state = State::Paused;
				break;
			case State::Paused:
				// Do nothing here, just wait
				usleep(50000);
				break;
			case State::Running:
				readDataFromInputDevice(client);
				break;
			default:
				PERROR("Wrong State");
				usleep(50000);
				break;
			}
		}
	}

	void StreamThreadRtp::writeDataToOutputDevice(mpegts::PacketBuffer &buffer, StreamClient &client) {
		unsigned char *rtpBuffer = buffer.getReadBufferPtr();

		// update sequence number
		++_cseq;
		rtpBuffer[2] = ((_cseq >> 8) & 0xFF); // sequence number
		rtpBuffer[3] =  (_cseq & 0xFF);       // sequence number

		// update timestamp
		const long timestamp = base::TimeCounter::getTicks() * 90;
		rtpBuffer[4] = (timestamp >> 24) & 0xFF; // timestamp
		rtpBuffer[5] = (timestamp >> 16) & 0xFF; // timestamp
		rtpBuffer[6] = (timestamp >>  8) & 0xFF; // timestamp
		rtpBuffer[7] = (timestamp >>  0) & 0xFF; // timestamp

		const unsigned int size = buffer.getBufferSize();

		// RTP packet octet count (Bytes)
		_stream.addRtpData(size, timestamp);

		// send the RTP/UDP packet
		SocketAttr &rtp = client.getRtpSocketAttr();
		if (!rtp.sendDataTo(rtpBuffer, size + RTP_HEADER_LEN, MSG_DONTWAIT)) {
			if (!client.isSelfDestructing()) {
				SI_LOG_ERROR("Stream: %d, Error sending RTP/UDP data to %s:%d", _stream.getStreamID(),
					rtp.getIPAddress().c_str(), rtp.getSocketPort());
				client.selfDestruct();
			}
		}
	}

} // namespace output
