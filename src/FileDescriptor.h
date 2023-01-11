/* FileDescriptor.h

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
#ifndef FILE_DESCRIPTOR_H_INCLUDE
#define FILE_DESCRIPTOR_H_INCLUDE

#include <unistd.h>

/// The class @c FileDescriptor can be used to make handling file descriptors
/// easier
class FileDescriptor {
		// =======================================================================
		//  -- Constructors and destructor ---------------------------------------
		// =======================================================================
	public:

		FileDescriptor(int fd = -1) : _fd(fd) {}

		virtual ~FileDescriptor() {
			close();
		}

		/// Conversion operator to int
//		operator int() const { return _fd; }

		/// Assignment with a file descriptor (int)
		FileDescriptor & operator=(int rhs_fd) {
			if (_fd > 0) {
				close();
			}
			_fd = rhs_fd;
			return *this;
		}

		/// Equality operator
		bool operator==(int rhs_fd) const {
			return _fd == rhs_fd;
		}

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================
	public:

		/// Is File descriptor opened
		bool isOpen() const {
			return _fd > 0;
		}

		/// Get File descriptor
		int  get() const {
			return _fd;
		}

		/// close the file descriptor
		void close() {
			::close(_fd);
		}

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================
	private:

		int _fd;
};

#endif // FILE_DESCRIPTOR_H_INCLUDE
