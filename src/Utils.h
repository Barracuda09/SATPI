/* Utils.h

   Copyright (C) 2014 - 2018 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef UTILS_H_INCLUDE
#define UTILS_H_INCLUDE UTILS_H_INCLUDE

#include <Log.h>

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#define ASSERT(EXPRESSION) assert(EXPRESSION)

#define N_ELEMENTS(array) (sizeof(array) / sizeof(array[0]))

#define CLOSE_FD(x) \
	if (x != -1) { \
		if (::close(x) == -1) { \
			PERROR("close error fd %d", x); \
		} \
		x = -1; \
	}

#define FREE_PTR(x) \
	{ \
		if (x != nullptr) { \
			::free(x); \
			x = nullptr; \
		} \
	}

/// If @a pointer is not nullptr, delete it and set it to nullptr.
#define DELETE(pointer)        \
	{                          \
		if (pointer != nullptr) { \
			delete pointer;    \
			pointer = nullptr;    \
		}                      \
	}

/// If @a arrayPointer is not nullptr, delete[] it and set it to nullptr.
#define DELETE_ARRAY(arrayPointer)   \
	{                                \
		if (arrayPointer != nullptr) {  \
			delete[] arrayPointer;   \
			arrayPointer = nullptr;     \
		}                            \
	}

#endif // UTILS_H_INCLUDE
