/* StringConverter.h

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
#ifndef STRING_CONVERTER_H_INCLUDE
#define STRING_CONVERTER_H_INCLUDE STRING_CONVERTER_H_INCLUDE

#include <Defs.h>
#include <input/InputSystem.h>

#include <string>
#include <string_view>
#include <cctype>
#include <sstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <iomanip>

/// The class @c StringConverter has some string manipulation functions
class StringConverter  {

	public:

		/// Returns a copy of the string where all specified markers are replaced
		/// with the specified arguments.<br>
		/// <b>Example:</b> @c std::string s = StringConverter::stringFormat(
		///   "Frontend: @#1, Close StreamClient[@#2] with SessionID @#3", 1, 0, "12345");
		/// @return A copy of the string where all specified markers are replaced
		/// with the specified arguments.
		template <typename... Args>
		static std::string stringFormat(const char *format, Args&&... args) {
			std::vector<std::string> vectArgs;
			vectArgs.push_back("?");
			// trick for pack expansion
			const int dummy[] = { 0, ((void) makeVectArgs(vectArgs, std::forward<Args>(args)), 0)... };
			(void)dummy;

			// Make as little reallocations as possible, so calc size
			std::string line;
			std::size_t size = std::strlen(format);
			for (std::size_t i = 1; i < vectArgs.size(); ++i) {
				size += vectArgs[i].size();
			}
			// remove @#-tags from size (-1 because of always '?' arg)
			size -= (vectArgs.size() - 1) * 2;
			line.reserve(size);

			for (; *format != '\0'; ++format) {
				if (*format == '@' && *(format + 1) != '\0' && *(format + 1) == '#') {
					++format;
					if (*(format + 1) != '\0' && std::isdigit(*(format + 1))) {
						++format;
						const char *formatDigit = format;
						while (std::isdigit(*formatDigit)) {
						  ++formatDigit;
						}
						const std::size_t digitCnt = formatDigit - format;
						const std::size_t index = std::stoul(std::string(format, digitCnt));
						if (index < vectArgs.size()) {
							line += vectArgs[index];
						} else {
							line += std::string(format - 2, digitCnt + 2);
						}
						// -1 because of ++format in for statement
						format += digitCnt - 1;
					} else {
						// Error @# near end of line
						line += "@#E";
					}
				} else {
					line += *format;
				}
			}
			return line;
		}

		static std::string convertToHexASCIITable(const unsigned char* p, std::size_t length, std::size_t blockSize);

		template<class T>
		static std::string hexString(const T &value, const int width) {
			std::ostringstream stream;
			stream.flags(std::ios_base::fmtflags(std::ios::hex | std::ios::uppercase));
			stream << "0x" << std::setfill('0') << std::setw(width);
			stream << static_cast<unsigned long>(value);
			return stream.str();
		}

		template<class T>
		static std::string hexPlainString(const T &value, const int width) {
			std::ostringstream stream;
			stream.flags(std::ios_base::fmtflags(std::ios::hex));
			stream << std::setfill('0') << std::setw(width);
			stream << static_cast<unsigned long>(value);
			return stream.str();
		}

		template<class T>
		static std::string alphaString(const T &value, const int width) {
			std::ostringstream stream;
			stream << std::setfill(' ') << std::setw(width) << value;
			return stream.str();
		}

		template<class T>
		static std::string digitString(const T &value, const int width) {
			std::ostringstream stream;
			stream << std::setfill('0') << std::setw(width) << value;
			return stream.str();
		}

		///
		template<class T>
		static std::string toStringFrom4BitBCD(const T bcd, const int charNr) {
			std::ostringstream stream;
			if (charNr > 0) {
				for (int i = 0; i < charNr; ++i) {
					stream << std::right << ((bcd >> ((charNr - 1 - i) * 4)) & 0xF);
				}
			}
			return stream.str();
		}

		/// Get next line with line_delim (if available) from msg
		/// @return @c line or empty line
		static std::string getline(std::string_view msg,
			std::string::size_type &begin, std::string_view delim);

		///
		static std::string trimWhitespace(std::string_view str);

		///
		static std::string stringToUpper(std::string_view str);

		///
		static void splitPath(const std::string &fullPath, std::string &path, std::string &file);

		///
		static StringVector split(std::string_view input, std::string_view delimiters);

		///
		static std::string getPercentDecoding(const std::string &msg);

		///
		static StringVector parseCommandArgumentString(const std::string &cmd);

		///
		static std::string_view transmode_to_string(int transmission_mode);

		///
		static std::string_view rolloff_to_sting(int rolloff);

		///
		static std::string_view modtype_to_sting(int modtype);

		///
		static std::string_view fec_to_string(int fec);

		///
		static std::string_view delsys_to_string(input::InputSystem delsys);

		///
		static std::string_view guardinter_to_string(int guard_interval);

		///
		static std::string_view pilot_tone_to_string(int pilot);

	protected:

		/// Helper function for stringFormat
		template <typename Type>
		static void makeVectArgs(std::vector<std::string> &vec, Type&& t) {
			std::ostringstream stream;
			stream.setf(std::ios::fixed);
			stream.precision(4);
			stream << std::move(t);
			vec.emplace_back(std::move(stream.str()));
		}
};

#define HEX(value, size) StringConverter::hexString(value, size)
#define HEX2(value) StringConverter::hexString(value, 2)

#define HEXPL(value, size) StringConverter::hexPlainString(value, size)

#define STR(value, size) StringConverter::alphaString(value, size)

#define DIGIT(value, size) StringConverter::digitString(value, size)

#define PID(value) StringConverter::digitString(value, 4)

#endif // STRING_CONVERTER_H_INCLUDE
