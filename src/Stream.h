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
#include <ChannelData.h>
#include <StreamClient.h>
#include <StreamInterface.h>
#include <StreamProperties.h>
#include <base/Mutex.h>
#include <base/XMLSupport.h>
#include <input/Device.h>
#ifdef LIBDVBCSA
#include <StreamInterfaceDecrypt.h>
#endif

#include <string>
#include <vector>

FW_DECL_NS0(SocketClient);
FW_DECL_NS0(StreamThreadBase);
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

		/// @see StreamInterfaceDecrypt
		virtual bool updateInputDevice();

		/// @see StreamInterfaceDecrypt
		virtual int getBatchCount() const;

		/// @see StreamInterfaceDecrypt
		virtual int getBatchParity() const;

		/// @see StreamInterfaceDecrypt
		virtual int getMaximumBatchSize() const;

		/// @see StreamInterfaceDecrypt
		virtual void decryptBatch(bool final);

		/// @see StreamInterfaceDecrypt
		virtual void setBatchData(unsigned char *ptr, int len, int parity, unsigned char *originalPtr);

		/// @see StreamInterfaceDecrypt
		virtual const dvbcsa_bs_key_s *getKey(int parity) const;

		/// @see StreamInterfaceDecrypt
		virtual bool isTableCollected(int tableID) const;

		/// @see StreamInterfaceDecrypt
		virtual void setTableCollected(int tableID, bool collected);

		/// @see StreamInterfaceDecrypt
		virtual const unsigned char *getTableData(int tableID) const;

		/// @see StreamInterfaceDecrypt
		virtual void collectTableData(int streamID, int tableID, const unsigned char *data);

		/// @see StreamInterfaceDecrypt
		virtual int getTableDataSize(int tableID) const;

		/// @see StreamInterfaceDecrypt
		virtual void setPMT(int pid, bool set);

		/// @see StreamInterfaceDecrypt
		virtual bool isPMT(int pid) const;

		/// @see StreamInterfaceDecrypt
		virtual void setECMFilterData(int demux, int filter, int pid, bool set);

		/// @see StreamInterfaceDecrypt
		virtual void getECMFilterData(int &demux, int &filter, int pid) const;

		/// @see StreamInterfaceDecrypt
		virtual bool getActiveECMFilterData(int &demux, int &filter, int &pid) const;

		/// @see StreamInterfaceDecrypt
		virtual bool isECM(int pid) const;

		/// @see StreamInterfaceDecrypt
		virtual void setKey(const unsigned char *cw, int parity, int index);

		/// @see StreamInterfaceDecrypt
		virtual void freeKeys();

		/// @see StreamInterfaceDecrypt
		virtual void setKeyParity(int pid, int parity);

		/// @see StreamInterfaceDecrypt
		virtual int getKeyParity(int pid) const;

		/// @see StreamInterfaceDecrypt
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

		/// @see StreamInterface
		virtual int  getStreamID() const;

		/// @see StreamInterface
		virtual StreamClient &getStreamClient(std::size_t clientNr) const;

		/// @see StreamInterface
		virtual input::Device *getInputDevice() const;

		/// @see StreamInterface
		virtual uint32_t getSSRC() const;

		/// @see StreamInterface
		virtual long getTimestamp() const;

		/// @see StreamInterface
		virtual uint32_t getSPC() const;

		/// @see StreamInterface
		virtual unsigned int getRtcpSignalUpdateFrequency() const;

		/// @see StreamInterface
		virtual uint32_t getSOC() const;

		/// @see StreamInterface
		virtual void addRtpData(uint32_t byte, long timestamp);

		/// @see StreamInterface
		virtual double getRtpPayload() const;

		/// @see StreamInterface
		virtual std::string attributeDescribeString(bool &active) const;

		/// @see StreamInterface
		virtual void setFrontendMonitorData(fe_status_t status, uint16_t strength, uint16_t snr,
		                            uint32_t ber, uint32_t ublocks);

		/// @see StreamInterface
		virtual void addPIDData(int pid, uint8_t cc);

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================

		/// Add the frontend paths to connect to this stream
		void addFrontendPaths(const std::string &fe,
		                      const std::string &dvr,
		                      const std::string &dmx) {
			base::MutexLock lock(_mutex);
			_frontend->addFrontendPaths(fe, dvr, dmx);
		}

		/// This will read the frontend information for this stream
		void setFrontendInfo() {
			base::MutexLock lock(_mutex);
			_frontend->setFrontendInfo();
		}

		/// Get the amount of delivery systems of this stream
		std::size_t getDeliverySystemSize() const {
			base::MutexLock lock(_mutex);
			return _frontend->getDeliverySystemSize();
		}

		/// Get the possible delivery systems of this stream
		const fe_delivery_system_t *getDeliverySystem() const {
			base::MutexLock lock(_mutex);
			return _frontend->getDeliverySystem();
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
		base::Mutex       _mutex;         ///
		StreamingType     _streamingType; ///
		bool              _enabled;       /// is this stream enabled, could we use it?
		bool              _streamInUse;   ///
		StreamClient     *_client;        /// defines the participants of this stream
		                                  /// index 0 is the owner of this stream
		StreamThreadBase *_streaming;     ///
		decrypt::dvbapi::Client *_decrypt;///
		StreamProperties _properties;     ///
		input::Device *_frontend;         ///
}; // class Stream

#endif // STREAM_H_INCLUDE