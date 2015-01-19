/* utils.h

   Copyright (C) 2014 Marc Postema

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

#ifndef _UTILS_H
#define _UTILS_H

#define UNUSED(x) (void)x;

#define N_ELEMENTS(array) (sizeof(array) / sizeof(array[0]))

#define CLOSE_FD(x) if (x != -1) { close(x); x = -1; }

#define FREE_PTR(x) \
	{ \
		if (x != NULL) { \
			free(x); \
			x = NULL; \
		} \
	}

// 
char *get_line_from(const char *buf, size_t *ptr_index, const char *line_delim);

//
char *get_header_field_from(const char *buf, const char *header_field);

//
char *make_xml_string(char *str);

// 
size_t addString(char **str, const char *fmt, ...);

// get time of day in ms
long getmsec();

#endif
