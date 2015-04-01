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

/// The class @c Stream carries all the data/information of an stream
class Stream  {
	public:
		static const unsigned int MAX_CLIENTS;

		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		Stream();
		virtual ~Stream();

		void setStreamID(int streamID) { _properties.setStreamID(streamID); }
		int  getStreamID() const       { return _properties.getStreamID(); }

		///
		std::string attribute_describe_string(bool &active) const { return _properties.attribute_describe_string(active); }

		///
		void addFrontendPaths(const std::string &fe,
		                      const std::string &dvr,
							  const std::string &dmx) { _frontend.addFrontendPaths(fe, dvr, dmx); }

		///
		void setFrontendInfo() { _frontend.setFrontendInfo(); }

		///
		size_t getDeliverySystemSize() const { return _frontend.getDeliverySystemSize(); }

		///
		const fe_delivery_system_t *getDeliverySystem() const { return _frontend.getDeliverySystem(); }

		bool findClientIDFor(SocketClient &socketClient,
		                     bool newSession,
		                     std::string sessionID,
		                     const std::string &method,
		                     int &clientID);

		///
		void copySocketClientAttr(const SocketClient &socketClient);

		///
		bool streamInUse() const { return _streamInUse; }


		///
		void close(int clientID);

		///
		bool teardown(int clientID, bool gracefull);

		///
		void checkStreamClientsWithTimeout();

		///
		void addToXML(std::string &xml) const;
		
		/// Get and Set DVR buffer size
		void          setDVRBufferSize(unsigned long size) { _properties.setDVRBufferSize(size); }
		unsigned long getDVRBufferSize() const             { return _properties.getDVRBufferSize(); }


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
		void setFrequency(uint32_t freq)                    { _properties.getChannelData().freq = freq; _properties.getChannelData().changed = true; }
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
		void setPID(int pid, bool val)                      { _properties.getChannelData().pid.data[pid].used = val;
		                                                      _properties.getChannelData().pid.changed = true; }
		void setAllPID(bool val)                            { _properties.getChannelData().pid.data[ALL_PIDS].used = val;
		                                                      _properties.getChannelData().pid.changed = true; }

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