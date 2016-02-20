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

#include <sys/socket.h>

namespace output {

	StreamThreadRtp::StreamThreadRtp(StreamInterface &stream, decrypt::dvbapi::Client *decrypt) :
		StreamThreadBase("RTP", stream, decrypt),
		_socket_fd(-1),
		_cseq(0),
		_rtcp(stream) {
	}

	StreamThreadRtp::~StreamThreadRtp() {
		terminateThread();
		CLOSE_FD(_socket_fd);
	}

	bool StreamThreadRtp::startStreaming() {
		const int streamID = _stream.getStreamID();

		// RTP
		if (_socket_fd == -1) {
			if ((_socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
				PERROR("socket RTP");
				return false;
			}

			const int val = 1;
			if (setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
				PERROR("RTP setsockopt SO_REUSEADDR");
				return false;
			}

			// Get default buffer size and set it twice as big
			int bufferSize = InterfaceAttr::getNetworkUDPBufferSize(_socket_fd);
			bufferSize *= 2;
			InterfaceAttr::setNetworkUDPBufferSize(_socket_fd, bufferSize);
			SI_LOG_INFO("Stream: %d, %s set network buffer size: %d KBytes", streamID, _protocol.c_str(), bufferSize / 1024);
		}
		// RTCP
		_rtcp.startStreaming();

		_cseq = 0x0000;
		StreamThreadBase::startStreaming();
		return true;
	}

	int StreamThreadRtp::getStreamSocketPort(int clientID) const {
		return  _stream.getStreamClient(clientID).getRtpSocketPort();
	}

	void StreamThreadRtp::threadEntry() {
		const StreamClient &client = _stream.getStreamClient(0);
		while (running()) {
			switch (_state) {
			case Pause:
				_state = Paused;
				break;
			case Paused:
				// Do nothing here, just wait
				usleep(50000);
				break;
			case Running:
				readDataFromInputDevice(client);
				break;
			default:
				PERROR("Wrong State");
				usleep(50000);
				break;
			}
		}
	}

	void StreamThreadRtp::writeDataToOutputDevice(mpegts::PacketBuffer &buffer, const StreamClient &client) {
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

		// send the RTP packet
		if (sendto(_socket_fd, rtpBuffer, size + RTP_HEADER_LEN, MSG_DONTWAIT,
				   (struct sockaddr *)&client.getRtpSockAddr(), sizeof(client.getRtpSockAddr())) == -1) {

			PERROR("send RTP");
		}
	}

} // namespace output
