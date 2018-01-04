/* Server.h

   Copyright (C) 2014 - 2018 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef UPNP_SSDP_SSDP_SERVER_H_INCLUDE
#define UPNP_SSDP_SSDP_SERVER_H_INCLUDE UPNP_SSDP_SSDP_SERVER_H_INCLUDE

#include <base/ThreadBase.h>
#include <base/XMLSupport.h>
#include <FwDecl.h>
#include <socket/SocketAttr.h>
#include <socket/SocketClient.h>
#include <socket/UdpSocket.h>

FW_DECL_NS0(InterfaceAttr);
FW_DECL_NS0(Properties);

namespace upnp {
namespace ssdp {

	/// SSDP Server
	class Server :
		public base::XMLSupport,
		public base::ThreadBase,
		public UdpSocket {
		public:

			// =======================================================================
			// -- Constructors and destructor ----------------------------------------
			// =======================================================================

			Server(const InterfaceAttr &interface, const Properties &properties);

			virtual ~Server();

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================

		public:
			virtual void addToXML(std::string &xml) const override;

			virtual void fromXML(const std::string &xml) override;

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		protected:

			/// Thread function
			virtual void threadEntry();

		private:

			///
			bool sendByeBye(unsigned int bootId, const char *uuid);

			///
			void incrementBootID();

			///
			void incrementDeviceID();

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:

			const InterfaceAttr &_interface;
			const Properties &_properties;
			SocketClient _udpMultiListen;
			SocketClient _udpMultiSend;
			std::size_t _announceTimeSec;
			std::size_t _bootID;
			std::size_t _deviceID;
	};

} // namespace ssdp
} // namespace upnp

#endif // UPNP_SSDP_SSDP_SERVER_H_INCLUDE
