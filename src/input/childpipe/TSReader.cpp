/* TSReader.cpp

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
#include <input/childpipe/TSReader.h>

#include <Log.h>
#include <Unused.h>
#include <Stream.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>

#include <chrono>
#include <thread>

namespace input {
namespace childpipe {

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
			StreamSpVector &streamVector,
			const std::string &appDataPath) {
		SI_LOG_INFO("Setting up Child PIPE - TS Reader using path: %s", appDataPath.c_str());
		const StreamSpVector::size_type size = streamVector.size();
		const input::childpipe::SpTSReader tsreader = std::make_shared<input::childpipe::TSReader>(size, appDataPath);
		streamVector.push_back(std::make_shared<Stream>(size, tsreader, nullptr));
	}

	// =========================================================================
	//  -- base::XMLSupport ----------------------------------------------------
	// =========================================================================

	void TSReader::doAddToXML(std::string &xml) const {
		ADD_XML_ELEMENT(xml, "frontendname", "Child PIPE - TS Reader");
		ADD_XML_ELEMENT(xml, "transformation", _transform.toXML());

		_deviceData.addToXML(xml);
	}

	void TSReader::doFromXML(const std::string &xml) {
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
		dvbc  += _transform.advertiseAsDVBC() ? 1 : 0;
		dvbc2 += 0;
	}

	bool TSReader::isDataAvailable() {
		const int pcrTimer = _deviceData.getPCRTimer();
		const std::int64_t pcrDelta = _deviceData.getFilterData().getPCRData()->getPCRDelta();
		if (pcrDelta != 0 && pcrTimer == 0) {
			_t2 = _t1;
			_t1 = std::chrono::steady_clock::now();

			const long interval = pcrDelta - std::chrono::duration_cast<std::chrono::microseconds>(_t1 - _t2).count();
			if (interval > 0) {
				std::this_thread::sleep_for(std::chrono::microseconds(interval));
			}
			_t1 = std::chrono::steady_clock::now();
			_deviceData.getFilterData().getPCRData()->clearPCRDelta();
		} else {
			std::this_thread::sleep_for(std::chrono::microseconds(150 + pcrTimer));
		}
		return true;
	}

	bool TSReader::readFullTSPacket(mpegts::PacketBuffer &buffer) {
		if (!_exec.isOpen()) {
			return false;
		}
		const int bytes = _exec.read(buffer.getWriteBufferPtr(), buffer.getAmountOfBytesToWrite());
		buffer.addAmountOfBytesWritten(bytes);
		buffer.trySyncing();
		if (!buffer.full()) {
			return false;
		}
		// Add data to Filter
		_deviceData.addFilterData(_streamID, buffer);
		return true;
	}

	bool TSReader::capableOf(const input::InputSystem system) const {
		return system == input::InputSystem::CHILDPIPE;
	}

	bool TSReader::capableToTransform(const std::string &msg,
			const std::string &method) const {
		const input::InputSystem system = _transform.getTransformationSystemFor(msg, method);
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
		SI_LOG_INFO("Stream: %d, Updating frontend...", _streamID);
		if (_deviceData.hasDeviceDataChanged()) {
			_deviceData.resetDeviceDataChanged();
			_exec.close();
		}
		if (!_exec.isOpen()) {
			const std::string execPath = _deviceData.getFilePath();
			_exec.open(execPath);
			if (_exec.isOpen()) {
				SI_LOG_INFO("Stream: %d, Child PIPE - TS Reader using exec: %s", _streamID, execPath.c_str());
				_t1 = std::chrono::steady_clock::now();
				_t2 = _t1;
			} else {
				SI_LOG_ERROR("Stream: %d, Child PIPE - TS Reader unable to use exec: %s", _streamID, execPath.c_str());
			}
		}
		SI_LOG_DEBUG("Stream: %d, Updating frontend (Finished)", _streamID);
		return true;
	}

	bool TSReader::teardown() {
		_deviceData.initialize();
		_transform.resetTransformFlag();
		_exec.close();
		return true;
	}

	std::string TSReader::attributeDescribeString() const {
		if (_exec.isOpen()) {
			const DeviceData &data = _transform.transformDeviceData(_deviceData);
			return data.attributeDescribeString(_streamID);
		}
		return "";
	}

	// =========================================================================
	//  -- Other member functions ----------------------------------------------
	// =========================================================================

} // namespace childpipe
} // namespace input
