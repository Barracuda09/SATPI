/* HeaderVector.cpp

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
#include <HeaderVector.h>

#include <StringConverter.h>

// =============================================================================
// -- Other member functions ---------------------------------------------------
// =============================================================================

std::string HeaderVector::getFieldParameter(const std::string_view reqHeader) const {
	for (const std::string& header : _vector) {
		const auto b = header.find(reqHeader, 0);
		const auto e = header.find_first_not_of(reqHeader, b);
		if (b != std::string::npos && reqHeader.size() == e && header[e] == ':') {
			return StringConverter::trimWhitespace(header.substr(e + 1));
		}
	}
	return std::string();
}

std::string HeaderVector::getStringFieldParameter(std::string_view header,
		std::string_view parameter) const {
	const std::string field = getFieldParameter(header);
	if (field.empty()) {
		return std::string();
	}
	StringVector params = StringConverter::split(field, ";\r\n");
	for (const std::string& param : params) {
		const auto p = param.find(parameter, 0);
		if (p != std::string::npos) {
			StringVector r = StringConverter::split(param, "=");
			return r.size() == 2 ? StringConverter::trimWhitespace(r[1]) : std::string();
		}
	}
	return std::string();
}

int HeaderVector::getIntFieldParameter(std::string_view header,
		std::string_view parameter) const {
	const std::string val = getStringFieldParameter(header, parameter);
	if (val.empty()) {
		return -1;
	}
	return std::isdigit(val[0]) ? std::stoi(val) : -1;
}
