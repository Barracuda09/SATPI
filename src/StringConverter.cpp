/* StringConverter.cpp

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
#include <base/Tokenizer.h>

#include <iostream>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <cstdarg>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void StringConverter::splitPath(const std::string &fullPath, std::string &path, std::string &file) {
	std::string::size_type end = fullPath.find_last_of("/\\");
	path = fullPath.substr(0, end);
	file = fullPath.substr(end + 1);
}

std::string StringConverter::stringToUpper(const char *str) {
	std::string result(str);
	std::size_t i = 0;
	while (*str) {
		if (std::islower(*str)) {
			result[i] = static_cast<char>(std::toupper(*str));
		}
		++str;
		++i;
	}
	return result;
}

std::string StringConverter::stringToUpper(const std::string &str) {
	return stringToUpper(str.c_str());
}

void StringConverter::trimWhitespace(const std::string &str, std::string &sub) {
	sub = str;
	if (str.size() > 0) {
		// trim leading
		while (sub.size() > 0 && std::isspace(sub[0])) {
			sub.erase(0, 1);
		}
		// trim trailing
		while (sub.size() > 1 && std::isspace(sub[sub.size() - 1])) {
			sub.erase(sub.size() - 1, 1);
		}
	}
}

std::string StringConverter::getline(const std::string &msg, std::string::size_type &begin, const char *line_delim) {
	const std::string::size_type end = msg.find(line_delim, begin);
	const std::string::size_type size = end - begin;
	std::string line;
	if (end != std::string::npos) {
		if (size > 2) {
			line = msg.substr(begin, end - begin);
		} else {
			line = "--- LINE END ---";
		}
		begin = end + strlen(line_delim);
		return line;
	} else if (begin == 0 && msg.size() > 2) {
		// if there is no delim found but msg size is more then 2, give just the string
		line = msg;
		begin = msg.size();
		return line;
	}
	return "";
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

std::string StringConverter::convertToHexASCIITable(const unsigned char *p, const std::size_t length, const std::size_t blockSize) {
	if (blockSize == 0) {
		return "";
	}
	std::stringstream hexString;
	std::stringstream asciiString;
	const std::size_t lengthNew = (((length / blockSize) + (length %  blockSize > 0 ? 1 : 0)) * blockSize);

	std::string out("");
	for (std::size_t i = 0; i < length; ++i) {
		const unsigned char c = p[i];
		const unsigned char ascii = std::isprint(c) ? c : '.';
		hexString << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(c);
		asciiString << ascii;
		if (((i + 1) %  blockSize) == 0) {
			out += hexString.str();
			out += "  ";
			out += asciiString.str();
			out += "\r\n";
			hexString.str("");
			asciiString.str("");
		} else {
			hexString << " ";
		}
	}
	// Is there remaining strings to add to out buffer
	const int diff = lengthNew - length;
	if (diff > 0) {
		std::string space(diff * 3, ' ');
		out += hexString.str();
		out += space;
		out += " ";
		out += asciiString.str();
		out += "\r\n";
	}
	return out;
}

bool StringConverter::isRootFile(const std::string &msg) {
	return msg.find("/ ") != std::string::npos;
}

std::string StringConverter::getProtocol(const std::string &msg) {
	// Protocol should be in the first line
	std::string::size_type nextline = 0;
	const std::string line = StringConverter::getline(msg, nextline, "\r\n");
	if (!line.empty()) {
		const std::string::size_type begin = line.find_last_of(" ") + 1;
		const std::string::size_type end   = line.find_last_of("/");
		if (begin != std::string::npos && end != std::string::npos) {
			return line.substr(begin, end - begin);
		}
	}
	return "";
}

std::string StringConverter::getRequestedFile(const std::string &msg) {
	const std::string method = StringConverter::getMethod(msg);
	if (!method.empty()) {
		const std::string param = StringConverter::getHeaderFieldParameter(msg, method);
		const std::string::size_type begin = param.find_first_of("/");
		const std::string::size_type end   = param.find_first_of(" ", begin);
		return param.substr(begin, end - begin);
	}
	return "";
}

std::string StringConverter::getMethod(const std::string &msg) {
	// request line should be in the first line (method)
	std::string::size_type nextline = 0;
	const std::string line = StringConverter::getline(msg, nextline, "\r\n");
	if (!line.empty()) {
		std::string::const_iterator it = line.begin();
		// remove any leading whitespace
		while (*it == ' ') ++it;

		// copy method (upper case)
		std::string method;
		while (*it != ' ') {
			method += std::toupper(*it);
			++it;
		}
		return method;
	}
	return "";
}

std::string StringConverter::getContentFrom(const std::string &msg) {
	const std::string parameter = StringConverter::getHeaderFieldParameter(msg, "Content-Length:");
	const std::size_t contentLength = parameter.empty() ? 0 : std::stoi(parameter);
	if (contentLength > 0) {
		const std::string::size_type headerSize = msg.find("\r\n\r\n", 0);
		if (headerSize != std::string::npos) {
			return msg.substr(headerSize + 4);
		}
	}
	return "";
}

bool StringConverter::hasTransportParameters(const std::string &msg) {
	// Transport Parameters should be in the first line (method)
	std::string::size_type nextline = 0;
	const std::string line = StringConverter::getline(msg, nextline, "\r\n");
	if (!line.empty()) {
		const std::string::size_type size = line.size() - 1;
		const std::string::size_type found = line.find_first_of("?");
		if (found != std::string::npos && found < size && line[found + 1] != ' ') {
			return true;
		}
	}
	return false;
}

std::string StringConverter::getHeaderFieldParameter(const std::string &msg, const std::string &header_field) {
	std::string::size_type nextline = 0;
	std::string parameter;
	for (;;) {
		const std::string line = StringConverter::getline(msg, nextline, "\r\n");
		if (line.empty()) {
			return "";
		}
		std::string::const_iterator it = line.begin();
		std::string::const_iterator it_h = header_field.begin();

		// remove any leading whitespace
		while (std::isspace(*it)) ++it;

		// check if we find header_field
		bool found = true;
		while (it != line.end() && it_h != header_field.end()) {
			if (std::toupper(*it) != std::toupper(*it_h)) {
				found = false;
				break;
			}
			++it;
			++it_h;
		}
		if (found) {
			// copy parameter and trim whitespace
			const std::string::size_type begin = it - line.begin();
			const std::string::size_type end = line.size();
			StringConverter::trimWhitespace(line.substr(begin, end - begin), parameter);
			return parameter;
		}
	}
}

std::string StringConverter::getStringParameter(const std::string &msg, const std::string &header_field,
	const std::string &delim, const std::string &parameter) {
	const std::string line = StringConverter::getHeaderFieldParameter(msg, header_field);
	if (line.empty()) {
		return "";
	}
	base::StringTokenizer tokenizer(line, delim);
	std::string token;
	std::string value;
	while (tokenizer.isNextToken(token)) {
		std::string::size_type begin = token.find(parameter);
		if (begin != std::string::npos && begin == 0) {
			begin += parameter.size();

			// trim leading whitespace
			while (std::isspace(token[begin])) ++begin;

			std::string::size_type end = token.find_first_of(delim + " \r\n", begin);
			if (end == std::string::npos) {
				end = token.size();
			}
			// copy and trim whitespace
			trimWhitespace(token.substr(begin, end - begin), value);
			return value;
		}
	}
	return "";
}

std::string StringConverter::getStringParameter(const std::string &msg,
	const std::string &header_field, const std::string &parameter) {
	return StringConverter::getStringParameter(msg, header_field, "/&?;", parameter);
}

std::string StringConverter::getURIParameter(const std::string &msg,
	const std::string &header_field, const std::string &uriParameter) {
	const std::string line = StringConverter::getHeaderFieldParameter(msg, header_field);
	if (line.empty()) {
		return "";
	}
	std::string::size_type begin = line.find(uriParameter);
	if (begin != std::string::npos) {
		begin += uriParameter.size();
		begin = line.find("\"", begin);
		if (begin != std::string::npos) {
			begin += 1;
			std::string::size_type end = line.find("\"", begin);
			if (end != std::string::npos) {
				return line.substr(begin, end - begin);
			}
		}
	}
	return "";
}

std::string StringConverter::getPercentDecoding(const std::string &msg) {
	std::string decoded = msg;
	// Search for ASCII Percent-Encoding and decode it
	std::string::size_type n = 0;
	while ((n = decoded.find("%", n)) != std::string::npos) {
		++n;
		if ( n + 2 > decoded.size()) {
			break;
		}
		const char c1 = decoded[n];
		const char c2 = decoded[n + 1];
		if (std::isdigit(c1) && (std::isdigit(c2) || std::isalpha(c2))) {
			// It was ex. %2F -> decode it to ASCII '/'
			const std::string sub = decoded.substr(n, 2);
			decoded.replace(n - 1, 3, 1, static_cast<char>(std::stoi(sub, 0 , 16)));
		} else if (c1 == '%' && std::isdigit(c2)) {
			// It was ex. %%2F -> remove first % (so double percent decoded)
			decoded.erase(n - 1, 1);
			n += 2;
		}
	}
	return decoded;
}

StringVector StringConverter::parseCommandArgumentString(const std::string &cmd) {
     std::string arg;
     StringVector argVector;
     size_t quoteCount = 0;
     bool inQuote = false;
     for (size_t i = 0; i < cmd.size(); ++i) {
       if (cmd[i] == '"') {
         ++quoteCount;
         if (quoteCount == 3) {
           quoteCount = 0;
           arg += cmd[i];
         }
         continue;
       }
       if (quoteCount > 0) {
         if (quoteCount == 1) {
           inQuote = !inQuote;
         }
         quoteCount = 0;
       }
       if (!inQuote && cmd[i] == ' ') {
         if (!arg.empty()) {
           argVector.push_back(arg);
           arg.clear();
         }
       } else {
         arg += cmd[i];
       }
     }
     if (!arg.empty()) {
       argVector.push_back(arg);
     }
     return argVector;
}

double StringConverter::getDoubleParameter(const std::string &msg,
	const std::string &header_field, const std::string &parameter) {
	const std::string val = StringConverter::getStringParameter(msg, header_field, parameter);
	return val.empty() ? -1.0 : std::stof(val);
}

int StringConverter::getIntParameter(const std::string &msg, const std::string &header_field, const std::string &parameter) {
	const std::string val = StringConverter::getStringParameter(msg, header_field, parameter);
	return val.empty() ? -1 : std::stoi(val);
}

input::InputSystem StringConverter::getMSYSParameter(const std::string &msg, const std::string &header_field) {
	const std::string val = StringConverter::getStringParameter(msg, header_field, "msys=");
	if (!val.empty()) {
		if (val == "dvbs2") {
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
			return input::InputSystem::FILE;
		} else if (val == "streamer") {
			return input::InputSystem::STREAMER;
		} else if (val == "childpipe") {
			return input::InputSystem::CHILDPIPE;
		}
	}
	return input::InputSystem::UNDEFINED;
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
		case input::InputSystem::CHILDPIPE:
			return "childPIPE";
		case input::InputSystem::FILE:
			return "file";
		case input::InputSystem::STREAMER:
			return "streamer";
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
