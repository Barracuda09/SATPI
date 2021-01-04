/* Log.h

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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

#include <string>
#include <deque>

#include <sys/types.h>
#include <syslog.h>
#include <string.h>

/// The class @c Log.
class Log {
	public:
		// =======================================================================
		// -- Static functions ---------------------------------------------------
		// =======================================================================
		static void openAppLog(const char *deamonName);

		static void closeAppLog();

		static void startSysLog(bool start);

		static bool getSysLogState();

		static void binlog(int priority, const unsigned char *p, int length, const char *fmt, ...);

		static void applog(int priority, const char *fmt, ...);

		static std::string makeJSON();

	private:

		struct LogElem {
			int priority;
			std::string msg;
			std::string timestamp;
		};

		using LogBuffer = std::deque<LogElem>;

		static LogBuffer _appLogBuffer;
		static bool _syslogOn;
};


#ifdef NDEBUG
#define PERROR(fmt, ...)                       Log::applog(LOG_ERR, fmt ": %s (code %d)", ##__VA_ARGS__, strerror(errno), errno)
#define GAI_PERROR(str, s)                     Log::applog(LOG_ERR, str ": %s (code %d)", gai_strerror(s), s)
#define SI_LOG_INFO(...)                       Log::applog(LOG_INFO, __VA_ARGS__)
#define SI_LOG_ERROR(...)                      Log::applog(LOG_ERR, __VA_ARGS__)
#define SI_LOG_DEBUG(...)                      Log::applog(LOG_DEBUG, __VA_ARGS__)
#define SI_LOG_COND_DEBUG(cond, fmt, ...)      if (cond) { Log::applog(LOG_DEBUG, fmt, ##__VA_ARGS__); }
#define SI_LOG_BIN_DEBUG(p, length, fmt, ...)  Log::binlog(LOG_DEBUG, p, length, fmt, ##__VA_ARGS__)
#else
#define PERROR(fmt, ...)                       Log::applog(LOG_ERR,   "[%45s:%03d] " fmt ": %s (code %d)", __FILE__, __LINE__, ##__VA_ARGS__, strerror(errno), errno)
#define GAI_PERROR(str, s)                     Log::applog(LOG_ERR,   "[%45s:%03d] " str ": %s (code %d)", __FILE__, __LINE__, gai_strerror(s), s)
#define SI_LOG_INFO(fmt, ...)                  Log::applog(LOG_INFO,  "[%45s:%03d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define SI_LOG_ERROR(fmt, ...)                 Log::applog(LOG_ERR,   "[%45s:%03d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define SI_LOG_DEBUG(fmt, ...)                 Log::applog(LOG_DEBUG, "[%45s:%03d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define SI_LOG_COND_DEBUG(cond, fmt, ...)      if (cond) { Log::applog(LOG_DEBUG, "[%45s:%03d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); }
#define SI_LOG_BIN_DEBUG(p, length, fmt, ...)  Log::binlog(LOG_DEBUG, p, length, "[%45s:%03d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#endif // LOG_H_INCLUDE
