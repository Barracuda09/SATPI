/* Utils.h

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#define UTILS_H_INCLUDE

#include <unistd.h>
#include <stdlib.h>

#define UNUSED(x)

#define N_ELEMENTS(array) (sizeof(array) / sizeof(array[0]))

#define CLOSE_FD(x) if (x != -1) { if (::close(x) == -1) { PERROR("close error"); } x = -1; }

#define FREE_PTR(x) \
	{ \
		if (x != NULL) { \
			::free(x); \
			x = NULL; \
		} \
	}

/// If @a pointer is not NULL, delete it and set it to NULL.
#define DELETE(pointer)        \
	{                          \
		if (pointer != NULL) { \
			delete pointer;    \
			pointer = NULL;    \
		}                      \
	}

/// If @a arrayPointer is not NULL, delete[] it and set it to NULL.
#define DELETE_ARRAY(arrayPointer)   \
	{                                \
		if (arrayPointer != NULL) {  \
			delete[] arrayPointer;   \
			arrayPointer = NULL;     \
		}                            \
	}

#endif // UTILS_H_INCLUDE