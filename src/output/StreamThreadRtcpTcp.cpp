/* StreamThreadRtcpTcp.cpp

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
#include <output/StreamThreadRtcpTcp.h>

#include <StreamClient.h>
#include <Stream.h>
#include <Log.h>

#include <sys/uio.h>

namespace output {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

StreamThreadRtcpTcp::StreamThreadRtcpTcp(StreamInterface &stream) :
		StreamThreadRtcpBase("RTCP/TCP", stream) {}

StreamThreadRtcpTcp::~StreamThreadRtcpTcp() {
	_thread.terminateThread();
	const FeID id = _stream.getFeID();
	const StreamClient &client = _stream.getStreamClient(_clientID);
	SI_LOG_INFO("Frontend: @#1, Destroy @#2 stream to @#3:@#4", id,
		_protocol, client.getIPAddressOfStream(), client.getHttpSocketPort());
}

// =============================================================================
//  -- output::StreamThreadRtcpBase --------------------------------------------
// =============================================================================

int StreamThreadRtcpTcp::getStreamSocketPort(const int clientID) const {
	return  _stream.getStreamClient(clientID).getHttpSocketPort();
}

void StreamThreadRtcpTcp::doSendDataToClient(const int clientID,
	const PacketPtr& sr, const int srlen,
	const PacketPtr& sdes, const int sdeslen,
	const PacketPtr& app, const int applen) {
	StreamClient &client = _stream.getStreamClient(clientID);

	const int len = srlen + sdeslen + applen;

	unsigned char header[4];
	header[0] = 0x24;
	header[1] = 0x01;
	header[2] = (len >> 8) & 0xFF;
	header[3] = (len >> 0) & 0xFF;

	iovec iov[4];
	iov[0].iov_base = header;
	iov[0].iov_len = 4;
	iov[1].iov_base = sr.get();
	iov[1].iov_len = srlen;
	iov[2].iov_base = sdes.get();
	iov[2].iov_len = sdeslen;
	iov[3].iov_base = app.get();
	iov[3].iov_len = applen;

	// send the RTCP/TCP packet
	if (!client.writeHttpData(iov, 4)) {
		SI_LOG_ERROR("Frontend: @#1, Error sending @#2 Stream Data to @#3",
			_stream.getFeID(), _protocol, client.getIPAddressOfStream());
	}
}

}
