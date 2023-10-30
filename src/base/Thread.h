/* Thread.h

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
#ifndef BASE_THREAD_H_INCLUDE
#define BASE_THREAD_H_INCLUDE BASE_THREAD_H_INCLUDE

#include <string>
#include <atomic>
#include <functional>

#include <pthread.h>
#include <unistd.h>

#ifndef HAS_NP_FUNCTIONS
#include <sys/prctl.h>
#endif

namespace base {

/// Thread can be use to implement thread functionality
class Thread {
	public:

		using FunctionPauseThread = std::function<bool()>;
		using FunctionThreadExecute = std::function<bool()>;

		enum class Priority {
			High,
			AboveNormal,
			Normal,
			BelowNormal,
			Idle
		};

		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:

		///
		/// @param name specifies the thread name as viewed in 'top'
		/// @param threadExecuteFunction specifies the execution function it should
		/// @return true to keep thread running and @return false will stop and then
		///  terminate this thread
		Thread(
			const std::string &name,
			FunctionThreadExecute threadExecuteFunction);

		virtual ~Thread() = default;

		// =====================================================================
		// -- static member functions ------------------------------------------
		// =====================================================================
	public:

		static std::string getThisThreadName() {
			char name[32];
#ifdef HAS_NP_FUNCTIONS
			pthread_t thread = pthread_self();
			pthread_getname_np(thread, name, sizeof(name));
#else
			prctl(PR_GET_NAME, name, 0, 0, 0);
#endif
			return name;
		}

		// =====================================================================
		// -- Other member functions -------------------------------------------
		// =====================================================================
	public:

		/// Start the Thread
		/// @return true if thread is running false if there was an error
		bool startThread();

		/// Check if this thread is started
		bool isStarted() const {
			return _state == State::Started || _state == State::Starting;
		}

		/// Check if this thread is paused
		bool isPaused() const {
			return _state == State::Paused || _state == State::Pausing;
		}

		/// Check if this thread is paused
		bool isStopped() const {
			return _state == State::Stopped || _state == State::Stopping || _state == State::Unknown;
		}

		/// Stop the running thread give 5.0 sec to stop else cancel it
		void stopThread();

		/// Pause the running thread, it will not call 'threadExecuteFunction'
		/// but will enter a sleep (150ms) loop
		void pauseThread();

		///
		void restartThread();

		/// Terminate this thread, if the thread did not stop within the
		/// timeout it will be cancelled
		void terminateThread();

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

	private:

		static void * threadEntryFunc(void *arg) {
			(static_cast<Thread *>(arg))->threadEntryBase();
			return nullptr;
		}

		void threadEntryBase();

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		enum class State {
			Unknown,
			Stopping,
			Stopped,
			Starting,
			Started,
			Pausing,
			Paused
		};
		std::atomic<State> _state;

		pthread_t        _thread;
		std::string      _name;

		FunctionThreadExecute _threadExecuteFunction;
};

} // namespace base

#endif // BASE_THREAD_H_INCLUDE
