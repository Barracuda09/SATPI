/* InterfaceAttr.h

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
#ifndef INTERFACEATTR_H_INCLUDE
#define INTERFACEATTR_H_INCLUDE INTERFACEATTR_H_INCLUDE

#include <string>

/// Interface attributes
class InterfaceAttr {
	public:
		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================

		///
		/// @param bindInterfaceName specifies the network interface name to bind to
		explicit InterfaceAttr(const std::string &bindInterfaceName);

		virtual ~InterfaceAttr() = default;

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================

	public:

		/// Get the IP address of the used interface
		const std::string &getIPAddress() const {
			return _ipAddr;
		}

		/// Get the IP address to bind the servers to (The requesed or first one that is UP)
		const std::string &getBindIPAddress() const {
			return _bindIPAddress;
		}

		/// Get the UUID of this device
		std::string getUUID() const;

	protected:

		/// Find the MAC and IP address of the first adapters that is UP, or requested ifaceName (i.e. eth0)
		bool getAdapterProperties(const std::string &ifaceName);

		// =====================================================================
		//  -- Data members ----------------------------------------------------
		// =====================================================================

	protected:

		std::string _ipAddr;           /// ip address of the used interface
		std::string _bindIPAddress;    /// bind ip address of the servers
		std::string _macAddrDecorated; /// mac address of the used interface
		std::string _macAddr;          /// mac address of the used interface
		std::string _ifaceName;        /// used interface name i.e. eth0
};

#endif // INTERFACEATTR_H_INCLUDE
