/* HttpServer.h

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#define HTTP_SERVER_H_INCLUDE HTTP_SERVER_H_INCLUDE

#include "HttpcServer.h"
#include "ThreadBase.h"

// forward declaration
class InterfaceAttr;
class Properties;
class Streams;
class DvbapiClient;

/// HTTP Server
class HttpServer :
		public ThreadBase,
		public HttpcServer {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		HttpServer(Streams &streams,
		           const InterfaceAttr &interface,
		           Properties &properties,
		           DvbapiClient *dvbapi);

		virtual ~HttpServer();

	protected:
		/// Thread function
		virtual void threadEntry();

		/// Method for getting the required files
		virtual bool methodGet(SocketClient &client);

		/// Method for getting the required files
		virtual bool methodPost(const SocketClient &client);

		///
		int readFile(const char *file, std::string &data);

		///
		void make_data_xml(std::string &xml);

		///
		std::string  make_config_xml(std::string &xml);

		///
		void make_streams_xml(std::string &xml);

		///
		void save_config_xml(std::string xml);

		///
		void parse_config_xml();

		// =======================================================================
		// Data members
		// =======================================================================
	private:
		Properties          &_properties;
		DvbapiClient        *_dvbapi;

}; // class HttpServer

#endif // HTTP_SERVER_H_INCLUDE
