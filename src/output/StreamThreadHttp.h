/* StreamThreadHttp.h

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef OUTPUT_STREAMTHREADHTTP_H_INCLUDE
#define OUTPUT_STREAMTHREADHTTP_H_INCLUDE OUTPUT_STREAMTHREADHTTP_H_INCLUDE

#include <FwDecl.h>
#include <output/StreamThreadBase.h>

FW_DECL_NS0(StreamClient);
FW_DECL_NS0(StreamInterface);

FW_DECL_UP_NS1(output, StreamThreadHttp);

namespace output {

	/// HTTP Streaming thread
	class StreamThreadHttp :
		public StreamThreadBase {
		public:
			// =======================================================================
			// -- Constructors and destructor ----------------------------------------
			// =======================================================================
			StreamThreadHttp(
				StreamInterface &stream);

			virtual ~StreamThreadHttp();

			// =======================================================================
			//  -- output::StreamThreadBase ------------------------------------------
			// =======================================================================

		public:

			virtual bool startStreaming() override;

		protected:

			virtual void threadEntry() override;

			virtual bool writeDataToOutputDevice(mpegts::PacketBuffer &buffer,
				StreamClient &client) override;

			virtual int getStreamSocketPort(int clientID) const override;

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:

			int _clientID;
	};

} // namespace output

#endif // STREAMTHREADHTTP_H_INCLUDE
