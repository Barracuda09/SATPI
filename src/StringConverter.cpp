/* StringConverter.cpp

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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
#include <StringConverter.h>

#include <Log.h>
#include <input/dvb/dvbfix.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void StringConverter::splitPath(const std::string &fullPath, std::string &path, std::string &file) {
	std::string::size_type end = fullPath.find_last_of("/\\");
	path = fullPath.substr(0, end).c_str();
	file = fullPath.substr(end + 1).c_str();
}

bool StringConverter::getline(const std::string &msg, std::string::size_type &begin, std::string &line, const char *line_delim) {
	std::string::size_type end = msg.find(line_delim, begin);
	std::string::size_type size = end - begin;
	if (end != std::string::npos) {
		if (size > 2) {
			line = msg.substr(begin, end - begin);
		} else {
			line = "--- LINE END ---";
		}
		begin = end + strlen(line_delim);
		return true;
	} else if (begin == 0 && msg.size() > 2) {
		// if there is no delim found but msg size is more then 2, give just the string
		line = msg;
		begin = msg.size();
		return true;
	}
	return false;
}

void StringConverter::addFormattedStringBasic(std::string &str, const char *fmt, va_list arglist) {
	char txt[1024 * 3];
	vsnprintf(txt, sizeof(txt)-1, fmt, arglist);
	str += txt;
}

void StringConverter::addFormattedString(std::string &str, const char *fmt, ...) {
	va_list arglist;
	va_start(arglist, fmt);
	addFormattedStringBasic(str, fmt, arglist);
	va_end(arglist);
}

std::string StringConverter::getFormattedString(const char *fmt, ...) {
	std::string str;
	va_list arglist;
	va_start(arglist, fmt);
	addFormattedStringBasic(str, fmt, arglist);
	va_end(arglist);
	return str;
}

bool StringConverter::isRootFile(const std::string &msg) {
	return msg.find("/ ") != std::string::npos;
}

bool StringConverter::getProtocol(const std::string &msg, std::string &protocol) {
	// Protocol should be in the first line
	std::string::size_type nextline = 0;
	std::string line;
	if (StringConverter::getline(msg, nextline, line, "\r\n")) {
		const std::string::size_type begin = line.find_last_of(" ") + 1;
		const std::string::size_type end   = line.find_last_of("/");
		if (begin != std::string::npos && end != std::string::npos) {
			protocol = line.substr(begin, end - begin);
			return true;
		}
	}
	return false;
}

bool StringConverter::getRequestedFile(const std::string &msg, std::string &file) {
	std::string param;
	if (StringConverter::getHeaderFieldParameter(msg, "GET", param) ||
	    StringConverter::getHeaderFieldParameter(msg, "POST", param)) {
		const std::string::size_type begin = param.find_first_of("/");
		const std::string::size_type end   = param.find_first_of(" ", begin);
		file = param.substr(begin, end - begin);
		return true;
	}
	return false;
}

bool StringConverter::getMethod(const std::string &msg, std::string &method) {
	// request line should be in the first line (method)
	std::string::size_type nextline = 0;
	std::string line;

	if (StringConverter::getline(msg, nextline, line, "\r\n")) {
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

bool StringConverter::getContentFrom(const std::string &msg, std::string &content) {
	bool ret = false;
	std::string parameter;
	const std::size_t contentLength = StringConverter::getHeaderFieldParameter(msg, "Content-Length", parameter) ?
	                                  atoi(parameter.c_str()) : 0;
	if (contentLength > 0) {
		const std::string::size_type headerSize = msg.find("\r\n\r\n", 0);
		if (headerSize != std::string::npos) {
			content = msg.substr(headerSize + 4);
			ret = true;
		}
	}
	return ret;
}

bool StringConverter::hasTransportParameters(const std::string &msg) {
	// Transport Parameters should be in the first line (method)
	std::string::size_type nextline = 0;
	std::string line;

	if (StringConverter::getline(msg, nextline, line, "\r\n")) {
		const std::string::size_type found = line.find_first_of("?");
		if (found != std::string::npos) {
			return true;
		}

	}
	return false;
}

bool StringConverter::getHeaderFieldParameter(const std::string &msg, const std::string &header_field, std::string &parameter) {
	std::string::size_type nextline = 0;
	std::string line;
	parameter = "";
	while (StringConverter::getline(msg, nextline, line, "\r\n")) {
		std::string::const_iterator it = line.begin();
		std::string::const_iterator it_h = header_field.begin();

		// remove any leading whitespace
		while (*it == ' ') ++it;

		// check if we find header_field
		bool found = true;
		while (*it != ':' && *it != ' ') {
			if (toupper(*it) != toupper(*it_h)) {
				found = false;
				break;
			}
			++it;
			++it_h;
		}
		if (found) {
			std::string::size_type begin = line.find_first_of(":") + 1;
			begin = line.find_first_not_of(" ", begin);
			std::string::size_type end = line.size();
			parameter = line.substr(begin, end - begin);
			return true;
		}
	}
	return false;
}

bool StringConverter::getStringParameter(const std::string &msg, const std::string &header_field,
                                         const std::string &parameter, std::string &value) {
	const char delim[] = "/?; &\r\n=";
	std::string line;
	if (getHeaderFieldParameter(msg, header_field, line)) {
		std::string::size_type begin = line.find_first_of(delim, 0);
		std::string::size_type end = begin;
		while (begin != std::string::npos) {
			end = line.find_first_of(delim, begin + 1);
			if (end != std::string::npos) {
				std::string token = line.substr(begin + 1, end - begin);
				if (strncmp(token.c_str(), parameter.c_str(), parameter.size()) == 0) {
					begin = end + 1;
					end = line.find_first_of(delim, begin);
					value = line.substr(begin, end - begin);
					return true;
				} else {
					begin = line.find_first_of(delim, end);
				}
			} else {
				break;
			}
		}
	}
	return false;
}

double StringConverter::getDoubleParameter(const std::string &msg, const std::string &header_field, const std::string &parameter) {
	std::string val;
	if (StringConverter::getStringParameter(msg, header_field, parameter, val) == 1) {
		return atof(val.c_str());
	}
	return -1.0;
}

int StringConverter::getIntParameter(const std::string &msg, const std::string &header_field, const std::string &parameter) {
	std::string val;
	if (StringConverter::getStringParameter(msg, header_field, parameter, val) == 1) {
		return atoi(val.c_str());
	}
	return -1;
}

input::InputSystem StringConverter::getMSYSParameter(const std::string &msg, const std::string &header_field) {
	std::string val;
	if (StringConverter::getStringParameter(msg, header_field, "msys=", val) == 1) {
		if (val.compare("dvbs2") == 0) {
			return input::InputSystem::DVBS2;
		} else if (val.compare("dvbs") == 0) {
			return input::InputSystem::DVBS;
		} else if (val.compare("dvbt") == 0) {
			return input::InputSystem::DVBT;
		} else if (val.compare("dvbt2") == 0) {
			return input::InputSystem::DVBT2;
		} else if (val.compare("dvbc") == 0) {
			return input::InputSystem::DVBC;
		} else if (val.compare("file") == 0) {
			return input::InputSystem::FILE;
		}
	}
	return input::InputSystem::UNDEFINED;
}

std::string StringConverter::makeXMLString(const std::string &msg) {
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

const char *StringConverter::fec_to_string(int fec) {
	switch (fec) {
	case FEC_1_2:
		return "12";
	case FEC_2_3:
		return "23";
	case FEC_3_4:
		return "34";
	case FEC_3_5:
		return "35";
	case FEC_4_5:
		return "45";
	case FEC_5_6:
		return "56";
	case FEC_6_7:
		return "67";
	case FEC_7_8:
		return "78";
	case FEC_8_9:
		return "89";
	case FEC_9_10:
		return "910";
	case FEC_AUTO:
		return " ";
	case FEC_NONE:
		return " ";
	default:
		return "UNKNOWN FEC";
	}
}

const char *StringConverter::delsys_to_string(input::InputSystem system) {
	switch (system) {
		case input::InputSystem::DVBS2:
			return "dvbs2";
		case input::InputSystem::DVBS:
			return "dvbs";
		case input::InputSystem::DVBT:
			return "dvbt";
		case input::InputSystem::DVBT2:
			return "dvbt2";
		case input::InputSystem::FILE:
			return "file";
		case input::InputSystem::DVBC:
			return "dvbc";
		default:
			return "UNKNOWN DELSYS";
	}
}

const char *StringConverter::modtype_to_sting(int modtype) {
	switch (modtype) {
	case QAM_16:
		return "16qam";
	case QAM_32:
		return "32qam";
	case QAM_64:
		return "64qam";
	case QAM_128:
		return "128qam";
	case QAM_256:
		return "256qam";
	case QPSK:
		return "qpsk";
	case PSK_8:
		return "8psk";
	case QAM_AUTO:
		return " ";
	default:
		return "UNKNOWN MODTYPE";
	}
}

const char *StringConverter::rolloff_to_sting(int rolloff) {
	switch (rolloff) {
	case ROLLOFF_35:
		return "0.35";
	case ROLLOFF_25:
		return "0.25";
	case ROLLOFF_20:
		return "0.20";
	case ROLLOFF_AUTO:
		return " ";
	default:
		return "UNKNOWN ROLLOFF";
	}
}

const char *StringConverter::pilot_tone_to_string(int pilot) {
	switch (pilot) {
	case PILOT_ON:
		return "on";
	case PILOT_OFF:
		return "off";
	case PILOT_AUTO:
		return " ";
	default:
		return "UNKNOWN PILOT";
	}
}

const char *StringConverter::transmode_to_string(int transmission_mode) {
	switch (transmission_mode) {
	case TRANSMISSION_MODE_2K:
		return "2k";
	case TRANSMISSION_MODE_8K:
		return "8k";
	case TRANSMISSION_MODE_AUTO:
		return " ";
	case TRANSMISSION_MODE_4K:
		return "4k";
	case TRANSMISSION_MODE_1K:
		return "1k";
	case TRANSMISSION_MODE_16K:
		return "16k";
	case TRANSMISSION_MODE_32K:
		return "32k";
#if FULL_DVB_API_VERSION >= 0x0509
	case TRANSMISSION_MODE_C1:
		return "c1";
	case TRANSMISSION_MODE_C3780:
		return "c3780";
#endif
	default:
		return "UNKNOWN TRANSMISSION MODE";
	}
}

const char *StringConverter::guardinter_to_string(int guard_interval) {
	switch (guard_interval) {
	case GUARD_INTERVAL_1_32:
		return "132";
	case GUARD_INTERVAL_1_16:
		return "116";
	case GUARD_INTERVAL_1_8:
		return "18";
	case GUARD_INTERVAL_1_4:
		return "14";
	case GUARD_INTERVAL_AUTO:
		return " ";
	case GUARD_INTERVAL_1_128:
		return "1128";
	case GUARD_INTERVAL_19_128:
		return "19128";
	case GUARD_INTERVAL_19_256:
		return "19256";
	default:
		return "UNKNOWN GUARD INTERVAL";
	}
}
