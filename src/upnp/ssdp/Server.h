/* Server.h

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
#ifndef UPNP_SSDP_SSDP_SERVER_H_INCLUDE
#define UPNP_SSDP_SSDP_SERVER_H_INCLUDE UPNP_SSDP_SSDP_SERVER_H_INCLUDE

#include <base/ThreadBase.h>
#include "SocketAttr.h"
#include "SocketClient.h"
#include "UdpSocket.h"

// forward declaration
class InterfaceAttr;
class Properties;

namespace upnp {
namespace ssdp {
	
/// SSDP Server
class Server :
		public base::ThreadBase,
		public UdpSocket {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		Server(const InterfaceAttr &interface,
		           Properties &properties);
		virtual ~Server();

	protected:
		/// Thread function
		virtual void threadEntry();

	private:
		///
		unsigned int readBootIDFromFile(const char *file);

		///
		bool sendByeBye(unsigned int bootId, const char *uuid);

		// =======================================================================
		// Data members
		// =======================================================================
		const InterfaceAttr &_interface;
		Properties          &_properties;
		SocketClient        _udpMultiListen;
		SocketClient        _udpMultiSend;
}; // class Server

} // namespace ssdp
} // namespace upnp

#endif // UPNP_SSDP_SSDP_SERVER_H_INCLUDE