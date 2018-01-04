/* InterfaceAttr.h

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
#ifndef INTERFACEATTR_H_INCLUDE
#define INTERFACEATTR_H_INCLUDE INTERFACEATTR_H_INCLUDE

#include <stdio.h>

#include <string>

	/// Socket attributes
	class InterfaceAttr {
		public:
			// ===================================================================
			//  -- Constructors and destructor -----------------------------------
			// ===================================================================
			InterfaceAttr();

			virtual ~InterfaceAttr();

			void get_interface_properties();

			// ===================================================================
			//  -- Static member functions ---------------------------------------
			// ===================================================================

		public:

			static int getNetworkUDPBufferSize();

			// ===================================================================
			//  -- Other member functions ----------------------------------------
			// ===================================================================

		public:

			const std::string &getIPAddress() const {
				return _ip_addr;
			}

			std::string getUUID() const {
				char uuid[50];
				snprintf(uuid, sizeof(uuid), "50c958a8-e839-4b96-b7ae-%s",
					_mac_addr.c_str());
				return uuid;
			}

			// ===================================================================
			//  -- Data members --------------------------------------------------
			// ===================================================================

		protected:

			std::string _ip_addr;            /// ip address of the used interface
			std::string _mac_addr_decorated; /// mac address of the used interface
			std::string _mac_addr;           /// mac address of the used interface
			std::string _iface_name;         /// used interface name i.e. eth0

	};

#endif // INTERFACEATTR_H_INCLUDE
