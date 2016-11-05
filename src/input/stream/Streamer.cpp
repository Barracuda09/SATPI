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

#include <cstring>

namespace input {
namespace stream {

	Streamer::Streamer(int streamID) :
		_streamID(streamID),
		_uri("None"),
		_multiAddr("None"),
		_port(0),
		_udp(false) {
		_pfd[0].events  = 0;
		_pfd[0].revents = 0;
		_pfd[0].fd      = -1;
	}

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
/*
			char *ptr = reinterpret_cast<char *>(buffer.getWriteBufferPtr());
			const auto size = buffer.getAmountOfBytesToWrite();

			struct sockaddr_in si_other;
			socklen_t addrlen = sizeof(si_other);
			const ssize_t readSize = ::recvfrom(_udpMultiListen.getFD(), ptr, size, MSG_DONTWAIT, (struct sockaddr *)&si_other, &addrlen);
			if (readSize > 0) {
				buffer.addAmountOfBytesWritten(readSize);
			} else {
				PERROR("_udpMultiListen");
			}


			return buffer.full();
*/
			static char buf[MTU_MAX_TS_PACKET_SIZE];
			static size_t bufIndex = 0;

			const size_t reqSize = MTU_MAX_TS_PACKET_SIZE - bufIndex;

			// Read recv buffer
			struct sockaddr_in si_other;
			socklen_t addrlen = sizeof(si_other);
			const ssize_t readSize = ::recvfrom(_udpMultiListen.getFD(), &buf[bufIndex], reqSize, MSG_DONTWAIT, (struct sockaddr *)&si_other, &addrlen);
			bufIndex += readSize;

			// Check are we in sync
			if (bufIndex >= MTU_MAX_TS_PACKET_SIZE) {
				for (size_t i = 0; i < bufIndex; ++i) {
					if (i < bufIndex - (TS_PACKET_SIZE * 3) &&
						buf[i + (TS_PACKET_SIZE * 0)] == 0x47 &&
						buf[i + (TS_PACKET_SIZE * 1)] == 0x47 &&
						buf[i + (TS_PACKET_SIZE * 2)] == 0x47) {

						// copy ALL or what is left
						const size_t sizeLeft = bufIndex - i;

						char *ptr = reinterpret_cast<char *>(buffer.getWriteBufferPtr());
						std::memcpy(ptr, &buf[i], sizeLeft);
						buffer.addAmountOfBytesWritten(sizeLeft);

						// is the buffer full
						if (sizeLeft < MTU_MAX_TS_PACKET_SIZE) {
							// How much do we need to fill buffer up again
							const ssize_t sizeNeeded = MTU_MAX_TS_PACKET_SIZE - sizeLeft;

							// Read the rest from recv buffer
							const ssize_t readSize = ::recvfrom(_udpMultiListen.getFD(), &buf, sizeNeeded, MSG_DONTWAIT, (struct sockaddr *)&si_other, &addrlen);

							// Copy rest to send buffer
							if (readSize == sizeNeeded) {
								char *ptr = reinterpret_cast<char *>(buffer.getWriteBufferPtr());
								std::memcpy(ptr, &buf, sizeNeeded);
								buffer.addAmountOfBytesWritten(sizeNeeded);
								bufIndex = 0;
							}
						} else if (sizeLeft == MTU_MAX_TS_PACKET_SIZE) {
							bufIndex = 0;
						}
						return buffer.full();
					}
				}
			}
		}
		return false;
	}

	bool Streamer::capableOf(const input::InputSystem system) const {
		return system == input::InputSystem::STREAMER;
	}

	void Streamer::monitorSignal(bool UNUSED(showStatus)) {}

	bool Streamer::hasDeviceDataChanged() const {
		return false;
	}

// Server side
// vlc -vvv "D:\test.ts" :sout=#udp{dst=239.0.0.1:12345} :sout-all :sout-keep --loop

// Client side
// http://192.168.178.10:8875/?msys=streamer&uri=udp://239.0.0.1:12345

	void Streamer::parseStreamString(const std::string &msg, const std::string &method) {
		if (StringConverter::getStringParameter(msg, method, "uri=", _uri) == true) {

			// Open stream
			_udp = _uri.find("udp") != std::string::npos;
			std::string::size_type begin = _uri.find("//");
			if (begin != std::string::npos) {
				std::string::size_type end = _uri.find_first_of(":", begin);
				if (end != std::string::npos) {
					begin += 2;
					_multiAddr = _uri.substr(begin, end - begin);
					begin = end + 1;
					end = _uri.size();
					_port = std::atoi(_uri.substr(begin, end - begin).c_str());

					//  Open mutlicast stream
					if(initMutlicastUDPSocket(_udpMultiListen, _multiAddr.c_str(), _port, "0.0.0.0")) {
						SI_LOG_INFO("Stream: %d, Streamer reading from: %s:%d  fd %d", _streamID, _multiAddr.c_str(), _port, _udpMultiListen.getFD());
						// set receive buffer to 8MB
						const int bufferSize =  1024 * 1024 * 8;
						_udpMultiListen.setNetworkReceiveBufferSize(bufferSize);

						_pfd[0].events  = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
						_pfd[0].revents = 0;
						_pfd[0].fd      = _udpMultiListen.getFD();

					} else {
						SI_LOG_ERROR("Stream: %d, Init UDP Multicast socket failed", _streamID);
					}
				}
			}
		}
	}

	bool Streamer::update() {
		return true;
	}

	bool Streamer::teardown() {
		// Close stream
		_udpMultiListen.closeFD();
		return true;
	}

	std::string Streamer::attributeDescribeString() const {
		std::string desc;
		if (_udpMultiListen.getFD() != -1) {
			// ver=1.5;tuner=<feID>;uri=<uri>
			StringConverter::addFormattedString(desc, "ver=1.5;tuner=%d;uri=%s",
					_streamID + 1, _uri.c_str());
		} else {
			desc = "";
		}
		return desc;
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

} // namespace stream
} // namespace input
