/* JSONSerializer.h

   Copyright (C) 2014 - 2019 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef BASE_JSONSERIALIZER_H_INCLUDE
#define BASE_JSONSERIALIZER_H_INCLUDE BASE_JSONSERIALIZER_H_INCLUDE

#include <string>

namespace base {

	/// The class @c JSONSerializer has some basic functions to serialize JSON strings
	class JSONSerializer {

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		public:

			JSONSerializer() {}

			virtual ~JSONSerializer() {}

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================
		public:

			void startArrayWithName(const std::string &name) {
				checkAddComma();
				_json += '\"';
				_json += name;
				_json += "\": [";
				++_objectStarted;
			}

			void endArray() {
				_json += ']';
				--_objectStarted;
			}

			void startObjectWithName(const std::string &name) {
				checkAddComma();
				_json += '\"';
				_json += name;
				_json += "\": {";
				++_objectStarted;
			}

			void startObject() {
				checkAddComma();
				_json += '{';
				++_objectStarted;
			}

			void endObject() {
				_json += '}';
				--_objectStarted;
			}

			void addValueNumber(const std::string &name, const std::string &value) {
				checkAddComma();
				_json += '\"';
				_json += name;
				_json += "\": ";
				_json += value;
			}

			void addValueString(const std::string &name, const std::string &value) {
				checkAddComma();
				_json += '\"';
				_json += name;
				_json += "\": \"";
				_json += makeJSONString(value);
				_json += '\"';
			}

			const std::string &getString() {
				if (_objectStarted != 0) {
					_json += "_ERR_";
				}
				return _json;
			}

		private:

			void checkAddComma() {
				const char c = _json.back();
				if (c == '\"' || c == '}' || c == ']' || std::isdigit(c)) {
					_json += ", ";
				}
			}

			std::string makeJSONString(const std::string &msg) {
				std::string json(msg);
				std::string::iterator it = json.begin();
				while (it != json.end()) {
					if (*it == '"' || *it == '\\' || *it == '/' || *it == '\b' ||
						*it == '\f' || *it == '\n' || *it == '\r' || *it == '\t') {
						it = json.insert(it, '\\');
						if (it != json.end()) {
							++it;
						}
					}
					++it;
				}
				return json;
			}

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================
		private:

			std::string _json;
			int _objectStarted = 0;
	};

} // namespace base

#endif // BASE_JSONSERIALIZER_H_INCLUDE
