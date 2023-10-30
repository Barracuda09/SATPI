/* StreamerData.cpp

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
#include <input/stream/StreamerData.h>

#include <Log.h>
#include <Unused.h>
#include <StringConverter.h>
#include <TransportParamVector.h>

namespace input::stream {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

StreamerData::StreamerData() {
	doInitialize();
}

StreamerData::~StreamerData() {}

// =============================================================================
// -- input::DeviceData --------------------------------------------------------
// =============================================================================

void StreamerData::doNextAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "pathname", _uri);
}

void StreamerData::doNextFromXML(const std::string &UNUSED(xml)) {}

void StreamerData::doInitialize() {
	_uri = "None";
	_multiAddr = "0.0.0.0";
	_port = 0;
	_udp = false;
}

void StreamerData::doParseStreamString(const FeID id, const TransportParamVector& params) {
	const std::string uri = params.getURIParameter("uri");
	if (uri.empty() || (hasFilePath() && uri == _uri)) {
		parseAndUpdatePidsTable(id, params);
		return;
	}
	initialize();
	_frequencyChanged = true;
	_uri = uri;

	// Parse uri ex. udp@224.0.1.3:1234
	_udp = _uri.find("udp") != std::string::npos;
	std::string::size_type begin = _uri.find("@");
	if (begin != std::string::npos) {
		std::string::size_type end = _uri.find_first_of(":", begin);
		if (end != std::string::npos) {
			begin += 1;
			_multiAddr = _uri.substr(begin, end - begin);
			begin = end + 1;
			end = _uri.size();
			_port = std::stoi(_uri.substr(begin, end - begin));
		}
	}
	parseAndUpdatePidsTable(id, params);
}

std::string StreamerData::doAttributeDescribeString(const FeID id) const {
	std::string desc;
	// ver=1.5;tuner=<feID>,<level>,<lock>,<quality>;uri=<file>
	return StringConverter::stringFormat("ver=1.5;tuner=@#1,@#2,@#3,@#4;uri=@#5",
			id, getSignalStrength(), hasLock(),
			getSignalToNoiseRatio(), _uri);
	return desc;
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

std::string StreamerData::getMultiAddr() const {
	base::MutexLock lock(_mutex);
	return _multiAddr;
}

int StreamerData::getPort() const {
	base::MutexLock lock(_mutex);
	return _port;
}

bool StreamerData::hasFilePath() const {
	base::MutexLock lock(_mutex);
	return _uri != "None";
}

}
