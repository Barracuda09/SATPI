/* ThreadBase.h

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#define THREAD_BASE_H_INCLUDE THREAD_BASE_H_INCLUDE

#include "Log.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE _GNU_SOURCE
#endif

#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <string>
#include <atomic>


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
		ThreadBase(std::string name) : _run(false), _exit(false), _name(name) {}
		virtual ~ThreadBase() {}

		/// Start the Thread
		/// @return true if thread is running false if there was an error
		bool startThread() {
			_run = true;
			_exit = false;
			const bool ok = (pthread_create(&_thread, nullptr, threadEntryFunc, this) == 0);
			if (ok) {
#ifdef HAS_NP_FUNCTIONS
				pthread_setname_np(_thread, _name.c_str());
#else
				prctl(PR_SET_NAME, _name.c_str(), 0, 0, 0);
#endif
			}
			return ok;
		}

		/// Is thread still running
		bool running() const {
			return _run;
		}

		///
		void terminateThread() {
			if (_run) {
				stopThread();
				joinThread();
			}
		}

		/// Stop the running thread give 5.0 sec to stop else cancel it
		void stopThread() {
			_run = false;
			size_t timeout = 0;
			while (!_exit) {
				usleep(100000);
				++timeout;
				if (timeout > 50) {
					cancelThread();
					SI_LOG_DEBUG("%s: Thread did not stop within timeout?  !!TIMEOUT!!", _name.c_str());
					break;
				}
			}
		}

		/// Cancel the running thread
		void cancelThread() {
			pthread_cancel(_thread);
			_exit = true;
		}

		/// Will not return until the internal thread has exited.
		void joinThread() {
			(void) pthread_join(_thread, nullptr);
		}

		/// This will set the threads affinity (which CPU is used).
		/// @param cpu Set threads affinity with this CPU.
		void setAffinity(int cpu) {
			if (cpu > 0 && cpu < getNumberOfProcessorsOnline()) {
				cpu_set_t cpus;
				CPU_ZERO(&cpus);
				CPU_SET(cpu, &cpus);
//				pthread_setaffinity_np(_thread, sizeof(cpu_set_t), &cpus);
				sched_setaffinity(_thread, sizeof(cpu_set_t), &cpus);
			}
		}

		/// This will get the scheduled affinity of this thread.
		/// @return @c returns the affinity of this thread.
		int getScheduledAffinity() const {
			return sched_getcpu();
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
					SI_LOG_ERROR("%s: setPriority: Unknown case", _name.c_str());
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

		/// Get the number of processors that are online (available)
		/// @return @c returns the number of processors that are online
		static int getNumberOfProcessorsOnline() {
			return sysconf(_SC_NPROCESSORS_ONLN);
		}

		/// Get the number of processors that are on this host
		/// @return @c returns the number of processors that are in this host
		static int getNumberOfProcessorsOnHost() {
			return sysconf(_SC_NPROCESSORS_CONF);
		}

	protected:
		/// thread entry
		virtual void threadEntry() = 0;

	private:
		static void * threadEntryFunc(void *arg) {((ThreadBase *)arg)->threadEntryBase(); return nullptr;}

		void threadEntryBase() {
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
			pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
			threadEntry();
			_exit = true;
		}

		// =======================================================================
		// Data members
		// =======================================================================
		pthread_t        _thread;
		std::atomic_bool _run;
		std::atomic_bool _exit;
		std::string      _name;
}; // class ThreadBase

#endif // THREAD_BASE_H_INCLUDE
