/* TSReader.cpp

   Copyright (C) 2014 - 2019 Marc Postema (mpostema09 -at- gmail.com)

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
#include <Unused.h>
#include <Stream.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>

#include <chrono>
#include <thread>

namespace input {
namespace file {

	// =========================================================================
	//  -- Constructors and destructor -----------------------------------------
	// =========================================================================
	TSReader::TSReader(
			int streamID,
			const std::string &appDataPath) :
			Device(streamID),
			_transform(appDataPath, _transformDeviceData) {}

	TSReader::~TSReader() {}

	// =========================================================================
	//  -- Static member functions ---------------------------------------------
	// =========================================================================

	void TSReader::enumerate(
			StreamVector &streamVector,
			const std::string &appDataPath) {
		SI_LOG_INFO("Setting up TS Reader using path: %s", appDataPath.c_str());
		const StreamVector::size_type size = streamVector.size();
		const input::file::SpTSReader tsreader = std::make_shared<input::file::TSReader>(size, appDataPath);
		streamVector.push_back(std::make_shared<Stream>(size, tsreader, nullptr));
	}

	// =========================================================================
	//  -- base::XMLSupport ----------------------------------------------------
	// =========================================================================

	void TSReader::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		ADD_XML_ELEMENT(xml, "frontendname", "TS Reader");
		ADD_XML_ELEMENT(xml, "transformation", _transform.toXML());

		_deviceData.addToXML(xml);
	}

	void TSReader::fromXML(const std::string &xml) {
		base::MutexLock lock(_mutex);
		std::string element;
		if (findXMLElement(xml, "transformation", element)) {
			_transform.fromXML(element);
		}
		_deviceData.fromXML(xml);
	}

	// =========================================================================
	//  -- input::Device -------------------------------------------------------
	// =========================================================================

	void TSReader::addDeliverySystemCount(
			std::size_t &dvbs2,
			std::size_t &dvbt,
			std::size_t &dvbt2,
			std::size_t &dvbc,
			std::size_t &dvbc2) {
		dvbs2 += _transform.advertiseAsDVBS2() ? 1 : 0;
		dvbt  += 0;
		dvbt2 += 0;
		dvbc  += 0;
		dvbc2 += 0;
	}

	bool TSReader::isDataAvailable() {
		if (_pcrDelta != 0) {
			_t2 = _t1;
			_t1 = std::chrono::steady_clock::now();

			const long interval = _pcrDelta - std::chrono::duration_cast<std::chrono::microseconds>(_t1 - _t2).count();
			if (interval > 0) {
				std::this_thread::sleep_for(std::chrono::microseconds(interval));
			}
			_t1 = std::chrono::steady_clock::now();
			_pcrDelta = 0;
		} else {
			std::this_thread::sleep_for(std::chrono::microseconds(1000));
		}
		return true;
	}

	bool TSReader::readFullTSPacket(mpegts::PacketBuffer &buffer) {
		if (_file.is_open()) {
			const auto size = buffer.getAmountOfBytesToWrite();
			_file.read(reinterpret_cast<char *>(buffer.getWriteBufferPtr()), size);
			buffer.addAmountOfBytesWritten(size);
			buffer.trySyncing();

			if (buffer.full()) {
				const std::size_t size = buffer.getNumberOfTSPackets();
				for (std::size_t i = 0; i < size; ++i) {
					// Get TS packet from the buffer
					const unsigned char *data = buffer.getTSPacketPtr(i);

					// Check is this the beginning of the TS and no Transport error indicator
					if (data[0] == 0x47 && (data[1] & 0x80) != 0x80) {
						// Add data to Filter
						getFilter().addData(_streamID, data);

						// get PID from TS
						const uint16_t pid = ((data[1] & 0x1f) << 8) | data[2];
						const uint16_t pcrPID = getFilter().getPMTData().getPCRPid();
						if (pid == pcrPID && (data[3] & 0x20) == 0x20 && (data[5] & 0x10) == 0x10) {
							// Check for 'adaptation field flag' and 'PCR field present'

							//        4           3          2          1          0
							// 76543210 98765432 10987654 32109876 54321098 76543210
							// xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xRRRRRRx xxxxxxxx
							// 21098765 43210987 65432109 87654321 0      8 76543210
							//   3          2          1           0               0
							//  b6       b7       b8       b9       b10      b11
							// PCR Base runs with 90KHz and PCR Ext runs with 27MHz
							#define PCR_BASE(DATA) static_cast<uint64_t>(DATA[6] << 24 | DATA[7] << 16 | DATA[8] << 8 | DATA[9]) << 1
							#define PCR_EXT(DATA)  static_cast<uint64_t>((DATA[10] & 0x01 << 8) | DATA[11])

							const uint64_t pcrCur = PCR_BASE(data);

							_pcrDelta = pcrCur - _pcrPrev;
							_pcrPrev = pcrCur;
							if (_pcrDelta < 0) {
								_pcrDelta = 1;
							}
							if (_pcrDelta > 75000) {
								_pcrDelta = 75000;
							}
							_pcrDelta *= 11;  // 11 -> PCR_Base runs with 90KHz
							#undef PCR_BASE
							#undef PCR_EXT
						}
					}
				}
			}
			return buffer.full();
		}
		return false;
	}

	bool TSReader::capableOf(const input::InputSystem system) const {
		return system == input::InputSystem::FILE;
	}

	bool TSReader::capableToTransform(const std::string &msg,
			const std::string &method) const {
		const double freq = StringConverter::getDoubleParameter(msg, method, "freq=");
		const input::InputSystem system = _transform.getTransformationSystemFor(freq);
		return capableOf(system);
	}

	void TSReader::monitorSignal(bool UNUSED(showStatus)) {
		_deviceData.setMonitorData(FE_HAS_LOCK, 240, 15, 0, 0);
	}

	bool TSReader::hasDeviceDataChanged() const {
		return _deviceData.hasDeviceDataChanged();
	}

	void TSReader::parseStreamString(const std::string &msg, const std::string &method) {
		SI_LOG_INFO("Stream: %d, Parsing transport parameters...", _streamID);

		// Do we need to transform this request?
		const std::string msgTrans = _transform.transformStreamString(_streamID, msg, method);

		_deviceData.parseStreamString(_streamID, msgTrans, method);
		SI_LOG_DEBUG("Stream: %d, Parsing transport parameters (Finished)", _streamID);
	}

	bool TSReader::update() {
		if (!_file.is_open()) {
			SI_LOG_INFO("Stream: %d, Updating frontend...", _streamID);
			const std::string filePath = _deviceData.getFilePath();
			_file.open(filePath, std::ifstream::binary | std::ifstream::in);
			if (_file.is_open()) {
				SI_LOG_INFO("Stream: %d, TS Reader using path: %s", _streamID, filePath.c_str());
				_pcrPrev = 0;
				_pcrDelta = 0;
				_t1 = std::chrono::steady_clock::now();
				_t2 = _t1;
			} else {
				SI_LOG_ERROR("Stream: %d, TS Reader unable to open path: %s", _streamID, filePath.c_str());
			}
		}
		return true;
	}

	bool TSReader::teardown() {
		_deviceData.clearData();
		_transform.resetTransformFlag();
		_file.close();
		return true;
	}

	std::string TSReader::attributeDescribeString() const {
		const DeviceData &data = _transform.transformDeviceData(_deviceData);
		return data.attributeDescribeString(_streamID);
	}

	// =========================================================================
	//  -- Other member functions ----------------------------------------------
	// =========================================================================

} // namespace file
} // namespace input
