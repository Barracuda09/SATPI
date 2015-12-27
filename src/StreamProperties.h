/* StreamProperties.h

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef STREAM_PROPERTIES_H_INCLUDE
#define STREAM_PROPERTIES_H_INCLUDE STREAM_PROPERTIES_H_INCLUDE

#include "ChannelData.h"
#include "Mutex.h"
#include "XMLSupport.h"
#ifdef LIBDVBCSA
	#include "DvbapiClientProperties.h"
#endif

#include <string>

/// The class @c StreamProperties carries all the available/open StreamProperties
class StreamProperties
#ifdef LIBDVBCSA
            : public XMLSupport,
              public DvbapiClientProperties {
#else
            : public XMLSupport {
#endif
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		/// Set the stream ID for this properties (Only used at initialize)
		StreamProperties(int streamID);
		virtual ~StreamProperties();

		/// Print channel information (just used as debug info)
		void printChannelInfo() const;

		/// Set the frontend  status information like strength, snr etc.
		void setFrontendMonitorData(fe_status_t status, uint16_t strength, uint16_t snr,
		                            uint32_t ber, uint32_t ublocks);

		/// Get the frontend status. Is the frontend locked or not
		fe_status_t getFrontendStatus() const;

		/// Add data to an XML for storing or web interface
		virtual void addToXML(std::string &xml) const;

		/// Get data from an XML for restoring or web interface
		virtual void fromXML(const std::string &xml);

		/// Get the stream Description for RTCP and DESCRIBE command
		std::string attribute_describe_string(bool &active) const;

		/// Get the stream ID to identify the properties
		int  getStreamID() const {
			MutexLock lock(_mutex);
			return _streamID;
		}

		/// Set if the stream is active/in use
		void setStreamActive(bool active) {
			MutexLock lock(_mutex);
			_streamActive = active;
		}

		/// Check if the stream is busy/in use
		bool getStreamActive() {
			MutexLock lock(_mutex);
			return _streamActive;
		}

		///
		uint32_t getSSRC() const {
			MutexLock lock(_mutex);
			return _ssrc;
		}

		///
		long getTimestamp() const {
			MutexLock lock(_mutex);
			return _timestamp;
		}

		///
		double getRtpPayload() const {
			MutexLock lock(_mutex);
			return _rtp_payload;
		}

		///
		uint32_t getSPC() const {
			MutexLock lock(_mutex);
			return _spc;
		}

		///
		uint32_t getSOC() const {
			MutexLock lock(_mutex);
			return _soc;
		}

		/// Get the DVR buffer size
		unsigned long getDVRBufferSize() const;


		/// Check if DiSEqC command has to be repeated
		bool diseqcRepeat() const;

		/// The Frontend Monitor update interval
		unsigned int getRtcpSignalUpdateFrequency() const;

		///
		void addRtpData(const uint32_t byte, long timestamp);

		// =======================================================================
		// Data members for ChannelData
		// =======================================================================

		/// Check the 'Channel Data changed' flag. If set we should update
		bool hasChannelDataChanged() const;

		/// Reset/clear the 'Channel Data changed' flag
		void resetChannelDataChanged();

		/// Get/Set the intermediate frequency in Mhz
		uint32_t getIntermediateFrequency() const;
		void setIntermediateFrequency(uint32_t frequency);

		/// Reset the pid
		void resetPid(int pid);

		/// Reset 'PID has changed' flag
		void resetPIDTableChanged();

		/// Check the 'PID has changed' flag
		bool hasPIDTableChanged() const;

		/// Set DMX file descriptor
		void setDMXFileDescriptor(int pid, int fd);

		/// Get DMX file descriptor
		int getDMXFileDescriptor(int pid) const;

		/// Close DMX file descriptor
		void closeDMXFileDescriptor(int pid);

		/// Get the amount of packet that were received of this pid
		uint32_t getPacketCounter(int pid) const;

		/// Get the CSV of all the requested PID
		std::string getPidCSV() const;

		/// Set the continuity counter for pid
		void addPIDData(int pid, uint8_t cc);

		/// Set pid used or not
		void setPID(int pid, bool val);

		/// Check if PID is used
		bool isPIDUsed(int pid) const;

		/// Set all PID
		void setAllPID(bool val);

		///
		void initializeChannelData();

		/// Get/Set the frequency in Mhz
		uint32_t getFrequency() const;
		void setFrequency(uint32_t freq);

		int  getSymbolRate() const;
		void setSymbolRate(int srate);

		/// Get/Set the current Delivery System
		fe_delivery_system_t getDeliverySystem() const;
		void setDeliverySystem(fe_delivery_system_t delsys);

		/// Get/Set modulation type
		int  getModulationType() const;
		void setModulationType(int modtype);

		/// Get/Set the LNB polarizaion
		int getPolarization() const;
		void setPolarization(int pol);

		/// Get/Set the DiSEqc source
		int getDiSEqcSource() const;
		void setDiSEqcSource(int source);

		void setRollOff(int rolloff);
		int  getRollOff() const;
		void setFEC(int fec);
		int  getFEC() const;
		void setPilotTones(int pilot);
		int  getPilotTones() const;
		void setSpectralInversion(int specinv);
		int  getSpectralInversion() const;
		void setBandwidthHz(int bandwidth);
		int  getBandwidthHz() const;
		void setTransmissionMode(int transmission);
		int  getTransmissionMode() const;
		void setGuardInverval(int guard);
		int  getGuardInverval() const;
		void setHierarchy(int hierarchy);
		int  getHierarchy() const;
		void setUniqueIDPlp(int plp);
		int  getUniqueIDPlp() const;
		void setUniqueIDT2(int id);
		void setSISOMISO(int sm);

		// =======================================================================

///////////////////////////////////////////////////////////////////////////////
		///
		void setPMT(int pid, bool set);

		///
		bool isPMT(int pid) const;

		///
		void setECMFilterData(int demux, int filter, int pid, bool set);

		///
		void getECMFilterData(int &demux, int &filter, int pid) const;

		///
		bool getActiveECMFilterData(int &demux, int &filter, int &pid) const;

		///
		bool isECM(int pid) const;

		///
		void setKeyParity(int pid, int parity);

		///
		int getKeyParity(int pid) const;

		///
		void setECMInfo(
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

///////////////////////////////////////////////////////////////////////////////


	protected:

	private:

		// =======================================================================
		// Data members
		// =======================================================================
		Mutex       _mutex;          //
		int         _streamID;       //
		bool        _streamActive;   //

		uint32_t    _ssrc;           // synchronisation source identifier of sender
		uint32_t    _spc;            // sender RTP packet count  (used in SR packet)
		uint32_t    _soc;            // sender RTP payload count (used in SR packet)
		long        _timestamp;      //
		ChannelData _channelData;    //
		double      _rtp_payload;    //

		unsigned long _dvrBufferSize;//
		bool          _diseqcRepeat; //
		unsigned int  _rtcpSignalUpdate;

		fe_status_t   _status;       // FE_HAS_LOCK | FE_HAS_SYNC | FE_HAS_SIGNAL
		uint16_t      _strength;     //
		uint16_t      _snr;          //
		uint32_t      _ber;          //
		uint32_t      _ublocks;      //
}; // class StreamProperties

#endif // STREAM_PROPERTIES_H_INCLUDE