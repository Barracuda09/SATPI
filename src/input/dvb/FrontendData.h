/* FrontendData.h

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
#ifndef INPUT_DVB_FRONTEND_DATA_H_INCLUDE
#define INPUT_DVB_FRONTEND_DATA_H_INCLUDE INPUT_DVB_FRONTEND_DATA_H_INCLUDE

#include <base/XMLSupport.h>
#include <input/DeviceData.h>
#ifdef LIBDVBCSA
#include <decrypt/dvbapi/ClientProperties.h>
#endif

#include <stdint.h>
#include <string>

namespace input {
namespace dvb {

	/// The class @c FrontendData carries all the data/information for tuning a frontend
	class FrontendData :
		public base::XMLSupport,
#ifdef LIBDVBCSA
		public decrypt::dvbapi::ClientProperties,
#endif
		public DeviceData {
		public:
			// =======================================================================
			// Constructors and destructor
			// =======================================================================
			FrontendData();
			virtual ~FrontendData();

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
			virtual void initialize();

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		public:

			///
			void setECMInfo(
				int pid,
				int serviceID,
				int caID,
				int provID,
				int emcTime,
				const std::string &cardSystem,
				const std::string &readerName,
				const std::string &sourceName,
				const std::string &protocolName,
				int hops);

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:

	};

} // namespace dvb
} // namespace input

#endif // INPUT_DVB_FRONTEND_DATA_H_INCLUDE
