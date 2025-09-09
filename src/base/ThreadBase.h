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
#ifndef BASE_THREADBASE_H_INCLUDE
#define BASE_THREADBASE_H_INCLUDE BASE_THREADBASE_H_INCLUDE

#include <pthread.h>
#include <string>
#include <atomic>

namespace base {

/// ThreadBase can be use to implement thread functionality by inheriting
/// this base class
class ThreadBase {
	public:
		enum class Priority {
			High,
			AboveNormal,
			Normal,
			BelowNormal,
			Idle
		};

		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
		explicit ThreadBase(const std::string &name);

		virtual ~ThreadBase();

		// =========================================================================
		// -- Static member functions ----------------------------------------------
		// =========================================================================
	public:

		/// Get the number of processors that are online (available)
		/// @return @c returns the number of processors that are online
		static int getNumberOfProcessorsOnline();

		/// Get the number of processors that are on this host
		/// @return @c returns the number of processors that are in this host
		static int getNumberOfProcessorsOnHost();

		// =========================================================================
		// -- Other member functions -----------------------------------------------
		// =========================================================================
	public:

		/// Start the Thread
		/// @return true if thread is running false if there was an error
		bool startThread();

		/// Is thread still running
		bool running() const {
			return _run;
		}

		/// Terminate this thread, if the thread did not stop within the
		/// timeout it will be cancelled
		void terminateThread() {
			if (_run) {
				stopThread();
				joinThread();
			}
		}

		/// Stop the running thread give 5.0 sec to stop else cancel it
		void stopThread();

		/// Cancel the running thread
		void cancelThread();

		/// Will not return until the internal thread has exited.
		void joinThread();

		/// This will set the threads affinity (which CPU is used).
		/// @param cpu Set threads affinity with this CPU.
		void setAffinity(int cpu);

		/// This will get the scheduled affinity of this thread.
		/// @return @c returns the affinity of this thread.
		int getScheduledAffinity() const;

		/// Set the thread priority of the current thread.
		/// @param priority The priority to set.
		/// @return @c true if the function was successful, otherwise @c false is
		/// returned.
		bool setPriority(const Priority priority);

	protected:

		/// thread entry, do not leave this function
		virtual void threadEntry() = 0;

	private:
		static void * threadEntryFunc(void *arg) {(static_cast<ThreadBase *>(arg))->threadEntryBase(); return nullptr;}

		void threadEntryBase();

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

		pthread_t        _thread;
		std::atomic_bool _run;
		std::atomic_bool _exit;
		std::string      _name;
};

} // namespace base

#endif // BASE_THREADBASE_H_INCLUDE
