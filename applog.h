/* applog.h

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

#ifndef _APPLOG_H
#define _APPLOG_H

#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

typedef struct {
	int priority;
	char *msg;
	char timestamp[35];
} LogElem_t;

#define LOG_SIZE 300

typedef struct {
	LogElem_t elem[LOG_SIZE];
	size_t begin;
	size_t end;
	int full;
} LogBuffer_t;

void satiplog(int priority, const char *fmt, ...);

char *make_log_xml();

void open_satip_log();
void close_satip_log();

#define PERROR(str)             satiplog(LOG_ERR, str ": %s (code %d)", strerror(errno), errno)
#define SI_LOG_INFO(...)        satiplog(LOG_INFO, __VA_ARGS__)
#define SI_LOG_ERROR(...)       satiplog(LOG_ERR, __VA_ARGS__)
#ifdef NDEBUG
#define SI_LOG_DEBUG(...)
#else
#define SI_LOG_DEBUG(...)       satiplog(LOG_DEBUG, __VA_ARGS__)
#endif

#endif
