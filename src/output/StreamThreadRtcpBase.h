/* StreamThreadRtcpBase.h

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef OUTPUT_STREAMTHREADRTCPBASE_H_INCLUDE
#define OUTPUT_STREAMTHREADRTCPBASE_H_INCLUDE OUTPUT_STREAMTHREADRTCPBASE_H_INCLUDE

#include <FwDecl.h>
#include <Unused.h>
#include <base/Thread.h>

#include <cstdint>
#include <memory>

FW_DECL_NS0(StreamInterface);

namespace output {

using PacketPtr = std::unique_ptr<uint8_t[]>;

/// The base class for RTCP Server
class StreamThreadRtcpBase {

		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:

		StreamThreadRtcpBase(
			const std::string &protocol,
			StreamInterface &stream);

		virtual ~StreamThreadRtcpBase();

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	public:

		/// Start streaming
		/// @param clientID specifies which client should start
		/// @return true if stream is started else false on error
		bool startStreaming(int clientID);

		/// Pause streaming
		/// @param clientID specifies which client should pause
		/// @return true if stream is paused else false on error
		bool pauseStreaming(int clientID);

		/// Restart streaming
		/// @param clientID specifies which client should restart
		/// @return true if stream is restarted else false on error
		bool restartStreaming(int clientID);

	protected:

		/// Thread execute function @see base::Thread should @return true to
		/// keep thread running a @return false will stop and then terminate this thread
		bool threadExecuteFunction();

		/// Returns the socket port for the specified client
		/// @param clientID specifies which client the port id requested
		/// @return the socket port for ex. to data send to
		virtual int getStreamSocketPort(int UNUSED(clientID)) const { return 0; }

	private:

		/// Specialization for @see startStreaming
		virtual void doStartStreaming(int UNUSED(clientID)) {}

		/// Specialization for @see pauseStreaming
		virtual void doPauseStreaming(int UNUSED(clientID)) {}

		/// Specialization for @see restartStreaming
		virtual void doRestartStreaming(int UNUSED(clientID)) {}

		/// Specialization for @see threadExecuteFunction to send data to client
		virtual void doSendDataToClient(int UNUSED(clientID),
			const PacketPtr& UNUSED(sr), int UNUSED(srlen),
			const PacketPtr& UNUSED(sdes), int UNUSED(sdeslen),
			const PacketPtr& UNUSED(app), int UNUSED(applen)) {}

		///
		PacketPtr getAPP(int &len);
		PacketPtr getSDES(int &len);
		PacketPtr getSR(int &len);

		// =====================================================================
		//  -- Data members ----------------------------------------------------
		// =====================================================================
	protected:

		int _clientID;
		std::string _protocol;
		StreamInterface &_stream;
		base::Thread _thread;
};

}

#endif // OUTPUT_STREAMTHREADRTCPBASE_H_INCLUDE
