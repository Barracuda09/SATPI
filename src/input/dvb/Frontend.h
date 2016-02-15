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
#include <input/dvb/FrontendData.h>
#ifdef LIBDVBCSA
#include <input/dvb/FrontendDecryptInterface.h>
#endif

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
#ifdef LIBDVBCSA
		public input::dvb::FrontendDecryptInterface,
#endif
		public input::Device {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
			Frontend(int streamID);
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

#ifdef LIBDVBCSA
			// =======================================================================
			// -- FrontendDecryptInterface -------------------------------------------
			// =======================================================================
		public:

			virtual int getStreamID() const;

			virtual bool updateInputDevice();

			virtual int getBatchCount() const;

			virtual int getBatchParity() const;

			virtual int getMaximumBatchSize() const;

			virtual void decryptBatch(bool final);

			virtual void setBatchData(unsigned char *ptr, int len, int parity, unsigned char *originalPtr);

			virtual const dvbcsa_bs_key_s *getKey(int parity) const;

			virtual bool isTableCollected(int tableID) const;

			virtual void setTableCollected(int tableID, bool collected);

			virtual const unsigned char *getTableData(int tableID) const;

			virtual void collectTableData(int streamID, int tableID, const unsigned char *data);

			virtual int getTableDataSize(int tableID) const;

			virtual void setPMT(int pid, bool set);

			virtual bool isPMT(int pid) const;

			virtual void setECMFilterData(int demux, int filter, int pid, bool set);

			virtual void getECMFilterData(int &demux, int &filter, int pid) const;

			virtual bool getActiveECMFilterData(int &demux, int &filter, int &pid) const;

			virtual bool isECM(int pid) const;

			virtual void setKey(const unsigned char *cw, int parity, int index);

			virtual void freeKeys();

			virtual void setKeyParity(int pid, int parity);

			virtual int getKeyParity(int pid) const;

			virtual void setECMInfo(
				int pid,
				int serviceID,
				int caID,
				int provID,
				int emcTime,
				const std::string &cardSystem,
				const std::string &readerName,
				const std::string &sourceName,
				const std::string &protocolName,
				int hops);
#endif

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

			virtual bool hasDeviceDataChanged() const;

			virtual void parseStreamString(const std::string &msg, const std::string &method);

			virtual bool update();

			virtual bool teardown();

			virtual bool setFrontendInfo(const std::string &fe,
				const std::string &dvr,	const std::string &dmx);

			virtual std::string attributeDescribeString() const;

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
			bool tune();

			///
			bool updatePIDFilters();

			///
			bool setupAndTune();

			///
			void resetPid(int pid);

			///
			void processPID_L(const std::string &pids, bool add);

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:
			base::Mutex _mutex;
			int _streamID;
			bool _tuned;
			int _fd_fe;
			int _fd_dvr;
			std::string _path_to_fe;
			std::string _path_to_dvr;
			std::string _path_to_dmx;
			struct dvb_frontend_info _fe_info;

			input::dvb::delivery::SystemVector _deliverySystem;
			input::dvb::FrontendData _frontendData;///

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
