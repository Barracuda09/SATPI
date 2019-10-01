/* StreamClient.h

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
#ifndef STREAM_CLIENT_H_INCLUDE
#define STREAM_CLIENT_H_INCLUDE STREAM_CLIENT_H_INCLUDE

#include <FwDecl.h>
#include <socket/SocketAttr.h>
#include <socket/SocketClient.h>
#include <base/Mutex.h>

#include <ctime>
#include <string>

/// StreamClient defines the owner/participants of an stream
class StreamClient {
	public:
		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================

		StreamClient();

		virtual ~StreamClient();

		// =======================================================================
		//  -- Other member functions --------------------------------------------
		// =======================================================================

		///
		void setStreamIDandClientID(int streamID, int id) {
			_streamID = streamID;
			_clientID = id;
		}

		///
		void teardown();

		/// Call this if the stream should stop because of some error
		void selfDestruct();

		/// Check if this client is self destructing
		bool isSelfDestructing() const;

		///
		void restartWatchDog();

		/// Check if this client has an session timeout
		bool sessionTimeout() const;

		///
		void setSocketClient(SocketClient &socket);

		///
		SocketAttr &getRtpSocketAttr();

		///
		SocketAttr &getRtcpSocketAttr();

		/// Set the IP address of this client
		/// @param ipAddress specifies the IP address of this client
		void setIPAddressOfStream(const std::string &ipAddress) {
			base::MutexLock lock(_mutex);
			_ipAddress = ipAddress;
		}

		/// Get the IP address of this client
		std::string getIPAddressOfStream() const {
			base::MutexLock lock(_mutex);
			return _ipAddress;
		}

		/// Set the User-Agent of this client
		void setUserAgent(const std::string &userAgent) {
			base::MutexLock lock(_mutex);
			_userAgent = userAgent;
		}

		/// Get the User-Agent of this client
		std::string getUserAgent() const {
			base::MutexLock lock(_mutex);
			return _userAgent;
		}

		/// Set the session ID for this client
		/// @param specifies the the session ID to use
		void setSessionID(const std::string &sessionID) {
			base::MutexLock lock(_mutex);
			_sessionID = sessionID;
		}

		///
		std::string getSessionID() const {
			base::MutexLock lock(_mutex);
			return _sessionID;
		}

		///
		void setCSeq(int cseq) {
			base::MutexLock lock(_mutex);
			_cseq = cseq;
		}

		///
		int getCSeq() const {
			base::MutexLock lock(_mutex);
			return _cseq;
		}

		///
		unsigned int getSessionTimeout() const {
			base::MutexLock lock(_mutex);
			return _sessionTimeout;
		}

		// =======================================================================
		//  -- HTTP member functions ---------------------------------------------
		// =======================================================================

		/// Send HTTP/RTSP data to connected client
		bool sendHttpData(const void *buf, std::size_t len, int flags);

		/// Send HTTP/RTSP data to connected client
		bool writeHttpData(const struct iovec *iov, int iovcnt);

		/// Get the HTTP/RTSP port of the connected client
		int getHttpSocketPort() const;

		/// Get the HTTP/RTSP network send buffer size for this Socket
		int getHttpNetworkSendBufferSize() const;

		/// Set the HTTP/RTSP network send buffer size for this Socket
		bool setHttpNetworkSendBufferSize(int size);

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================

	private:

		base::Mutex  _mutex;           ///
		SocketClient *_httpStream;     /// for HTTP stream
		int          _streamID;        ///
		int          _clientID;
		std::string  _ipAddress;
		std::time_t  _watchdog;        /// watchdog
		unsigned int _sessionTimeout;
		std::string  _sessionID;
		std::string  _userAgent;
		int          _cseq;            /// RTSP sequence number
		SocketAttr   _rtp;
		SocketAttr   _rtcp;
};

#endif // STREAM_CLIENT_H_INCLUDE
