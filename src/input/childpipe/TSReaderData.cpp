/* TSReaderData.cpp

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
#include <input/childpipe/TSReaderData.h>

#include <Log.h>
#include <Unused.h>
#include <StringConverter.h>
#include <TransportParamVector.h>

namespace input::childpipe {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

TSReaderData::TSReaderData() {
	doInitialize();
}

// =============================================================================
// -- input::DeviceData --------------------------------------------------------
// =============================================================================

void TSReaderData::doNextAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "pathname", base::XMLSupport::makeXMLString(_filePath));
}

void TSReaderData::doNextFromXML(const std::string &UNUSED(xml)) {}

void TSReaderData::doInitialize() {
	_filePath = "None";
	_pcrTimer = 0;
	_genPSI = false;
}

void TSReaderData::doParseStreamString(const FeID id, const TransportParamVector& params) {
/*
	const std::string genPSI = params.getParameter("genPSI");
	if (genPSI == "yes") {
		initialize();
		parseAndUpdatePidsTable(id, params);
		_frequencyChanged = true;
		_genPSI = true;
		_pcrTimer = 100;
		return;
	}
*/
	const std::string filePath = params.getURIParameter("exec");
	// Check did we receive an new path or just the same again
	if (filePath.empty() || (hasFilePath() && filePath == _filePath)) {
		parseAndUpdatePidsTable(id, params);
		return;
	}
	initialize();
	_frequencyChanged = true;
	_filePath = StringConverter::getPercentDecoding(filePath);
	// when 'pcrtimer=' is not set or zero the PCR from the stream is used, else this timer
	// will be used as read interval.
	const int pcrTimer = params.getIntParameter("pcrtimer");
	if (pcrTimer != -1) {
		_pcrTimer = pcrTimer;
	}
	parseAndUpdatePidsTable(id, params);
}

std::string TSReaderData::doAttributeDescribeString(const FeID id) const {
	// ver=1.5;tuner=<feID>,<level>,<lock>,<quality>;exec=<file>
	return StringConverter::stringFormat("ver=1.5;tuner=@#1,@#2,@#3,@#4;exec=@#5",
		id, getSignalStrength(), hasLock(),
		getSignalToNoiseRatio(), _filePath);
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

std::string TSReaderData::getFilePath() const {
	base::MutexLock lock(_mutex);
	return _filePath;
}

bool TSReaderData::hasFilePath() const {
	base::MutexLock lock(_mutex);
	return _filePath != "None";
}

int TSReaderData::getPCRTimer() const {
//	base::MutexLock lock(_mutex);
	return _pcrTimer;
}

}
