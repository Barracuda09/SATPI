/* StreamThreadBase.h

   Copyright (C) 2014 - 2018 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef OUTPUT_STREAMTHREADBASE_H_INCLUDE
#define OUTPUT_STREAMTHREADBASE_H_INCLUDE OUTPUT_STREAMTHREADBASE_H_INCLUDE

#include <FwDecl.h>
#include <base/ThreadBase.h>
#include <mpegts/PacketBuffer.h>

#include <atomic>
#include <chrono>

FW_DECL_NS0(StreamClient);
FW_DECL_NS0(StreamInterface);

FW_DECL_UP_NS1(output, StreamThreadBase);

namespace output {

	/// Streaming thread
	class StreamThreadBase :
		public base::ThreadBase {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================

			StreamThreadBase(
				const std::string &protocol,
				StreamInterface &stream);

			virtual ~StreamThreadBase();

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

			/// Start streaming
			/// @return true if stream is started else false on error
			virtual bool startStreaming();

			/// Pause streaming
			/// @return true if stream is paused else false on error
			virtual bool pauseStreaming(int clientID);

			/// Restart streaming
			/// @return true if stream is restarted else false on error
			virtual bool restartStreaming(int clientID);

		protected:

			/// @see ThreadBase
			virtual void threadEntry() override {}

			/// This function will read data from the input device
			/// @param client specifies were it should be sended to
			virtual void readDataFromInputDevice(StreamClient &client);

			/// send the TS packets to an output device
			virtual bool writeDataToOutputDevice(mpegts::PacketBuffer &buffer,
				StreamClient &client) = 0;

			///
			virtual int getStreamSocketPort(int clientID) const = 0;

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		protected:

			enum class State {
				Running,
				Pause,
				Paused,
			};

			StreamInterface &_stream;
			std::string _protocol;
			std::atomic<State> _state;

		private:

			static constexpr size_t MAX_BUF = 100;
			mpegts::PacketBuffer _tsBuffer[MAX_BUF];
			size_t _writeIndex;
			size_t _readIndex;
			unsigned long _sendInterval;
			std::chrono::steady_clock::time_point _t1;
			std::chrono::steady_clock::time_point _t2;

	};

} // namespace output

#endif // OUTPUT_STREAMTHREADBASE_H_INCLUDE
