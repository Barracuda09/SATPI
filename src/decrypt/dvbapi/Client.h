/* Client.h

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef DECRYPT_DVBAPI_CLIENT_H_INCLUDE
#define DECRYPT_DVBAPI_CLIENT_H_INCLUDE DECRYPT_DVBAPI_CLIENT_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <base/ThreadBase.h>
#include <base/XMLSupport.h>
#include <socket/SocketClient.h>

#include <atomic>
#include <string>
#include <map>

FW_DECL_NS0(StreamManager);
FW_DECL_NS1(mpegts, PacketBuffer);
FW_DECL_NS1(mpegts, PMT);
FW_DECL_NS1(mpegts, SDT);

FW_DECL_SP_NS2(decrypt, dvbapi, Client);

namespace decrypt::dvbapi {

/*
ca_pmt () {
	ca_pmt_tag                                                      24 uimsbf
	length_field()
	ca_pmt_list_management                                           8 uimsbf
	program_number                                                  16 uimsbf
	reserved                                                         2 bslbf
	version_number                                                   5 uimsbf
	current_next_indicator                                           1 bslbf
	reserved                                                         4 bslbf
	program_info_length                                             12 uimsbf
	if (program_info_length != 0) {
		ca_pmt_cmd_id // at program level                            8 uimsbf
		for (i=0; i<n; i++) {
			CA_descriptor() // CA descriptor at programme level
		}
	}
	for (i=0; i<n; i++) {
		stream_type                                                  8 uimsbf
		reserved                                                     3 bslbf
		elementary_PID // elementary stream PID                     13 uimsbf
		reserved                                                     4 bslbf
		ES_info_length                                              12 uimsbf
		if (ES_info_length != 0) {
			ca_pmt_cmd_id //at ES level                              8 uimsbf
			for (i=0; i<n; i++) {
				CA_descriptor() // CA descriptor at elementary stream level
			}
		}
	}
}
 */
/// The class @c Client is for decrypting streams
class Client :
	public base::ThreadBase,
	public base::XMLSupport {
		// ================================================================
		// -- Constructors and destructor ---------------------------------
		// ================================================================
	public:

		explicit Client(StreamManager &streamManager);

		virtual ~Client();

		// ================================================================
		//  -- base::ThreadBase -------------------------------------------
		// ================================================================
	protected:

		virtual void threadEntry() final;

		// ================================================================
		//  -- base::XMLSupport -------------------------------------------
		// ================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// ================================================================
		//  -- Other member functions -------------------------------------
		// ================================================================
	public:

		///
		void decrypt(FeIndex index, FeID id, mpegts::PacketBuffer &buffer);

		///
		bool stopDecrypt(FeIndex index, FeID id);

	private:

		///
		bool initClientSocket(
			SocketClient &client,
			const std::string &ipAddr,
			int port);

		///
		void sendClientInfo();

		///
		void sendPMT(FeIndex index, FeID id, const mpegts::SDT &sdt, const mpegts::PMT &pmt);

		///
		void cleanPMT(unsigned char *data);

		// =================================================================
		// -- Data members -------------------------------------------------
		// =================================================================
	private:

		using UCharPtr = std::shared_ptr<unsigned char[]>;

		struct PMTEntry {
			UCharPtr caPtr;
			int size;
		};

		SocketClient     _client;
		std::atomic_bool _connected;
		std::atomic_bool _enabled;
		std::atomic_bool _rewritePMT;
		std::atomic<int> _serverPort;
		std::atomic<int> _adapterOffset;
		std::string      _serverIPAddr;
		std::string      _serverName;
		std::map<int, PMTEntry> _capmtMap;

		StreamManager &_streamManager;
};

}

#endif // DECRYPT_DVBAPI_CLIENT_H_INCLUDE
