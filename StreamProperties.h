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

/// The class @c StreamProperties carries all the available/open StreamProperties
class StreamProperties  {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		StreamProperties();
		virtual ~StreamProperties();

		///
		void printChannelInfo() const;

		///
		void addRtpData(const uint32_t byte, long timestamp);

		///
		void addPIDData(uint16_t pid, uint8_t cc);

		///
		void setFrontendMonitorData(fe_status_t status, uint16_t strength, uint16_t snr,
		                            uint32_t ber, uint32_t ublocks);

		///
		fe_status_t getFrontendStatus() const;

		///
		void addToXML(std::string &xml) const;

		///
		std::string attribute_describe_string(bool &active) const;

		void setStreamID(int streamID)     { _streamID = streamID; }
		int  getStreamID() const           { return _streamID; }

		void setStreamActive(bool active)  { _streamActive = active; }
		bool getStreamActive()             { return _streamActive; }

		uint32_t     getSSRC() const       { return _ssrc; }
		long         getTimestamp() const  { return _timestamp; }
		ChannelData &getChannelData()      { return _channelData; }
		double       getRtpPayload() const { return _rtp_payload; }
		uint32_t     getSPC() const        { return _spc; }
		uint32_t     getSOC() const        { return _soc; }

	protected:

	private:
		///
		std::string getPidCSV() const;

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

		fe_status_t _status;         // FE_HAS_LOCK | FE_HAS_SYNC | FE_HAS_SIGNAL
		uint16_t    _strength;       //
		uint16_t    _snr;            //
		uint32_t    _ber;            //
		uint32_t    _ublocks;        //

}; // class StreamProperties

#endif // STREAM_PROPERTIES_H_INCLUDE