/* StringConverter.h

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
#ifndef STRING_CONVERTER_H_INCLUDE
#define STRING_CONVERTER_H_INCLUDE STRING_CONVERTER_H_INCLUDE

#include <input/InputSystem.h>

#include <string>
#include <sstream>
#include <cstring>
#include <vector>

using StringVector = std::vector<std::string>;

/// The class @c StringConverter has some string manipulation functions
class StringConverter  {

	public:

		/// Returns a copy of the string where all specified markers are replaced
		/// with the specified arguments.<br>
		/// <b>Example:</b> @c std::string s = StringConverter::stringFormat(
		///   "Stream: @#1, Close StreamClient[@#2] with SessionID @#3", 1, 0, "12345");
		/// @return A copy of the string where all specified markers are replaced
		/// with the specified arguments.
		template <typename... Args>
		static std::string stringFormat(const char* format, Args&&... args) {
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

		static std::string convertToHexASCIITable(const unsigned char *p, std::size_t length, std::size_t blockSize);

		/// Get next line with line_delim (if available) from msg
		/// @return @c line or empty line
		static std::string getline(const std::string &msg, std::string::size_type &begin, const char *line_delim);

		///
		static void addFormattedStringBasic(std::string &str, const char *fmt, va_list arglist);

		///
		static void addFormattedString(std::string &str, const char *fmt, ...);

		///
		static void trimWhitespace(const std::string &str, std::string &sub);

		///
		static std::string stringToUpper(const char *str);

		///
		static std::string stringToUpper(const std::string &str);

		///
		static std::string getFormattedString(const char *fmt, ...);

		///
		static bool isRootFile(const std::string &msg);

		///
		static std::string getProtocol(const std::string &msg);

		///
		static std::string getRequestedFile(const std::string &msg);

		///
		static void splitPath(const std::string &fullPath, std::string &path, std::string &file);

		///
		static std::string getMethod(const std::string &msg);

		///
		static std::string getContentFrom(const std::string &msg);

		///
		static bool hasTransportParameters(const std::string &msg);

		///
		static std::string getHeaderFieldParameter(const std::string &msg, const std::string &header_field);

		///
		static std::string getStringParameter(const std::string &msg, const std::string &header_field,
		   const std::string &delim, const std::string &parameter);

		///
		static std::string getStringParameter(const std::string &msg,
		    const std::string &header_field, const std::string &parameter);

		///
		static double getDoubleParameter(const std::string &msg, const std::string &header_field, const std::string &parameter);

		///
		static std::string getURIParameter(const std::string &msg, const std::string &header_field, const std::string &uriParameter);

		///
		static std::string getPercentDecoding(const std::string &msg);

		///
		static StringVector parseCommandArgumentString(const std::string &cmd);

		///
		static int getIntParameter(const std::string &msg, const std::string &header_field, const std::string &parameter);

		///
		static input::InputSystem getMSYSParameter(const std::string &msg, const std::string &header_field);

		///
		static const char *transmode_to_string(int transmission_mode);

		///
		static const char *rolloff_to_sting(int rolloff);

		///
		static const char *modtype_to_sting(int modtype);

		///
		static const char *fec_to_string(int fec);

		///
		static const char *delsys_to_string(input::InputSystem delsys);

		///
		static const char *guardinter_to_string(int guard_interval);

		///
		static const char *pilot_tone_to_string(int pilot);

	protected:

		/// Helper function for stringFormat
		template <typename Type>
		static void makeVectArgs(std::vector<std::string> &vec, Type t) {
			std::ostringstream stream;
			stream.setf(std::ios::fixed);
			stream.precision(4);
			stream << t;
			vec.push_back(stream.str());
		}
};

#endif // STRING_CONVERTER_H_INCLUDE
