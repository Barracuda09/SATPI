/* ThreadBase.h

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
#include <base/ThreadBase.h>

#include <Log.h>

#include <chrono>
#include <thread>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE _GNU_SOURCE
#endif

#ifndef HAS_NP_FUNCTIONS
#include <sys/prctl.h>
#endif

#include <unistd.h>

namespace base {

	// =========================================================================
	//  -- Constructors and destructor -----------------------------------------
	// =========================================================================
	ThreadBase::ThreadBase(const std::string &name) :
		_thread(0u),
		_run(false),
		_exit(false),
		_name(name) {}

	ThreadBase::~ThreadBase() {}

	// =========================================================================
	// -- Static member functions ----------------------------------------------
	// =========================================================================

	int ThreadBase::getNumberOfProcessorsOnline() {
		return sysconf(_SC_NPROCESSORS_ONLN);
	}

	int ThreadBase::getNumberOfProcessorsOnHost() {
		return sysconf(_SC_NPROCESSORS_CONF);
	}

	// =========================================================================
	// -- Other member functions -----------------------------------------------
	// =========================================================================

	bool ThreadBase::startThread() {
		_run = true;
		_exit = false;
		return (pthread_create(&_thread, nullptr, threadEntryFunc, this) == 0);
	}

	void ThreadBase::stopThread() {
		_run = false;
		size_t timeout = 0u;
		while (!_exit) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			++timeout;
			if (timeout > 50u) {
				cancelThread();
				SI_LOG_DEBUG("@#1: Thread did not stop within timeout?  !!TIMEOUT!!", _name);
				break;
			}
		}
	}

	void ThreadBase::cancelThread() {
		pthread_cancel(_thread);
		_exit = true;
	}

	void ThreadBase::joinThread() {
		(void) pthread_join(_thread, nullptr);
	}

	void ThreadBase::setAffinity(int cpu) {
		if (cpu > 0 && cpu < getNumberOfProcessorsOnline()) {
			cpu_set_t cpus;
			CPU_ZERO(&cpus);
			CPU_SET(cpu, &cpus);
//			pthread_setaffinity_np(_thread, sizeof(cpu_set_t), &cpus);
			sched_setaffinity(_thread, sizeof(cpu_set_t), &cpus);
		}
	}

	int ThreadBase::getScheduledAffinity() const {
		return sched_getcpu();
	}

	bool ThreadBase::setPriority(const Priority priority) {
		double factor = 0.5;
		switch (priority) {
			case Priority::High:
				factor = 1.0;
				break;
			case Priority::AboveNormal:
				factor = 0.75;
				break;
			case Priority::Normal:
				factor = 0.5;
				break;
			case Priority::BelowNormal:
				factor = 0.25;
				break;
			case Priority::Idle:
				factor = 0.0;
				break;
			default:
				SI_LOG_ERROR("@#1: setPriority: Unknown case", _name);
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

	void ThreadBase::threadEntryBase() {
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
#ifdef HAS_NP_FUNCTIONS
		pthread_setname_np(_thread, _name.data());
#else
		prctl(PR_SET_NAME, _name.data(), 0, 0, 0);
#endif
		try {
			threadEntry();
			_exit = true;
		} catch (...) {
			SI_LOG_ERROR("@#1: Catched an exception", _name);
			_exit = true;
			throw;
		}
	}

} // namespace base
