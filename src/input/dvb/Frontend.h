/* Frontend.h

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
#ifndef INPUT_DVB_FRONTEND_H_INCLUDE
#define INPUT_DVB_FRONTEND_H_INCLUDE INPUT_DVB_FRONTEND_H_INCLUDE

#include <FwDecl.h>
#include <dvbfix.h>
#include <base/Mutex.h>
#include <input/Device.h>
#include <input/dvb/delivery/System.h>

#include <vector>
#include <string>

FW_DECL_NS1(input, DeviceData);
FW_DECL_NS2(decrypt, dvbapi, Client);
FW_DECL_NS3(input, dvb, delivery, System);

// @todo Forward decl
FW_DECL_NS0(Stream);
typedef std::vector<Stream *> StreamVector;

namespace input {
namespace dvb {

	#define MAX_DELSYS 5

	/// The class @c Frontend carries all the data/information of an frontend
	/// and to tune it
	class Frontend :
		public input::Device {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
			Frontend();
			virtual ~Frontend();

			// =======================================================================
			//  -- Static member functions -------------------------------------------
			// =======================================================================

		public:

			static void enumerate(
				StreamVector &stream,
				decrypt::dvbapi::Client *decrypt,
				const std::string &path);

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================

		public:
			///
			virtual void addToXML(std::string &xml) const;

			///
			virtual void fromXML(const std::string &xml);

			// =======================================================================
			//  -- input::Device------------------------------------------------------
			// =======================================================================

		public:

			virtual void addDeliverySystemCount(
				std::size_t &dvbs2,
				std::size_t &dvbt,
				std::size_t &dvbt2,
				std::size_t &dvbc,
				std::size_t &dvbc2);

			virtual bool isDataAvailable();

			virtual bool readFullTSPacket(mpegts::PacketBuffer &buffer);

			virtual bool capableOf(fe_delivery_system_t msys);

			virtual void monitorSignal(bool showStatus);

			virtual bool update(int streamID, input::DeviceData *data);

			virtual bool teardown(int streamID, input::DeviceData *data);

			virtual bool setFrontendInfo(const std::string &fe,
				const std::string &dvr,	const std::string &dmx);

			virtual std::string attributeDescribeString(int streamID,
				const input::DeviceData *data) const;

			virtual bool isTuned() const {
				return (_fd_dvr != -1) && _tuned;
			}

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		protected:

			///
			int open_fe(const std::string &path, bool readonly) const;

			///
			int open_dvr(const std::string &path);

			///
			int open_dmx(const std::string &path);

			///
			bool set_demux_filter(int fd, uint16_t pid);

			///
			bool tune(int streamID, const input::dvb::FrontendData &frontendData);

			///
			bool updatePIDFilters(int streamID, input::dvb::FrontendData &frontendData);

			///
			bool setupAndTune(int streamID, const input::dvb::FrontendData &frontendData);

			///
			void resetPid(int pid, input::dvb::FrontendData &frontendData);

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:
			base::Mutex _mutex;          //
			bool _tuned;
			int _fd_fe;
			int _fd_dvr;
			std::string _path_to_fe;
			std::string _path_to_dvr;
			std::string _path_to_dmx;
			struct dvb_frontend_info _fe_info;

			input::dvb::delivery::SystemVector _deliverySystem;

			std::size_t _dvbs2;
			std::size_t _dvbt;
			std::size_t _dvbt2;
			std::size_t _dvbc;
			std::size_t _dvbc2;

			fe_status_t _status;    /// FE_HAS_LOCK | FE_HAS_SYNC | FE_HAS_SIGNAL
			uint16_t _strength;     ///
			uint16_t _snr;          ///
			uint32_t _ber;          ///
			uint32_t _ublocks;      ///
			unsigned long _dvrBufferSize; ///

	};

} // namespace dvb
} // namespace input

#endif // INPUT_DVB_FRONTEND_H_INCLUDE
