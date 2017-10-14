/* StreamThreadTSWriter.cpp

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
#include <output/StreamThreadTSWriter.h>

#include <InterfaceAttr.h>
#include <Log.h>
#include <StreamInterface.h>
#include <StreamClient.h>
#include <Unused.h>
#include <base/TimeCounter.h>

#include <chrono>
#include <thread>

namespace output {

	StreamThreadTSWriter::StreamThreadTSWriter(
		StreamInterface &stream,
		const std::string &file) :
		StreamThreadBase("TSWRITER", stream),
		_filePath(file) {}

	StreamThreadTSWriter::~StreamThreadTSWriter() {
		terminateThread();
	}

	int StreamThreadTSWriter::getStreamSocketPort(int UNUSED(clientID)) const {
		return 0;
	}

	void StreamThreadTSWriter::threadEntry() {
		StreamClient &client = _stream.getStreamClient(0);
		_file.open(_filePath, std::ofstream::binary);
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

	bool StreamThreadTSWriter::writeDataToOutputDevice(mpegts::PacketBuffer &buffer, StreamClient &UNUSED(client)) {
		const unsigned char *tsBuffer = buffer.getTSReadBufferPtr();

		const unsigned int size = buffer.getBufferSize();

		const long timestamp = base::TimeCounter::getTicks() * 90;

		// RTP packet octet count (Bytes)
		_stream.addRtpData(size, timestamp);

		// write TS packets to file
		if (_file.is_open()) {
			_file.write(reinterpret_cast<const char *>(tsBuffer), size);
		}
		return true;
	}

} // namespace output
