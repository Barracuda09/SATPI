/* SsdpServer.h

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#ifndef SSDP_SERVER_H_INCLUDE
#define SSDP_SERVER_H_INCLUDE

#include "ThreadBase.h"
#include "SocketAttr.h"
#include "SocketClient.h"
#include "UdpSocket.h"

// forward declaration
class InterfaceAttr;
class Properties;

/// RTSP Server
class SsdpServer :
		public ThreadBase,
		public UdpSocket {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		SsdpServer(const InterfaceAttr &interface,
		           Properties &properties);
		virtual ~SsdpServer();

	protected:
		/// Thread function
		virtual void threadEntry();
		
	private:
		///
		unsigned int read_bootID_from_file(const char *file);
		
		///
		bool send_byebye(unsigned int bootId, const char *uuid);

		// =======================================================================
		// Data members
		// =======================================================================
		const InterfaceAttr &_interface;
		Properties &_properties;
		
		SocketClient _udpMultiListen;
		SocketClient _udpMultiSend;
}; // class SsdpServer

#endif // SSDP_SERVER_H_INCLUDE