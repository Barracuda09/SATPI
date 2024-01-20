/* StopWatch.h

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
#ifndef BASE_STOP_WATCH
#define BASE_STOP_WATCH BASE_STOP_WATCH

#include <Log.h>

#include <chrono>

namespace base {

class StopWatch {
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		StopWatch() {
			start();
		}

		~StopWatch() = default;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		void start() {
			_t1 = std::chrono::steady_clock::now();
		}
		void stop() {
			_t2 = std::chrono::steady_clock::now();
		}

		unsigned long getIntervalMS() const {
			const auto t = std::chrono::steady_clock::now();
			return std::chrono::duration_cast<std::chrono::milliseconds>(t - _t1).count();
		}

		unsigned long getIntervalUS() const {
			const auto t = std::chrono::steady_clock::now();
			return std::chrono::duration_cast<std::chrono::microseconds>(t - _t1).count();
		}

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

		std::chrono::steady_clock::time_point _t1;
		std::chrono::steady_clock::time_point _t2;

};

#define SW_DECL(name) base::StopWatch name
#define SW_LOG_INTERVAL_US(name, msg) SI_LOG_DEBUG(msg, name.getIntervalUS());
#define SW_LOG_INTERVAL_US_NOT_ZERO(name, msg) { const auto t = name.getIntervalUS(); if (t > 0) { SI_LOG_DEBUG(msg, t); } };
#define SW_LOG_INTERVAL_MS(name, msg) SI_LOG_DEBUG(msg, name.getIntervalMS());
#define SW_LOG_INTERVAL_MS_NOT_ZERO(name, msg) { const auto t = name.getIntervalMS(); if (t > 0) { SI_LOG_DEBUG(msg, t); } };

} // namespace base

#endif // BASE_STOP_WATCH
