/* StreamClient.h

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
		// Specifies were on the session timeout should react
		enum class SessionTimeoutCheck {
			WATCHDOG,
			FILE_DESCRIPTOR,
			TEARDOWN
		};

		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:

		StreamClient();

		virtual ~StreamClient();

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	public:

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

		/// Set the client that is sending data
		void setSocketClient(SocketClient &socket);

		///
		void setSessionTimeoutCheck(SessionTimeoutCheck sessionTimeoutCheck) {
			base::MutexLock lock(_mutex);
			_sessionTimeoutCheck = sessionTimeoutCheck;
		}

		/// For RTP stream
		SocketAttr &getRtpSocketAttr();

		/// For RTCP stream
		SocketAttr &getRtcpSocketAttr();

		/// Set the IP address of the stream output
		/// @param ipAddress specifies the IP address of the stream output
		void setIPAddressOfStream(const std::string &ipAddress) {
			base::MutexLock lock(_mutex);
			_ipAddress = ipAddress;
		}

		/// Get the IP address of the stream output
		std::string getIPAddressOfStream() const {
			base::MutexLock lock(_mutex);
			return _ipAddress;
		}

		/// Get the IP address of this socket client (Owner)
		std::string getIPAddressOfSocket() const {
			base::MutexLock lock(_mutex);
			return (_socketClient == nullptr) ? "0.0.0.0" : _socketClient->getIPAddressOfSocket();
		}

		/// Set the User-Agent of the socket client (Owner)
		void setUserAgent(const std::string &userAgent) {
			base::MutexLock lock(_mutex);
			_userAgent = userAgent;
		}

		/// Get the User-Agent of the socket client (Owner
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

		// =====================================================================
		//  -- HTTP member functions -------------------------------------------
		// =====================================================================

		/// Send HTTP/RTP_TCP data to connected client
		bool sendHttpData(const void *buf, std::size_t len, int flags);

		/// Send HTTP/RTP_TCP data to connected client
		bool writeHttpData(const struct iovec *iov, int iovcnt);

		/// Get the HTTP/RTP_TCP port of the connected client
		int getHttpSocketPort() const;

		/// Get the HTTP/RTP_TCP network send buffer size for this Socket
		int getHttpNetworkSendBufferSize() const;

		/// Set the HTTP/RTP_TCP network send buffer size for this Socket
		bool setHttpNetworkSendBufferSize(int size);

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		base::Mutex  _mutex;
		SocketClient *_socketClient;
		SessionTimeoutCheck _sessionTimeoutCheck;
		std::string  _ipAddress;
		std::time_t  _watchdog;
		unsigned int _sessionTimeout;
		std::string  _sessionID;
		std::string  _userAgent;
		int          _cseq;
		SocketAttr   _rtp;
		SocketAttr   _rtcp;
};

#endif // STREAM_CLIENT_H_INCLUDE
