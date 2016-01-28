/* Log.cpp

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
#include <Log.h>
#include <StringConverter.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream>

#define LOG_SIZE 350

static LogBuffer_t appLog;
static pthread_mutex_t mutex;
int syslog_on = 1;

void open_app_log() {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &attr);
}

void close_app_log() {
	// @TODO destroy mutex??
}

void binlog(int priority, const unsigned char *p, int length, const char *fmt, ...) {
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

void applog(int priority, const char *fmt, ...) {
	pthread_mutex_lock(&mutex);

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
		char asc_time[30];
		ctime_r(&time_stamp.tv_sec, asc_time);
		// remove '\n' from string
		const std::size_t l = strlen(asc_time);
		asc_time[l-1] = 0;
		// cut line to insert nsec '.000000000'
		asc_time[19] = 0;
		char time[100];
		snprintf(time, sizeof(time), "%s.%04lu %s", &asc_time[0], time_stamp.tv_nsec/100000, &asc_time[20]);
		elem.timestamp = time;

		// set message
		elem.msg = line;

		// save to deque
		if (appLog.size() == LOG_SIZE) {
			appLog.pop_front();
		}
		appLog.push_back(elem);

		// log to syslog
		if (syslog_on != 0) {
			syslog(priority, "%s", elem.msg.c_str());
		}
#ifdef DEBUG
		std::cout << elem.msg << "\r\n";
		std::flush(std::cout);
#endif
	}
	pthread_mutex_unlock(&mutex);
}

std::string make_log_xml() {
	std::string log;

	// make data xml
	log  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<loglist>\r\n";

	pthread_mutex_lock(&mutex);
	if (!appLog.empty()) {
		for (LogBuffer_t::iterator it = appLog.begin(); it != appLog.end(); ++it) {
			LogElem_t elem = *it;
			StringConverter::addFormattedString(log, "<log><timestamp>%s</timestamp><msg>%s</msg><prio>%d</prio></log>\r\n",
			                                    elem.timestamp.c_str(), StringConverter::makeXMLString(elem.msg).c_str(), elem.priority);
		}
	}
	pthread_mutex_unlock(&mutex);

	log += "</loglist>";
	return log;
}
