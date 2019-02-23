/* StreamThreadRtcpTcp.h

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
#ifndef OUTPUT_STREAMTHREADRTCP_TCP_H_INCLUDE
#define OUTPUT_STREAMTHREADRTCP_TCP_H_INCLUDE OUTPUT_STREAMTHREADRTCP_TCP_H_INCLUDE

#include <FwDecl.h>
#include <output/StreamThreadRtcpBase.h>

FW_DECL_NS0(StreamInterface);

namespace output {

/// RTCP Server over TCP
class StreamThreadRtcpTcp :
	public StreamThreadRtcpBase {

		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================

	public:

		StreamThreadRtcpTcp(StreamInterface &stream);

		virtual ~StreamThreadRtcpTcp();

		// =====================================================================
		//  -- output::StreamThreadRtcpBase ------------------------------------
		// =====================================================================

	public:

		/// Start streaming
		/// @return true if stream is started else false on error
		virtual bool startStreaming() override;

		/// Pause streaming
		/// @return true if stream is paused else false on error
		virtual bool pauseStreaming(int clientID) override;

		/// Restart streaming
		/// @return true if stream is restarted else false on error
		virtual bool restartStreaming(int clientID) override;

	protected:

		/// Thread execute function
		virtual bool threadExecuteFunction() override;

};

}

#endif // OUTPUT_STREAMTHREADRTCP_TCP_H_INCLUDE
