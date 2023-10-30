/* Log.cpp

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

#define LOG_SIZE 550

static base::Mutex logMutex;

bool Log::_syslogOn = false;
bool Log::_coutLog = true;
bool Log::_logDebug = true;

Log::LogBuffer Log::_appLogBuffer;

void Log::openAppLog(const char *deamonName, const bool daemonize) {
	// initialize the logging interface
	openlog(deamonName, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	if (daemonize) {
		_coutLog = false;
	}
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

void Log::log(const int priority, const std::string &msg) {
	if ((priority & MPEGTS_TABLES) == MPEGTS_TABLES) {
		return;
	}
	if (priority == LOG_DEBUG && !_logDebug) {
		return;
	}
	// set timestamp
	struct timespec timeStamp;
	struct tm result;
	char asciiTime[100];
	clock_gettime(CLOCK_REALTIME, &timeStamp);
	localtime_r(&timeStamp.tv_sec, &result);
	std::strftime(asciiTime, sizeof(asciiTime), "%c", &result);

	// cut line to insert nsec '.000000000'
	asciiTime[19] = 0;
	const std::string time = StringConverter::stringFormat("@#1.@#2 @#3",
		&asciiTime[0], DIGIT(timeStamp.tv_nsec/100000, 4), &asciiTime[20]);

	std::string::size_type index = 0;
	base::MutexLock lock(logMutex);
	for (;;) {
		std::string line = StringConverter::getline(msg, index, "\r\n");
		if (line.empty()) {
			return;
		}

		// log to syslog
		if (_syslogOn) {
			syslog(priority, "%s", line.data());
		}

#ifdef DEBUG
		if (_coutLog) {
			std::cout << time << " " << line << std::endl;
		}
#endif

		// save to deque as last, because of std::move
		if (_appLogBuffer.size() == LOG_SIZE) {
			_appLogBuffer.pop_front();
		}
		_appLogBuffer.emplace_back(priority, std::move(line), time);
	}
}

std::string Log::makeJSON() {
	base::JSONSerializer json;
	json.startObject();

	json.startArrayWithName("log");
	{
		base::MutexLock lock(logMutex);
		if (!_appLogBuffer.empty()) {
			for (const LogElem& elem : _appLogBuffer) {
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
