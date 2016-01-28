/* Streams.h

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
#ifndef STREAMS_H_INCLUDE
#define STREAMS_H_INCLUDE

#include <FwDecl.h>
#include <dvbfix.h>
#include <StreamClient.h>
#include <base/Mutex.h>
#include <base/XMLSupport.h>

#include <string>

FW_DECL_NS0(SocketClient);
FW_DECL_NS0(Stream);
FW_DECL_NS0(StreamProperties);
FW_DECL_NS2(decrypt, dvbapi, Client);

/// The class @c Streams carries all the available/open streams
class Streams :
	public base::XMLSupport {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		Streams();
		virtual ~Streams();

		/// enumerate all available frontends
		/// @param path
		int enumerateFrontends(const std::string &path, decrypt::dvbapi::Client *decrypt);

		///
		Stream *findStreamAndClientIDFor(SocketClient &socketClient, int &clientID);

		///
		void checkStreamClientsWithTimeout();

		///
		const std::string &getXMLDeliveryString() const {
			base::MutexLock lock(_mutex);
			return _del_sys_str;
		}

		///
		int getMaxStreams() const {
			base::MutexLock lock(_mutex);
			return _maxStreams;
		}

		///
		int getMaxDvbSat() const {
			base::MutexLock lock(_mutex);
			return _nr_dvb_s2;
		}

		///
		int getMaxDvbTer() const {
			base::MutexLock lock(_mutex);
			return _nr_dvb_t;
		}

		///
		int getMaxDvbCable() const {
			base::MutexLock lock(_mutex);
			return _nr_dvb_c;
		}

		///
		std::string attributeDescribeString(unsigned int stream, bool &active) const;

		/// Add streams data to an XML for storing or web interface
		virtual void addToXML(std::string &xml) const;

		/// Get streams data from an XML for restoring or web interface
		virtual void fromXML(const std::string &xml);

		///
		StreamProperties &getStreamProperties(int streamID);

		///
		bool updateFrontend(int streamID);

	protected:

	private:
		///
		int getAttachedFrontendCount_L(const std::string &path, int count);

		// =======================================================================
		// Data members
		// =======================================================================
		base::Mutex   _mutex;       //
		Stream     **_stream;
		int          _maxStreams;
		Stream      *_dummyStream;
		std::string  _del_sys_str;
		int          _nr_dvb_s2;
		int          _nr_dvb_t;
		int          _nr_dvb_t2;
		int          _nr_dvb_c;
#if FULL_DVB_API_VERSION >= 0x0505
		int          _nr_dvb_c2;
#endif

}; // class Streams

#endif // STREAMS_H_INCLUDE