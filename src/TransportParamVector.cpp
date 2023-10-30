/* TransportParamVector.cpp

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
#include <TransportParamVector.h>

#include <Log.h>
#include <StringConverter.h>

// =============================================================================
// -- Other member functions ---------------------------------------------------
// =============================================================================

std::string TransportParamVector::getParameter(const std::string_view parameter) const {
	for (const std::string& param : _vector) {
		const auto b = param.find(parameter, 0);
		const auto e = param.find_first_not_of(parameter, b);
		if (b != std::string::npos && parameter.size() == e && param[e] == '=') {
			return StringConverter::trimWhitespace(param.substr(e + 1));
		}
	}
	return std::string();
}

void TransportParamVector::replaceParameter(
		const std::string_view parameter,
		const std::string_view value) {
	for (std::string& param : _vector) {
		const auto b = param.find(parameter, 0);
		const auto e = param.find_first_not_of(parameter, b);
		if (b != std::string::npos && parameter.size() == e && param[e] == '=') {
			// Found parameter, so change and return
			param.erase(e + 1);
			param += value;
			return;
		}
	}
	// Not parameter found, so add it
    std::string p = std::string(parameter) + "=" + std::string(value);
    _vector.emplace_back(p);
}

double TransportParamVector::getDoubleParameter(const std::string_view parameter) const {
	const std::string val = getParameter(parameter);
	if (val.empty()) {
		return -1.0;
	}
	return std::isdigit(val[0]) ? std::stof(val) : -1.0;
}

int TransportParamVector::getIntParameter(std::string_view parameter) const {
	const std::string val = getParameter(parameter);
	if (val.empty()) {
		return -1.0;
	}
	return std::isdigit(val[0]) ? std::stoi(val) : -1;
}

input::InputSystem TransportParamVector::getMSYSParameter() const {
	const std::string val = getParameter("msys");
	if (!val.empty()) {
		if (val == "dvbs2" || val == "dvbs2x") {
			return input::InputSystem::DVBS2;
		} else if (val == "dvbs") {
			return input::InputSystem::DVBS;
		} else if (val == "dvbt") {
			return input::InputSystem::DVBT;
		} else if (val == "dvbt2") {
			return input::InputSystem::DVBT2;
		} else if (val == "dvbc") {
			return input::InputSystem::DVBC;
		} else if (val == "file") {
			return input::InputSystem::FILE_SRC;
		} else if (val == "streamer") {
			return input::InputSystem::STREAMER;
		} else if (val == "childpipe") {
			return input::InputSystem::CHILDPIPE;
		}
	}
	return input::InputSystem::UNDEFINED;
}

std::string TransportParamVector::getURIParameter(const std::string_view parameter) const {
	const std::string line = getParameter(parameter);
	if (!line.empty()) {
		auto begin = line.find("\"", 0);
		if (begin != std::string::npos) {
			begin += 1;
			auto  end = line.find("\"", begin);
			if (end != std::string::npos) {
				return line.substr(begin, end - begin);
			}
		}
	}
	return std::string();
}
