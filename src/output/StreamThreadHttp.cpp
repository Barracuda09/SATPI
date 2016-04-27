/* StreamThreadHttp.cpp

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
#include <output/StreamThreadHttp.h>

#include <InterfaceAttr.h>
#include <Log.h>
#include <StreamInterface.h>
#include <StreamClient.h>
#include <Utils.h>
#include <base/TimeCounter.h>

namespace output {

	StreamThreadHttp::StreamThreadHttp(
		StreamInterface &stream,
		decrypt::dvbapi::Client *decrypt) :
		StreamThreadBase("HTTP", stream, decrypt) {
	}

	StreamThreadHttp::~StreamThreadHttp() {
		terminateThread();
	}

	int StreamThreadHttp::getStreamSocketPort(int clientID) const {
		return _stream.getStreamClient(clientID).getRtpSocketAttr().getSocketPort();
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

	void StreamThreadHttp::writeDataToOutputDevice(mpegts::PacketBuffer &buffer, StreamClient &client) {
		unsigned char *tsBuffer = buffer.getTSReadBufferPtr();

		const unsigned int size = buffer.getBufferSize();

		const long timestamp = base::TimeCounter::getTicks() * 90;

		// RTP packet octet count (Bytes)
		_stream.addRtpData(size, timestamp);

		// send the HTTP packet
		if (!client.sendHttpData(tsBuffer, size, MSG_NOSIGNAL)) {
			SI_LOG_ERROR("Send HTTP Stream Data failed");
		}
	}

} // namespace output
