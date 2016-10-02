/* RtcpThread.h

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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
#define RTCP_THREAD_H_INCLUDE RTCP_THREAD_H_INCLUDE

#include <FwDecl.h>
#include <base/ThreadBase.h>

#include <stdint.h>

FW_DECL_NS0(StreamClient);
FW_DECL_NS0(StreamInterface);

	/// RTSP Server
	class RtcpThread :
		public base::ThreadBase {
		public:
			// ===================================================================
			//  -- Constructors and destructor -----------------------------------
			// ===================================================================
			RtcpThread(StreamInterface &stream, bool tcp);
			virtual ~RtcpThread();

			// ===================================================================
			//  -- Other member functions ----------------------------------------
			// ===================================================================

		public:

			/// Start streaming
			/// @return true if stream is started else false on error
			bool startStreaming();

		protected:
			/// Thread function
			virtual void threadEntry();

		private:
			///
			uint8_t *get_app_packet(std::size_t *len);
			uint8_t *get_sdes_packet(std::size_t *len);
			uint8_t *get_sr_packet(std::size_t *len);

			// ===================================================================
			//  -- Data members --------------------------------------------------
			// ===================================================================

		private:

			int _clientID;
			StreamInterface &_stream;
			bool _tcpMode;
			uint8_t _sr[28];              // Sender Report (SR Packet)
			uint8_t _srb[24];             // Sender Report Block n (SR Packet)
			uint8_t _sdes[24];            // Source Description (SDES Packet)
			uint8_t _app[500];            // Application Defined packet (APP Packet)

	};

#endif // RTCP_THREAD_H_INCLUDE
