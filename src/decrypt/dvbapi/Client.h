/* Client.h

   Copyright (C) 2014 - 2019 Marc Postema (mpostema09 -at- gmail.com)

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

#include <FwDecl.h>
#include <base/ThreadBase.h>
#include <base/XMLSupport.h>
#include <socket/SocketClient.h>

#include <atomic>
#include <string>

FW_DECL_NS0(StreamManager);
FW_DECL_NS1(mpegts, PacketBuffer);
FW_DECL_NS1(mpegts, PMT);

FW_DECL_SP_NS2(input, dvb, FrontendDecryptInterface);
FW_DECL_SP_NS2(decrypt, dvbapi, Client);

namespace decrypt {
namespace dvbapi {

/// The class @c Client is for decrypting streams
class Client :
	public base::ThreadBase,
	public base::XMLSupport {
	public:
		// ================================================================
		// -- Constructors and destructor ---------------------------------
		// ================================================================

		explicit Client(StreamManager &streamManager);

		virtual ~Client();

		// ================================================================
		//  -- base::ThreadBase -------------------------------------------
		// ================================================================

	protected:

		virtual void threadEntry() override;

		// ================================================================
		//  -- base::XMLSupport -------------------------------------------
		// ================================================================

	public:

		virtual void addToXML(std::string &xml) const override;

		virtual void fromXML(const std::string &xml) override;

		// ================================================================
		//  -- Other member functions -------------------------------------
		// ================================================================

	public:

		///
		void decrypt(int streamID, mpegts::PacketBuffer &buffer);

		///
		bool stopDecrypt(int streamID);

	private:

		///
		bool initClientSocket(
			SocketClient &client,
			const std::string &ipAddr,
			int port);

		///
		void sendClientInfo();

		///
		void sendPMT(int streamID, const mpegts::PMT &pmt);

		///
		void cleanPMT(unsigned char *data);

		// =================================================================
		// -- Data members -------------------------------------------------
		// =================================================================

	private:

		SocketClient     _client;
		std::atomic_bool _connected;
		std::atomic_bool _enabled;
		std::atomic_bool _rewritePMT;
		std::atomic<int> _serverPort;
		std::atomic<int> _adapterOffset;
		std::string      _serverIPAddr;
		std::string      _serverName;

		StreamManager &_streamManager;
};

} // namespace dvbapi
} // namespace decrypt

#endif // DECRYPT_DVBAPI_CLIENT_H_INCLUDE
