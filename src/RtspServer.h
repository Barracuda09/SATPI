/* RtspServer.h

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
#ifndef RTSP_SERVER_H_INCLUDE
#define RTSP_SERVER_H_INCLUDE RTSP_SERVER_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <HttpcServer.h>
#include <base/ThreadBase.h>

FW_DECL_NS0(SocketClient);
FW_DECL_NS0(Stream);
FW_DECL_NS0(StreamManager);

/// RTSP Server
class RtspServer :
	public base::ThreadBase,
	public HttpcServer {
		// =========================================================================
		// -- Constructors and destructor ------------------------------------------
		// =========================================================================
	public:

		RtspServer(StreamManager& streamManager, const std::string& bindIPAddress);

		virtual ~RtspServer();

		/// Call this to initialize, setup and start this server
		virtual void initialize(int port, bool nonblock);

	protected:
		/// Thread function
		virtual void threadEntry() final;

	private:

		///
		virtual void methodDescribe(const std::string& sessionID, int cseq, FeIndex index, std::string& htmlBody) final;

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================

};

#endif // RTSP_SERVER_H_INCLUDE
