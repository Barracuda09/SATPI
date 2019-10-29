/* ChildPIPEReader.h

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
#ifndef BASE_CHILD_PIPE_READER
#define BASE_CHILD_PIPE_READER BASE_CHILD_PIPE_READER

#include <memory>
#include <string>

#include <stdio.h>

namespace base {

class ChildPIPEReader {
		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:
		ChildPIPEReader() :
			_pipe(::popen("", "r"), ::pclose) {
			_pipe.release();
		}

		virtual ~ChildPIPEReader() {}

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	public:

		void open(const std::string &cmd) {
			_pipe.reset(::popen(cmd.c_str(), "r"));
		}

		void close() {
			_pipe.release();
		}

		bool isOpen() const {
			return _pipe ? true : false;
		}

		std::size_t read(char *buffer, std::size_t size) {
			return std::fread(buffer, 1, size, _pipe.get());
		}

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		std::unique_ptr<std::FILE, decltype(&::pclose)> _pipe;
};

} // namespace base

#endif // BASE_CHILD_PIPE_READER