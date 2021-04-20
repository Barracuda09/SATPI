/* Streamer.cpp

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
#include <input/stream/Streamer.h>

#include <Log.h>
#include <Unused.h>
#include <Stream.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>

#include <cstring>

namespace input {
namespace stream {

// =============================================================================
//  -- Constructors and destructor ---------------------------------------------
// =============================================================================

Streamer::Streamer(
	int streamID,
	const std::string &bindIPAddress,
	const std::string &appDataPath) :
	Device(streamID),
	_transform(appDataPath, _transformDeviceData),
	_bindIPAddress(bindIPAddress) {
	_pfd[0].events  = 0;
	_pfd[0].revents = 0;
	_pfd[0].fd      = -1;
}

Streamer::~Streamer() {}

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
	streamVector.push_back(std::make_shared<Stream>(streamer, nullptr));
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

bool Streamer::readFullTSPacket(mpegts::PacketBuffer &buffer) {
	if (_udpMultiListen.getFD() != -1) {
		// Read from stream
		char *ptr = reinterpret_cast<char *>(buffer.getWriteBufferPtr());
		const auto size = buffer.getAmountOfBytesToWrite();

		const ssize_t readSize = _udpMultiListen.recvDatafrom(ptr, size, MSG_DONTWAIT);
		if (readSize > 0) {
			buffer.addAmountOfBytesWritten(readSize);
			buffer.trySyncing();
		} else {
			PERROR("_udpMultiListen");
		}
		return buffer.full();
	}
	return false;
}

bool Streamer::capableOf(const input::InputSystem system) const {
	return system == input::InputSystem::STREAMER;
}

bool Streamer::capableToTransform(const std::string &msg,
		const std::string &method) const {
	const input::InputSystem system = _transform.getTransformationSystemFor(msg, method);
	return capableOf(system);
}

void Streamer::monitorSignal(const bool UNUSED(showStatus)) {
	_deviceData.setMonitorData(FE_HAS_LOCK, 240, 15, 0, 0);
}

bool Streamer::hasDeviceDataChanged() const {
	return _deviceData.hasDeviceDataChanged();
}

// Server side
// vlc -vvv "D:\test.ts" :sout=#udp{dst=224.0.1.3:1234} :sout-all :sout-keep --loop

// Client side
// http://192.168.178.10:8875/?msys=streamer&uri="udp@224.0.1.3:1234"
void Streamer::parseStreamString(const std::string &msg, const std::string &method) {
	SI_LOG_INFO("Stream: %d, Parsing transport parameters...", _streamID);

	// Do we need to transform this request?
	const std::string msgTrans = _transform.transformStreamString(_streamID, msg, method);

	_deviceData.parseStreamString(_streamID, msgTrans, method);
	SI_LOG_DEBUG("Stream: %d, Parsing transport parameters (Finished)", _streamID);
}

bool Streamer::update() {
	SI_LOG_INFO("Stream: %d, Updating frontend...", _streamID);
	if (_deviceData.hasDeviceDataChanged()) {
		_deviceData.resetDeviceDataChanged();
		_udpMultiListen.closeFD();
	}
	if (_udpMultiListen.getFD() == -1) {
		//  Open mutlicast stream
		const std::string multiAddr = _deviceData.getMultiAddr();
		const int port = _deviceData.getPort();
		if(initMutlicastUDPSocket(_udpMultiListen, multiAddr, _bindIPAddress, port)) {
			SI_LOG_INFO("Stream: %d, Streamer reading from: %s:%d  fd %d", _streamID,
				multiAddr.c_str(), port, _udpMultiListen.getFD());
			// set receive buffer to 8MB
			constexpr int bufferSize =  1024 * 1024 * 8;
			_udpMultiListen.setNetworkReceiveBufferSize(bufferSize);

			_pfd[0].events  = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
			_pfd[0].revents = 0;
			_pfd[0].fd      = _udpMultiListen.getFD();

		} else {
			SI_LOG_ERROR("Stream: %d, Init UDP Multicast socket failed", _streamID);
		}
	}
	SI_LOG_DEBUG("Stream: %d, Updating frontend (Finished)", _streamID);
	return true;
}

bool Streamer::teardown() {
	_deviceData.initialize();
	_transform.resetTransformFlag();
	_udpMultiListen.closeFD();
	return true;
}

std::string Streamer::attributeDescribeString() const {
	if (_udpMultiListen.getFD() != -1) {
		const DeviceData &data = _transform.transformDeviceData(_deviceData);
		return data.attributeDescribeString(_streamID);
	}
	return "";
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

} // namespace stream
} // namespace input
