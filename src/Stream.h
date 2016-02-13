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
#ifdef LIBDVBCSA
#include <StreamInterfaceDecrypt.h>
#endif

#include <string>
#include <vector>
#include <cstddef>
#include <atomic>

FW_DECL_NS0(SocketClient);
FW_DECL_NS0(StreamThreadBase);
FW_DECL_NS1(input, DeviceData);
FW_DECL_NS2(input, dvb, FrontendData);
FW_DECL_NS2(decrypt, dvbapi, Client);

// @todo Forward decl
FW_DECL_NS0(Stream);
typedef std::vector<Stream *> StreamVector;

/// The class @c Stream carries all the data/information of an stream
class Stream :
	public StreamInterface,
#ifdef LIBDVBCSA
	public StreamInterfaceDecrypt,
#endif
	public base::XMLSupport {
	public:
		static const unsigned int MAX_CLIENTS;

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		Stream(int streamID, decrypt::dvbapi::Client *decrypt);

		virtual ~Stream();

#ifdef LIBDVBCSA
		// =======================================================================
		// -- StreamInterfaceDecrypt ---------------------------------------------
		// =======================================================================
	public:

		virtual bool updateInputDevice();

		virtual int getBatchCount() const;

		virtual int getBatchParity() const;

		virtual int getMaximumBatchSize() const;

		virtual void decryptBatch(bool final);

		virtual void setBatchData(unsigned char *ptr, int len, int parity, unsigned char *originalPtr);

		virtual const dvbcsa_bs_key_s *getKey(int parity) const;

		virtual bool isTableCollected(int tableID) const;

		virtual void setTableCollected(int tableID, bool collected);

		virtual const unsigned char *getTableData(int tableID) const;

		virtual void collectTableData(int streamID, int tableID, const unsigned char *data);

		virtual int getTableDataSize(int tableID) const;

		virtual void setPMT(int pid, bool set);

		virtual bool isPMT(int pid) const;

		virtual void setECMFilterData(int demux, int filter, int pid, bool set);

		virtual void getECMFilterData(int &demux, int &filter, int pid) const;

		virtual bool getActiveECMFilterData(int &demux, int &filter, int &pid) const;

		virtual bool isECM(int pid) const;

		virtual void setKey(const unsigned char *cw, int parity, int index);

		virtual void freeKeys();

		virtual void setKeyParity(int pid, int parity);

		virtual int getKeyParity(int pid) const;

		virtual void setECMInfo(
			int pid,
			int serviceID,
			int caID,
			int provID,
			int emcTime,
			const std::string &cardSystem,
			const std::string &readerName,
			const std::string &sourceName,
			const std::string &protocolName,
			int hops);
#endif

		// =======================================================================
		// -- StreamInterface ----------------------------------------------------
		// =======================================================================
	public:

		virtual int  getStreamID() const;

		virtual StreamClient &getStreamClient(std::size_t clientNr) const;

		virtual input::Device *getInputDevice() const;

		virtual uint32_t getSSRC() const;

		virtual long getTimestamp() const;

		virtual uint32_t getSPC() const;

		virtual unsigned int getRtcpSignalUpdateFrequency() const;

		virtual uint32_t getSOC() const;

		virtual void addRtpData(uint32_t byte, long timestamp);

		virtual double getRtpPayload() const;

		virtual std::string attributeDescribeString(bool &active) const;

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================
	public:

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
			_device->addDeliverySystemCount(dvbs2, dvbt, dvbt2, dvbc, dvbc2);
		}

		/// Find the clientID for the requested parameters
		bool findClientIDFor(SocketClient &socketClient,
		                     bool newSession,
		                     std::string sessionID,
		                     const std::string &method,
		                     int &clientID);

		/// Copy the connected  client data to this stream
		void copySocketClientAttr(const SocketClient &socketClient);

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

		/// Add stream data to an XML for storing or web interface
		virtual void addToXML(std::string &xml) const;

		/// Get stream data from an XML for restoring or web interface
		virtual void fromXML(const std::string &xml);

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
			return _client[clientID].getRtpSocketPort();
		}

		///
		int getRtcpSocketPort(int clientID) const {
			base::MutexLock lock(_mutex);
			return _client[clientID].getRtcpSocketPort();
		}

	protected:

	private:

		///
		void parseStreamString_L(const std::string &msg, const std::string &method);

		///
		void processPID_L(const std::string &pids, bool add);

		///
		void processStopStream_L(int clientID, bool gracefull);


		enum StreamingType {
			NONE,
			HTTP,
			RTSP
		};

		// =======================================================================
		// Data members
		// =======================================================================
		base::Mutex _mutex;               ///
		int _streamID;                    ///

		StreamingType     _streamingType; ///
		bool              _enabled;       /// is this stream enabled, could we use it?
		bool              _streamInUse;   ///
		bool              _streamActive;  ///

		StreamClient     *_client;        /// defines the participants of this stream
		                                  /// index 0 is the owner of this stream
		StreamThreadBase *_streaming;     ///
		decrypt::dvbapi::Client *_decrypt;///
		input::Device *_device;           ///
		input::dvb::FrontendData *_frontendData;///
		std::atomic<uint32_t> _ssrc;      /// synchronisation source identifier of sender
		std::atomic<uint32_t> _spc;       /// sender RTP packet count  (used in SR packet)
		std::atomic<uint32_t> _soc;       /// sender RTP payload count (used in SR packet)
		std::atomic<long> _timestamp;     ///
		std::atomic<long> _rtp_payload;   ///
		unsigned int _rtcpSignalUpdate;   ///

}; // class Stream

#endif // STREAM_H_INCLUDE