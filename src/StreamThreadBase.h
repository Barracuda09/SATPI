/* StreamThreadBase.h

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef STREAM_THREAD_BASE_H_INCLUDE
#define STREAM_THREAD_BASE_H_INCLUDE STREAM_THREAD_BASE_H_INCLUDE

#include "ThreadBase.h"
#include "TSPacketBuffer.h"

#include <poll.h>
#include <atomic>

#define MAX_BUF 150

// forward declaration
class StreamClient;
class StreamProperties;
class DvbapiClient;

/// Streaming thread
class StreamThreadBase :
		public ThreadBase {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		StreamThreadBase(std::string protocol, StreamClient *clients, StreamProperties &properties, DvbapiClient *dvbapi);

		virtual ~StreamThreadBase();

		/// Start streaming
		/// @param fd_dvr
		/// @return true if stream is started else false on error
		virtual bool startStreaming(int fd_dvr);

		/// Start streaming should be implemented by the subclasses that needs an frontend monitor.
		/// @param fd_dvr
		/// @param fd_fe
		/// @return true if stream is started else false on error
		virtual bool startStreaming(int /*fd_dvr*/, int /*fd_fe*/) { return false; }

		/// Pause streaming
		/// @return true if stream is paused else false on error
		virtual bool pauseStreaming(int clientID);

		/// Restart streaming
		/// @return true if stream is restarted else false on error
		virtual bool restartStreaming(int fd_dvr, int clientID);

	protected:
		/// @see ThreadBase
		virtual void threadEntry() = 0;

		/// This function will poll the DVR (_pfd)
		/// @param client
		virtual void pollDVR(const StreamClient &client);

		/// send the TS packets to client
		virtual void sendTSPacket(TSPacketBuffer &buffer, const StreamClient &client) = 0;

		///
		virtual int getStreamSocketPort(int clientID) const = 0;

		///
		bool readFullTSPacket(TSPacketBuffer &buffer);

		// =======================================================================
		// Data members
		// =======================================================================
	protected:
		enum State {
			Running,
			Pause,
			Paused,
		};

		StreamProperties   &_properties;     //
		StreamClient       *_clients;        //
		std::string         _protocol;       //
		std::atomic<State>  _state;          //

		// =======================================================================
		// Data members
		// =======================================================================
	private:
		TSPacketBuffer   _tsBuffer[MAX_BUF]; //
		size_t           _writeIndex;        //
		size_t           _readIndex;         //
		struct pollfd    _pfd[1];            //
		DvbapiClient     *_dvbapi;           //

}; // class StreamThreadBase

#endif // STREAM_THREAD_BASE_H_INCLUDE