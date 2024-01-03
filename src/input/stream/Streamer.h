/* Streamer.h

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
#ifndef INPUT_STREAM_STREAMER_H_INCLUDE
#define INPUT_STREAM_STREAMER_H_INCLUDE INPUT_STREAM_STREAMER_H_INCLUDE

#include <FwDecl.h>
#include <input/Device.h>
#include <input/Transformation.h>
#include <input/stream/StreamerData.h>
#include <socket/SocketClient.h>
#include <socket/UdpSocket.h>

#include <string>

#include <poll.h>

FW_DECL_SP_NS2(input, stream, Streamer);

FW_DECL_VECTOR_OF_SP_NS0(Stream);

namespace input::stream {

/// The class @c Streamer is for reading from an TS stream as input device.
/// Stream can be an Multicast UDP e.g.
/// http://ip.of.your.box:8875/?msys=streamer&uri="udp@224.0.1.3:1234"
class Streamer :
	public input::Device,
	protected UdpSocket {
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		Streamer(
			FeIndex index,
			const std::string &bindIPAddress,
			const std::string &appDataPath);

		virtual ~Streamer() = default;

		// =========================================================================
		//  -- Static member functions ---------------------------------------------
		// =========================================================================
	public:

		static void enumerate(
			StreamSpVector &streamVector,
			const std::string &bindIPAddress,
			const std::string &appDataPath);

		// =========================================================================
		// -- base::XMLSupport -----------------------------------------------------
		// =========================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =========================================================================
		//  -- input::Device--------------------------------------------------------
		// =========================================================================
	public:

		virtual void addDeliverySystemCount(
			std::size_t &dvbs2,
			std::size_t &dvbt,
			std::size_t &dvbt2,
			std::size_t &dvbc,
			std::size_t &dvbc2) final;

		virtual bool isDataAvailable() final;

		virtual bool readTSPackets(mpegts::PacketBuffer& buffer) final;

		virtual bool capableOf(input::InputSystem msys) const final;

		virtual bool capableToShare(const TransportParamVector& params) const final;

		virtual bool capableToTransform(const TransportParamVector& params) const final;

		virtual bool isLockedByOtherProcess() const final;

		virtual bool monitorSignal(bool showStatus) final;

		virtual bool hasDeviceFrequencyChanged() const final;

		virtual void parseStreamString(const TransportParamVector& params) final;

		virtual bool update() final;

		virtual bool teardown() final;

		virtual std::string attributeDescribeString() const final;

		virtual mpegts::Filter &getFilter() final {
			return _deviceData.getFilter();
		}

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	protected:

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

		StreamerData _deviceData;
		input::Transformation _transform;

		pollfd _pfd[1];
		SocketClient _udpMultiListen;

		std::string _bindIPAddress;
};

}

#endif // INPUT_STREAM_STREAMER_H_INCLUDE
