/* FrontendData.h

   Copyright (C) 2014 - 2018 Marc Postema (mpostema09 -at- gmail.com)

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

namespace input {
namespace file {

	/// The class @c TSReaderData carries all the data/information for Reading
	/// from an file
	class TSReaderData :
		public DeviceData {
		public:
			// =======================================================================
			// Constructors and destructor
			// =======================================================================
			TSReaderData();
			virtual ~TSReaderData();

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================

		public:
			///
			virtual void addToXML(std::string &xml) const override;

			///
			virtual void fromXML(const std::string &xml) override;

			// =======================================================================
			// -- input::DeviceData --------------------------------------------------
			// =======================================================================

		public:

			///
			virtual void initialize() override;

			///
			virtual void parseStreamString(int streamID, const std::string &msg, const std::string &method) override;

			///
			virtual std::string attributeDescribeString(int streamID) const override;

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		public:

			///
			std::string getFilePath() const;
			
			bool hasFilePath() const;

			///
			void clearData();

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:

			std::string _filePath;

	};

} // namespace file
} // namespace input

#endif // INPUT_FILE_TSREADER_DATA_H_INCLUDE
