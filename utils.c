/* utils.c

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

#include <linux/dvb/frontend.h>

#include "utils.h"
#include "applog.h"

/*
 *
 */
char *get_line_from(const char *buf, size_t *ptr_index, const char *line_delim) {
	char *line = NULL;
	size_t buf_size = strlen(buf);
	if (buf && buf_size > *ptr_index) {
		const char *begin = buf + *ptr_index;
		size_t size = 0;
		const char *end = strstr(begin, line_delim);
		if (end != NULL) {
			size = end - begin;
			*ptr_index += size + strlen(line_delim);
		} else if (begin) {
			size = strlen(begin);
			*ptr_index += size;
		}
		if (size) {
			line = malloc(size + 1);
			if (line == NULL) {
//				PERROR("get_line_from malloc");
				return NULL;
			}
			memcpy(line, begin, size);
			line[size] = 0;
		}
	}
	return line;
}

/*
 *
 */
char *get_header_field_from(const char *buf, const char *header_field) {
	char *line = NULL;
	size_t ptr_index = 0;
	size_t i;
	do {
		line = get_line_from(buf, &ptr_index, "\r\n");
		if (line) {
			i = 0;
			while (line[i] != ':' && line[i] != ' ') {
				if (toupper(line[i]) != toupper(header_field[i])) {
					break;
				}
				++i;
			}
			if (i == strlen(header_field)) {
				break;
			}
			free(line); // Do not use FREE_PTR (it will do line = NULL)
		}
	} while (line != NULL);
	return line;
}

/*
 *
 */
char *make_xml_string(char *str) {
	char *newStr = NULL;
	char *begin = str;
	char *end = str;
	do {
		if (*end == '&') {
			*end = 0;
			addString(&newStr, "%s&amp;", begin);
			*end = '&';
			begin = end + 1;
		} else if (*end == '"') {
			*end = 0;
			addString(&newStr, "%s&quot;", begin);
			*end = '"';
			begin = end + 1;
		} else if (*end == '>') {
			*end = 0;
			addString(&newStr, "%s&gt;", begin);
			*end = '>';
			begin = end + 1;
		} else if (*end == '<') {
			*end = 0;
			addString(&newStr, "%s&lt;", begin);
			*end = '<';
			begin = end + 1;
		}
		++end;
	} while (*end != '\0');
	addString(&newStr, "%s", begin);
	return newStr;
}

/*
 *
 */
size_t addString(char **str, const char *fmt, ...) {
    char txt[500];
    va_list arglist;
    va_start(arglist, fmt);
    const size_t cnt = vsnprintf(txt, sizeof(txt)-1, fmt, arglist);
    va_end(arglist);
	txt[cnt] = 0;
	if (*str == NULL) {
		*str = malloc(strlen(txt) + 1);
		if (*str == NULL) {
			PERROR("addString malloc");
			return 0;
		}
		*str[0] = 0;
	} else {
		const size_t len = strlen(txt) + strlen(*str);
		char *ptr = *str;
		*str = realloc(*str, len + 1);
		if (*str == NULL) {
			PERROR("addString realloc");
			*str = ptr;
			return 0;
		}
	}
	strcat(*str, txt);
	return strlen(*str);
}

/*
 *
 */
long getmsec() {
  struct timeval tv;
  gettimeofday(&tv,(struct timezone*) NULL);
  return(tv.tv_sec%1000000)*1000 + tv.tv_usec/1000;
}

/*
 *
 */
const char *fec_to_string(int fec) {
	switch (fec) {
		case FEC_1_2:
			return "FEC_1_2";
		case FEC_2_3:
			return "FEC_2_3";
		case FEC_3_4:
			return "FEC_3_4";
		case FEC_3_5:
			return "FEC_3_5";
		case FEC_4_5:
			return "FEC_4_5";
		case FEC_5_6:
			return "FEC_5_6";
		case FEC_6_7:
			return "FEC_6_7";
		case FEC_7_8:
			return "FEC_7_8";
		case FEC_8_9:
			return "FEC_8_9";
		case FEC_9_10:
			return "FEC_9_10";
		case FEC_AUTO:
			return "FEC_AUTO";
		case FEC_NONE:
			return "FEC_NONE";
		default:
			return "UNKNOWN FEC";
	}
}

/*
 *
 */
const char *delsys_to_string(int delsys) {
	switch (delsys) {
		case SYS_DVBS2:
			return "SYS_DVBS2";
		case SYS_DVBS:
			return "SYS_DVBS";
		default:
			return "UNKNOWN DELSYS";
	}
}

/*
 *
 */
const char *modtype_to_sting(int modtype) {
	switch (modtype) {
		case QPSK:
			return "QPSK";
		case PSK_8:
			return "PSK_8";
		default:
			return "UNKNOWN MODTYPE";
	}
}

/*
 *
 */
const char *rolloff_to_sting(int rolloff) {
	switch (rolloff) {
		case ROLLOFF_35:
			return "0.35";
		case ROLLOFF_25:
			return "0.25";
		case ROLLOFF_20:
			return "0.20";
		case ROLLOFF_AUTO:
			return "auto";
		default:
			return "UNKNOWN ROLLOFF";
	}
}
