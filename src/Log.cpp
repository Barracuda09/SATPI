/* Log.cpp

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#include "Log.h"
#include "StringConverter.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream>

static LogBuffer_t satip_log;
static pthread_mutex_t mutex;
int syslog_on = 1;

/*
 *
 */
void open_satip_log() {
	pthread_mutexattr_t attr;
	size_t i;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &attr);

	satip_log.begin = 0;
	satip_log.end = 0;
	satip_log.full = 0;
	for (i = 0; i < LOG_SIZE; ++i) {
		satip_log.elem[i].priority = 0;
	}
}

void close_satip_log() {
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
		const size_t index = satip_log.end;
		satip_log.elem[index].priority = priority;
		// set timestamp
		char asc_time[30];
		ctime_r(&time_stamp.tv_sec, asc_time);
		// remove '\n' from string
		const size_t l = strlen(asc_time);
		asc_time[l-1] = 0;
		// cut line to insert nsec '.000000000'
		asc_time[19] = 0;
		char time[100];
		snprintf(time, sizeof(time), "%s.%09lu %s", &asc_time[0], time_stamp.tv_nsec, &asc_time[20]);
		satip_log.elem[index].timestamp = time;
		// set message
		satip_log.elem[index].msg = line;
		satip_log.end = (satip_log.end + 1) % LOG_SIZE;
		if (satip_log.full == 0 && satip_log.end == satip_log.begin) {
			satip_log.full = 1;
		} else if (satip_log.full) {
			satip_log.begin = satip_log.end;
		}
		// log to syslog
		if (syslog_on != 0) {
			syslog(priority, "%zu %s", index, satip_log.elem[index].msg.c_str());
		}
#ifdef DEBUG
		std::cout << satip_log.elem[index].msg << "\r\n";
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
	const size_t size = satip_log.full ? LOG_SIZE :
	                   ((satip_log.end < satip_log.begin) ? (satip_log.end + LOG_SIZE - satip_log.begin) :
	                                                        (satip_log.end - satip_log.begin));

	size_t i;
	size_t l = satip_log.begin;
	for (i = 0; i < size; ++i) {
		std::string line = StringConverter::makeXMLString(satip_log.elem[l].msg);
		const size_t size = line.size() + satip_log.elem[l].timestamp.size() + 100;
		char *tmp = new char[size+1];
		snprintf(tmp, size, "<log><timestamp>%s</timestamp><msg>%s</msg><prio>%d</prio></log>\r\n", satip_log.elem[l].timestamp.c_str(), line.c_str(), satip_log.elem[l].priority);
		log += tmp;
		delete[] tmp;
		l = (l + 1) % LOG_SIZE;
	}
	pthread_mutex_unlock(&mutex);

	log += "</loglist>";
	return log;
}
