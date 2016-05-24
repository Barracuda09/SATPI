/* StreamInterface.h

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
#ifndef STREAMINTERFACE_H_INCLUDE
#define STREAMINTERFACE_H_INCLUDE STREAMINTERFACE_H_INCLUDE

#include <FwDecl.h>

FW_DECL_NS0(StreamClient);
FW_DECL_SP_NS1(input, Device);

/// The class @c StreamInterface is an interface to an @c Stream
class StreamInterface {
	public:
		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		StreamInterface() {}

		virtual ~StreamInterface() {}

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================

		/// Get the streamID of this stream
		virtual int getStreamID() const = 0;

		///
		virtual StreamClient &getStreamClient(std::size_t clientNr) const = 0;

		///
		virtual input::SpDevice getInputDevice() const = 0;

		virtual uint32_t getSSRC() const = 0;

		///
		virtual long getTimestamp() const = 0;

		///
		virtual uint32_t getSPC() const = 0;

		/// The Frontend Monitor update interval
		virtual unsigned int getRtcpSignalUpdateFrequency() const = 0;

		///
		virtual uint32_t getSOC() const  = 0;

		///
		virtual void addRtpData(uint32_t byte, long timestamp)  = 0;

		///
		virtual double getRtpPayload() const = 0;

		/// Get the stream Description string for RTCP and DESCRIBE command
		virtual std::string attributeDescribeString(bool &active) const = 0;

};

#endif // STREAMINTERFACE_H_INCLUDE
