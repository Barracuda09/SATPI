/* M3UParser.cpp

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
	//  -- Static functions --------------------------------------------------
	// =======================================================================

	static std::string findAndParseToken(const std::string &str, const std::string &findToken) {
		std::string::size_type begin = str.find(findToken);
		if (begin != std::string::npos && begin == 0) {
			begin += findToken.size();
			std::string::size_type end = str.find_first_of("\"", begin);
			if (end != std::string::npos && begin != end) {
				return str.substr(begin, end - begin);
			}
		}
		return "";
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool M3UParser::parse(const std::string &filePath) {
		std::ifstream file;
		file.open(filePath);
		if (!file.is_open()) {
			SI_LOG_ERROR("Error: could not open file: @#1", filePath);
			return false;
		}
		_transformationMap.clear();

		// get first line from file
		std::string lineEXTM3U;
		std::getline(file, lineEXTM3U);
		// first line should be '#EXTM3U'
		if (lineEXTM3U.find("#EXTM3U") == std::string::npos) {
			SI_LOG_ERROR("Error: Seems not a valid M3U file: @#1", filePath);
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
			TransformationElement element;
			// now find 'satip-freq' and 'satip-src'
			for (std::string token; tokenizerTrackInfo.isNextToken(token); ) {
				const std::string srcStr = findAndParseToken(token, "satip-src=\"");
				if (!srcStr.empty()) {
					element.src = std::atof(srcStr.data());
				}
				const std::string freqStr = findAndParseToken(token, "satip-freq=\"");
				if (!freqStr.empty()) {
					const double freq = std::stof(freqStr);
					if (exist(freq)) {
						SI_LOG_ERROR("Error: freq: @#1 already exists in file: @#2", freq, filePath);
						break;
					}
					element.freq = freq;
				}
			}
			// Now check if we found something we could add
			if (element.freq != -1.0) {
				// next line should be URI
				std::getline(file, line);
				if (!line.empty()) {
					line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
					element.uri = line;
					_transformationMap[element.freq] = element;
				}
			}
		}
		return !_transformationMap.empty();
	}

	M3UParser::TransformationElement M3UParser::findTransformationElementFor(
			double freq) const {
		const auto uriMap = _transformationMap.find(freq);
		if(uriMap != _transformationMap.end()) {
			return uriMap->second;
		}
		TransformationElement tmp;
		return tmp;
	}

	bool M3UParser::exist(const double freq) const {
		const auto uriMap = _transformationMap.find(freq);
		return uriMap != _transformationMap.end();
	}

} // namespace base
