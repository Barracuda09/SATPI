/* TSReader.cpp

   Copyright (C) 2014 - 2023 Marc Postema (mpostema09 -at- gmail.com)

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
#include <mpegts/Generator.h>
#include <mpegts/PacketBuffer.h>

#include <chrono>
#include <thread>

namespace input::childpipe {

// =============================================================================
//  -- Static constexpr variables ----------------------------------------------
// =============================================================================
static constexpr int WAIT_TIMER = 10;

// =============================================================================
//  -- Constructors and destructor ---------------------------------------------
// =============================================================================

TSReader::TSReader(
		FeIndex index,
		const std::string &appDataPath,
		const bool enableUnsecureFrontends) :
		Device(index),
		_transform(appDataPath, input::InputSystem::CHILDPIPE),
		_enableUnsecureFrontends(enableUnsecureFrontends) {}

// =============================================================================
//  -- Static member functions -------------------------------------------------
// =============================================================================

void TSReader::enumerate(
		StreamSpVector &streamVector,
		const std::string &appDataPath,
		const bool enableUnsecureFrontends) {
	SI_LOG_INFO("Setting up Child PIPE - TS Reader using path: @#1", appDataPath);
	const StreamSpVector::size_type size = streamVector.size();
	const input::childpipe::SpTSReader tsreader =
		std::make_shared<input::childpipe::TSReader>(size, appDataPath, enableUnsecureFrontends);
	streamVector.push_back(Stream::makeSP(tsreader, nullptr));
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

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

// =============================================================================
//  -- input::Device -----------------------------------------------------------
// =============================================================================

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
	const std::int64_t pcrDelta = _deviceData.getFilter().getPCRData()->getPCRDelta();
	if (pcrDelta != 0 && pcrTimer == 0) {
		_t2 = _t1;
		_t1 = std::chrono::steady_clock::now();

		const long interval = pcrDelta - std::chrono::duration_cast<std::chrono::microseconds>(_t1 - _t2).count();
		if (interval > 0) {
			std::this_thread::sleep_for(std::chrono::microseconds(interval));
		}
		_t1 = std::chrono::steady_clock::now();
		_deviceData.getFilter().getPCRData()->clearPCRDelta();
	} else {
		std::this_thread::sleep_for(std::chrono::microseconds(WAIT_TIMER + pcrTimer));
	}
	return true;
}

bool TSReader::readTSPackets(mpegts::PacketBuffer& buffer) {
/*
	if (_deviceData.generatePSI()) {
		const mpegts::TSData data = _deviceData.getPSIGenerator().generatePSIFrom(
			_feID, _transform.getTransformationMap());

		std::memcpy(buffer.getWriteBufferPtr(), data.data(), data.size());
		buffer.addAmountOfBytesWritten(data.size());

		_deviceData.getFilter().filterData(_feID, buffer, _deviceData.isInternalPidFilteringEnabled());

		return buffer.full();
	}
*/
	if (!_exec.isOpen()) {
		return false;
	}
	const bool filter = _deviceData.isInternalPidFilteringEnabled();
	for (int i = 0; i < 21; ++i) {
		const int readSize = _exec.read(buffer.getWriteBufferPtr(), buffer.getAmountOfBytesToWrite());
		if (readSize > 0) {
			buffer.addAmountOfBytesWritten(readSize);
			buffer.trySyncing();
			_deviceData.getFilter().filterData(_feID, buffer, filter);
			if (buffer.full()) {
				return true;
			}
		}
	}
	// Check again if buffer is full
	return buffer.full();
}

bool TSReader::capableOf(const input::InputSystem system) const {
	if (_enableUnsecureFrontends) {
		return system == input::InputSystem::CHILDPIPE;
	}
	return false;
}

bool TSReader::capableToShare(const TransportParamVector& UNUSED(params)) const {
	return false;
}

bool TSReader::capableToTransform(const TransportParamVector& params) const {
	const input::InputSystem system = _transform.getTransformationSystemFor(params);
	return system == input::InputSystem::CHILDPIPE;
}

bool TSReader::isLockedByOtherProcess() const {
	return false;
}

bool TSReader::monitorSignal(bool UNUSED(showStatus)) {
	_deviceData.setMonitorData(FE_HAS_LOCK, 240, 15, 0, 0);
	return true;
}

bool TSReader::hasDeviceFrequencyChanged() const {
	return _deviceData.hasDeviceFrequencyChanged();
}

void TSReader::parseStreamString(const TransportParamVector& params) {
	SI_LOG_INFO("Frontend: @#1, Parsing transport parameters...", _feID);

	// Do we need to transform this request?
	const TransportParamVector transParams = _transform.transformStreamString(_feID, params);

	_deviceData.parseStreamString(_feID, transParams);
	SI_LOG_DEBUG("Frontend: @#1, Parsing transport parameters (Finished)", _feID);
}

bool TSReader::update() {
	SI_LOG_INFO("Frontend: @#1, Updating frontend...", _feID);
	if (_deviceData.hasDeviceFrequencyChanged()) {
		_deviceData.resetDeviceFrequencyChanged();
		closeActivePIDFilters();
		_exec.close();
	}
	if (!_exec.isOpen()) {
		const std::string execPath = _deviceData.getFilePath();
		_exec.open(execPath);
		if (_exec.isOpen()) {
			SI_LOG_INFO("Frontend: @#1, Child PIPE - TS Reader using exec: @#2", _feID, execPath);
			_t1 = std::chrono::steady_clock::now();
			_t2 = _t1;
		} else {
			SI_LOG_ERROR("Frontend: @#1, Child PIPE - TS Reader unable to use exec: @#2", _feID, execPath);
		}
	}
	updatePIDFilters();
	SI_LOG_DEBUG("Frontend: @#1, Updating frontend (Finished)", _feID);
	return true;
}

bool TSReader::teardown() {
	closeActivePIDFilters();
	_deviceData.initialize();
	_transform.resetTransformFlag();
	_exec.close();
	return true;
}

std::string TSReader::attributeDescribeString() const {
	if (_exec.isOpen()) {
		const DeviceData &data = _transform.transformDeviceData(_deviceData);
		return data.attributeDescribeString(_feID);
	}
	return "";
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

}
