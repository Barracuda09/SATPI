/* RtcpThread.h

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
#ifndef RTCP_THREAD_H_INCLUDE
#define RTCP_THREAD_H_INCLUDE

#include "ThreadBase.h"

#include <stdint.h>

#include <linux/dvb/frontend.h>

// forward declaration
class StreamClient;
class StreamProperties;

/// RTSP Server
class RtcpThread :
		public ThreadBase {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		RtcpThread(StreamClient *clients, StreamProperties &properties);
		virtual ~RtcpThread();

		/// Start streaming
		/// @return true if stream is started else false on error
		bool startStreaming(int fd_fe);

		/// will return when thread is stopped
		void stopStreaming(int clientID);

	protected:
		/// Thread function
		virtual void threadEntry();

	private:
		///
		uint8_t *get_app_packet(size_t *len);
		uint8_t *get_sdes_packet(size_t *len);
		uint8_t *get_sr_packet(size_t *len);

		///
		void monitorFrontend(bool showStatus);

		// =======================================================================
		// Data members
		// =======================================================================
		int              _socket_fd;   //
		StreamClient     *_clients;    //
		StreamProperties &_properties; //
		int              _fd_fe;       //

		uint8_t  _sr[28];              // Sender Report (SR Packet)
		uint8_t  _srb[24];             // Sender Report Block n (SR Packet)
		uint8_t  _sdes[24];            // Source Description (SDES Packet)
		uint8_t  _app[500];            // Application Defined packet (APP Packet)
}; // class RtcpThread

#endif // RTCP_THREAD_H_INCLUDE