/* FrontendDecryptInterface.h

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
#ifndef INPUT_DVB_FRONTENDDECRYPTINTERFACE_H_INCLUDE
#define INPUT_DVB_FRONTENDDECRYPTINTERFACE_H_INCLUDE INPUT_DVB_FRONTENDDECRYPTINTERFACE_H_INCLUDE

#include <FwDecl.h>

extern "C" {
	#include <dvbcsa/dvbcsa.h>
}

namespace input {
namespace dvb {

	/// The class @c FrontendDecryptInterface is an interface to an @c Stream for decrypting
	class FrontendDecryptInterface {
		public:
			// =======================================================================
			// -- Constructors and destructor ----------------------------------------
			// =======================================================================
			FrontendDecryptInterface() {}

			virtual ~FrontendDecryptInterface() {}

			// =======================================================================
			// -- Other member functions ---------------------------------------------
			// =======================================================================

			/// Get the streamID of this stream
			virtual int getStreamID() const = 0;

			/// Update the input device
			virtual bool updateInputDevice() = 0;

			///
			virtual int getBatchCount() const = 0;

			///
			virtual int getBatchParity() const = 0;

			///
			virtual int getMaximumBatchSize() const = 0;

			///
			virtual void decryptBatch(bool final) = 0;

			///
			virtual void setBatchData(unsigned char *ptr, int len, int parity, unsigned char *originalPtr) = 0;

			///
			virtual const dvbcsa_bs_key_s *getKey(int parity) const = 0;

			///
			virtual bool isTableCollected(int tableID) const = 0;

			///
			virtual void setTableCollected(int tableID, bool collected) = 0;


			///
			virtual const unsigned char *getTableData(int tableID) const = 0;

			///
			virtual void collectTableData(int streamID, int tableID, const unsigned char *data) = 0;

			///
			virtual int getTableDataSize(int tableID) const = 0;

			///
			virtual void setPMT(int pid, bool set) = 0;

			///
			virtual bool isPMT(int pid) const = 0;

			///
			virtual void setECMFilterData(int demux, int filter, int pid, bool set) = 0;

			///
			virtual void getECMFilterData(int &demux, int &filter, int pid) const = 0;

			///
			virtual bool getActiveECMFilterData(int &demux, int &filter, int &pid) const = 0;

			///
			virtual bool isECM(int pid) const = 0;

			///
			virtual void setKey(const unsigned char *cw, int parity, int index) = 0;

			///
			virtual void freeKeys() = 0;

			///
			virtual void setKeyParity(int pid, int parity) = 0;

			///
			virtual int getKeyParity(int pid) const = 0;

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

	};

} // namespace dvb
} // namespace input

#endif // INPUT_DVB_FRONTENDDECRYPTINTERFACE_H_INCLUDE
