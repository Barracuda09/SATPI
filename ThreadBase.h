/* ThreadBase.h

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
#ifndef THREAD_BASE_H_INCLUDE
#define THREAD_BASE_H_INCLUDE

#include "Log.h"
#include "Mutex.h"

#include <pthread.h>
#include <unistd.h>
#include <string>

/// ThreadBase can be use to implement thread functionality
class ThreadBase {
	public:
		enum Priority {
			High,
			AboveNormal,
			Normal,
			BelowNormal,
			Idle
		};

		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		ThreadBase(const std::string &name) : _run(false), _exit(false), _name(name) {}
		virtual ~ThreadBase() {}

		/// Start the Thread
		/// @return true if thread is running false if there was an error
		bool startThread() {
			_run = true;
			_exit = false;
			const bool ok = (pthread_create(&_thread, NULL, threadEntryFunc, this) == 0);
			if (ok) {
				pthread_setname_np(_thread, _name.c_str());
			}
			return ok;
		}

		/// Is thread still running
		bool running() const {
			MutexLock lock(_mutex);
			return _run;
		}
		
		/// Stop the running thread give 1.5 sec to stop else cancel it
		void stopThread() {
			MutexLock lock(_mutex);
			_run = false;
			size_t timeout = 0;
			while (!_exit) {
				usleep(10000);
				++timeout;
				if (timeout > 150) {
					cancelThread();
					SI_LOG_DEBUG("Thread did not stop within timeout?");
					break;
				}
			}
		}

		/// Cancel the running thread
		void cancelThread() {
			pthread_cancel(_thread);
			{
				MutexLock lock(_mutex);
				_exit = true;
			}
		}

		/// Will not return until the internal thread has exited.
		void joinThread() {
			(void) pthread_join(_thread, NULL);
		}

		/// Set the thread priority of the current thread.
		/// @param priority The priority to set.
		/// @return @c true if the function was successful, otherwise @c false is
		/// returned.
		bool setPriority(const Priority priority) {
			double factor = 0.5;
			switch (priority) {
				case High:
					factor = 1.0;
					break;
				case AboveNormal:
					factor = 0.75;
					break;
				case Normal:
					factor = 0.5;
					break;
				case BelowNormal:
					factor = 0.25;
					break;
				case Idle:
					factor = 0.0;
					break;
				default:
					SI_LOG_ERROR("setPriority: Unknown case");
					return false;
			}
			int policy = 0;
			pthread_attr_t threadAttributes;
			pthread_attr_init(&threadAttributes);
			pthread_attr_getschedpolicy(&threadAttributes, &policy);
			pthread_attr_destroy(&threadAttributes);
			const int minPriority = sched_get_priority_min(policy);
			const int maxPriority = sched_get_priority_max(policy);
			const int linuxPriority = minPriority + ((maxPriority - minPriority) * factor);
			return (pthread_setschedprio(_thread, linuxPriority) == 0);
		}

	protected:
		/// thread entry
		virtual void threadEntry() = 0;

	private:
		static void * threadEntryFunc(void *arg) {((ThreadBase *)arg)->threadEntryBase(); return NULL;}

		void threadEntryBase() {
			threadEntry();
			{
				MutexLock lock(_mutex);
				_exit = true;
			}
		}
		
		// =======================================================================
		// Data members
		// =======================================================================
		pthread_t   _thread;
		bool        _run;
		bool        _exit;
		std::string _name;
		Mutex       _mutex;
}; // class ThreadBase

#endif // THREAD_BASE_H_INCLUDE
