/* Streamer.cpp

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
#include <input/stream/Streamer.h>

#include <Log.h>
#include <Utils.h>
#include <Stream.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>

namespace input {
namespace stream {

	Streamer::Streamer(int streamID) :
		_streamID(streamID),
		_uri("None") {}

	Streamer::~Streamer() {}

	// =======================================================================
	//  -- Static member functions -------------------------------------------
	// =======================================================================

	void Streamer::enumerate(StreamVector &streamVector) {
		SI_LOG_INFO("Setting up TS Streamer");
		const StreamVector::size_type size = streamVector.size();
		input::stream::SpStreamer streamer = std::make_shared<input::stream::Streamer>(size);
		streamVector.push_back(std::make_shared<Stream>(size, streamer, nullptr));
	}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void Streamer::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		StringConverter::addFormattedString(xml, "<frontendname>%s</frontendname>", "Streamer");
		StringConverter::addFormattedString(xml, "<pathname>%s</pathname>", _uri.c_str());
	}

	void Streamer::fromXML(const std::string &UNUSED(xml)) {
		base::MutexLock lock(_mutex);
	}

	// =======================================================================
	//  -- input::Device -----------------------------------------------------
	// =======================================================================

	void Streamer::addDeliverySystemCount(
		std::size_t &dvbs2,
		std::size_t &dvbt,
		std::size_t &dvbt2,
		std::size_t &dvbc,
		std::size_t &dvbc2) {
		dvbs2 += 0;
		dvbt  += 0;
		dvbt2 += 0;
		dvbc  += 0;
		dvbc2 += 0;
	}

	bool Streamer::isDataAvailable() {
		usleep(1000);
		return true;
	}

	bool Streamer::readFullTSPacket(mpegts::PacketBuffer &UNUSED(buffer)) {
/*
		if (_file.is_open()) {
			auto size = buffer.getAmountOfBytesToWrite();
			_file.read(reinterpret_cast<char *>(buffer.getWriteBufferPtr()), size);
			buffer.addAmountOfBytesWritten(size);
			return buffer.full();
		}
*/
		// Read from stream
		return false;
	}

	bool Streamer::capableOf(const input::InputSystem system) const {
		return system == input::InputSystem::STREAMER;
	}

	void Streamer::monitorSignal(bool UNUSED(showStatus)) {}

	bool Streamer::hasDeviceDataChanged() const {
		return false;
	}

	void Streamer::parseStreamString(const std::string &msg, const std::string &method) {
		std::string uri;
		if (StringConverter::getStringParameter(msg, method, "uri=", uri) == true) {
			// Open stream
		}
	}

	bool Streamer::update() {
		return true;
	}

	bool Streamer::teardown() {
		// Close stream
		return true;
	}

	std::string Streamer::attributeDescribeString() const {
		std::string desc;
		// ver=1.5;tuner=<feID>;file=<file>
		StringConverter::addFormattedString(desc, "ver=1.5;tuner=%d;file=%s",
				_streamID + 1, _uri.c_str());

		return desc;
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

} // namespace stream
} // namespace input
