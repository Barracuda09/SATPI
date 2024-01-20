/* StreamClient.h

   Copyright (C) 2014 - 2023 Marc Postema (mpostema09 -at- gmail.com)

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

#include <Defs.h>
#include <FwDecl.h>
#include <Unused.h>
#include <base/Mutex.h>
#include <base/XMLSupport.h>
#include <mpegts/PacketBuffer.h>
#include <socket/SocketAttr.h>
#include <socket/SocketClient.h>
#include <Stream.h>

#include <atomic>
#include <ctime>
#include <string>

FW_DECL_SP_NS1(output, StreamClient);

namespace output {

/// StreamClient defines the owner/participants of an stream
class StreamClient : public base::XMLSupport {
	public:
		// Specifies were on the session timeout should react
		enum class SessionTimeoutCheck {
			WATCHDOG,
			FILE_DESCRIPTOR,
			TEARDOWN
		};

		enum class StreamingType {
			NONE,
			HTTP,
			RTSP_UNICAST,
			RTSP_MULTICAST,
			RTP_TCP,
			FILE_SRC
		};

		// =========================================================================
		// -- Constructors and destructor ------------------------------------------
		// =========================================================================
	public:

		StreamClient(FeID feID);

		virtual ~StreamClient() = default;

		// =========================================================================
		// -- base::XMLSupport -----------------------------------------------------
		// =========================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string& xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string& xml) final;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		///
		bool processStreamingRequest(const SocketClient& client);

		///
		void startStreaming();

		///
		bool writeData(mpegts::PacketBuffer& buffer);

		///
		void writeRTCPData(const std::string& attributeDescribeString);

		///
		void teardown();

		/// Check if this client has an session timeout
		bool sessionTimeout() const;

		/// Set the client that is sending data
		void setSocketClient(SocketClient &socket);

		/// Set the session ID for this client
		/// @param specifies the the session ID to use
		void setSessionID(const std::string& sessionID) {
			base::MutexLock lock(_mutex);
			_sessionID = sessionID;
		}

		///
		std::string getSessionID() const {
			base::MutexLock lock(_mutex);
			return _sessionID;
		}

	protected:


		/// Call this if the stream should stop because of some error
		void selfDestruct();

		/// Check if this client is self destructing
		bool isSelfDestructing() const;

		// =========================================================================
		//  -- StreamClient specialization functions -------------------------------
		// =========================================================================
	public:

		///
		virtual std::string getSetupMethodReply(StreamID streamID);

		///
		virtual std::string getPlayMethodReply(
				StreamID streamID,
				const std::string& ipAddressOfServer);

		///
		virtual std::string getOptionsMethodReply() const;

		///
		virtual std::string getTeardownMethodReply() const;

		///
		/// @param fmtp specifies the specific Media Format description Parameter
		virtual std::string getSDPMediaLevelString(
				StreamID UNUSED(streamID),
				const std::string& UNUSED(fmtp)) const {
			return "";
		}

	private:

		///
		virtual bool doProcessStreamingRequest(const SocketClient& UNUSED(client)) {
			return false;
		}

		///
		virtual void doStartStreaming() {}

		///
		virtual void doTeardown() {}

		///
		virtual bool doWriteData(mpegts::PacketBuffer& UNUSED(buffer)) {
			return false;
		}

		///
		virtual void doWriteRTCPData(
				const PacketPtr& UNUSED(sr), int UNUSED(srlen),
				const PacketPtr& UNUSED(sdes), int UNUSED(sdeslen),
				const PacketPtr& UNUSED(app), int UNUSED(applen)) {}

		// =========================================================================
		//  -- RTCP member functions -----------------------------------------------
		// =========================================================================
	protected:

		///
		/// @param desc specifies the Attribute Describe String of this streamClient
		std::pair<PacketPtr, int> getAPP(const std::string &desc) const;

		///
		std::pair<PacketPtr, int> getSDES() const;

		///
		std::pair<PacketPtr, int> getSR() const;

		// =========================================================================
		//  -- HTTP member functions -----------------------------------------------
		// =========================================================================
	protected:

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

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	protected:

		base::Mutex  _mutex;
		FeID _feID;
		bool _streamActive;
		SocketClient *_socketClient;
		SessionTimeoutCheck _sessionTimeoutCheck;
		std::string _ipAddressOfStream;
		std::time_t _watchdog;
		unsigned int _sessionTimeout;
		std::string _sessionID;
		std::string _userAgent;
		int _commandSeq;
		SocketAttr _rtp;
		SocketAttr _rtcp;
		uint32_t _ssrc;
		std::atomic<uint32_t> _senderRtpPacketCnt;
		std::atomic<uint32_t> _senderOctectPayloadCnt;
		std::atomic<long> _timestamp;
		std::atomic<long> _payload;

};

}

#endif // STREAM_CLIENT_H_INCLUDE
