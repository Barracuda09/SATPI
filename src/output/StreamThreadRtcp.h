/* StreamThreadRtcp.h

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
#ifndef OUTPUT_STREAMTHREADRTCP_H_INCLUDE
#define OUTPUT_STREAMTHREADRTCP_H_INCLUDE OUTPUT_STREAMTHREADRTCP_H_INCLUDE

#include <FwDecl.h>
#include <output/StreamThreadRtcpBase.h>

FW_DECL_NS0(StreamInterface);

namespace output {

/// RTCP Server over UDP
class StreamThreadRtcp :
	public StreamThreadRtcpBase {
		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:

		StreamThreadRtcp(StreamInterface &stream);

		virtual ~StreamThreadRtcp();

		// =====================================================================
		//  -- output::StreamThreadRtcpBase ------------------------------------
		// =====================================================================
	protected:

		/// @see StreamThreadRtcpBase
		virtual int getStreamSocketPort(int clientID) const override;

	private:

		/// @see StreamThreadRtcpBase
		virtual void doStartStreaming(int clientID) override;

		/// @see StreamThreadRtcpBase
		virtual void doSendDataToClient(int clientID,
			uint8_t *sr,
			std::size_t srlen,
			uint8_t *sdes,
			std::size_t sdeslen,
			uint8_t *app,
			std::size_t applen) override;

};

}

#endif // OUTPUT_STREAMTHREADRTCP_H_INCLUDE
