/* Transformation.h

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
#ifndef INPUT_TRANSFORMATION_H_INCLUDE
#define INPUT_TRANSFORMATION_H_INCLUDE INPUT_TRANSFORMATION_H_INCLUDE

#include <FwDecl.h>
#include <base/M3UParser.h>
#include <base/XMLSupport.h>
#include <input/InputSystem.h>

#include <string>

FW_DECL_NS1(input, DeviceData);

namespace input {

	/// The class @c Transformation is an interface for transform
	/// an input request to an different request red from an M3U file.
	class Transformation :
		public base::XMLSupport  {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
			Transformation(input::DeviceData &transformedDeviceData);

			virtual ~Transformation();

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================

		public:

			virtual void addToXML(std::string &xml) const override;

			virtual void fromXML(const std::string &xml) override;

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		public:

			/// Check if transformation is enabled
			bool isEnabled() const;

			///
			bool advertiseAsDVBS2() const;

			///
			void resetTransformFlag();

			/// This function may return the input system for the
			/// requested input frequency.
			input::InputSystem getTransformationSystemFor(double frequency) const;

			/// This function may return the transformed input message
			std::string transformStreamString(int streamID,
				const std::string &msg,
				const std::string &method);

			/// This function may return the transformed Device Data
			const DeviceData &transformDeviceData(
				const DeviceData &deviceData) const;

		private:

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:

			bool _enabled;
			bool _transform;
			bool _advertiseAsDVBS2;
			base::M3UParser _m3u;
			std::string _transformFileM3U;
			input::DeviceData &_transformedDeviceData;
	};

} // namespace input

#endif // INPUT_TRANSFORMATION_H_INCLUDE
