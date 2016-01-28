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
#include <StreamProperties.h>
#include <base/Mutex.h>
#include <base/XMLSupport.h>
#include <input/Device.h>

#include <string>

FW_DECL_NS0(SocketClient);
FW_DECL_NS0(StreamThreadBase);
FW_DECL_NS2(decrypt, dvbapi, Client);

/// The class @c StreamInterface is an interface to an @c Stream
class StreamInterface {
	public:
		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		StreamInterface() {}

		virtual ~StreamInterface() {}

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================

		/// Get the streamID of this stream
		virtual int getStreamID() const = 0;

		///
		virtual StreamClient &getStreamClient(std::size_t clientNr) const = 0;

		///
		virtual input::Device *getInputDevice() const = 0;
		
		virtual uint32_t getSSRC() const = 0;

		///
		virtual long getTimestamp() const = 0;

		///
		virtual uint32_t getSPC() const = 0;

		/// The Frontend Monitor update interval
		virtual unsigned int getRtcpSignalUpdateFrequency() const = 0;

		///
		virtual uint32_t getSOC() const  = 0;

		///
		virtual void addRtpData(uint32_t byte, long timestamp)  = 0;

		///
		virtual double getRtpPayload() const = 0;

		/// Set the frontend  status information like strength, snr etc.
		virtual void setFrontendMonitorData(fe_status_t status, uint16_t strength, uint16_t snr,
		                            uint32_t ber, uint32_t ublocks) = 0;

		///
		virtual std::string attributeDescribeString(bool &active) const = 0;

};

/// The class @c Stream carries all the data/information of an stream
class Stream :
	public StreamInterface,
	public base::XMLSupport  {
	public:
		static const unsigned int MAX_CLIENTS;

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		Stream(int streamID, decrypt::dvbapi::Client *decrypt);

		virtual ~Stream();

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

		/// Get the stream properties for this stream
		StreamProperties & getStreamProperties() {
			return _properties;
		}

		/// Update the frontend of this stream
		bool updateFrontend();

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
		base::Mutex       _mutex;         //
		StreamingType     _streamingType; //
		bool              _enabled;       // is this stream enabled, could we use it?
		bool              _streamInUse;   //
		StreamClient     *_client;        // defines the participants of this stream
		                                  // index 0 is the owner of this stream
		StreamThreadBase *_streaming;     //
		decrypt::dvbapi::Client *_decrypt;//
		StreamProperties  _properties;    //
		input::Device *_frontend;      //
}; // class Stream

#endif // STREAM_H_INCLUDE