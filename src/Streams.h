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

#include "Stream.h"
#include "StreamClient.h"

#include <string>

// Forward declarations
class SocketClient;

/// The class @c Streams carries all the available/open streams
class Streams  {
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

		///
		void make_streams_xml(std::string &xml) const;

		///
		void fromXML(const std::string &className, const std::string &streamID,
		             const std::string &variableName, const std::string &value);
	protected:

	private:
		///
		int getAttachedFrontendCount(const std::string &path, int count);

		// =======================================================================
		// Data members
		// =======================================================================
		Stream *_stream;
		int  _maxStreams;
		Stream _dummyStream;
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