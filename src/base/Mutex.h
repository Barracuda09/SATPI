/* Mutex.h

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
#ifndef BASE_MUTEX_H_INCLUDE
#define BASE_MUTEX_H_INCLUDE BASE_MUTEX_H_INCLUDE

#include <Log.h>
#include <Utils.h>
#include <base/Thread.h>

#include <chrono>
#include <thread>

#include <pthread.h>

namespace base {

/// The class @c Mutex can be locked exclusively per thread
/// to guarantee thread safety.
class Mutex {
		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:

		Mutex() {
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&_mutex, &attr);
		}

		virtual ~Mutex() {
			pthread_mutex_destroy(&_mutex);
		}

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	public:

		/// Exclusively lock the @c Mutex per thread.
		void lock() const {
			pthread_mutex_lock(&_mutex);
		}

		/// Exclusively try to lock the @c Mutex per thread for a maximum time of
		/// timeout msec.
		/// @param timeout specifies the time, in msec, to try locking this mutex.
		bool tryLock(unsigned int timeout) const {
			while (pthread_mutex_trylock(&_mutex) != 0) {
				if (timeout == 0) {
					return false;
				}
				std::this_thread::sleep_for(std::chrono::microseconds(1000));
				--timeout;
			}
			return true;
		}

		/// Unlocking of the @c Mutex.
		bool unlock() const {
			return pthread_mutex_unlock(&_mutex) == 0;
		}

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		mutable pthread_mutex_t _mutex;
};

/// The class @c MutexLock can be used for @c Mutex to 'auto' lock and unlock
class MutexLock {
		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:

		MutexLock(const Mutex &mutex, unsigned int timeout = TIMEOUT_15SEC) : _mutex(mutex) {
			if (!_mutex.tryLock(timeout)) {
				SI_LOG_ERROR("Mutex in @#1 did not lock within timeout?  !!DEADLOCK!!",
					Thread::getThisThreadName());
				Utils::createBackTrace("MutexLock");
			}
		}

		virtual ~MutexLock() {
			if (!_mutex.unlock()) {
				SI_LOG_ERROR("Mutex in @#1 not unlocked!!", Thread::getThisThreadName());
			}
		}

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		static constexpr unsigned int TIMEOUT_15SEC = 15000;
		const Mutex &_mutex;
};

} // namespace base

#endif // BASE_MUTEX_H_INCLUDE
