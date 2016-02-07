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
#include <input/Device.h>

#include <vector>
#include <string>

FW_DECL_NS0(StreamProperties);
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
				const std::string &path,
				int &nr_dvb_s2,
				int &nr_dvb_t,
				int &nr_dvb_t2,
				int &nr_dvb_c,
				int &nr_dvb_c2);

		private:
			///
			static int getAttachedFrontends(
				StreamVector &stream,
				decrypt::dvbapi::Client *decrypt,
				const std::string &path, int count);

			static void countNumberOfDeliverySystems(
				StreamVector &stream,
				int &nr_dvb_s2,
				int &nr_dvb_t,
				int &nr_dvb_t2,
				int &nr_dvb_c,
				int &nr_dvb_c2);

			// =======================================================================
			//  -- input::Device------------------------------------------------------
			// =======================================================================

		public:
			/// @see Device
			virtual bool isDataAvailable();

			/// @see Device
			virtual bool readFullTSPacket(mpegts::PacketBuffer &buffer);

			/// @see Device
			virtual bool capableOf(fe_delivery_system_t msys);

			/// @see Device
			virtual void monitorFrontend(fe_status_t &status, uint16_t &strength, uint16_t &snr, uint32_t &ber,
				uint32_t &ublocks, bool showStatus);

			/// @see Device
			virtual bool update(StreamProperties &properties);

			/// @see Device
			virtual bool teardown(StreamProperties &properties);

			///
			virtual void addFrontendPaths(const std::string &fe,
				const std::string &dvr,
				const std::string &dmx) {
				_path_to_fe = fe;
				_path_to_dvr = dvr;
				_path_to_dmx = dmx;
			}

			///
			virtual void addToXML(std::string &xml) const;

			///
			virtual void fromXML(const std::string &xml);

			///
			virtual bool setFrontendInfo();

			/// Check if this frontend is tuned
			virtual bool isTuned() const {
				return (_fd_dvr != -1) && _tuned;
			}

			///
			virtual std::size_t getDeliverySystemSize() const {
				return _del_sys_size;
			}

			///
			virtual const fe_delivery_system_t *getDeliverySystem() const {
				return _info_del_sys;
			}

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
			bool setProperties(const StreamProperties &properties);

			///
			bool tune(StreamProperties &properties);

			///
			bool updatePIDFilters(StreamProperties &properties);

			///
			bool setupAndTune(StreamProperties &properties);

			///
			void resetPid(StreamProperties &properties, int pid);

		private:

			// =======================================================================
			// Data members
			// =======================================================================
			bool _tuned;
			int _fd_fe;
			int _fd_dvr;
			std::string _path_to_fe;
			std::string _path_to_dvr;
			std::string _path_to_dmx;
			struct dvb_frontend_info _fe_info;
			fe_delivery_system_t _info_del_sys[MAX_DELSYS];
			std::size_t _del_sys_size;

			input::dvb::delivery::System *_deliverySystem;

	};

} // namespace dvb
} // namespace input

#endif // INPUT_DVB_FRONTEND_H_INCLUDE
