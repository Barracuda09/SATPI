/* StreamClientOutputHttp.h

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
#ifndef STREAM_CLIENT_OUTPUT_HTTP_H_INCLUDE
#define STREAM_CLIENT_OUTPUT_HTTP_H_INCLUDE STREAM_CLIENT_OUTPUT_HTTP_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <output/StreamClient.h>

FW_DECL_SP_NS1(output, StreamClientOutputHttp);

namespace output {

/// StreamClient defines the owner/participants of an stream
class StreamClientOutputHttp : public output::StreamClient {
	public:

		// =========================================================================
		// -- Constructors and destructor ------------------------------------------
		// =========================================================================
	public:

		StreamClientOutputHttp(FeID feID) : StreamClient(feID) {};

		virtual ~StreamClientOutputHttp() = default;

		// =========================================================================
		// -- static member functions ----------------------------------------------
		// =========================================================================
	public:

		static SpStreamClientOutputHttp makeSP();
		template<class... ARGS>
		static SpStreamClient makeSP(ARGS&&... args) {
			return std::make_shared<StreamClientOutputHttp>(std::forward<ARGS>(args)...);
		}

		// =========================================================================
		// -- StreamClient ---------------------------------------------------------
		// =========================================================================
	private:

		/// Specialization for @see processStreamingRequest
		virtual bool doProcessStreamingRequest(const SocketClient& client) final;

		/// Specialization for @see startStreaming
		virtual void doStartStreaming() final;

		/// Specialization for @see teardown
		virtual void doTeardown() final;

		/// Specialization for @see writeData
		virtual bool doWriteData(mpegts::PacketBuffer& buffer) final;

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

};

}

#endif
