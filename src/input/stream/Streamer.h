/* Streamer.h

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
#ifndef INPUT_STREAM_STREAMER_H_INCLUDE
#define INPUT_STREAM_STREAMER_H_INCLUDE INPUT_STREAM_STREAMER_H_INCLUDE

#include <FwDecl.h>
#include <base/Mutex.h>
#include <input/Device.h>

#include <vector>
#include <string>

FW_DECL_NS1(input, DeviceData);

// @todo Forward decl
FW_DECL_NS0(Stream);
typedef std::vector<Stream *> StreamVector;

namespace input {
namespace stream {

	/// The class @c Streamer is for reading from an TS stream as input device
	class Streamer :
		public input::Device {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
			Streamer(int streamID);
			virtual ~Streamer();

			// =======================================================================
			//  -- Static member functions -------------------------------------------
			// =======================================================================

		public:

			static void enumerate(
				StreamVector &streamVector);

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================

		public:
			///
			virtual void addToXML(std::string &xml) const override;

			///
			virtual void fromXML(const std::string &xml) override;


			// =======================================================================
			//  -- input::Device------------------------------------------------------
			// =======================================================================

		public:

			virtual void addDeliverySystemCount(
				std::size_t &dvbs2,
				std::size_t &dvbt,
				std::size_t &dvbt2,
				std::size_t &dvbc,
				std::size_t &dvbc2) override;

			virtual bool isDataAvailable() override;

			virtual bool readFullTSPacket(mpegts::PacketBuffer &buffer) override;

			virtual bool capableOf(input::InputSystem msys) const override;

			virtual void monitorSignal(bool showStatus) override;

			virtual bool hasDeviceDataChanged() const override;

			virtual void parseStreamString(const std::string &msg, const std::string &method) override;

			virtual bool update() override;

			virtual bool teardown() override;

			virtual bool setFrontendInfo(const std::string &fe,
				const std::string &dvr,	const std::string &dmx) override;

			virtual std::string attributeDescribeString() const override;

			virtual bool isTuned() const  override {
				return true;
			}

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		protected:

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:
			base::Mutex _mutex;
			int _streamID;
			std::string _uri;
	};

} // namespace file
} // namespace input

#endif // INPUT_STREAM_STREAMER_H_INCLUDE
