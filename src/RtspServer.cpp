/* RtspServer.cpp

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
#include <RtspServer.h>

#include <Log.h>
#include <Stream.h>
#include <StreamManager.h>
#include <socket/SocketClient.h>
#include <output/StreamClient.h>
#include <StringConverter.h>

extern const char* const satpi_version;

RtspServer::RtspServer(StreamManager& streamManager, const std::string& bindIPAddress) :
		ThreadBase("RtspServer"),
		HttpcServer(20, "RTSP", streamManager, bindIPAddress) {}

RtspServer::~RtspServer() {
	cancelThread();
	joinThread();
}

void RtspServer::initialize(
		int port,
		bool nonblock) {
	HttpcServer::initialize(port, nonblock);
	startThread();
}

void RtspServer::threadEntry() {
	SI_LOG_INFO("Setting up RTSP server");

	for (;;) {
		// call poll with a timeout of 500 ms
		poll(500);

		_streamManager.checkForSessionTimeout();
	}
}

void RtspServer::methodDescribe(
		const std::string& sessionID, const int cseq, const FeIndex feIndex, std::string& htmlBody) {
	static const char* RTSP_DESCRIBE =
		"@#1" \
		"Server: satpi/@#2\r\n" \
		"CSeq: @#3\r\n" \
		"Content-Type: application/sdp\r\n" \
		"Content-Base: rtsp://@#4/\r\n" \
		"Content-Length: @#5\r\n" \
		"@#6" \
		"\r\n" \
		"@#7";

	std::string desc = _streamManager.getSDPSessionLevelString(
			_bindIPAddress, sessionID);
	std::size_t streamsSetup = 0;

	// Lambda Expression 'setupDescribeMediaLevelString'
	const auto setupDescribeMediaLevelString = [&](const FeIndex feIdx) {
		const std::string mediaLevel = _streamManager.getSDPMediaLevelString(feIdx);
		if (mediaLevel.size() > 5) {
			++streamsSetup;
			desc += mediaLevel;
		}
	};
	// Check if there is a specific Frontend ID index requested
	if (feIndex == -1) {
		for (std::size_t i = 0; i < _streamManager.getMaxStreams(); ++i) {
			setupDescribeMediaLevelString(i);
		}
	} else {
		setupDescribeMediaLevelString(feIndex);
	}

	// check if we are in session, then we need to send the Session ID
	const std::string sessionIDHeaderField = (sessionID.size() > 2) ?
		StringConverter::stringFormat("Session: @#1\r\n", sessionID) : "";

	// Are there any streams setup already
	if (streamsSetup != 0) {
		htmlBody = StringConverter::stringFormat(RTSP_DESCRIBE, "RTSP/1.0 200 OK\r\n", satpi_version,
			cseq, _bindIPAddress, desc.size(), sessionIDHeaderField, desc);
	} else {
		htmlBody = StringConverter::stringFormat(RTSP_DESCRIBE, "RTSP/1.0 404 Not Found\r\n", satpi_version,
			cseq, _bindIPAddress, 0, sessionIDHeaderField, "");
	}
}

