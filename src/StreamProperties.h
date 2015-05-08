/* StreamProperties.h

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
#ifndef STREAM_PROPERTIES_H_INCLUDE
#define STREAM_PROPERTIES_H_INCLUDE

#include "ChannelData.h"
#include "Mutex.h"

#include <string>

typedef struct {
	std::string data;
	int cc;
	int pid;
	bool collected;
} TSCollectedData_t;

/// The class @c StreamProperties carries all the available/open StreamProperties
class StreamProperties  {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		/// Set the stream ID for this properties (Only used at initialize)
		StreamProperties(int streamID);
		virtual ~StreamProperties();

		///
		void printChannelInfo() const;

		///
		void setFrontendMonitorData(fe_status_t status, uint16_t strength, uint16_t snr,
		                            uint32_t ber, uint32_t ublocks);

		///
		fe_status_t getFrontendStatus() const;

		///
		void addToXML(std::string &xml) const;

		///
		void fromXML(const std::string className, const std::string streamID,
		             const std::string variableName, const std::string value);

		/// Get the stream Description for RTCP and DESCRIBE command
		std::string attribute_describe_string(bool &active) const;

		/// Get the stream ID to identify the properties
		int  getStreamID() const           { return _streamID; }

		/// Set if the stream is active/in use
		void setStreamActive(bool active)  { _streamActive = active; }
		/// Check if the stream is busy/in use
		bool getStreamActive()             { return _streamActive; }

		uint32_t     getSSRC() const       { return _ssrc; }
		long         getTimestamp() const  { return _timestamp; }
		ChannelData &getChannelData()      { return _channelData; }
		double       getRtpPayload() const { return _rtp_payload; }
		uint32_t     getSPC() const        { return _spc; }
		uint32_t     getSOC() const        { return _soc; }

		/// Set if PAT has been collected or not
		void setPATCollected(bool collected);
		/// Check if PAT is collected
		bool isPATCollected() const;
		/// Add PAT data that was collected
		bool addPATData(const unsigned char *data, int length, int pid, int cc);
		/// Get the collected PAT Data
		const unsigned char *getPATData() const;
		/// Get the current size of the PAT Packet !!Whitout the CRC (4Bytes)!!
		int getPATDataSize() const;

		/// Set if PMT has been collected or not
		void setPMTCollected(bool collected);
		/// Check if PMT is collected
		bool isPMTCollected() const;
		/// Add PMT data that was collected
		bool addPMTData(const unsigned char *data, int length, int pid, int cc);
		/// Get the collected PMT Data
		const unsigned char *getPMTData() const;
		/// Get the current size of the PMT Packet !!Whitout the CRC (4Bytes)!!
		int getPMTDataSize() const;

		///
		void addRtpData(const uint32_t byte, long timestamp);

		// =======================================================================
		// Data members for ChannelData
		// =======================================================================
		/// See @c ChannelData
		void addPIDData(int pid, uint8_t cc);

		/// See @c ChannelData
		void setPMTPID(bool pmt, int pid);

		/// See @c ChannelData
		bool isPMTPID(int pid);

		/// See @c ChannelData
		void setECMFilterData(int demux, int filter, int pid, int parity);

		/// See @c ChannelData
		void getECMFilterData(int &demux, int &filter, int pid) const;
		
		/// See @c ChannelData
		void setKeyParity(int pid, int parity);

		/// See @c ChannelData
		int getKeyParity(int pid) const;

		/// See @c ChannelData
		bool isECMPID(int pid);
		// =======================================================================

		/// Get the DVR buffer size
		unsigned long getDVRBufferSize() const;

		/// Check if DiSEqC command has to be repeated
		bool diseqcRepeat() const;

		/// The Frontend Monitor update interval
		unsigned int getRtcpSignalUpdateFrequency() const;

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

		TSCollectedData_t _pat;      //
		TSCollectedData_t _pmt;      //
}; // class StreamProperties

#endif // STREAM_PROPERTIES_H_INCLUDE