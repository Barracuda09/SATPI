/* TSReaderData.cpp

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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

namespace input {
namespace childpipe {

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================

	TSReaderData::TSReaderData() {
		doInitialize();
	}

	TSReaderData::~TSReaderData() {}

	// =======================================================================
	// -- input::DeviceData --------------------------------------------------
	// =======================================================================

	void TSReaderData::doNextAddToXML(std::string &xml) const {
		ADD_XML_ELEMENT(xml, "pathname", base::XMLSupport::makeXMLString(_filePath));
	}

	void TSReaderData::doNextFromXML(const std::string &UNUSED(xml)) {}

	void TSReaderData::doInitialize() {
		_filePath = "None";
	}

	void TSReaderData::doParseStreamString(
			const int UNUSED(streamID),
			const std::string &msg,
			const std::string &method) {
		const std::string filePath = StringConverter::getURIParameter(msg, method, "exec=");
		if (filePath.empty() || (hasFilePath() && filePath == _filePath)) {
			return;
		}
		initialize();
		_changed = true;
		_filePath = filePath;
	}

	std::string TSReaderData::doAttributeDescribeString(const int streamID) const {
		std::string desc;
		// ver=1.5;tuner=<feID>,<level>,<lock>,<quality>;exec=<file>
		StringConverter::addFormattedString(desc, "ver=1.5;tuner=%d,%d,%d,%d;exec=%s",
				streamID + 1,
				getSignalStrength(),
				hasLock(),
				getSignalToNoiseRatio(),
				_filePath.c_str());
		return desc;
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	std::string TSReaderData::getFilePath() const {
		base::MutexLock lock(_mutex);
		return _filePath;
	}

	bool TSReaderData::hasFilePath() const {
		base::MutexLock lock(_mutex);
		return _filePath != "None";
	}

} // namespace childpipe
} // namespace input
