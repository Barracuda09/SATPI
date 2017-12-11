/* XMLSaveSupport.cpp

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
#include <base/XMLSaveSupport.h>

#include <StringConverter.h>
#include <Log.h>

#include <stdio.h>

#include <cassert>
#include <iostream>
#include <fstream>

namespace base {

XMLSaveSupport::XMLSaveSupport(const std::string &filePath) : _filePath(filePath) {}

XMLSaveSupport::~XMLSaveSupport() {}

std::string XMLSaveSupport::makeXMLString(const std::string &msg) {
	std::string xml;
	std::string::const_iterator it = msg.begin();
	while (it != msg.end()) {
		if (*it == '&') {
			xml += "&amp;";
		} else if (*it == '"') {
			xml += "&quot;";
		} else if (*it == '>') {
			xml += "&gt;";
		} else if (*it == '<') {
			xml += "&lt;";
		} else {
			xml += *it;
		}
		++it;
	}
	return xml;
}

std::string XMLSaveSupport::getFileName() const {
	std::string file("");
	if (!_filePath.empty()) {
		std::string path;
		StringConverter::splitPath(_filePath, path, file);
	}
	return file;
}

bool XMLSaveSupport::saveXML(const std::string &xml) const {
	if (!_filePath.empty()) {
		std::ofstream file;
		file.open(_filePath);
		if (file.is_open()) {
			file << xml;
			file.close();
			return false;
		}
	}
	return false;
}

bool XMLSaveSupport::restoreXML(std::string &xml) {
	if (!_filePath.empty()) {
		std::ifstream file;
		file.open(_filePath);
		if (file.is_open()) {
			xml = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();
			return true;
		}
	}
	return false;
}

} // namespace base
