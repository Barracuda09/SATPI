/* Log.cpp

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
#include <Log.h>

#include <StringConverter.h>
#include <base/Mutex.h>
#include <base/JSONSerializer.h>

#include <iostream>
#include <ctime>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef NDEBUG
#define LOG_SIZE 400
#else
#define LOG_SIZE 550
#endif

static base::Mutex logMutex;

bool Log::_syslogOn = false;
Log::LogBuffer Log::_appLogBuffer;

void Log::openAppLog(const char *deamonName) {
	// initialize the logging interface
	openlog(deamonName, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}

void Log::closeAppLog() {
	// close logging interface
	closelog();
}

void Log::startSysLog(bool start) {
	_syslogOn = start;
}

bool Log::getSysLogState() {
	return _syslogOn;
}

void Log::binlog(const int priority, const unsigned char *p, const int length, const char *fmt, ...) {
	const std::string strData = StringConverter::convertToHexASCIITable(p, length, 16);
	char txt[4096];
	va_list arglist;
	va_start(arglist, fmt);
	vsnprintf(txt, sizeof(txt)-1, fmt, arglist);
	va_end(arglist);

	applog(priority, "%s\r\n%s\r\nEND\r\n", txt, strData.c_str());
}

void Log::applog(const int priority, const char *fmt, ...) {
	base::MutexLock lock(logMutex);

	char txt[4096];
	va_list arglist;
	va_start(arglist, fmt);
	vsnprintf(txt, sizeof(txt)-1, fmt, arglist);
	va_end(arglist);

	struct timespec time_stamp;
	clock_gettime(CLOCK_REALTIME, &time_stamp);
	std::string msg(txt);
	std::string line;
	std::string::size_type index = 0;

	for (;;) {
		const std::string line = StringConverter::getline(msg, index, "\r\n");
		if (line.empty()) {
			return;
		}
		LogElem elem;

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
		if (snprintf(time, sizeof(time), "%s.%04lu %s", &ascTime[0], time_stamp.tv_nsec/100000, &ascTime[20]) < 0) {
			continue;
		}
		elem.timestamp = time;

		// set message
		elem.msg = line;

		// save to deque
		if (_appLogBuffer.size() == LOG_SIZE) {
			_appLogBuffer.pop_front();
		}
		_appLogBuffer.push_back(elem);

		// log to syslog
		if (_syslogOn) {
			syslog(priority, "%s", elem.msg.c_str());
		}
#ifdef DEBUG
		std::cout << elem.msg << "\r\n";
		std::flush(std::cout);
#endif
	}
}

std::string Log::makeJSON() {
	base::JSONSerializer json;
	json.startObject();

	json.startArrayWithName("log");
	{
		base::MutexLock lock(logMutex);
		if (!_appLogBuffer.empty()) {
			for (const LogElem &elem : _appLogBuffer) {
				json.startObject();
				json.addValueString("timestamp", elem.timestamp);
				json.addValueString("msg", elem.msg);
				json.addValueNumber("prio", StringConverter::stringFormat("@#1", elem.priority));
				json.endObject();
			}
		}
	}
	json.endArray();

	json.endObject();
	return json.getString();
}
