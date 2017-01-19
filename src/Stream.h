/* Stream.h

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef STREAM_H_INCLUDE
#define STREAM_H_INCLUDE STREAM_H_INCLUDE

#include <FwDecl.h>
#include <StreamClient.h>
#include <StreamInterface.h>
#include <base/Mutex.h>
#include <base/XMLSupport.h>
#include <input/Device.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

FW_DECL_NS0(SocketClient);
FW_DECL_NS1(output, StreamThreadBase);
FW_DECL_NS1(input, DeviceData);

FW_DECL_UP_NS1(output, StreamThreadBase);
FW_DECL_SP_NS2(decrypt, dvbapi, Client);
FW_DECL_SP_NS2(input, dvb, FrontendDecryptInterface);

FW_DECL_VECTOR_NS0(Stream);

/// The class @c Stream carries all the data/information of an stream
class Stream :
	public StreamInterface,
	public base::XMLSupport {
	public:
		static const unsigned int MAX_CLIENTS;

		enum class StreamingType {
			NONE,
			HTTP,
			RTSP,
			RTP_TCP,
			FILE
		};

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		Stream(int streamID, input::SpDevice device, decrypt::dvbapi::SpClient decrypt);

		virtual ~Stream();

		// =======================================================================
		// -- base::XMLSupport ---------------------------------------------------
		// =======================================================================

	public:
		virtual void addToXML(std::string &xml) const override;

		virtual void fromXML(const std::string &xml) override;

		// =======================================================================
		// -- StreamInterface ----------------------------------------------------
		// =======================================================================
	public:

		virtual int  getStreamID() const override;

		virtual StreamClient &getStreamClient(std::size_t clientNr) const override;

		virtual input::SpDevice getInputDevice() const override;

		virtual uint32_t getSSRC() const override;

		virtual long getTimestamp() const override;

		virtual uint32_t getSPC() const override;

		virtual unsigned int getRtcpSignalUpdateFrequency() const override;

		virtual uint32_t getSOC() const override;

		virtual void addRtpData(uint32_t byte, long timestamp) override;

		virtual double getRtpPayload() const override;

		virtual std::string attributeDescribeString(bool &active) const override;

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================
	public:

#ifdef LIBDVBCSA
		///
		input::dvb::SpFrontendDecryptInterface getFrontendDecryptInterface();
#endif

		///
		void addDeliverySystemCount(
				std::size_t &dvbs2,
				std::size_t &dvbt,
				std::size_t &dvbt2,
				std::size_t &dvbc,
				std::size_t &dvbc2) {
			base::MutexLock lock(_mutex);
			if (_enabled) {
				_device->addDeliverySystemCount(dvbs2, dvbt, dvbt2, dvbc, dvbc2);
			}
		}

		/// Find the clientID for the requested parameters
		bool findClientIDFor(SocketClient &socketClient,
		                     bool newSession,
		                     std::string sessionID,
		                     const std::string &method,
		                     int &clientID);

		/// Copy the connected  client data to this stream
		void setSocketClient(SocketClient &socketClient);

		/// Check is this stream used already
		bool streamInUse() const {
			base::MutexLock lock(_mutex);
			return _streamInUse;
		}

		/// Check is this stream enabled, can we use it?
		bool streamEnabled() const {
			base::MutexLock lock(_mutex);
			return _enabled;
		}

		/// Get the stream type of this stream
		StreamingType getStreamingType() const {
			base::MutexLock lock(_mutex);
			return _streamingType;
		}

		/// Close the stream client with clientID
		void close(int clientID);

		/// Teardown the stream client with clientID can be graceful or not
		/// graceful means it is just closed by the client itself else a time-out
		/// probably occurred.
		bool teardown(int clientID, bool gracefull);

		/// Check if there are any stream clients with a session time-out
		/// that should be closed
		void checkForSessionTimeout();

		// =======================================================================
		// -- Functions used for RTSP Server -------------------------------------
		// =======================================================================
	public:

		///
		bool processStreamingRequest(const std::string &msg, int clientID, const std::string &method);

		///
		bool update(int clientID, bool start);

		///
		int getCSeq(int clientID) const {
			base::MutexLock lock(_mutex);
			return _client[clientID].getCSeq();
		}

		///
		std::string getSessionID(int clientID) const {
			base::MutexLock lock(_mutex);
			return _client[clientID].getSessionID();
		}

		///
		unsigned int getSessionTimeout(int clientID) const {
			base::MutexLock lock(_mutex);
			return _client[clientID].getSessionTimeout();
		}

		///
		std::string getIPAddress(int clientID) const {
			base::MutexLock lock(_mutex);
			return _client[clientID].getIPAddress();
		}

		///
		int getRtpSocketPort(int clientID) const {
			base::MutexLock lock(_mutex);
			return _client[clientID].getRtpSocketAttr().getSocketPort();
		}

		///
		int getRtcpSocketPort(int clientID) const {
			base::MutexLock lock(_mutex);
			return _client[clientID].getRtcpSocketAttr().getSocketPort();
		}

	protected:

	private:

		///
		void processStopStream_L(int clientID, bool gracefull);


		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================
		base::Mutex _mutex;               ///
		int _streamID;                    ///

		StreamingType     _streamingType; ///
		bool              _enabled;       /// is this stream enabled, could we use it?
		bool              _streamInUse;   ///
		bool              _streamActive;  ///

		StreamClient     *_client;        /// defines the participants of this stream
		                                  /// index 0 is the owner of this stream
		output::UpStreamThreadBase _streaming; ///
		decrypt::dvbapi::SpClient _decrypt;///
		input::SpDevice _device;          ///
		std::atomic<uint32_t> _ssrc;      /// synchronisation source identifier of sender
		std::atomic<uint32_t> _spc;       /// sender RTP packet count  (used in SR packet)
		std::atomic<uint32_t> _soc;       /// sender RTP payload count (used in SR packet)
		std::atomic<long> _timestamp;     ///
		std::atomic<double> _rtp_payload; ///
		unsigned int _rtcpSignalUpdate;   ///

};

#endif // STREAM_H_INCLUDE
