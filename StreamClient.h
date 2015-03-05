/* StreamClient.h

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
#ifndef STREAM_CLIENT_H_INCLUDE
#define STREAM_CLIENT_H_INCLUDE

#include "SocketAttr.h"

#include <string>

// Forward declarations
class SocketClient;
class Stream;

/// StreamClient defines the owner/participants of an stream
class StreamClient {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		StreamClient();
		virtual ~StreamClient();

		///
		void teardown(bool gracefull);

		///
		void restartWatchDog();

		///
		bool checkWatchDogTimeout();

		///
		void copySocketClientAttr(const SocketClient &socket);

		///
		const std::string getMessage() const { return _rtspMsg; }

		///
		const std::string &getIPAddress() const  { return _ip_addr; }

		///
		const std::string &getSessionID() const         { return _sessionID; }
		void setSessionID(const std::string &sessionID) { _sessionID = sessionID; }

		///
		unsigned int getSessionTimeout() const   { return _sessionTimeout; }

		int  getRtspFD() const  { return _rtspFD; }

		void setCanClose(bool close);
		bool canClose() const           { return _canClose;  }

		///
		void setCSeq(int cseq)  { _cseq = cseq; }
		int  getCSeq() const    { return _cseq; }

		void setRtpSocketPort(int port);
		int  getRtpSocketPort() const;

		void setRtcpSocketPort(int port);
		int  getRtcpSocketPort() const;

		const struct sockaddr_in &getRtpSockAddr() const;
		const struct sockaddr_in &getRtcpSockAddr() const;

		// =======================================================================
		// Data members
		// =======================================================================
		int _clientID;
	protected:

	private:
		// =======================================================================
		// Data members
		// =======================================================================
		int          _rtspFD;          /// For sending reply to
		std::string  _rtspMsg;
		std::string  _ip_addr;         /// IP address of client
		std::string  _sessionID;
		time_t       _watchdog;        /// watchdog
		unsigned int _sessionTimeout;
		int          _cseq;
		bool         _canClose;
		SocketAttr   _rtp;
		SocketAttr   _rtcp;
}; // class StreamClient

#endif // STREAM_CLIENT_H_INCLUDE