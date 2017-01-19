/* Mutex.h

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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

#include <pthread.h>
#include <unistd.h>
#include <sys/prctl.h>

namespace base {

/// The class @c Mutex can be locked exclusively per thread
/// to guarantee thread safety.
class Mutex {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		Mutex() {
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&_mutex, &attr);
		}
		virtual ~Mutex() {
			pthread_mutex_destroy(&_mutex);
		}

		/// Exclusively lock the @c Mutex per thread.
		void lock() const {
			pthread_mutex_lock(&_mutex);
		}

		/// Exclusively try to lock the @c Mutex per thread for a maximum time of
		/// timeout msec.
		/// @param timeout specifies the time, in msec, to try locking this mutex.
		bool tryLock(unsigned int timeout) const {
			bool locked = true;
			while (pthread_mutex_trylock(&_mutex) != 0) {
				usleep(1000);
				--timeout;
				if (timeout == 0) {
					locked = false;
					break;
				}
			}
			return locked;
		}

		/// Unlocking of the @c Mutex.
		void unlock() const {
			pthread_mutex_unlock(&_mutex);
		}

	protected:

	private:
		// =======================================================================
		// Data members
		// =======================================================================
		mutable pthread_mutex_t _mutex;
}; // class Mutex

/// The class @c MutexLock can be used for @c Mutex to 'auto' lock and unlock
class MutexLock {
	public:
		static const unsigned int TIMEOUT_15SEC = 15000;

		// =======================================================================
		// Constructors and destructor
		// =======================================================================
/*
		MutexLock(const Mutex &mutex) : _mutex(mutex) {
			_mutex.lock();
		}
*/
		MutexLock(const Mutex &mutex, unsigned int timeout = TIMEOUT_15SEC) : _mutex(mutex) {
			if (!_mutex.tryLock(timeout)) {
				char name[32];
#ifdef HAS_NP_FUNCTIONS
				pthread_t thread = pthread_self();
				pthread_getname_np(thread, name, sizeof(name));
#else
				prctl(PR_GET_NAME, name, 0, 0, 0);
#endif
				SI_LOG_ERROR("Mutex in %s did not lock within timeout?  !!DEADLOCK!!", name);
			}
		}

		virtual ~MutexLock() {
			_mutex.unlock();
		}

	protected:

	private:
		// =======================================================================
		// Data members
		// =======================================================================
		const Mutex &_mutex;
};

} // namespace base

#endif // BASE_MUTEX_H_INCLUDE
