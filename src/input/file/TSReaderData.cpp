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
#include <input/file/TSReaderData.h>

#include <Log.h>
#include <Unused.h>
#include <StringConverter.h>
#include <TransportParamVector.h>

namespace input::file {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

TSReaderData::TSReaderData() {
	doInitialize();
}

TSReaderData::~TSReaderData() {}

// =============================================================================
// -- input::DeviceData --------------------------------------------------------
// =============================================================================

void TSReaderData::doNextAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "pathname", _filePath);
}

void TSReaderData::doNextFromXML(const std::string &UNUSED(xml)) {}

void TSReaderData::doInitialize() {
	_filePath = "None";
}

void TSReaderData::doParseStreamString(const FeID UNUSED(id), const TransportParamVector& params) {
	const std::string filePath = params.getURIParameter("uri");
	if (filePath.empty() || (hasFilePath() && filePath == _filePath)) {
		return;
	}
	initialize();
	_frequencyChanged = true;
	_filePath = filePath;
}

std::string TSReaderData::doAttributeDescribeString(const FeID id) const {
	std::string desc;
	// ver=1.5;tuner=<feID>,<level>,<lock>,<quality>;uri=<file>
	return StringConverter::stringFormat("ver=1.5;tuner=@#1,@#2,@#3,@#4;uri=@#5",
			id, getSignalStrength(), hasLock(),
			getSignalToNoiseRatio(), _filePath);
	return desc;
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

}
