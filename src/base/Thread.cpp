/* Thread.h

   Copyright (C) 2014 - 2019 Marc Postema (mpostema09 -at- gmail.com)

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
#include <base/Thread.h>

#include <Log.h>

#include <chrono>
#include <thread>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE _GNU_SOURCE
#endif

#include <sys/prctl.h>
#include <unistd.h>

namespace base {

	// =========================================================================
	//  -- Constructors and destructor -----------------------------------------
	// =========================================================================
	Thread::Thread(
			const std::string &name,
			FunctionThreadExecute threadExecuteFunction) :
		_state(State::Stopped),
		_thread(0u),
		_name(name),
		_threadExecuteFunction(threadExecuteFunction) {}

	Thread::~Thread() {}

	// =========================================================================
	// -- Other member functions -----------------------------------------------
	// =========================================================================

	bool Thread::startThread() {
		if (_threadExecuteFunction != nullptr) {
			_state = State::Starting;
			return (pthread_create(&_thread, nullptr, threadEntryFunc, this) == 0);
		}
		return false;
	}

	void Thread::stopThread() {
		_state = State::Stopping;
		size_t timeout = 0u;
		while (_state != State::Stopped) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			++timeout;
			if (timeout > 50u) {
				cancelThread();
				SI_LOG_DEBUG("%s: Thread did not stop within timeout?  !!TIMEOUT!!", _name.c_str());
				break;
			}
		}
	}

	void Thread::pauseThread() {
		_state = State::Pausing;
	}

	void Thread::restartThread() {
		_state = State::Starting;
	}

	void Thread::terminateThread() {
		stopThread();
		joinThread();
	}

	void Thread::cancelThread() {
		pthread_cancel(_thread);
		_state = State::Stopped;
	}

	void Thread::joinThread() {
		(void) pthread_join(_thread, nullptr);
	}

	void Thread::setAffinity(int cpu) {
//		if (cpu > 0 && cpu < getNumberOfProcessorsOnline()) {
//			cpu_set_t cpus;
//			CPU_ZERO(&cpus);
//			CPU_SET(cpu, &cpus);
////			pthread_setaffinity_np(_thread, sizeof(cpu_set_t), &cpus);
//			sched_setaffinity(_thread, sizeof(cpu_set_t), &cpus);
//		}
	}

	int Thread::getScheduledAffinity() const {
		return sched_getcpu();
	}

	bool Thread::setPriority(const Priority priority) {
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

	void Thread::threadEntryBase() {
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
#ifdef HAS_NP_FUNCTIONS
		pthread_setname_np(_thread, _name.c_str());
#else
		prctl(PR_SET_NAME, _name.c_str(), 0, 0, 0);
#endif
		for (;;) {
			switch (_state) {
				case State::Starting:
					_state = State::Started;
					break;
				case State::Started:
					if (!_threadExecuteFunction()) {
						_state = State::Stopping;
					}
					break;
				case State::Pausing:
					_state = State::Paused;
					break;
				case State::Paused:
					// Do nothing here, just wait
					std::this_thread::sleep_for(std::chrono::milliseconds(150));
					break;
				case State::Stopping:
					_state = State::Stopped;
					break;
				case State::Stopped:
					return;
				default:
					break;
			}
		}
	}

} // namespace base
