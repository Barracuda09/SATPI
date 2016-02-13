/* DeviceData.h

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
#ifndef INPUT_DEVICE_DATA_H_INCLUDE
#define INPUT_DEVICE_DATA_H_INCLUDE INPUT_DEVICE_DATA_H_INCLUDE

namespace input {

	/// The class @c DeviceData. carries all the data/information for a device
	class DeviceData {
		public:
			// =======================================================================
			// Constructors and destructor
			// =======================================================================
			DeviceData() {}
			virtual ~DeviceData() {}

	};

} // namespace input

#endif // INPUT_DEVICE_DATA_H_INCLUDE
