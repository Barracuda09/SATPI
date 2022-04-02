/* StreamThreadBase.h

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
#include <Unused.h>
#include <base/Thread.h>
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
		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:

		StreamThreadBase(
			const std::string &protocol,
			StreamInterface &stream);

		virtual ~StreamThreadBase();

		// =====================================================================
		//  -- base::ThreadBase ------------------------------------------------
		// =====================================================================
	protected:

		/// @see ThreadBase
		virtual void threadEntry() final;

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	public:

		/// Start streaming
		/// @param clientID specifies which client should start
		/// @return true if stream is started else false on error
		bool startStreaming(int clientID);

		/// Pause streaming
		/// @param clientID specifies which client should pause
		/// @return true if stream is paused else false on error
		bool pauseStreaming(int clientID);

		/// Restart streaming
		/// @param clientID specifies which client should restart
		/// @return true if stream is restarted else false on error
		bool restartStreaming(int clientID);

	protected:

		/// Send the TS packets to an output device
		virtual bool writeDataToOutputDevice(
			mpegts::PacketBuffer &UNUSED(buffer),
			StreamClient &UNUSED(client)) { return false; };

		/// Returns the socket port for the specified client
		/// @param clientID specifies which client the port id requested
		/// @return the socket port for ex. to data send to
		virtual int getStreamSocketPort(int UNUSED(clientID)) const { return 0; }

	private:

		/// Specialization for @see startStreaming
		virtual void doStartStreaming(int UNUSED(clientID)) {}

		/// Specialization for @see pauseStreaming
		virtual void doPauseStreaming(int UNUSED(clientID)) {}

		/// Specialization for @see restartStreaming
		virtual void doRestartStreaming(int UNUSED(clientID)) {}

		/// This function will read data from the input device
		/// @param client specifies were it should be sended to
		void readDataFromInputDevice(StreamClient &client);

		/// Thread execute function @see base::Thread should @return true to
		/// keep thread running and @return false will stop and then terminate this thread
		bool threadExecuteDeviceMonitor();

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	protected:

		enum class State {
			Running,
			Pause,
			Paused,
		};

		StreamInterface &_stream;
		std::string _protocol;
		std::atomic<State> _state;
		std::atomic_bool _signalLock;
		int _clientID;
		uint16_t _cseq;

	private:

		base::Thread _threadDeviceMonitor;

		static constexpr size_t MAX_BUF = 100;
		mpegts::PacketBuffer _tsBuffer[MAX_BUF];
		mpegts::PacketBuffer _tsEmpty;
		size_t _writeIndex;
		size_t _readIndex;
		unsigned long _sendInterval;
		std::chrono::steady_clock::time_point _t1;
		std::chrono::steady_clock::time_point _t2;
};

} // namespace output

#endif // OUTPUT_STREAMTHREADBASE_H_INCLUDE
