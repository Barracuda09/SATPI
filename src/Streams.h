/* Streams.h

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
#ifndef STREAMS_H_INCLUDE
#define STREAMS_H_INCLUDE

#include "StreamClient.h"
#include "dvbfix.h"
#include "XMLSupport.h"

#ifdef LIBDVBCSA
	#include "DvbapiClient.h"
#endif

#include <string>

// Forward declarations
class SocketClient;
class Stream;

/// The class @c Streams carries all the available/open streams
class Streams : public XMLSupport
#ifdef LIBDVBCSA
		: public DvbapiClient
#endif
		{
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		Streams();
		virtual ~Streams();

		/// enumerate all available frontends
		/// @param path
		int enumerateFrontends(const std::string &path);

		///
		Stream *findStreamAndClientIDFor(SocketClient &socketClient, int &clientID);

		///
		void checkStreamClientsWithTimeout();

		///
		const std::string &getXMLDeliveryString() const { return _del_sys_str;	}

		///
		int getMaxStreams() const { return _maxStreams; }

		///
		int getMaxDvbSat() const { return _nr_dvb_s2; }

		///
		int getMaxDvbTer() const { return _nr_dvb_t; }

		///
		int getMaxDvbCable() const { return _nr_dvb_c; }

		///
		std::string attribute_describe_string(unsigned int stream, bool &active) const;

		/// Add streams data to an XML for storing or web interface
		virtual void addToXML(std::string &xml) const;

		/// Get streams data from an XML for restoring or web interface
		virtual void fromXML(const std::string &xml);
	protected:
		///
		virtual void setECMPID(int streamID, int pid, bool set);

	private:
		///
		int getAttachedFrontendCount(const std::string &path, int count);

		// =======================================================================
		// Data members
		// =======================================================================
		Stream **_stream;
		int  _maxStreams;
		Stream *_dummyStream;
		std::string _del_sys_str;

		int _nr_dvb_s2;
		int _nr_dvb_t;
		int _nr_dvb_t2;
		int _nr_dvb_c;
#if FULL_DVB_API_VERSION >= 0x0505
		int _nr_dvb_c2;
#endif

}; // class Streams

#endif // STREAMS_H_INCLUDE