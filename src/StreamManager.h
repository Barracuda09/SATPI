/* StreamManager.h

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
#ifndef STREAM_MANAGER_H_INCLUDE
#define STREAM_MANAGER_H_INCLUDE STREAM_MANAGER_H_INCLUDE

#include <FwDecl.h>
#include <base/Mutex.h>
#include <base/XMLSupport.h>

#include <string>
#include <vector>

FW_DECL_NS0(SocketClient);
FW_DECL_NS0(Stream);
FW_DECL_NS2(decrypt, dvbapi, Client);
FW_DECL_NS2(input, dvb, FrontendDecryptInterface);

/// The class @c StreamManager manages all the available/open streams
class StreamManager :
	public base::XMLSupport {
	public:
		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		StreamManager(const std::string &xmlFilePath);
		virtual ~StreamManager();

		/// enumerate all available devices
		void enumerateDevices();

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
		std::size_t getMaxStreams() const {
			base::MutexLock lock(_mutex);
			return _stream.size();
		}

		///
		std::size_t getMaxDvbSat() const {
			base::MutexLock lock(_mutex);
			return _nr_dvb_s2;
		}

		///
		std::size_t getMaxDvbTer() const {
			base::MutexLock lock(_mutex);
			return _nr_dvb_t;
		}

		///
		std::size_t getMaxDvbCable() const {
			base::MutexLock lock(_mutex);
			return _nr_dvb_c;
		}

		///
		std::string attributeDescribeString(std::size_t stream, bool &active) const;

		/// Add stream data to an XML for storing or web interface
		virtual void addToXML(std::string &xml) const;

		/// Get stream data from an XML for restoring or web interface
		virtual void fromXML(const std::string &xml);

#ifdef LIBDVBCSA
		///
		input::dvb::FrontendDecryptInterface *getFrontendDecryptInterface(int streamID);

		///
		decrypt::dvbapi::Client *getDecrypt() const;
#endif


		typedef std::vector<Stream *> StreamVector;
		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================

	private:
		base::Mutex _mutex;       //
		std::string _xmlFilePath;
		decrypt::dvbapi::Client *_decrypt;
		StreamVector _stream;
		Stream *_dummyStream;
		std::string _del_sys_str;
		std::size_t _nr_dvb_s2;
		std::size_t _nr_dvb_t;
		std::size_t _nr_dvb_t2;
		std::size_t _nr_dvb_c;
		std::size_t _nr_dvb_c2;

}; // class StreamManager

#endif // STREAM_MANAGER_H_INCLUDE