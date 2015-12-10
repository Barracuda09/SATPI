/* InterfaceAttr.h

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#define INTERFACE_ATTR_H_INCLUDE INTERFACE_ATTR_H_INCLUDE

#include "Log.h"
#include "Utils.h"
#include "StringConverter.h"

#include <netinet/in.h>
#include <string>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h>

#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <unistd.h>


/// Socket attributes
class InterfaceAttr {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		InterfaceAttr() {
			get_interface_properties();
		}
		virtual ~InterfaceAttr() {}

		void get_interface_properties() {
			struct ifaddrs *ifaddr, *ifa;
			// get list of interfaces
			if (getifaddrs(&ifaddr) == -1) {
				PERROR("getifaddrs");
				return;
			}
			// go through list, find first usable interface
			for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
				if (ifa->ifa_addr == nullptr) {
					// try next one
					continue;
				}
				const int family = ifa->ifa_addr->sa_family;
				// we are not looking for 'lo' interface, but any other AF_INET* interface is good
				if ((strcmp(ifa->ifa_name, "lo")) != 0 && (family == AF_INET /*|| family == AF_INET6*/)) {
					char host[NI_MAXHOST];
					int s, fd;
					if ((s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
					                     host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST)) != 0) {
						GAI_PERROR("getnameinfo()", s);
						continue;
					}
					// Get hardware address
					if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
						PERROR("Server socket");
						continue;
					}
					struct ifreq ifr;
					ifr.ifr_addr.sa_family = family;
					memcpy(&ifr.ifr_name, ifa->ifa_name, IFNAMSIZ);
					if (ioctl(fd, SIOCGIFHWADDR, &ifr) != 0) {
						PERROR("ioctl SIOCGIFHWADDR");
						CLOSE_FD(fd);
						continue;
					}
					CLOSE_FD(fd);
					const unsigned char* mac=(unsigned char*)ifr.ifr_hwaddr.sa_data;
					StringConverter::addFormattedString(_mac_addr_decorated, "%02x:%02x:%02x:%02x:%02x:%02x",
					                                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
					StringConverter::addFormattedString(_mac_addr, "%02x%02x%02x%02x%02x%02x",
					                                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
					_iface_name = ifr.ifr_name;
					_ip_addr = host;
					SI_LOG_INFO("%s: %s [%s]", _iface_name.c_str(), _ip_addr.c_str(), _mac_addr_decorated.c_str());
					break;
				}
			}
			// free linked list
			freeifaddrs(ifaddr);
		}

		const std::string &getIPAddress() const {
			return _ip_addr;
		}

		std::string getUUID() const {
			char uuid[50];
			snprintf(uuid, sizeof(uuid), "50c958a8-e839-4b96-b7ae-%s", _mac_addr.c_str());
			return uuid;
		}

		static int getNetworkUDPBufferSize() {
			int socket_fd = -1;
			if ((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
				PERROR("socket");
				return 0;
			}
			int bufferSize = getNetworkUDPBufferSize(socket_fd);
			CLOSE_FD(socket_fd);
			return bufferSize / 2;
		}

		static int getNetworkUDPBufferSize(int socket_fd) {
			int bufferSize;
			socklen_t optlen = sizeof(bufferSize);
			if (getsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, &bufferSize, &optlen) == -1) {
				PERROR("getsockopt");
				bufferSize = 0;
			}
			return bufferSize / 2;
		}

		static bool setNetworkUDPBufferSize(int fd, int size) {
			if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) == -1) {
				PERROR("setsockopt");
				return false;
			}
			return true;
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