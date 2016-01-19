/* RtspServer.h

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
#ifndef RTSP_SERVER_H_INCLUDE
#define RTSP_SERVER_H_INCLUDE RTSP_SERVER_H_INCLUDE

#include "HttpcServer.h"
#include "Properties.h"
#include "ThreadBase.h"

// Forward declarations
class Stream;
class Streams;
class StreamClient;

/// RTSP Server
class RtspServer :
		public ThreadBase,
		public HttpcServer {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		RtspServer(Streams &streams, const Properties &properties, const InterfaceAttr &interface);

		virtual ~RtspServer();

	protected:
		/// Thread function
		virtual void threadEntry();

	private:

		///
		virtual bool methodSetup(Stream &stream, int clientID, std::string &htmlBody);

		///
		virtual bool methodPlay(Stream &stream, int clientID, std::string &htmlBody);

		///
		virtual bool methodOptions(Stream &stream, int clientID, std::string &htmlBody);

		///
		virtual bool methodDescribe(Stream &stream, int clientID, std::string &htmlBody);

		///
		virtual bool methodTeardown(Stream &stream, int clientID, std::string &htmlBody);


		// =======================================================================
		// Data members
		// =======================================================================

}; // class RtspServer

#endif // RTSP_SERVER_H_INCLUDE
