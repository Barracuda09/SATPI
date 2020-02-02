/* FrontendDecryptInterface.h

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef INPUT_DVB_FRONTENDDECRYPTINTERFACE_H_INCLUDE
#define INPUT_DVB_FRONTENDDECRYPTINTERFACE_H_INCLUDE INPUT_DVB_FRONTENDDECRYPTINTERFACE_H_INCLUDE

#include <FwDecl.h>

FW_DECL_NS0(dvbcsa_bs_key_s);

FW_DECL_SP_NS1(mpegts, PMT);
FW_DECL_SP_NS2(input, dvb, FrontendDecryptInterface);

namespace input {
namespace dvb {

/// The class @c FrontendDecryptInterface is an interface to an @c Stream for decrypting
class FrontendDecryptInterface {
		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:

		FrontendDecryptInterface() {}

		virtual ~FrontendDecryptInterface() {}

		// =====================================================================
		// -- Other member functions -------------------------------------------
		// =====================================================================
	public:

		/// Get the streamID of this stream
		virtual int getStreamID() const = 0;

		///
		virtual int getBatchCount() const = 0;

		///
		virtual int getBatchParity() const = 0;

		///
		virtual int getMaximumBatchSize() const = 0;

		///
		virtual void decryptBatch() = 0;

		///
		virtual void setBatchData(unsigned char *ptr, int len, int parity,
			unsigned char *originalPtr) = 0;

		///
		virtual const dvbcsa_bs_key_s *getKey(int parity) const = 0;

		///
		virtual void setKey(const unsigned char *cw, int parity, int index) = 0;

		///
		virtual void startOSCamFilterData(int pid, int demux, int filter,
				   const unsigned char *filterData, const unsigned char *filterMask) = 0;

		///
		virtual void stopOSCamFilterData(int pid, int demux, int filter) = 0;

		///
		virtual bool findOSCamFilterData(int streamID, int pid,
			const unsigned char *tsPacket, int &tableID,
			int &filter, int &demux, mpegts::TSData &filterData) = 0;

		///
		virtual void stopOSCamFilters(int streamID) = 0;

		///
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
			int hops) = 0;

		///
		virtual bool isMarkedAsActivePMT(int pid) const = 0;

		///
		virtual mpegts::SpPMT getPMTData() const = 0;
};

} // namespace dvb
} // namespace input

#endif // INPUT_DVB_FRONTENDDECRYPTINTERFACE_H_INCLUDE
