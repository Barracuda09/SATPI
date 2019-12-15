/* HttpServer.h

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
#ifndef HTTP_SERVER_H_INCLUDE
#define HTTP_SERVER_H_INCLUDE HTTP_SERVER_H_INCLUDE

#include <FwDecl.h>
#include <base/ThreadBase.h>
#include <HttpcServer.h>

FW_DECL_NS0(Properties);
FW_DECL_NS0(StreamManager);
FW_DECL_NS1(base, XMLSupport);

/// HTTP Server
class HttpServer :
	public base::ThreadBase,
	public HttpcServer {
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
	public:

		HttpServer(
			base::XMLSupport &xml,
			StreamManager &streamManager,
			const std::string &bindIPAddress,
			Properties &properties);

		virtual ~HttpServer();

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================
	public:

		/// Call this to initialize, setup and start this server
		virtual void initialize(
			int port,
			bool nonblock);

	protected:

		/// Thread function
		virtual void threadEntry() final;

		/// Method for getting the required files
		virtual bool methodGet(SocketClient &client) final;

		/// Method for getting the required files
		virtual bool methodPost(SocketClient &client) final;

		///
		std::size_t readFile(const char *filePath, std::string &data) const;

		// =======================================================================
		// Data members
		// =======================================================================
	private:

		Properties &_properties;
		base::XMLSupport &_xml;

};

#endif // HTTP_SERVER_H_INCLUDE
