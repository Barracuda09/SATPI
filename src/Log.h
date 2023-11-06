/* Log.h

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
#ifndef LOG_H_INCLUDE
#define LOG_H_INCLUDE LOG_H_INCLUDE

#include <StringConverter.h>

#include <string>
#include <deque>

#include <sys/types.h>
#include <syslog.h>
#include <string.h>

/// The class @c Log.
class Log {
	public:
		// =========================================================================
		// -- Static functions -----------------------------------------------------
		// =========================================================================
		static void openAppLog(const char *deamonName, bool daemonize);

		static void closeAppLog();

		static void startSysLog(bool start);

		static bool getSysLogState();

		static void logDebug(bool log) {
			_logDebug = log;
		}

		static bool getLogDebugState() {
			return _logDebug;
		}

		template <typename... Args>
		static void binlog(int priority, const unsigned char* p, int length, const char * format, Args&&... args) {
			std::string data = StringConverter::convertToHexASCIITable(p, length, 16);
			std::string line = StringConverter::stringFormat(format, std::forward<Args>(args)...);
			log(priority, StringConverter::stringFormat("@#1\r\n@#2\r\nEND\r\n", line, data));
		}

		template <typename... Args>
		static void applog(int priority, const char * format, Args&&... args) {
			std::string line = StringConverter::stringFormat(format, std::forward<Args>(args)...);
			log(priority, line);
		}

		static std::string makeJSON();

	private:

		static void log(int priority, const std::string &msg);

		struct LogElem {
			LogElem(const int prio, const std::string m, const std::string t) :
				priority(prio), msg(std::move(m)), timestamp(std::move(t)) {}

			LogElem(LogElem&& other) :
				priority(std::move(other.priority)),
				msg(std::move(other.msg)),
				timestamp(std::move(other.timestamp)) {}

			LogElem& operator=(const LogElem& other) = default;

			int priority = 0;
			std::string msg;
			std::string timestamp;
		};

		using LogBuffer = std::deque<LogElem>;

		static LogBuffer _appLogBuffer;
		static bool _syslogOn;
		static bool _coutLog;
		static bool _logDebug;
};

#define MPEGTS_TABLES 0x100

#ifdef DEBUG_LOG
#define SI_LOG_INFO(format, ...)              Log::applog(LOG_INFO,  "[@#1:@#2] @#3", STR(__FILE__, 45), DIGIT(__LINE__, 3), StringConverter::stringFormat(format, ##__VA_ARGS__))
#define SB_LOG_INFO(subsys, format, ...)      Log::applog(LOG_INFO | subsys, "[@#1:@#2] @#3", STR(__FILE__, 45), DIGIT(__LINE__, 3), StringConverter::stringFormat(format, ##__VA_ARGS__))
#define SI_LOG_ERROR(format, ...)             Log::applog(LOG_ERR,   "[@#1:@#2] @#3", STR(__FILE__, 45), DIGIT(__LINE__, 3), StringConverter::stringFormat(format, ##__VA_ARGS__))
#define SI_LOG_DEBUG(format, ...)             Log::applog(LOG_DEBUG, "[@#1:@#2] @#3", STR(__FILE__, 45), DIGIT(__LINE__, 3), StringConverter::stringFormat(format, ##__VA_ARGS__))
#define SI_LOG_PERROR(format, ...)            Log::applog(LOG_ERR,   "[@#1:@#2] @#3: @#4 (code @#5)", STR(__FILE__, 45), DIGIT(__LINE__, 3), StringConverter::stringFormat(format, ##__VA_ARGS__), strerror(errno), errno)
#define SI_LOG_GIA_PERROR(format, err, ...)   Log::applog(LOG_ERR,   "[@#1:@#2] @#3: @#4 (code @#5)", STR(__FILE__, 45), DIGIT(__LINE__, 3), StringConverter::stringFormat(format, ##__VA_ARGS__), gai_strerror(err), err)
#define SI_LOG_COND_DEBUG(cond, format, ...)  if (cond) { SI_LOG_DEBUG(format, ##__VA_ARGS__); }
#define SI_LOG_BIN_DEBUG(p, length, fmt, ...) Log::binlog(LOG_DEBUG, p, length, "[@#1:@#2] @#3", STR(__FILE__, 45), DIGIT(__LINE__, 3), StringConverter::stringFormat(fmt, ##__VA_ARGS__))
#else
#define SI_LOG_INFO(format, ...)              Log::applog(LOG_INFO,  format, ##__VA_ARGS__)
#define SB_LOG_INFO(subsys, format, ...)      Log::applog(LOG_INFO | subsys,  format, ##__VA_ARGS__)
#define SI_LOG_ERROR(format, ...)             Log::applog(LOG_ERR,   format, ##__VA_ARGS__)
#define SI_LOG_DEBUG(format, ...)             Log::applog(LOG_DEBUG, format, ##__VA_ARGS__)
#define SI_LOG_PERROR(format, ...)            Log::applog(LOG_ERR,   "@#1: @#2 (code @#3)", StringConverter::stringFormat(format, ##__VA_ARGS__), strerror(errno), errno)
#define SI_LOG_GIA_PERROR(format, err, ...)   Log::applog(LOG_ERR,   "@#1: @#2 (code @#3)", StringConverter::stringFormat(format, ##__VA_ARGS__), gai_strerror(err), err)
#define SI_LOG_COND_DEBUG(cond, format, ...)  if (cond) { SI_LOG_DEBUG(format, ##__VA_ARGS__); }
#define SI_LOG_BIN_DEBUG(p, length, fmt, ...) Log::binlog(LOG_DEBUG, p, length, fmt, ##__VA_ARGS__)
#endif

#endif // LOG_H_INCLUDE
