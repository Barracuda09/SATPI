/* RtpThread.h

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
#ifndef RTP_THREAD_H_INCLUDE
#define RTP_THREAD_H_INCLUDE

#include "ThreadBase.h"
#include "Mutex.h"

#include <poll.h>
#include <stdint.h>

#define RTP_HEADER_LEN  12

#define MTU             1500
#define TS_PACKET_SIZE  188

// forward declaration
class StreamClient;
class StreamProperties;
class DvbapiClient;

/// RTSP Server
class RtpThread :
		public ThreadBase {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		RtpThread(StreamClient *clients, StreamProperties &properties, DvbapiClient *dvbapi);
		virtual ~RtpThread();

		/// Start streaming
		/// @return true if stream is started else false on error
		bool startStreaming(int fd_dvr);

		/// will return when thread is stopped
		void stopStreaming(int clientID);

		/// Pause streaming
		bool pauseStreaming(int clientID);

		/// Restart streaming
		bool restartStreaming(int clientID);

	protected:
		/// Thread function
		virtual void threadEntry();

		///
		int open_dvr(const std::string &path);

	private:
		long getmsec();
		
		enum State {
			Running,
			Pause,
			Paused,
		};
		
		State getState() const      { MutexLock lock(_mutex); return _state; }
		void  setState(State state) { MutexLock lock(_mutex); _state = state; }

		// =======================================================================
		// Data members
		// =======================================================================
		int           _socket_fd;      //
		StreamClient  *_clients;       //
		unsigned char _buffer[MTU];    // RTP/UDP buffer
		unsigned char *_bufPtr;        // RTP/UDP buffer pointer
		long          _send_interval;  // RTP interval time (100ms)
		uint16_t      _cseq;           // RTP sequence number
		StreamProperties &_properties; //
		struct pollfd _pfd[1];         //
		Mutex         _mutex;          //
		State         _state;          //

		DvbapiClient  *_dvbapi;        //

}; // class RtpThread

#endif // RTP_THREAD_H_INCLUDE