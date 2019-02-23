/* StreamThreadRtcp.cpp

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
#include <output/StreamThreadRtcp.h>

#include <StreamClient.h>
#include <Stream.h>
#include <Log.h>
#include <Utils.h>
#include <Unused.h>

#include <chrono>
#include <cstring>

namespace output {

StreamThreadRtcp::StreamThreadRtcp(StreamInterface &stream) :
		StreamThreadRtcpBase(stream) {}

StreamThreadRtcp::~StreamThreadRtcp() {
	_thread.terminateThread();

	SocketAttr &rtcp = _stream.getStreamClient(_clientID).getRtcpSocketAttr();
	SI_LOG_INFO("Stream: %d, Destroy RTCP/UDP stream to %s:%d", _stream.getStreamID(),
		rtcp.getIPAddress().c_str(), rtcp.getSocketPort());
	rtcp.closeFD();
}

bool StreamThreadRtcp::startStreaming() {
	SocketAttr &rtcp = _stream.getStreamClient(_clientID).getRtcpSocketAttr();

	if (!rtcp.setupSocketHandle(SOCK_DGRAM, IPPROTO_UDP)) {
		SI_LOG_ERROR("Stream: %d, Get RTCP handle failed", _stream.getStreamID());
	}

	if (!_thread.startThread()) {
		SI_LOG_ERROR("Stream: %d, Start RTCP/UDP stream to %s:%d ERROR", _stream.getStreamID(),
					 rtcp.getIPAddress().c_str(), rtcp.getSocketPort());
		return false;
	}
	SI_LOG_INFO("Stream: %d, Start RTCP/UDP stream to %s:%d", _stream.getStreamID(),
				rtcp.getIPAddress().c_str(), rtcp.getSocketPort());

	_mon_update = 0;
	return true;
}

bool StreamThreadRtcp::pauseStreaming(int clientID) {
	_thread.pauseThread();

	SocketAttr &rtcp = _stream.getStreamClient(clientID).getRtcpSocketAttr();
	SI_LOG_INFO("Stream: %d, Pause RTCP/UDP stream to %s:%d", _stream.getStreamID(),
			rtcp.getIPAddress().c_str(), rtcp.getSocketPort());
	return true;
}

bool StreamThreadRtcp::restartStreaming(int clientID) {
	_thread.restartThread();

	SocketAttr &rtcp = _stream.getStreamClient(clientID).getRtcpSocketAttr();
	SI_LOG_INFO("Stream: %d, Restart RTCP/UDP stream to %s:%d", _stream.getStreamID(),
			rtcp.getIPAddress().c_str(), rtcp.getSocketPort());
	return true;
}

bool StreamThreadRtcp::threadExecuteFunction() {
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
		uint8_t data[len];

		std::memcpy(data, sr, srlen);
		std::memcpy(data + srlen, sdes, sdeslen);
		std::memcpy(data + srlen + sdeslen, app, applen);

		// send the RTCP/UDP packet
		SocketAttr &rtcp = client.getRtcpSocketAttr();
		if (!rtcp.sendDataTo(data, len, 0)) {
			SI_LOG_ERROR("Stream: %d, Error sending RTCP/UDP data to %s:%d", _stream.getStreamID(),
						 rtcp.getIPAddress().c_str(), rtcp.getSocketPort());
		}
	}
	DELETE_ARRAY(sr);
	DELETE_ARRAY(sdes);
	DELETE_ARRAY(app);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	return true;
}

}
