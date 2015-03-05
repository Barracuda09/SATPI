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

#include <pthread.h>

/// ThreadBase can be use to implement thread functionality
class ThreadBase {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		ThreadBase() : _run(false) {}
		virtual ~ThreadBase() {}

		/// Start the Thread
		/// @return true if thread is running false if there was an error
		bool startThread() {
			_run = true;
			return (pthread_create(&_thread, NULL, threadEntryFunc, this) == 0);
		}

		/// Cancel the running thread
		bool running() const {
			return _run;
		}
		
		/// Cancel the running thread
		void stopThread() {
			_run = false;
		}

		/// Cancel the running thread
		void cancelThread() {
			pthread_cancel(_thread);
		}

		/// Will not return until the internal thread has exited.
		void joinThread() {
			(void) pthread_join(_thread, NULL);
		}

	protected:
		/// thread entry
		virtual void threadEntry() = 0;

	private:
		static void * threadEntryFunc(void *arg) {((ThreadBase *)arg)->threadEntry(); return NULL;}

		// =======================================================================
		// Data members
		// =======================================================================
		pthread_t _thread;
		bool      _run;
}; // class ThreadBase

#endif // THREAD_BASE_H_INCLUDE
