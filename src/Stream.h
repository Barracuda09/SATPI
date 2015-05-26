/* Stream.h

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#define STREAM_H_INCLUDE

#include "Frontend.h"
#include "ChannelData.h"
#include "StreamClient.h"
#include "RtpThread.h"
#include "RtcpThread.h"
#include "StreamProperties.h"

#include <string>

// Forward declarations
class SocketClient;
class DvbapiClient;

/// The class @c Stream carries all the data/information of an stream
class Stream  {
	public:
		static const unsigned int MAX_CLIENTS;

		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		Stream(int streamID, DvbapiClient *dvbapi);
		virtual ~Stream();

		/// Get the streamID of this stream
		int  getStreamID() const       { return _properties.getStreamID(); }

		/// Get the describe string for this stream
		std::string attribute_describe_string(bool &active) const { return _properties.attribute_describe_string(active); }

		/// Add the frontend paths to connect to this stream
		void addFrontendPaths(const std::string &fe,
		                      const std::string &dvr,
							  const std::string &dmx) { _frontend.addFrontendPaths(fe, dvr, dmx); }

		/// This will read the frontend information for this stream
		void setFrontendInfo() { _frontend.setFrontendInfo(); }

		/// Get the amount of delivery systems of this stream
		size_t getDeliverySystemSize() const { return _frontend.getDeliverySystemSize(); }

		/// Get the possible delivery systems of this stream
		const fe_delivery_system_t *getDeliverySystem() const { return _frontend.getDeliverySystem(); }

		/// Find the clientID for the requested parameters
		bool findClientIDFor(SocketClient &socketClient,
		                     bool newSession,
		                     std::string sessionID,
		                     const std::string &method,
		                     int &clientID);

		/// Copy the connected  client data to this stream
		void copySocketClientAttr(const SocketClient &socketClient);

		/// Check is this stream used already
		bool streamInUse() const { return _streamInUse; }


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
		void addToXML(std::string &xml) const;
		
		/// Get stream data from an XML for restoring or web interface
		void fromXML(const std::string className, const std::string streamID,
		             const std::string variableName, const std::string value);

		/// Set externally an ECM (or just any other pid)
		void setECMPID(int pid, bool set);

		// =======================================================================
		// Functions used for RTSP Server
		// =======================================================================
		bool processStream(const std::string &msg, int clientID, const std::string &method);
		bool update(int clientID);
		int  getRtspFD(int clientID) const                  { return _client[clientID].getRtspFD(); }
		int  getCSeq(int clientID) const                    { return _client[clientID].getCSeq(); }
		const std::string &getSessionID(int clientID) const { return _client[clientID].getSessionID(); }
		unsigned int getSessionTimeout(int clientID) const  { return _client[clientID].getSessionTimeout(); }
		int  getRtpSocketPort(int clientID) const           { return _client[clientID].getRtpSocketPort(); }
		int  getRtcpSocketPort(int clientID) const          { return _client[clientID].getRtcpSocketPort(); }

	protected:

	private:
		/// Set functions for @c ChannelData
		void initializeChannelData()                        { _properties.getChannelData().initialize(); }
		void setFrequency(uint32_t freq)                    { _properties.getChannelData().freq = freq;
		                                                      _properties.getChannelData().changed = true; }
		void setSymbolRate(int srate)                       { _properties.getChannelData().srate = srate;	}
		void setDeliverySystem(fe_delivery_system_t delsys) { _properties.getChannelData().delsys = delsys; }
		void setModulationType(int modtype)                 { _properties.getChannelData().modtype = modtype; }
		void setLNBPolarization(int pol)                    { _properties.getChannelData().pol_v = pol; }
		void setLNBSource(int source)                       { _properties.getChannelData().src = source; 	}
		void setRollOff(int rolloff)                        { _properties.getChannelData().rolloff = rolloff; }
		void setFEC(int fec)                                { _properties.getChannelData().fec = fec; }
		void setPilotTones(int pilot)                       { _properties.getChannelData().pilot = pilot; }
		void setSpectralInversion(int specinv)              { _properties.getChannelData().inversion = specinv; }
		void setBandwidthHz(int bandwidth)                  { _properties.getChannelData().bandwidthHz = bandwidth; }
		void setTransmissionMode(int transmission)          { _properties.getChannelData().transmission = transmission; }
		void setGuardInverval(int guard)                    { _properties.getChannelData().guard = guard; }
		void setUniqueIDPlp(int plp)                        { _properties.getChannelData().plp_id = plp; }
		void setUniqueIDT2(int id)                          { _properties.getChannelData().t2_system_id = id; }
		void setSISOMISO(int sm)                            { _properties.getChannelData().siso_miso = sm; }
		void setPID(int pid, bool val)                      { _properties.getChannelData().setPID(pid, val); }
		bool isPIDUsed(int pid)                             { return _properties.getChannelData().isPIDUsed(pid); }
		void setAllPID(bool val)                            { _properties.getChannelData().setAllPID(val); }

		///
		void parseStreamString(const std::string &msg, const std::string &method);

		///
		void processPID(const std::string &pids, bool add);

		///
		void processStopStream(int clientID, bool gracefull);

		// =======================================================================
		// Data members
		// =======================================================================
		bool             _streamInUse; //
		StreamClient     *_client;     // defines the participants of this stream
		                               // index 0 is the owner of this stream
		StreamProperties _properties;  //
		Frontend         _frontend;    //
		RtpThread        _rtpThread;   //
		RtcpThread       _rtcpThread;  //
}; // class Stream

#endif // STREAM_H_INCLUDE