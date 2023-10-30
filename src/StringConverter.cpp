/* StringConverter.cpp

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
#include <StringConverter.h>

#include <Log.h>
#include <input/dvb/dvbfix.h>

#include <iostream>
#include <cstdarg>

void StringConverter::splitPath(const std::string &fullPath, std::string &path, std::string &file) {
	std::string::size_type end = fullPath.find_last_of("/\\");
	path = fullPath.substr(0, end);
	file = fullPath.substr(end + 1);
}

StringVector StringConverter::split(const std::string_view input,
		const std::string_view delimiters) {
	StringVector tokens;
	std::string_view::size_type start = 0;
	std::string_view::size_type end = 0;
	do {
		// try to find end of a token by finding the delimiter from current start pos
		end = input.find_first_of(delimiters, start);
		// if there is no delimiter found than use string end
		if (end == std::string::npos) {
			end = input.size();
		}
		// get the subpart of the string and store it as a token
		std::string_view token = input.substr(start, end - start);
		if (!token.empty()) {
			tokens.emplace_back(token);
		}
		// advance 'start' to pos just after found token + delimiter
		start = input.find_first_not_of(delimiters, end);
	} while (end < input.size() && start < input.size());
	return tokens;
}

std::string StringConverter::stringToUpper(const std::string_view str) {
	std::string result(str);
	for (auto& c : result) {
		if (std::islower(c)) {
			c = std::toupper(c);
		}
	}
	return result;
}

std::string StringConverter::trimWhitespace(const std::string_view str) {
	std::string sub(str);
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
	return sub;
}

std::string StringConverter::getline(const std::string_view msg,
		std::string::size_type &begin, const std::string_view delim) {
	const std::string_view::size_type end = msg.find(delim, begin);
	const std::string_view::size_type size = end - begin;
	std::string_view line;
	if (end != std::string::npos) {
		if (size > 2) {
			line = msg.substr(begin, end - begin);
		} else {
			line = "<CRLF>";
		}
		begin = end + strlen(delim.data());
	} else if (begin == 0 && msg.size() > 2) {
		// if there is no delim found but msg size is more then 2, give just the string
		line = msg;
		begin = msg.size();
	}
	return std::string(line);
}

std::string StringConverter::convertToHexASCIITable(const unsigned char* p, const std::size_t length, const std::size_t blockSize) {
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

std::string_view StringConverter::fec_to_string(const int fec) {
	switch (fec) {
	case FEC_1_2:
		return "12";
	case FEC_2_3:
		return "23";
#if FULL_DVB_API_VERSION >= 0x0509
	case FEC_2_5:
		return "25";
#endif
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
		return "";
	case FEC_NONE:
		return "";
	default:
		return "UNKNOWN FEC";
	}
}

std::string_view StringConverter::delsys_to_string(const input::InputSystem system) {
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
		case input::InputSystem::FILE_SRC:
			return "file";
		case input::InputSystem::STREAMER:
			return "streamer";
		case input::InputSystem::DVBC:
			return "dvbc";
		default:
			return "UNKNOWN DELSYS";
	}
}

std::string_view StringConverter::modtype_to_sting(const int modtype) {
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
	case DQPSK:
		return "dqpsk";
	case APSK_16:
		return "16apsk";
	case APSK_32:
		return "32apsk";
	case QAM_AUTO:
		return "";
	default:
		return "UNKNOWN MODTYPE";
	}
}

std::string_view StringConverter::rolloff_to_sting(const int rolloff) {
	switch (rolloff) {
	case ROLLOFF_35:
		return "0.35";
	case ROLLOFF_25:
		return "0.25";
	case ROLLOFF_20:
		return "0.20";
	case ROLLOFF_AUTO:
		return "";
	default:
		return "UNKNOWN ROLLOFF";
	}
}

std::string_view StringConverter::pilot_tone_to_string(const int pilot) {
	switch (pilot) {
	case PILOT_ON:
		return "on";
	case PILOT_OFF:
		return "off";
	case PILOT_AUTO:
		return "";
	default:
		return "UNKNOWN PILOT";
	}
}

std::string_view StringConverter::transmode_to_string(const int transmission_mode) {
	switch (transmission_mode) {
	case TRANSMISSION_MODE_2K:
		return "2k";
	case TRANSMISSION_MODE_8K:
		return "8k";
	case TRANSMISSION_MODE_AUTO:
		return "";
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

std::string_view StringConverter::guardinter_to_string(const int guard_interval) {
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
		return "";
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
