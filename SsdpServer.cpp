/* SsdpServer.cpp

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
#include "SsdpServer.h"
#include "InterfaceAttr.h"
#include "Properties.h"
#include "StringConverter.h"
#include "Log.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <poll.h>


#define UTIME_DEL    200000

SsdpServer::SsdpServer(const InterfaceAttr &interface,
                       Properties &properties) :
		ThreadBase("SsdpServer"),
		_interface(interface),
		_properties(properties) {;}

SsdpServer::~SsdpServer() {
	cancelThread();
	joinThread();
}

void SsdpServer::threadEntry() {
#define UPNP_ROOTDEVICE	"NOTIFY * HTTP/1.1\r\n" \
						"HOST: 239.255.255.250:1900\r\n" \
						"CACHE-CONTROL: max-age=1800\r\n" \
						"LOCATION: http://%s:%d/desc.xml\r\n" \
						"NT: upnp:rootdevice\r\n" \
						"NTS: ssdp:alive\r\n" \
						"SERVER: Linux/1.0 UPnP/1.1 SatPI/%s\r\n" \
						"USN: uuid:%s::upnp:rootdevice\r\n" \
						"BOOTID.UPNP.ORG: %d\r\n" \
						"CONFIGID.UPNP.ORG: 0\r\n" \
						"DEVICEID.SES.COM: %d\r\n" \
						"\r\n"

#define UPNP_ALIVE	"NOTIFY * HTTP/1.1\r\n" \
					"HOST: 239.255.255.250:1900\r\n" \
					"CACHE-CONTROL: max-age=1800\r\n" \
					"LOCATION: http://%s:%d/desc.xml\r\n" \
					"NT: uuid:%s\r\n" \
					"NTS: ssdp:alive\r\n" \
					"SERVER: Linux/1.0 UPnP/1.1 SatPI/%s\r\n" \
					"USN: uuid:%s\r\n" \
					"BOOTID.UPNP.ORG: %d\r\n" \
					"CONFIGID.UPNP.ORG: 0\r\n" \
					"DEVICEID.SES.COM: %d\r\n" \
					"\r\n"

#define UPNP_DEVICE "NOTIFY * HTTP/1.1\r\n" \
					"HOST: 239.255.255.250:1900\r\n" \
					"CACHE-CONTROL: max-age=1800\r\n" \
					"LOCATION: http://%s:%d/desc.xml\r\n" \
					"NT: urn:ses-com:device:SatIPServer:1\r\n" \
					"NTS: ssdp:alive\r\n" \
					"SERVER: Linux/1.0 UPnP/1.1 SatPI/%s\r\n" \
					"USN: uuid:%s::urn:ses-com:device:SatIPServer:1\r\n" \
					"BOOTID.UPNP.ORG: %d\r\n" \
					"CONFIGID.UPNP.ORG: 0\r\n" \
					"DEVICEID.SES.COM: %d\r\n" \
					"\r\n"

#define UPNP_M_SEARCH "M-SEARCH * HTTP/1.1\r\n" \
                      "HOST: %s:%d\r\n" \
                      "MAN: \"ssdp:discover\"\r\n" \
                      "ST: urn:ses-com:device:SatIPServer:1\r\n" \
                      "USER-AGENT: Linux/1.0 UPnP/1.1 SatPI/%s\r\n" \
                      "DEVICEID.SES.COM: %d\r\n" \
                      "\r\n"

#define UPNP_M_SEARCH_OK "HTTP/1.1 200 OK\r\n" \
                         "CACHE-CONTROL: max-age=1800\r\n" \
                         "EXT:\r\n" \
                         "LOCATION: http://%s:%d/desc.xml\r\n" \
                         "SERVER: Linux/1.0 UPnP/1.1 SatPI/%s\r\n" \
                         "ST: urn:ses-com:device:SatIPServer:1\r\n" \
                         "USN: uuid:%s::urn:ses-com:device:SatIPServer:1\r\n" \
                         "BOOTID.UPNP.ORG: %d\r\n" \
                         "CONFIGID.UPNP.ORG: 0\r\n" \
                         "DEVICEID.SES.COM: %d\r\n" \
                         "\r\n"

	_properties.setBootID(read_bootID_from_file("bootID"));

	SI_LOG_INFO("Setting up SSDP server with BOOTID: %d", _properties.getBootID());

	init_udp_socket(_udpMultiSend, SSDP_PORT, inet_addr("239.255.255.250"));
	init_mutlicast_udp_socket(_udpMultiListen, SSDP_PORT, _interface.getIPAddress().c_str());

	time_t repeat_time = 0;
	struct pollfd pfd[1];
	pfd[0].fd = _udpMultiListen.getFD();
	pfd[0].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
	pfd[0].revents = 0;

	for (;;) {
		// call poll with a timeout of 500 ms
		const int pollRet = poll(pfd, 1, 500);
		if (pollRet > 0) {
			if (pfd[0].revents != 0) {
				struct sockaddr_in si_other;
				socklen_t addrlen = sizeof(si_other);
				const ssize_t size = recvfromHttpcMessage(_udpMultiListen, MSG_DONTWAIT, &si_other, &addrlen);

				if (size > 0) {
					char ip_addr[25];
					// save client ip address
					strcpy(ip_addr, (char *)inet_ntoa(si_other.sin_addr));

					// @TODO we should probably listen to only one message
					// check do we hear our echo
					if (strcmp(_interface.getIPAddress().c_str(), ip_addr) != 0 && strcmp("127.0.0.1", ip_addr) != 0) {
						// get method from message
						std::string method;
						if (StringConverter::getMethod(_udpMultiListen.getMessage(), method)) {
							if (method.compare("NOTIFY") == 0) {
								std::string param;
								if (StringConverter::getHeaderFieldParameter(_udpMultiListen.getMessage(), "DEVICEID.SES.COM", param)) {
									// get device id of other
									unsigned int other_deviceId = atoi(param.c_str());

									// check server found with clashing DEVICEID?
									if (_properties.getDeviceID() == other_deviceId) {
										SI_LOG_INFO("Found SAT>IP Server %s: with clashing DEVICEID %d", ip_addr, other_deviceId);
										SocketClient udpSend;
										init_udp_socket(udpSend, SSDP_PORT, inet_addr(ip_addr));
										char msg[1024];
										// send message back
										snprintf(msg, sizeof(msg), UPNP_M_SEARCH, _interface.getIPAddress().c_str(), SSDP_PORT,
										         _properties.getSoftwareVersion().c_str(), _properties.getDeviceID());
										if (sendto(udpSend.getFD(), msg, strlen(msg), 0, (struct sockaddr *)&udpSend._addr, sizeof(udpSend._addr)) == -1) {
											PERROR("send");
										}
										udpSend.closeFD();
									} else {
										SI_LOG_INFO("Found SAT>IP Server %s: with DEVICEID %d", ip_addr, other_deviceId);
									}
								}
							} else if (method.compare("M-SEARCH") == 0) {
								std::string param;
								if (StringConverter::getHeaderFieldParameter(_udpMultiListen.getMessage(), "DEVICEID.SES.COM", param)) {
									// someone contacted us, so this should mean we have the same DEVICEID
									SI_LOG_INFO("SAT>IP Server %s: contacted us because of clashing DEVICEID %d", ip_addr, _properties.getDeviceID());

									// send message back
									char msg[1024];
									snprintf(msg, sizeof(msg), UPNP_M_SEARCH_OK, _interface.getIPAddress().c_str(), HTTP_PORT,
									        _properties.getSoftwareVersion().c_str(), _properties.getUUID().c_str(), _properties.getBootID(),
									        _properties.getDeviceID());
									if (sendto(_udpMultiSend.getFD(), msg, strlen(msg), 0, (struct sockaddr *)&si_other, sizeof(si_other)) == -1) {
										PERROR("send");
									}
									// we should increment DEVICEID and send bye bye
									_properties.setDeviceID(_properties.getDeviceID() + 1);
									send_byebye(_properties.getDeviceID(), _properties.getUUID().c_str());

									// now increment bootID
									_properties.setBootID(read_bootID_from_file("bootID"));
									SI_LOG_INFO("Changing BOOTID to: %d", _properties.getBootID());

									// reset repeat time to annouce new DEVICEID
									repeat_time =  time(NULL) + 5;
								}
								std::string st_param;
								if (StringConverter::getHeaderFieldParameter(_udpMultiListen.getMessage(), "ST", st_param)) {
									if (st_param.compare("urn:ses-com:device:SatIPServer:1") == 0) {
										// client is sending a discover
										SI_LOG_INFO("SAT>IP Client %s : tries to discover the network, sending an reply", ip_addr);

										// send message back
										char msg[1024];
										snprintf(msg, sizeof(msg), UPNP_M_SEARCH_OK, _interface.getIPAddress().c_str(), HTTP_PORT,
										         _properties.getSoftwareVersion().c_str(), _properties.getUUID().c_str(),
										         _properties.getBootID(), _properties.getDeviceID());
										if (sendto(_udpMultiSend.getFD(), msg, strlen(msg), 0, (struct sockaddr *)&si_other, sizeof(si_other)) == -1) {
											PERROR("send");
										}
									}
								}
							}

						}
					}
				}
			}
		}
		// Notify/announce ourself
		const time_t curr_time = time(NULL);
		if (repeat_time < curr_time) {
			char msg[1024];
			// set next announce time
			repeat_time = _properties.getSsdpAnnounceTimeSec() + curr_time;

			// broadcast message
			snprintf(msg, sizeof(msg), UPNP_ROOTDEVICE, _interface.getIPAddress().c_str(), HTTP_PORT,
			         _properties.getSoftwareVersion().c_str(), _properties.getUUID().c_str(), _properties.getBootID(), _properties.getDeviceID());
			if (sendto(_udpMultiSend.getFD(), msg, strlen(msg), 0, (struct sockaddr *)&_udpMultiSend._addr, sizeof(_udpMultiSend._addr)) == -1) {
				PERROR("send UPNP_ROOTDEVICE");
			}
			usleep(UTIME_DEL);

			// broadcast message
			snprintf(msg, sizeof(msg), UPNP_ALIVE, _interface.getIPAddress().c_str(), HTTP_PORT,
			         _properties.getSoftwareVersion().c_str(), _properties.getUUID().c_str(), _properties.getUUID().c_str(), _properties.getBootID(), _properties.getDeviceID());
			if (sendto(_udpMultiSend.getFD(), msg, strlen(msg), 0, (struct sockaddr *)&_udpMultiSend._addr, sizeof(_udpMultiSend._addr)) == -1) {
				PERROR("send UPNP_ALIVE");
			}
			usleep(UTIME_DEL);

			// broadcast message
			snprintf(msg, sizeof(msg), UPNP_DEVICE, _interface.getIPAddress().c_str(), HTTP_PORT,
			         _properties.getSoftwareVersion().c_str(), _properties.getUUID().c_str(), _properties.getBootID(), _properties.getDeviceID());
			if (sendto(_udpMultiSend.getFD(), msg, strlen(msg), 0, (struct sockaddr *)&_udpMultiSend._addr, sizeof(_udpMultiSend._addr)) == -1) {
				PERROR("send UPNP_DEVICE");
			}
		}
	}
}

bool SsdpServer::send_byebye(unsigned int bootId, const char *uuid) {
#define UPNP_ROOTDEVICE_BB "NOTIFY * HTTP/1.1\r\n" \
                           "HOST: 239.255.255.250:1900\r\n" \
                           "NT: upnp:rootdevice\r\n" \
                           "NTS: ssdp:byebye\r\n" \
                           "USN: uuid:%s::upnp:rootdevice\r\n" \
                           "BOOTID.UPNP.ORG: %d\r\n" \
                           "CONFIGID.UPNP.ORG: 0\r\n" \
                           "\r\n"

#define UPNP_BYEBYE "NOTIFY * HTTP/1.1\r\n" \
                    "HOST: 239.255.255.250:1900\r\n" \
                    "NT: uuid:%s\r\n" \
                    "NTS: ssdp:byebye\r\n" \
                    "USN: uuid:%s\r\n" \
                    "BOOTID.UPNP.ORG: %d\r\n" \
                    "CONFIGID.UPNP.ORG: 0\r\n" \
                    "\r\n"

#define UPNP_DEVICE_BB "NOTIFY * HTTP/1.1\r\n" \
                       "HOST: 239.255.255.250:1900\r\n" \
                       "NT: urn:ses-com:device:SatIPServer:1\r\n" \
                       "NTS: ssdp:byebye\r\n" \
                       "USN: uuid:%s::urn:ses-com:device:SatIPServer:1\r\n" \
                       "BOOTID.UPNP.ORG: %d\r\n" \
                       "CONFIGID.UPNP.ORG: 0\r\n" \
                       "\r\n"

	char msg[1024];

	// broadcast message
	snprintf(msg, sizeof(msg), UPNP_ROOTDEVICE_BB, uuid, bootId);
	if (sendto(_udpMultiSend.getFD(), msg, strlen(msg), 0, (struct sockaddr *)&_udpMultiSend._addr, sizeof(_udpMultiSend._addr)) == -1) {
		PERROR("send");
		return false;
	}
	usleep(UTIME_DEL);

	// broadcast message
	snprintf(msg, sizeof(msg), UPNP_BYEBYE, uuid, uuid, bootId);
	if (sendto(_udpMultiSend.getFD(), msg, strlen(msg), 0, (struct sockaddr *)&_udpMultiSend._addr, sizeof(_udpMultiSend._addr)) == -1) {
		PERROR("send");
		return false;
	}
	usleep(UTIME_DEL);

	// broadcast message
	snprintf(msg, sizeof(msg), UPNP_DEVICE_BB, uuid, bootId);
	if (sendto(_udpMultiSend.getFD(), msg, strlen(msg), 0, (struct sockaddr *)&_udpMultiSend._addr, sizeof(_udpMultiSend._addr)) == -1) {
		PERROR("send");
		return false;
	}

	return true;
}

unsigned int SsdpServer::read_bootID_from_file(const char *file) {
#define BOOTID_STR "bootID=%d"

	// Get BOOTID from file, increment and save it again
	unsigned int bootId = 0;
	int fd_bootId = -1;
	char content[50];
	if ((fd_bootId = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH)) > 0) {
		if (read(fd_bootId, content, sizeof(content)) >= 0) {
			if (strlen(content) != 0) {
				sscanf(content, BOOTID_STR, &bootId);
				lseek(fd_bootId, 0, SEEK_SET);
			}
		} else {
			PERROR("Unable to read file: bootID");
		}
		++bootId;
		sprintf(content, BOOTID_STR, bootId);
		if (write(fd_bootId, content, strlen(content)) == -1) {
			PERROR("Unable to write file: bootID");
		}
		CLOSE_FD(fd_bootId);
	} else {
		PERROR("Unable to open file: bootID");
	}
	return bootId;
}
