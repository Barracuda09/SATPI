/* Translation.h

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
#ifndef INPUT_TRANSLATION_H_INCLUDE
#define INPUT_TRANSLATION_H_INCLUDE INPUT_TRANSLATION_H_INCLUDE

#include <base/M3UParser.h>
#include <input/InputSystem.h>
#include <input/DeviceData.h>

#include <string>

namespace input {

	/// The class @c Translation is an interface for translation
	/// an input request to an different request red from an M3U file.
	class Translation {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
			Translation();

			virtual ~Translation();

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		public:

			///
			void resetTranslationFlag();

			/// This function may return the input system for the
			/// requested input frequency.
			input::InputSystem getTranslationSystemFor(double frequency) const;

			/// This function may return the translated input message
			std::string translateStreamString(int streamID,
				const std::string &msg,
				const std::string &method);

			/// This function may return the translated Device Data
			const DeviceData &translateDeviceData(
				const DeviceData &deviceData) const;

		private:

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:

			bool _translate;
			base::M3UParser _m3u;
			mutable input::DeviceData _translatedDeviceData;
	};

} // namespace input

#endif // INPUT_TRANSLATION_H_INCLUDE
