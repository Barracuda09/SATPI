/* TSReaderData.h

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
#ifndef INPUT_FILE_TSREADER_DATA_H_INCLUDE
#define INPUT_FILE_TSREADER_DATA_H_INCLUDE INPUT_FILE_TSREADER_DATA_H_INCLUDE

#include <input/DeviceData.h>

#include <string>

namespace input::file {

/// The class @c TSReaderData carries all the data/information for Reading
/// from an file
class TSReaderData :
	public DeviceData {
		// =========================================================================
		// Constructors and destructor ---------------------------------------------
		// =========================================================================
	public:

		TSReaderData();

		virtual ~TSReaderData();

		// =========================================================================
		// -- input::DeviceData ----------------------------------------------------
		// =========================================================================
	private:

		/// @see DeviceData
		virtual void doNextAddToXML(std::string &xml) const final;

		/// @see DeviceData
		virtual void doNextFromXML(const std::string &xml) final;

		/// @see DeviceData
		virtual void doInitialize() final;

		/// @see DeviceData
		virtual void doParseStreamString(FeID id, const TransportParamVector& params) final;

		/// @see DeviceData
		virtual std::string doAttributeDescribeString(FeID id) const final;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		///
		std::string getFilePath() const;

		bool hasFilePath() const;

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

		std::string _filePath;

};

}

#endif // INPUT_FILE_TSREADER_DATA_H_INCLUDE
