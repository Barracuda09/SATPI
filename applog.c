/* applog.c

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
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#include "applog.h"
#include "utils.h"

static LogBuffer_t satip_log;
static pthread_mutex_t mutex;

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
		satip_log.elem[i].msg = NULL;
		satip_log.elem[i].priority = 0;
	}
}

void close_satip_log() {
	size_t i;
	for (i = 0; i < LOG_SIZE; ++i) {
		if (satip_log.elem[i].msg) {
			FREE_PTR(satip_log.elem[i].msg);
			satip_log.elem[i].priority = 0;
		}
	}
}

/*
 *
 */
void satiplog(int priority, const char *fmt, ...) {
	pthread_mutex_lock(&mutex);

    char txt[1024];
    va_list arglist;
    va_start(arglist, fmt);
    vsnprintf(txt, sizeof(txt), fmt, arglist);
    va_end(arglist);

	struct timespec time_stamp;
	clock_gettime(CLOCK_REALTIME, &time_stamp);
	char *line;
	size_t ptr_index = 0;
	while ((line = get_line_from(txt, &ptr_index, "\r\n")) != NULL) {
		const size_t index = satip_log.end;
		// already filled free it
		if (satip_log.elem[index].msg) {
			FREE_PTR(satip_log.elem[index].msg);
		}
		satip_log.elem[index].priority = priority;
		// set timestamp
		char asc_time[30];
		ctime_r(&time_stamp.tv_sec, asc_time);
		// remove '\n' from string
		const size_t l = strlen(asc_time);
		asc_time[l-1] = 0;
		// cut line to insert nsec '.000000000'
		asc_time[19] = 0;
		snprintf(satip_log.elem[index].timestamp, sizeof(satip_log.elem[index].timestamp), "%s.%09lu %s", &asc_time[0], time_stamp.tv_nsec, &asc_time[20]);
		// set message
		satip_log.elem[index].msg = line;
		satip_log.end = (satip_log.end + 1) % LOG_SIZE;
		if (satip_log.full == 0 && satip_log.end == satip_log.begin) {
			satip_log.full = 1;
		} else if (satip_log.full) {
			satip_log.begin = satip_log.end;
		}
		// log to syslog
		syslog(priority, "%zu %s", index, satip_log.elem[index].msg);
	}
	pthread_mutex_unlock(&mutex);
}

/*
 *
 */
char *make_log_xml() {
	char *ptr = NULL;

	// make data xml
	addString(&ptr, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
	addString(&ptr, "<loglist>\r\n");

	pthread_mutex_lock(&mutex);
	const size_t size = satip_log.full ? LOG_SIZE :
	                   ((satip_log.end < satip_log.begin) ? (satip_log.end + LOG_SIZE - satip_log.begin) :
	                                                        (satip_log.end - satip_log.begin));
	size_t i;
	size_t l = satip_log.begin;
	for (i = 0; i < size; ++i) {
		char *line = make_xml_string(satip_log.elem[l].msg);
		addString(&ptr, "<log><timestamp>%s</timestamp><msg>%s</msg><prio>%d</prio></log>\r\n", satip_log.elem[l].timestamp, line, satip_log.elem[l].priority);
		l = (l + 1) % LOG_SIZE;
		FREE_PTR(line);
	}
	pthread_mutex_unlock(&mutex);

	addString(&ptr, "</loglist>\r\n");
	return ptr;
}
