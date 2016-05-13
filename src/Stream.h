/* Stream.h

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
#ifndef STREAM_H_INCLUDE
#define STREAM_H_INCLUDE STREAM_H_INCLUDE

#include <FwDecl.h>
#include <StreamClient.h>
#include <StreamInterface.h>
#include <base/Mutex.h>
#include <base/XMLSupport.h>
#include <input/Device.h>

#include <string>
#include <vector>
#include <cstddef>
#include <atomic>

FW_DECL_NS0(SocketClient);
FW_DECL_NS1(output, StreamThreadBase);
FW_DECL_NS1(input, DeviceData);
FW_DECL_NS2(decrypt, dvbapi, Client);
FW_DECL_NS2(input, dvb, FrontendDecryptInterface);

// @todo Forward decl
FW_DECL_NS0(Stream);
typedef std::vector<Stream *> StreamVector;

/// The class @c Stream carries all the data/information of an stream
class Stream :
	public StreamInterface,
	public base::XMLSupport {
	public:
		static const unsigned int MAX_CLIENTS;

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		Stream(int streamID, input::Device *device, decrypt::dvbapi::Client *decrypt);

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

		virtual input::Device *getInputDevice() const override;

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
		input::dvb::FrontendDecryptInterface *getFrontendDecryptInterface();
#endif

		/// This will read the frontend information for this stream
		void setFrontendInfo(const std::string &fe,
		                     const std::string &dvr,
		                     const std::string &dmx) {
			base::MutexLock lock(_mutex);
			_device->setFrontendInfo(fe, dvr, dmx);
		}

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

		/// Close the stream client with clientID
		void close(int clientID);

		/// Teardown the stream client with clientID can be graceful or not
		/// graceful means it is just closed by the client itself else a time-out
		/// probably occurred.
		bool teardown(int clientID, bool gracefull);

		/// Check if there are any stream clients with a time-out that should be
		/// closed
		void checkStreamClientsWithTimeout();

		// =======================================================================
		// -- Functions used for RTSP Server -------------------------------------
		// =======================================================================
	public:

		///
		bool processStream(const std::string &msg, int clientID, const std::string &method);

		///
		bool update(int clientID);

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


		enum class StreamingType {
			NONE,
			HTTP,
			RTSP,
			FILE
		};

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
		output::StreamThreadBase *_streaming; ///
		decrypt::dvbapi::Client *_decrypt;///
		input::Device *_device;           ///
		std::atomic<uint32_t> _ssrc;      /// synchronisation source identifier of sender
		std::atomic<uint32_t> _spc;       /// sender RTP packet count  (used in SR packet)
		std::atomic<uint32_t> _soc;       /// sender RTP payload count (used in SR packet)
		std::atomic<long> _timestamp;     ///
		std::atomic<double> _rtp_payload;   ///
		unsigned int _rtcpSignalUpdate;   ///

}; // class Stream

#endif // STREAM_H_INCLUDE
