/* M3UParser.cpp

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
#include <base/M3UParser.h>

#include <StringConverter.h>

#include <algorithm>

namespace base {

	// =======================================================================
	// -- Constructors and destructor ----------------------------------------
	// =======================================================================
	M3UParser::M3UParser() {}

	M3UParser::~M3UParser() {}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool M3UParser::parse(const std::string &filePath) {
		std::ifstream file;
		file.open(filePath);
		if (!file.is_open()) {
			SI_LOG_ERROR("Error: could not open file: %s", filePath.c_str());
			return false;
		}
		_transformationMap.clear();

		// get first line from file
		std::string lineEXTM3U;
		std::getline(file, lineEXTM3U);
		// first line should be '#EXTM3U'
		if (lineEXTM3U.find("#EXTM3U") == std::string::npos) {
			SI_LOG_ERROR("Error: Seems not a valid M3U file: %s", filePath.c_str());
			return false;
		}
		// seems we are dealing with an M3U file
		for (std::string line; std::getline(file, line); ) {
			if (line.empty()) {
				continue;
			}
			// try to split line into track info an title
			base::StringTokenizer tokenizerInfo(line, ",");
			const std::string trackInfo = tokenizerInfo.getNextToken();
			if (trackInfo.empty()) {
				continue;
			}
			// parse info part
			base::StringTokenizer tokenizerTrackInfo(trackInfo, ": ");
			const std::string tokenEXTINF = tokenizerTrackInfo.getNextToken();
			// first token should be '#EXTINF'
			if (tokenEXTINF.find("#EXTINF") == std::string::npos) {
				continue;
			}
			// now find 'satip-freq'
			const std::string satipFreq("satip-freq=\"");
			for (std::string token; tokenizerTrackInfo.isNextToken(token); ) {
				std::string::size_type begin = token.find(satipFreq);
				if (begin != std::string::npos && begin == 0) {
					begin += satipFreq.size();
					std::string::size_type end = token.find_first_of("\"", begin);
					if (end != std::string::npos && begin != end) {
						const std::string freqStr = token.substr(begin, end - begin);
						const double freq = std::atof(freqStr.c_str());
						// next line should be
						std::getline(file, line);
						if (line.empty()) {
							break;
						}
						if (exist(freq)) {
							SI_LOG_ERROR("Error: freq: %f already exists in file: %s", freq, filePath.c_str());
						} else {
							line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
							_transformationMap[freq] = StringConverter::getPercentDecoding(line);
						}
						// do next one
						break;
					}
				}
			}
		}
		return !_transformationMap.empty();
	}

	std::string M3UParser::findURIFor(double freq) const {
		const auto uriMap = _transformationMap.find(freq);
		if(uriMap != _transformationMap.end()) {
			return uriMap->second;
		}
		return "";
	}

	bool M3UParser::exist(const double freq) const {
		const auto uriMap = _transformationMap.find(freq);
		return uriMap != _transformationMap.end();
	}

} // namespace base
