/* HttpcMessage.cpp

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#include "HttpcMessage.h"
#include "StringConverter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


HttpcMessage::HttpcMessage() :
		_nextline(0) {;}

HttpcMessage::~HttpcMessage() {;}

bool HttpcMessage::endOfMessage() {
// @TODO check file content length
	return _msg.find("\r\n\r\n", 0) != std::string::npos;
}

bool HttpcMessage::getMethod(std::string &method) const {
	// request line should be in the first line (method)
	_nextline = 0;
	std::string line;

	if (StringConverter::getline(_msg, _nextline, line, "\r\n")) {
		std::string::const_iterator it = line.begin();
		// remove any leading whitespace
		while (*it == ' ') ++it;

		// copy method (upper case)
		while (*it != ' ') {
			method += toupper(*it);
			++it;
		}
	}
	return true;
}

bool HttpcMessage::getHeaderFieldParameter(const char *header_field, std::string &parameter) const {
	return StringConverter::getHeaderFieldParameter(_msg, header_field, parameter);
}

bool HttpcMessage::getRequestedFile(std::string &file) const {
	std::string param;
	if (getHeaderFieldParameter("GET", param)) {
		std::string::size_type begin = param.find_first_of("/");
		std::string::size_type end = param.find_first_of(" ", begin);
		file = param.substr(begin, end - begin);
		return true;
	}
	return false;
}
