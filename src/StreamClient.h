/* StreamClient.h

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
#ifndef STREAM_CLIENT_H_INCLUDE
#define STREAM_CLIENT_H_INCLUDE STREAM_CLIENT_H_INCLUDE

#include <FwDecl.h>
#include <socket/SocketAttr.h>
#include <socket/SocketClient.h>
#include <base/Mutex.h>

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
		void setClientID(int id) {
			_clientID = id;
		}

		///
		void teardown(bool gracefull);

		///
		void restartWatchDog();

		///
		bool checkWatchDogTimeout();

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

		///
		std::string getSessionID() const {
			base::MutexLock lock(_mutex);
			return _sessionID;
		}

		///
		void setSessionID(const std::string &sessionID) {
			base::MutexLock lock(_mutex);
			_sessionID = sessionID;
		}

		///
		unsigned int getSessionTimeout() const {
			base::MutexLock lock(_mutex);
			return _sessionTimeout;
		}

		///
		void setCanClose(bool close);

		///
		bool canClose() const  {
			base::MutexLock lock(_mutex);
			return _canClose;
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

		// =======================================================================
		//  -- HTTP member functions ---------------------------------------------
		// =======================================================================

		///
		bool sendHttpData(const void *buf, std::size_t len, int flags);

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================

	private:

		base::Mutex  _mutex;           ///
		SocketClient *_httpc;          /// Client of this stream. Used for sending
		                               /// reply to and checking connections
		int          _clientID;
		std::string  _sessionID;       ///
		time_t       _watchdog;        /// watchdog
		unsigned int _sessionTimeout;
		int          _cseq;            /// RTSP sequence number
		bool         _canClose;
		SocketAttr   _rtp;
		SocketAttr   _rtcp;
};

#endif // STREAM_CLIENT_H_INCLUDE
