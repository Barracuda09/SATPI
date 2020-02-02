/* StreamManager.h

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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
#include <base/XMLSupport.h>

#include <string>

FW_DECL_NS0(SocketClient);

FW_DECL_VECTOR_OF_SP_NS0(Stream);

FW_DECL_SP_NS2(decrypt, dvbapi, Client);
FW_DECL_SP_NS2(input, dvb, FrontendDecryptInterface);

/// The class @c StreamManager manages all the available/open streams
class StreamManager :
	public base::XMLSupport {
		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:

		explicit StreamManager();

		virtual ~StreamManager();

		// =====================================================================
		// -- base::XMLSupport -------------------------------------------------
		// =====================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =====================================================================
		// -- Other member functions -------------------------------------------
		// =====================================================================
	public:

		/// enumerate all available devices
		/// @param bindIPAddress specifies the IP address to bind to
		/// @param appDataPath specifies the path were to store application data
		/// @param dvbPath specifies the path were to find dvb devices eg. /dev/dvb
		/// @param enableChildPIPE to enable frontend 'Child PIPE - TS Reader'
		void enumerateDevices(
			const std::string &bindIPAddress,
			const std::string &appDataPath,
			const std::string &dvbPath,
			bool enableChildPIPE);

		///
		SpStream findStreamAndClientIDFor(
			SocketClient &socketClient,
			int &clientID);

		///
		void checkForSessionTimeout();

		///
		std::string getXMLDeliveryString() const;

		///
		std::string getRTSPDescribeString() const;

		///
		std::size_t getMaxStreams() const {
			base::MutexLock lock(_mutex);
			return _stream.size();
		}

		///
		std::string getDescribeMediaLevelString(int streamID) const;

#ifdef LIBDVBCSA
		///
		input::dvb::SpFrontendDecryptInterface getFrontendDecryptInterface(
			int streamID);
#endif

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		base::Mutex _mutex;
		decrypt::dvbapi::SpClient _decrypt;
		StreamSpVector _stream;
};

#endif // STREAM_MANAGER_H_INCLUDE
