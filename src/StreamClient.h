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

#include <string>
#include <ctime>

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
		void close();

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

		///
		std::string getMessage() const {
			base::MutexLock lock(_mutex);
			return (_httpc == nullptr) ? "" : _httpc->getMessage();
		}

		///
		std::string getIPAddress() const {
			base::MutexLock lock(_mutex);
			return (_httpc == nullptr) ? "0.0.0.0" : _httpc->getIPAddress();
		}

		/// Get the User-Agent of this client
		std::string getUserAgent() const {
			base::MutexLock lock(_mutex);
			return (_httpc == nullptr) ? "None" : _httpc->getUserAgent();
		}

		///
		std::string getSessionID() const {
			base::MutexLock lock(_mutex);
			return (_httpc == nullptr) ? "-1" : _httpc->getSessionID();
		}

		///
		int getCSeq() const {
			base::MutexLock lock(_mutex);
			return (_httpc == nullptr) ? 0 : _httpc->getCSeq();
		}

		///
		unsigned int getSessionTimeout() const {
			base::MutexLock lock(_mutex);
			return _sessionTimeout;
		}

		///
		void setSessionCanClose(bool close);

		///
		bool sessionCanClose() const  {
			base::MutexLock lock(_mutex);
			return _canClose;
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
		SocketClient *_httpc;          /// Client of this stream. Used for sending
		                               /// reply to and checking connections
		int          _streamID;        ///
		int          _clientID;
		std::time_t  _watchdog;        /// watchdog
		unsigned int _sessionTimeout;
		bool         _canClose;
		SocketAttr   _rtp;
		SocketAttr   _rtcp;
};

#endif // STREAM_CLIENT_H_INCLUDE
