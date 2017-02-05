/* Log.cpp

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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
#include <Log.h>

#include <StringConverter.h>
#include <base/Mutex.h>

#include <iostream>
#include <ctime>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define LOG_SIZE 350

int syslog_on = 1;
static base::Mutex logMutex;

Log::LogBuffer_t Log::appLogBuffer;

void Log::open_app_log() {}

void Log::close_app_log() {}

void Log::binlog(int priority, const unsigned char *p, int length, const char *fmt, ...) {
	std::string strData;
	for (int i = 1; i <= length; ++i) {
		StringConverter::addFormattedString(strData, "%02X ", p[i-1]);
		if ((i % 16) == 0) {
			StringConverter::addFormattedString(strData, "\r\n");
		}
	}
	char txt[2048];
	va_list arglist;
	va_start(arglist, fmt);
	vsnprintf(txt, sizeof(txt)-1, fmt, arglist);
	va_end(arglist);

	applog(priority, "%s\r\n%s\r\nEND\r\n", txt, strData.c_str());
}

void Log::applog(int priority, const char *fmt, ...) {
	base::MutexLock lock(logMutex);

	char txt[2048];
	va_list arglist;
	va_start(arglist, fmt);
	vsnprintf(txt, sizeof(txt)-1, fmt, arglist);
	va_end(arglist);

	struct timespec time_stamp;
	clock_gettime(CLOCK_REALTIME, &time_stamp);
	std::string msg(txt);
	std::string line;
	std::string::size_type index = 0;

	while (StringConverter::getline(msg, index, line, "\r\n")) {
		LogElem_t elem;

		// set priority
		elem.priority = priority;

		// set timestamp
		char ascTime[100];
		struct tm result;
		localtime_r(&time_stamp.tv_sec, &result);
		std::strftime(ascTime, sizeof(ascTime), "%c", &result);

		// cut line to insert nsec '.000000000'
		ascTime[19] = 0;
		char time[150];
		snprintf(time, sizeof(time), "%s.%04lu %s", &ascTime[0], time_stamp.tv_nsec/100000, &ascTime[20]);
		elem.timestamp = time;

		// set message
		elem.msg = line;

		// save to deque
		if (appLogBuffer.size() == LOG_SIZE) {
			appLogBuffer.pop_front();
		}
		appLogBuffer.push_back(elem);

		// log to syslog
		if (syslog_on != 0) {
			syslog(priority, "%s", elem.msg.c_str());
		}
#ifdef DEBUG
		std::cout << elem.msg << "\r\n";
		std::flush(std::cout);
#endif
	}
}

std::string Log::makeXml() {
	std::string log("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<loglist>\r\n");
	{
		base::MutexLock lock(logMutex);
		if (!appLogBuffer.empty()) {
			for (LogBuffer_t::iterator it = appLogBuffer.begin(); it != appLogBuffer.end(); ++it) {
				const LogElem_t elem = *it;
				StringConverter::addFormattedString(log,
					"<log><timestamp>%s</timestamp><msg>%s</msg><prio>%d</prio></log>\r\n",
					elem.timestamp.c_str(), StringConverter::makeXMLString(elem.msg).c_str(),
					elem.priority);
			}
		}
	}
	log += "</loglist>";
	return log;
}
