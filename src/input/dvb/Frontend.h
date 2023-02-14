/* Frontend.h

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
#ifndef INPUT_DVB_FRONTEND_H_INCLUDE
#define INPUT_DVB_FRONTEND_H_INCLUDE INPUT_DVB_FRONTEND_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <input/Device.h>
#include <input/Transformation.h>
#include <input/dvb/delivery/System.h>
#include <input/dvb/FrontendData.h>
#ifdef LIBDVBCSA
#include <input/dvb/FrontendDecryptInterface.h>
#include <decrypt/dvbapi/ClientProperties.h>
#endif

#include <string>

FW_DECL_NS1(input, DeviceData);
FW_DECL_NS3(input, dvb, delivery, System);

FW_DECL_SP_NS2(decrypt, dvbapi, Client);
FW_DECL_SP_NS2(input, dvb, Frontend);

FW_DECL_VECTOR_OF_SP_NS0(Stream);

namespace input::dvb {

/// The class @c Frontend carries all the data/information of an frontend
/// and to tune it
class Frontend :
#ifdef LIBDVBCSA
	public input::dvb::FrontendDecryptInterface,
#endif
	public input::Device {
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		Frontend(
			FeIndex index,
			const std::string &appDataPath,
			const std::string &fe,
			const std::string &dvr,
			const std::string &dmx);

		virtual ~Frontend() = default;

		// =========================================================================
		//  -- Static member functions ---------------------------------------------
		// =========================================================================
	public:

		static void enumerate(
			StreamSpVector &streamVector,
			const std::string &appDataPath,
			decrypt::dvbapi::SpClient decrypt,
			const std::string &dvbAdapterPath);

		// =========================================================================
		// -- base::XMLSupport -----------------------------------------------------
		// =========================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

#ifdef LIBDVBCSA
		// =========================================================================
		// -- FrontendDecryptInterface ---------------------------------------------
		// =========================================================================
	public:

		virtual FeID getFeID() const final;

		virtual int getBatchCount() const final;

		virtual int getBatchParity() const final;

		virtual int getMaximumBatchSize() const final;

		virtual void decryptBatch() final;

		virtual void setBatchData(unsigned char *ptr, int len, int parity, unsigned char *originalPtr) final;

		virtual const dvbcsa_bs_key_s *getKey(int parity) const final;

		virtual void setKey(const unsigned char *cw, int parity, int index) final;

		virtual void setICAM(const unsigned char ecm, int parity) final;

		virtual void startOSCamFilterData(int pid, int demux, int filter,
			const unsigned char *filterData, const unsigned char *filterMask) final;

		virtual void stopOSCamFilterData(int pid, int demux, int filter) final;

		virtual bool findOSCamFilterData(int pid, const unsigned char *tsPacket, int tableID,
			int &filter, int &demux, mpegts::TSData &filterData) final;

		virtual std::vector<int> getActiveOSCamDemuxFilters() const final;

		virtual void stopOSCamFilters(FeID id) final;

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
			int hops) final;

		virtual bool isMarkedAsActivePMT(int pid) const final;

		virtual mpegts::SpPMT getPMTData(int pid) const final;

		virtual mpegts::SpSDT getSDTData() const final;
#endif

		// =========================================================================
		//  -- input::Device -------------------------------------------------------
		// =========================================================================
	public:

		virtual void addDeliverySystemCount(
			std::size_t &dvbs2,
			std::size_t &dvbt,
			std::size_t &dvbt2,
			std::size_t &dvbc,
			std::size_t &dvbc2) final;

		virtual bool isDataAvailable() final;

		virtual bool readTSPackets(mpegts::PacketBuffer &buffer, bool finalCall) final;

		virtual bool capableOf(InputSystem system) const final;

		virtual bool capableToTransform(const TransportParamVector& params) const final;

		virtual bool monitorSignal(bool showStatus) final;

		virtual bool hasDeviceDataChanged() const final;

		virtual void parseStreamString(const TransportParamVector& params) final;

		virtual bool update() final;

		virtual bool teardown() final;

		virtual std::string attributeDescribeString() const final;

		virtual mpegts::Filter &getFilter() final {
			return _frontendData.getFilter();
		}

		///
		virtual void updatePIDFilters() final;

		///
		virtual void closeActivePIDFilters() final;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	private:

		void setupFrontend();

		///
		int openFE(const std::string &path, bool readonly) const;

		///
		void closeFE();

		///
		int openDMX(const std::string &path) const;

		///
		void closeDMX();

		///
		bool tune();

		///
		bool setupAndTune();

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

		bool _tuned;
		int _fd_fe;
		int _fd_dmx;
		std::string _path_to_fe;
		std::string _path_to_dvr;
		std::string _path_to_dmx;
		struct dvb_frontend_info _fe_info;
		unsigned int _dvbVersion;

		input::dvb::delivery::SystemUpVector _deliverySystem;
		input::dvb::FrontendData _frontendData;
#ifdef LIBDVBCSA
		decrypt::dvbapi::ClientProperties _dvbapiData;
#endif
		input::Transformation _transform;
		std::size_t _dvbs;
		std::size_t _dvbs2;
		std::size_t _dvbt;
		std::size_t _dvbt2;
		std::size_t _dvbc;
		std::size_t _dvbc2;

		unsigned long _dvrBufferSizeMB;
		unsigned long _waitOnLockTimeout;
		bool _oldApiCallStats;
};

}

#endif // INPUT_DVB_FRONTEND_H_INCLUDE
