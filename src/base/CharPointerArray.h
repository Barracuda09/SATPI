/* CharPointerArray.h

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
#ifndef BASE_CHAR_POINTER_ARRAY
#define BASE_CHAR_POINTER_ARRAY BASE_CHAR_POINTER_ARRAY

#include <string>
#include <cstring>

namespace base {

class CharPointerArray {
	public:
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
		template<class Container>
		CharPointerArray(const Container &container) {
			// Allocate Size of container + null terminator (and nullptr it)
			_data = new char *[container.size() + 1]{nullptr};
			int counter = 0;
			for (const std::string& str : container) {
				// Allocate Size of str + null terminator (and zero it)
				_data[counter] = new char [str.size() + 1]{0};
				// Copy str
				std::memcpy(_data[counter], str.data(), str.size());
				++counter;
			}
		}

		virtual ~CharPointerArray() {
			for (int i = 0; _data[i] != nullptr; ++i) {
				delete _data[i];
			}
			delete [] _data;
		}

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		char **getData() const {
			return _data;
		}

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:
		char **_data;
};

} // namespace base

#endif // BASE_CHAR_POINTER_ARRAY
