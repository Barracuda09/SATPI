/* InterfaceAttr.h

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#ifndef INTERFACE_ATTR_H_INCLUDE
#define INTERFACE_ATTR_H_INCLUDE

#include "Log.h"
#include "Utils.h"

#include <netinet/in.h>
#include <string>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>

/// Socket attributes
class InterfaceAttr {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		InterfaceAttr() { get_interface_properties(); }
		virtual ~InterfaceAttr() {;}

		void get_interface_properties() {
			struct ifreq ifr;
			struct sockaddr_in addr;
			int fd = -1;
			
			memset(&addr, 0, sizeof(addr));
			addr.sin_family      = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port        = htons(8843);

			if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
				PERROR("Server socket");
				return;
			}

			memset(&ifr, 0, sizeof(ifr));
			ifr.ifr_addr.sa_family = AF_INET;

			// get interface name
			if (ioctl(fd, SIOCGIFNAME, &ifr) != 0) {
				PERROR("ioctl SIOCGIFNAME");
				CLOSE_FD(fd);
				return;
			}

			// Get hardware address
			if (ioctl(fd, SIOCGIFHWADDR, &ifr) != 0) {
				PERROR("ioctl SIOCGIFHWADDR");
				CLOSE_FD(fd);
				return;
			}

			const unsigned char* mac=(unsigned char*)ifr.ifr_hwaddr.sa_data;
			char mac_addr_decorated[18];
			char mac_addr[18];
			snprintf(mac_addr_decorated, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
						mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			snprintf(mac_addr, 18, "%02x%02x%02x%02x%02x%02x",
						mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			_mac_addr_decorated =  mac_addr_decorated;
			_mac_addr = mac_addr;

			memset(&ifr, 0, sizeof(ifr));
			ifr.ifr_addr.sa_family = AF_INET;
			
			// get interface name
			if (ioctl(fd, SIOCGIFNAME, &ifr) != 0) {
				PERROR("ioctl SIOCGIFNAME");
				CLOSE_FD(fd);
				return;
			}
			
			// get PA address
			if (ioctl(fd, SIOCGIFADDR, &ifr) != 0) {
				PERROR("ioctl SIOCGIFHWADDR");
				CLOSE_FD(fd);
				return;
			}
			CLOSE_FD(fd);
			_ip_addr = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
			_iface_name = ifr.ifr_name;
			
			SI_LOG_INFO("%s: %s [%s]", _iface_name.c_str(), _ip_addr.c_str(), _mac_addr_decorated.c_str());
		}
		
		const std::string &getIPAddress() const {
			return _ip_addr;
		}
		
		std::string getUUID() const {
			char uuid[50];
			snprintf(uuid, sizeof(uuid), "50c958a8-e839-4b96-b7ae-%s", _mac_addr.c_str());
			return uuid;			
		}

	protected:
		// =======================================================================
		// Data members
		// =======================================================================
		std::string _ip_addr;               // ip address of the used interface
		std::string _mac_addr_decorated;    // mac address of the used interface
		std::string _mac_addr;              // mac address of the used interface
		std::string _iface_name;            // used interface name i.e. eth0
}; // class InterfaceAttr

#endif // INTERFACE_ATTR_H_INCLUDE