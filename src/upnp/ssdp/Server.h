/* Server.h

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
#ifndef UPNP_SSDP_SSDP_SERVER_H_INCLUDE
#define UPNP_SSDP_SSDP_SERVER_H_INCLUDE UPNP_SSDP_SSDP_SERVER_H_INCLUDE

#include <FwDecl.h>
#include <base/Mutex.h>
#include <base/ThreadBase.h>
#include <base/XMLSupport.h>
#include <socket/SocketClient.h>
#include <socket/UdpSocket.h>

#include <map>
#include <string_view>

FW_DECL_NS0(Properties);

namespace upnp::ssdp {

/// SSDP Server
class Server :
	public base::XMLSupport,
	public base::ThreadBase,
	protected UdpSocket {
		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:

		Server(int ssdpTTL, const std::string &bindIPAddress, const Properties &properties);

		virtual ~Server();

		// =====================================================================
		// -- base::XMLSupport -------------------------------------------------
		// =====================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	protected:

		/// Thread function
		virtual void threadEntry();

	private:

		///
		void checkReply(
			SocketClient& udpMultiListen,
			SocketClient& udpMultiSend,
			struct sockaddr_in& si_other);

		///
		void sendSATIPClientDiscoverResponse(
			SocketClient& udpMultiSend,
			struct sockaddr_in& si_other,
			const std::string& ip_addr,
			const std::string& userAgent);

		///
		void sendRootDeviceDiscoverResponse(
			SocketClient& udpMultiSend,
			struct sockaddr_in& si_other,
			const std::string& ip_addr,
			const std::string& userAgent);

		///
		void sendDiscoverResponse(
			SocketClient& udpMultiSend,
			const std::string& searchTarget,
			struct sockaddr_in& si_other);

		///
		void checkDefendDeviceID(
			unsigned int otherDeviceID,
			std::string_view ip_addr,
			const std::string& server);

		///
		void sendGiveUpDeviceID(
			SocketClient& udpMultiSend,
			struct sockaddr_in& si_other,
			const std::string& ip_addr);

		///
		void sendAnnounce(SocketClient& udpMultiSend);

		///
		bool sendByeBye(SocketClient& udpMultiSend, unsigned int bootId,
			std::string_view uuid);

		///
		void incrementBootID();

		///
		void incrementDeviceID();

		///
		void constructLocation();

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================
	private:
		using ServerMap = std::map<int, std::string>;

		ServerMap _servers;

		base::Mutex _mutex;
		const Properties &_properties;
		std::string _bindIPAddress;
		std::string _xmlDeviceDescriptionFile;
		std::string _location;
		std::size_t _announceTimeSec;
		std::size_t _bootID;
		std::size_t _deviceID;
		int _ttl;
};

}

#endif // UPNP_SSDP_SSDP_SERVER_H_INCLUDE
