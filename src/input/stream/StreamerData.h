/* StreamerData.h

   Copyright (C) 2014 - 2019 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef INPUT_STREAM_STREAMER_DATA_H_INCLUDE
#define INPUT_STREAM_STREAMER_DATA_H_INCLUDE INPUT_STREAM_STREAMER_DATA_H_INCLUDE

#include <input/DeviceData.h>

#include <string>

namespace input {
namespace stream {

/// The class @c StreamerData carries all the data/information for Reading
/// from an Child PIPE
class StreamerData :
	public DeviceData {
		// =====================================================================
		// Constructors and destructor -----------------------------------------
		// =====================================================================
	public:

		StreamerData();

		virtual ~StreamerData();

		// =====================================================================
		// -- input::DeviceData ------------------------------------------------
		// =====================================================================
	private:

		/// @see DeviceData
		virtual void doNextAddToXML(std::string &xml) const final;

		/// @see DeviceData
		virtual void doNextFromXML(const std::string &xml) final;

		/// @see DeviceData
		virtual void doInitialize() final;

		/// @see DeviceData
		virtual void doParseStreamString(int streamID, const std::string &msg, const std::string &method) final;

		/// @see DeviceData
		virtual std::string doAttributeDescribeString(int streamID) const final;

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	public:

		///
		std::string getMultiAddr() const;

		int getPort() const;

		bool hasFilePath() const;

		///
		void clearData();

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================

	private:

		std::string _uri;
		std::string _multiAddr;
		int _port;
		bool _udp;

};

} // namespace stream
} // namespace input

#endif // INPUT_STREAM_STREAMER_DATA_H_INCLUDE
