/* Mutex.h

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
#ifndef MUTEX_H_INCLUDE
#define MUTEX_H_INCLUDE

#include <pthread.h>

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
			pthread_mutex_init(&mutex, &attr);
		}
		virtual ~Mutex() {
			pthread_mutex_destroy(&mutex);
		}
		
		/// Exclusively lock the @c Mutex per thread.
		void lock() const {
			pthread_mutex_lock(&mutex);

		}
		
		// @TODO make tryLock function ?

		/// Unlocking of the @c Mutex.
		void unlock() const {
			pthread_mutex_unlock(&mutex);
		}

	protected:

	private:
		// =======================================================================
		// Data members
		// =======================================================================
		mutable pthread_mutex_t mutex;
}; // class Mutex

/// The class @c MutexLock can be used for @c Mutex to 'auto' lock and unlock
class MutexLock {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		MutexLock(const Mutex &mutex) : _mutex(mutex) { _mutex.lock(); }
		virtual ~MutexLock() { _mutex.unlock();	}
	protected:

	private:
		// =======================================================================
		// Data members
		// =======================================================================
		const Mutex &_mutex;
}; // class Mutex

#endif // MUTEX_H_INCLUDE
