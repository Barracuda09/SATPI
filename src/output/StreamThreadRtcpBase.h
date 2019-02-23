/* StreamThreadRtcpBase.h

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
#ifndef OUTPUT_STREAMTHREADRTCPBASE_H_INCLUDE
#define OUTPUT_STREAMTHREADRTCPBASE_H_INCLUDE OUTPUT_STREAMTHREADRTCPBASE_H_INCLUDE

#include <FwDecl.h>
#include <base/Thread.h>

#include <cstdint>

FW_DECL_NS0(StreamInterface);

namespace output {

/// The base class for RTCP Server
class StreamThreadRtcpBase {

		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================

	public:

		StreamThreadRtcpBase(StreamInterface &stream);

		virtual ~StreamThreadRtcpBase();

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================

	public:

		/// Start streaming
		/// @return true if stream is started else false on error
		virtual bool startStreaming() = 0;

		/// Pause streaming
		/// @return true if stream is paused else false on error
		virtual bool pauseStreaming(int clientID) = 0;

		/// Restart streaming
		/// @return true if stream is restarted else false on error
		virtual bool restartStreaming(int clientID) = 0;

	protected:

		/// Thread execute function
		virtual bool threadExecuteFunction() = 0;

		///
		uint8_t *get_app_packet(std::size_t *len);
		uint8_t *get_sdes_packet(std::size_t *len);
		uint8_t *get_sr_packet(std::size_t *len);

		// =====================================================================
		//  -- Data members ----------------------------------------------------
		// =====================================================================

	protected:

		int _clientID;
		StreamInterface &_stream;
		int _mon_update;

		base::Thread _thread;
};

}

#endif // OUTPUT_STREAMTHREADRTCPBASE_H_INCLUDE
