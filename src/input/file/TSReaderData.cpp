/* TSReaderData.cpp

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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

namespace input {
namespace file {

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================

	TSReaderData::TSReaderData() :
		_filePath("None") {
		initialize();
	}

	TSReaderData::~TSReaderData() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void TSReaderData::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		ADD_CONFIG_TEXT(xml, "pathname", _filePath.c_str());
	}

	void TSReaderData::fromXML(const std::string &UNUSED(xml)) {}

	// =======================================================================
	// -- input::DeviceData --------------------------------------------------
	// =======================================================================

	void TSReaderData::initialize() {
		base::MutexLock lock(_mutex);

		DeviceData::initialize();
	}

	void TSReaderData::parseStreamString(
			const int UNUSED(streamID),
			const std::string &msg,
			const std::string &method) {
		base::MutexLock lock(_mutex);
		std::string file;
		if (StringConverter::getStringParameter(msg, method, "uri=", file) == true) {
			initialize();
			_filePath = file;
			_changed = true;
		}
	}

	std::string TSReaderData::attributeDescribeString(const int streamID) const {
		base::MutexLock lock(_mutex);
		std::string desc;
		// ver=1.5;tuner=<feID>,<level>,<lock>,<quality>;file=<file>
		StringConverter::addFormattedString(desc, "ver=1.5;tuner=%d,%d,%d,%d;file=%s",
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

	void TSReaderData::clearData() {
		base::MutexLock lock(_mutex);
		_filePath = "None";
		setMonitorData(static_cast<fe_status_t>(0), 0, 0, 0, 0);
	}

} // namespace file
} // namespace input
