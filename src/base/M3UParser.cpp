/* M3UParser.cpp

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
#include <base/M3UParser.h>

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
		std::string line;
		if (file.is_open()) {
			_transformationMap.clear();
			// first line should be '#EXTM3U'
			if (std::getline(file, line) && line.find("#EXTM3U") != std::string::npos) {
				// seems we are dealing with an M3U file
				while (std::getline(file, line)) {
					if (!line.empty()) {
						// try to split line into track info an title
						base::StringTokenizer tokenizerTrackInfo(line, ",");
						std::string info;
						// parse info part
						if (tokenizerTrackInfo.isNextToken(info) && !info.empty()) {
							base::StringTokenizer tokenizerInfo(info, ": ");
							std::string token;
							// first token should be '#EXTINF'
							if (tokenizerInfo.isNextToken(token) && token.find("#EXTINF") != std::string::npos) {
								// now find 'satip-freq'
								const std::string satipFreq("satip-freq=\"");
								while (tokenizerInfo.isNextToken(token)) {
									std::string::size_type begin = token.find(satipFreq);
									if (begin != std::string::npos && begin == 0) {
										begin += satipFreq.size();
										std::string::size_type end = token.find_first_of("\"", begin);
										if (end != std::string::npos && begin != end) {
											const std::string freqStr = token.substr(begin, end - begin);
											const double freq = std::atof(freqStr.c_str());
											// next line should be
											if (std::getline(file, line) && !line.empty()) {
												if (exist(freq)) {
													SI_LOG_ERROR("Error: freq: %f already exists in file: %s", freq, filePath.c_str());
												} else {
													_transformationMap[freq] = line;
												}
											}
											// do next one
											break;
										}
									}
								}
							}
						}
					}
				}
			}
			file.close();
			return !_transformationMap.empty();
		} else {
			SI_LOG_ERROR("Error: could not open file: %s", filePath.c_str());
			return false;
		}
	}

	bool M3UParser::findURIFor(double freq, std::string &uri) const {
		const auto uriMap = _transformationMap.find(freq);
		if(uriMap != _transformationMap.end()) {
			uri = uriMap->second;
			return true;
		}
		return false;
	}

	bool M3UParser::exist(const double freq) const {
		const auto uriMap = _transformationMap.find(freq);
		return uriMap != _transformationMap.end();
	}

} // namespace base
