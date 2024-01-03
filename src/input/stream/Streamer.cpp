/* Streamer.cpp

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
#include <input/stream/Streamer.h>

#include <Log.h>
#include <Unused.h>
#include <Stream.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>

#include <cstring>

namespace input::stream {

// =============================================================================
//  -- Constructors and destructor ---------------------------------------------
// =============================================================================

Streamer::Streamer(
	FeIndex index,
	const std::string &bindIPAddress,
	const std::string &appDataPath) :
	Device(index),
	_transform(appDataPath),
	_bindIPAddress(bindIPAddress) {
	_pfd[0].events  = 0;
	_pfd[0].revents = 0;
	_pfd[0].fd      = -1;
}

// =============================================================================
//  -- Static member functions -------------------------------------------------
// =============================================================================

void Streamer::enumerate(
		StreamSpVector &streamVector,
		const std::string &bindIPAddress,
		const std::string &appDataPath) {
	SI_LOG_INFO("Setting up TS Streamer");
	const StreamSpVector::size_type size = streamVector.size();
	const input::stream::SpStreamer streamer = std::make_shared<Streamer>(size, bindIPAddress, appDataPath);
	streamVector.push_back(Stream::makeSP(streamer, nullptr));
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void Streamer::doAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "frontendname", "Streamer");
	ADD_XML_ELEMENT(xml, "transformation", _transform.toXML());
	_deviceData.addToXML(xml);
}

void Streamer::doFromXML(const std::string &xml) {
	std::string element;
	if (findXMLElement(xml, "transformation", element)) {
		_transform.fromXML(element);
	}
	_deviceData.fromXML(xml);
}

// =============================================================================
//  -- input::Device -----------------------------------------------------------
// =============================================================================

void Streamer::addDeliverySystemCount(
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

bool Streamer::isDataAvailable() {
	// call poll with a timeout of 500 ms
	const int pollRet = poll(_pfd, 1, 500);
	if (pollRet > 0) {
		return _pfd[0].revents != 0;
	}
	return false;
}

bool Streamer::readTSPackets(mpegts::PacketBuffer& buffer) {
	if (_udpMultiListen.getFD() == -1) {
		return false;
	}
	// Read from stream
	char *ptr = reinterpret_cast<char *>(buffer.getWriteBufferPtr());
	const auto size = buffer.getAmountOfBytesToWrite();
	const ssize_t readSize = _udpMultiListen.recvDatafrom(ptr, size, MSG_DONTWAIT);
	if (readSize > 0) {
		buffer.addAmountOfBytesWritten(readSize);
		buffer.trySyncing();
		// Add data to Filter
		_deviceData.getFilter().filterData(_feID, buffer, _deviceData.isInternalPidFilteringEnabled());
	}
	// Check again if buffer is full
	return buffer.full();
}

bool Streamer::capableOf(const input::InputSystem system) const {
	return system == input::InputSystem::STREAMER;
}

bool Streamer::capableToShare(const TransportParamVector& UNUSED(params)) const {
	return false;
}

bool Streamer::capableToTransform(const TransportParamVector& params) const {
	const input::InputSystem system = _transform.getTransformationSystemFor(params);
	return capableOf(system);
}

bool Streamer::isLockedByOtherProcess() const {
	return false;
}

bool Streamer::monitorSignal(const bool UNUSED(showStatus)) {
	_deviceData.setMonitorData(FE_HAS_LOCK, 240, 15, 0, 0);
	return true;
}

bool Streamer::hasDeviceFrequencyChanged() const {
	return _deviceData.hasDeviceFrequencyChanged();
}

// Server side
// vlc -vvv "D:\test.ts" :sout=#udp{dst=224.0.1.3:1234} :sout-all :sout-keep --loop

// Client side
// http://192.168.178.10:8875/?msys=streamer&uri="udp@224.0.1.3:1234"
void Streamer::parseStreamString(const TransportParamVector& params) {
	SI_LOG_INFO("Frontend: @#1, Parsing transport parameters...", _feID);

	// Do we need to transform this request?
	const TransportParamVector transParams = _transform.transformStreamString(_feID, params);

	_deviceData.parseStreamString(_feID, transParams);
	SI_LOG_DEBUG("Frontend: @#1, Parsing transport parameters (Finished)", _feID);
}

bool Streamer::update() {
	SI_LOG_INFO("Frontend: @#1, Updating frontend...", _feID);
	if (_deviceData.hasDeviceFrequencyChanged()) {
		_deviceData.resetDeviceFrequencyChanged();
		closeActivePIDFilters();
		_udpMultiListen.closeFD();
	}
	if (_udpMultiListen.getFD() == -1) {
		//  Open mutlicast stream
		const std::string multiAddr = _deviceData.getMultiAddr();
		const int port = _deviceData.getPort();
		if(initMutlicastUDPSocket(_udpMultiListen, multiAddr, _bindIPAddress, port, 1)) {
			SI_LOG_INFO("Frontend: @#1, Streamer reading from: @#2:@#3  fd @#4",
				_feID, multiAddr, port, _udpMultiListen.getFD());
			// set receive buffer to 8MB
			constexpr int bufferSize =  1024 * 1024 * 8;
			_udpMultiListen.setNetworkReceiveBufferSize(bufferSize);

			_pfd[0].events  = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
			_pfd[0].revents = 0;
			_pfd[0].fd      = _udpMultiListen.getFD();

		} else {
			SI_LOG_ERROR("Frontend: @#1, Init UDP Multicast socket failed", _feID);
		}
	}
	updatePIDFilters();
	SI_LOG_DEBUG("Frontend: @#1, Updating frontend (Finished)", _feID);
	return true;
}

bool Streamer::teardown() {
	closeActivePIDFilters();
	_deviceData.initialize();
	_transform.resetTransformFlag();
	_udpMultiListen.closeFD();
	return true;
}

std::string Streamer::attributeDescribeString() const {
	if (_udpMultiListen.getFD() != -1) {
		const DeviceData &data = _transform.transformDeviceData(_deviceData);
		return data.attributeDescribeString(_feID);
	}
	return "";
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

}
