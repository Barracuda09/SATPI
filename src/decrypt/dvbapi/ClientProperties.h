/* ClientProperties.h

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
#ifndef DECRYPT_DVBAPI_CLIENT_PROPERTIES_H_INCLUDE
#define DECRYPT_DVBAPI_CLIENT_PROPERTIES_H_INCLUDE DECRYPT_DVBAPI_CLIENT_PROPERTIES_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <mpegts/TableData.h>
#include <base/TimeCounter.h>
#include <decrypt/dvbapi/Filter.h>
#include <decrypt/dvbapi/Keys.h>

FW_DECL_NS0(dvbcsa_bs_batch_s);

namespace decrypt::dvbapi {

	///
	class ClientProperties {
		public:

			// ================================================================
			// -- Constructors and destructor ---------------------------------
			// ================================================================
			ClientProperties();

			virtual ~ClientProperties();

			ClientProperties(const ClientProperties&) = delete;

			ClientProperties& operator=(const ClientProperties&) = delete;

			// ================================================================
			//  -- Other member functions -------------------------------------
			// ================================================================

			/// Get the maximum decrypt batch size
			int getMaximumBatchSize() const {
				return _batchSize;
			}

			/// Get how big this decrypt batch is
			int getBatchCount() const {
				return _batchCount;
			}

			/// Get the global parity of this decrypt batch
			int getBatchParity() const {
				return _parity;
			}

			/// Set the pointers into the decrypt batch
			/// @param ptr specifies the pointer to de data that should be decrypted
			/// @param len specifies the lenght of data
			/// @param originalPtr specifies the original TS packet (so we can clear scramble flag when finished)
			void setBatchData(unsigned char *ptr, int len, int parity, unsigned char *originalPtr);

			/// This function will decrypt the batch upon success it will clear scramble flag
			/// on failure it will make a NULL TS Packet and clear scramble flag
			void decryptBatch();

			/// Set the 'next' key for the requested parity
			void setKey(const unsigned char* cw, int parity, int index) {
				_keys.set(cw, parity, index);
			}

			void setICAM(const unsigned char ecm, const int parity) {
				_keys.setICAM(ecm, parity);
			}

			/// Get the active key for the requested parity
			const dvbcsa_bs_key_s *getKey(int parity) const {
				return _keys.get(parity);
			}

			/// Start and add the requested filter
			void startOSCamFilterData(const FeID id, int pid, int demux, int filter,
				const unsigned char* filterData, const unsigned char* filterMask) {
				_filter.start(id, pid, demux, filter, filterData, filterMask);
			}

			/// Stop the requested filter
			void stopOSCamFilterData(int demux, int filter) {
				_filter.stop(demux, filter);
			}

			/// Find the correct filter for the 'collected' data or ts packet
			bool findOSCamFilterData(const FeID id, int pid, const unsigned char* tsPacket, const int tableID,
				int &filter, int &demux, mpegts::TSData &filterData) {
				return _filter.find(id, pid, tsPacket, tableID, filter, demux, filterData);
			}

			/// Get the vector of current 'active' demux filters
			std::vector<int> getActiveOSCamDemuxFilters() const {
				return _filter.getActiveDemuxFilters();
			}

			/// Get the vector of current 'active' PIDs that are being filtered
			std::vector<int> getActiveFilterPIDs(int demux) const {
				return _filter.getActiveFilterPIDs(demux);
			}

			/// Clear all 'active' filters
			void stopOSCamFilters(FeID id);

			///
			void setECMInfo(
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

			// ================================================================
			//  -- Data members -----------------------------------------------
			// ================================================================

		private:

			struct dvbcsa_bs_batch_s *_batch;
			struct dvbcsa_bs_batch_s *_ts;
			int _batchSize;
			int _batchCount;
			int _parity;
			Keys _keys;
			Filter _filter;

	};

}

#endif // DECRYPT_DVBAPI_CLIENT_PROPERTIES_H_INCLUDE
