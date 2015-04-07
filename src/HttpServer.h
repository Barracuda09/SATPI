/* HttpServer.h

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
#ifndef HTTP_SERVER_H_INCLUDE
#define HTTP_SERVER_H_INCLUDE

#include "TcpSocket.h"
#include "ThreadBase.h"

// forward declaration
class InterfaceAttr;
class Properties;
class Streams;

/// HTTP Server serves the XML and other related web pages
class HttpServer :
		public ThreadBase,
		public TcpSocket {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		HttpServer(const InterfaceAttr &interface,
		           Streams &streams,
		           Properties &properties);
		virtual ~HttpServer();

	protected:
		/// Thread function
		virtual void threadEntry();

		/// Method for getting the required files
		bool getMethod(const SocketClient &client);

		/// Method for getting the required files
		bool postMethod(const SocketClient &client);
		
		/// Process the @c SocketClient when there is data received
		virtual bool process(SocketClient &client);

		/// Is called when the connection is closed by the client and should
		/// take appropriate action
		virtual bool closeConnection(SocketClient &/*client*/) { return true; }

		///
		void make_data_xml(std::string &xml);

		///
		void make_config_xml(std::string &xml);

		///
		void make_streams_xml(std::string &xml);
	private:
		// =======================================================================
		// Data members
		// =======================================================================
		/// @c InterfaceAttr has all network items
		const InterfaceAttr &_interface;
		Streams             &_streams;
		Properties          &_properties;

}; // class HttpServer

#endif // HTTP_SERVER_H_INCLUDE