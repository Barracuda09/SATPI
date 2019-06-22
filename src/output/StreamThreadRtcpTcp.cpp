/* StreamThreadRtcpTcp.cpp

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
#include <output/StreamThreadRtcpTcp.h>

#include <StreamClient.h>
#include <Stream.h>
#include <Log.h>
#include <Utils.h>
#include <Unused.h>

#include <chrono>
#include <cstring>
#include <thread>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

namespace output {

StreamThreadRtcpTcp::StreamThreadRtcpTcp(StreamInterface &stream) :
		StreamThreadRtcpBase(stream) {}

StreamThreadRtcpTcp::~StreamThreadRtcpTcp() {
	_thread.terminateThread();
	const StreamClient &client = _stream.getStreamClient(_clientID);
	SI_LOG_INFO("Stream: %d, Destroy RTCP/TCP stream to %s:%d", _stream.getStreamID(),
		client.getIPAddressOfStream().c_str(), client.getHttpSocketPort());
}

bool StreamThreadRtcpTcp::startStreaming() {
	const StreamClient &client = _stream.getStreamClient(_clientID);
	if (!_thread.startThread()) {
		SI_LOG_ERROR("Stream: %d, Start RTCP/TCP stream  to %s:%d ERROR", _stream.getStreamID(),
			client.getIPAddressOfStream().c_str(), client.getHttpSocketPort());
		return false;
	}
	SI_LOG_INFO("Stream: %d, Start RTCP/TCP stream to %s:%d", _stream.getStreamID(),
		client.getIPAddressOfStream().c_str(), client.getHttpSocketPort());

	_mon_update = 0;
	return true;
}

bool StreamThreadRtcpTcp::pauseStreaming(int UNUSED(clientID)) {
	_thread.pauseThread();

	const StreamClient &client = _stream.getStreamClient(_clientID);
	SI_LOG_INFO("Stream: %d, Pause RTCP/TCP stream to %s:%d", _stream.getStreamID(),
			client.getIPAddressOfStream().c_str(), client.getHttpSocketPort());
	return true;
}

bool StreamThreadRtcpTcp::restartStreaming(int UNUSED(clientID)) {
	_thread.restartThread();

	const StreamClient &client = _stream.getStreamClient(_clientID);
	SI_LOG_INFO("Stream: %d, Restart RTCP/TCP stream to %s:%d", _stream.getStreamID(),
			client.getIPAddressOfStream().c_str(), client.getHttpSocketPort());
	return true;
}

bool StreamThreadRtcpTcp::threadExecuteFunction() {
	StreamClient &client = _stream.getStreamClient(_clientID);

	// check do we need to update Device monitor signals
	if (_mon_update == 0) {
		_stream.getInputDevice()->monitorSignal(false);

		_mon_update = _stream.getRtcpSignalUpdateFrequency();
	} else {
		--_mon_update;
	}

	// RTCP compound packets must start with a SR, SDES then APP
	std::size_t srlen   = 0;
	std::size_t sdeslen = 0;
	std::size_t applen  = 0;
	uint8_t *sr   = get_sr_packet(&srlen);
	uint8_t *sdes = get_sdes_packet(&sdeslen);
	uint8_t *app  = get_app_packet(&applen);

	if (sr && sdes && app) {
		const std::size_t len = srlen + sdeslen + applen;
		unsigned char header[4];
		header[0] = 0x24;
		header[1] = 0x01;
		header[2] = (len >> 8) & 0xFF;
		header[3] = (len >> 0) & 0xFF;

		iovec iov[4];
		iov[0].iov_base = header;
		iov[0].iov_len = 4;
		iov[1].iov_base = sr;
		iov[1].iov_len = srlen;
		iov[2].iov_base = sdes;
		iov[2].iov_len = sdeslen;
		iov[3].iov_base = app;
		iov[3].iov_len = applen;

		// send the RTCP/TCP packet
		if (!client.writeHttpData(iov, 4)) {
			SI_LOG_ERROR("Stream: %d, Error sending RTCP/TCP Stream Data to %s", _stream.getStreamID(),
				client.getIPAddressOfStream().c_str());
		}
	}
	DELETE_ARRAY(sr);
	DELETE_ARRAY(sdes);
	DELETE_ARRAY(app);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	return true;
}

}
