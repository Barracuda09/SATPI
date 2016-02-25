/* TSReader.cpp

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
#include <input/file/TSReader.h>

#include <Log.h>
#include <Utils.h>
#include <Stream.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>

namespace input {
namespace file {

	TSReader::TSReader(int streamID) :
		_streamID(streamID),
		_filePath("None") {}

	TSReader::~TSReader() {}

	// =======================================================================
	//  -- Static member functions -------------------------------------------
	// =======================================================================

	void TSReader::enumerate(StreamVector &streamVector, const std::string &path) {
		SI_LOG_INFO("Setting up TS Reader using path: %s", path.c_str());
		const StreamVector::size_type size = streamVector.size();
		input::file::TSReader *tsreader = new input::file::TSReader(size);
		streamVector.push_back(new Stream(size, tsreader, nullptr));
	}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void TSReader::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		StringConverter::addFormattedString(xml, "<frontendname>%s</frontendname>", "TS Reader");
		StringConverter::addFormattedString(xml, "<pathname>%s</pathname>", _filePath.c_str());
	}

	void TSReader::fromXML(const std::string &UNUSED(xml)) {
		base::MutexLock lock(_mutex);
	}

	// =======================================================================
	//  -- input::Device -----------------------------------------------------
	// =======================================================================

	void TSReader::addDeliverySystemCount(
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

	bool TSReader::isDataAvailable() {
		usleep(1000);
		return true;
	}

	bool TSReader::readFullTSPacket(mpegts::PacketBuffer &buffer) {
		if (_file.is_open()) {
			auto size = buffer.getAmountOfBytesToWrite();
			_file.read(reinterpret_cast<char *>(buffer.getWriteBufferPtr()), size);
			buffer.addAmountOfBytesWritten(size);
			return buffer.full();
		}
		return false;
	}

	bool TSReader::capableOf(fe_delivery_system_t UNUSED(msys)) {
		return false;
	}

	void TSReader::monitorSignal(bool UNUSED(showStatus)) {}

	bool TSReader::hasDeviceDataChanged() const {
		return false;
	}

	void TSReader::parseStreamString(const std::string &msg, const std::string &method) {
		std::string file;
		if (StringConverter::getStringParameter(msg, method, "file=", file) == true) {
			if (!_file.is_open()) {
				_filePath = file;
				_file.open(_filePath, std::ifstream::binary);
				SI_LOG_INFO("TS Reader using path: %s", _filePath.c_str());
			}		
		}
	}

	bool TSReader::update() {
		return true;
	}

	bool TSReader::teardown() {
		_file.close();
		return true;
	}

	bool TSReader::setFrontendInfo(const std::string &UNUSED(fe),
				const std::string &UNUSED(dvr),	const std::string &UNUSED(dmx)) {
		return true;
	}

	std::string TSReader::attributeDescribeString() const {
		std::string desc;
		// ver=1.5;tuner=<feID>;file=<file>
		StringConverter::addFormattedString(desc, "ver=1.5;tuner=%d;file=%s",
				_streamID + 1, _filePath.c_str());

		return desc;
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

} // namespace file
} // namespace input
