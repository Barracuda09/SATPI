/* Log.h

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef LOG_H_INCLUDE
#define LOG_H_INCLUDE LOG_H_INCLUDE

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <string>
#include <deque>

typedef struct {
	int priority;
	std::string msg;
	std::string timestamp;
} LogElem_t;

typedef std::deque<LogElem_t> LogBuffer_t;

void binlog(int priority, const unsigned char *p, int length, const char *fmt, ...);

void applog(int priority, const char *fmt, ...);

std::string make_log_xml();

void open_app_log();
void close_app_log();

#ifdef NDEBUG
#define PERROR(str)                 applog(LOG_ERR, str ": %s (code %d)", strerror(errno), errno)
#define GAI_PERROR(str, s)          applog(LOG_ERR, str ": %s (code %d)", gai_strerror(s), s)
#define SI_LOG_INFO(...)            applog(LOG_INFO, __VA_ARGS__)
#define SI_LOG_ERROR(...)           applog(LOG_ERR, __VA_ARGS__)
#define SI_LOG_DEBUG(...)
#define SI_LOG_COND_DEBUG(cond, fmt, ...)
#define SI_LOG_BIN_DEBUG(p, length, fmt, ...)
#else
#define PERROR(str)                            applog(LOG_ERR,   "[%28s:%03d] " str ": %s (code %d)", __FILE__, __LINE__, strerror(errno), errno)
#define GAI_PERROR(str, s)                     applog(LOG_ERR,   "[%28s:%03d] " str ": %s (code %d)", __FILE__, __LINE__, gai_strerror(s), s)
#define SI_LOG_INFO(fmt, ...)                  applog(LOG_INFO,  "[%28s:%03d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define SI_LOG_ERROR(fmt, ...)                 applog(LOG_ERR,   "[%28s:%03d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define SI_LOG_DEBUG(fmt, ...)                 applog(LOG_DEBUG, "[%28s:%03d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define SI_LOG_COND_DEBUG(cond, fmt, ...)      if (cond) { applog(LOG_DEBUG, "[%28s:%03d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); }
#define SI_LOG_BIN_DEBUG(p, length, fmt, ...)  binlog(LOG_DEBUG, p, length, "[%28s:%03d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#endif // LOG_H_INCLUDE
