/* Client.h

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
#ifndef DECRYPT_DVBAPI_CLIENT_H_INCLUDE
#define DECRYPT_DVBAPI_CLIENT_H_INCLUDE DECRYPT_DVBAPI_CLIENT_H_INCLUDE

#include <FwDecl.h>
#include <base/Mutex.h>
#include <base/Functor1Ret.h>
#include <base/ThreadBase.h>
#include <base/XMLSupport.h>
#include <socket/SocketClient.h>

#include <atomic>
#include <string>

FW_DECL_NS1(mpegts, PacketBuffer);
FW_DECL_NS2(input, dvb, FrontendDecryptInterface);

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

			Client(const std::string &xmlFilePath,
				const base::Functor1Ret<input::dvb::FrontendDecryptInterface *, int> getFrontendDecryptInterface);

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
			bool initClientSocket(SocketClient &client, int port,
				const char *ip_addr);

			///
			void sendClientInfo();

			///
			void collectPAT(input::dvb::FrontendDecryptInterface *frontend,
				const unsigned char *data);

			///
			void collectPMT(input::dvb::FrontendDecryptInterface *frontend,
				const unsigned char *data);

			///
			void cleanPMT(input::dvb::FrontendDecryptInterface *frontend,
				unsigned char *data);

			// =================================================================
			// -- Data members -------------------------------------------------
			// =================================================================

		private:

			SocketClient     _client;
			base::Mutex      _mutex;
			std::atomic_bool _connected;
			std::atomic_bool _enabled;
			std::atomic_bool _rewritePMT;
			std::atomic<int> _serverPort;
			std::atomic<int> _adapterOffset;
			std::string      _serverIpAddr;
			std::string      _serverName;

			base::Functor1Ret<input::dvb::FrontendDecryptInterface *, int> _getFrontendDecryptInterface;

	};

} // namespace dvbapi
} // namespace decrypt

#endif // DECRYPT_DVBAPI_CLIENT_H_INCLUDE
