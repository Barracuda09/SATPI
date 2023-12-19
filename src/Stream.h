/* Stream.h

   Copyright (C) 2014 - 2023 Marc Postema (mpostema09 -at- gmail.com)

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
#define STREAM_H_INCLUDE STREAM_H_INCLUDE

#include <FwDecl.h>
#include <base/Mutex.h>
#include <base/Thread.h>
#include <base/XMLSupport.h>
#include <mpegts/PacketBuffer.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

FW_DECL_NS0(SocketClient);

FW_DECL_SP_NS1(input, Device);
FW_DECL_SP_NS1(output, StreamClient);
FW_DECL_SP_NS2(decrypt, dvbapi, Client);
FW_DECL_SP_NS2(input, dvb, FrontendDecryptInterface);

FW_DECL_VECTOR_OF_SP_NS0(Stream);

/// The class @c Stream carries all the data/information of an stream
class Stream :
	public base::XMLSupport {
	public:

		// =========================================================================
		// -- Constructors and destructor ------------------------------------------
		// =========================================================================
	public:

		Stream(input::SpDevice device, decrypt::dvbapi::SpClient decrypt);

		virtual ~Stream() = default;

		// =========================================================================
		// -- static member functions ----------------------------------------------
		// =========================================================================
	public:

		static SpStream makeSP(input::SpDevice device, decrypt::dvbapi::SpClient decrypt);

		// =========================================================================
		// -- base::XMLSupport -----------------------------------------------------
		// =========================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =========================================================================
		// -- Other member functions -----------------------------------------------
		// =========================================================================
	public:

		StreamID getStreamID() const;

		FeID getFeID() const;

		int getFeIndex() const;

#ifdef LIBDVBCSA
		///
		input::dvb::SpFrontendDecryptInterface getFrontendDecryptInterface();
#endif

		///
		void addDeliverySystemCount(
				std::size_t &dvbs2,
				std::size_t &dvbt,
				std::size_t &dvbt2,
				std::size_t &dvbc,
				std::size_t &dvbc2);

		/// Find the StreamClient for the requested parameters
		output::SpStreamClient findStreamClientFor(SocketClient &socketClient,
				bool newSession, std::string sessionID);

		/// Check is this stream enabled, can we use it?
		bool streamEnabled() const {
			base::MutexLock lock(_mutex);
			return _enabled;
		}

		/// Teardown the specified StreamClient
		/// @param streamClient specifies the client that will be used
		bool teardown(output::SpStreamClient streamClient);

		/// Check if there are any stream clients with a session time-out
		/// that should be closed
		void checkForSessionTimeout();

	private:

		///
		void startStreaming(output::SpStreamClient streamClient);

		///
		void pauseStreaming(output::SpStreamClient streamClient);

		///
		void restartStreaming(output::SpStreamClient streamClient);

		/// Call this when there are no StreamClients using this stream anymore
		void stopStreaming();

		///
		void determineAndMakeStreamClientType(FeID feID, const SocketClient &client);

		/// Thread execute function @see base::Thread should @return true to
		/// keep thread running and @return false will stop and then terminate this thread
		bool threadExecuteDeviceDataReader();

		/// Write data to Streamclients
		void executeStreamClientWriter();

		/// Thread execute function @see base::Thread should @return true to
		/// keep thread running and @return false will stop and then terminate this thread
		bool threadExecuteDeviceMonitor();

		// =========================================================================
		// -- Functions used for RTSP Server ---------------------------------------
		// =========================================================================
	public:

		///
		bool processStreamingRequest(const SocketClient &client, output::SpStreamClient streamClient);

		///
		bool update(output::SpStreamClient streamClient);

		///
		std::string getSDPMediaLevelString() const;

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

		base::Mutex _mutex;

		bool _enabled;
		bool _streamInUse;

		std::vector<output::SpStreamClient> _streamClientVector;

		decrypt::dvbapi::SpClient _decrypt;
		input::SpDevice _device;
		unsigned int _rtcpSignalUpdate;
		base::Thread _threadDeviceDataReader;
		base::Thread _threadDeviceMonitor;
		std::array<mpegts::PacketBuffer, 100> _tsBuffer;
		mpegts::PacketBuffer _tsEmpty;
		std::size_t _writeIndex;
		std::size_t _readIndex;
		unsigned long _sendInterval;
		std::chrono::steady_clock::time_point _t1;
		std::chrono::steady_clock::time_point _t2;
		std::atomic_bool _signalLock;

};

#endif // STREAM_H_INCLUDE
