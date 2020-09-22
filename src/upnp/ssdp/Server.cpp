/* Server.cpp

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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
#include <upnp/ssdp/Server.h>

#include <InterfaceAttr.h>
#include <Properties.h>
#include <StringConverter.h>
#include <Log.h>
#include <Utils.h>

#include <chrono>
#include <thread>

#include <poll.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace upnp {
namespace ssdp {

#define UTIME_DEL 200000
#define SSDP_PORT 1900


// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

Server::Server(
		const std::string &bindIPAddress,
		const Properties &properties) :
	XMLSupport(),
	ThreadBase("SSDP Server"),
	_properties(properties),
	_bindIPAddress(bindIPAddress),
	_xmlDeviceDescriptionFile("Not Set"),
	_location("Not Set"),
	_announceTimeSec(60),
	_bootID(0),
	_deviceID(1) {}

Server::~Server() {
	cancelThread();
	joinThread();
	// we should send bye bye
	sendByeBye(_deviceID, _properties.getUUID().c_str());
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void Server::doAddToXML(std::string &xml) const {
	base::MutexLock lock(_mutex);
	ADD_XML_NUMBER_INPUT(xml, "annouceTime", _announceTimeSec, 0, 1800);
	ADD_XML_ELEMENT(xml, "bootID", _bootID);
	ADD_XML_ELEMENT(xml, "deviceID", _deviceID);
	size_t i = 0;
	for (auto server : servers) {
		ADD_XML_N_ELEMENT(xml, "satipserver", i, server.second);
		++i;
	}
}

void Server::doFromXML(const std::string &xml) {
	base::MutexLock lock(_mutex);
	std::string element;
	if (findXMLElement(xml, "annouceTime.value", element)) {
		_announceTimeSec = std::stoi(element.c_str());
	}
	if (findXMLElement(xml, "bootID", element)) {
		_bootID = std::stoi(element.c_str());
	}
	if (findXMLElement(xml, "deviceID", element)) {
		_deviceID = std::stoi(element.c_str());
	}
}

// =============================================================================
// -- Other member functions ---------------------------------------------------
// =============================================================================

void Server::threadEntry() {
	incrementBootID();
	SI_LOG_INFO("Setting up SSDP server with BOOTID: %d  annouce interval: %d Sec", _bootID, _announceTimeSec);

	// Get file and constuct new location
	constructLocation();

	initUDPSocket(_udpMultiSend, "239.255.255.250", SSDP_PORT);
	initMutlicastUDPSocket(_udpMultiListen, "239.255.255.250", _bindIPAddress, SSDP_PORT);

	std::time_t repeat_time = 0;
	struct pollfd pfd[1];
	pfd[0].fd = _udpMultiListen.getFD();
	pfd[0].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
	pfd[0].revents = 0;

	for (;; ) {
		// call poll with a timeout of 500 ms
		const int pollRet = poll(pfd, 1, 500);

		base::MutexLock lock(_mutex);

		if (pollRet > 0 && pfd[0].revents != 0) {
			sockaddr_in si_other;
			socklen_t addrlen = sizeof(si_other);
			const ssize_t size = recvfromHttpcMessage(_udpMultiListen, MSG_DONTWAIT, &si_other, &addrlen);
			if (size > 0) {
				checkReply(si_other, _udpMultiListen.getMessage());
			}
		}

		// Did the Device Description File change?
		if (_xmlDeviceDescriptionFile != _properties.getXMLDeviceDescriptionFile()) {
			// Get new file and constuct new location
			constructLocation();

			// we should send bye bye
			sendByeBye(_deviceID, _properties.getUUID().c_str());

			// now increment bootID
			incrementBootID();

			// reset repeat time to annouce new 'Device Description File'
			repeat_time =  std::time(nullptr) + 5;
		}

		// Notify/announce ourself
		const std::time_t curr_time = std::time(nullptr);
		if (repeat_time < curr_time) {
			// set next announce time
			repeat_time = _announceTimeSec + curr_time;

			sendAnnounce();
		}
	}
}

void Server::checkReply(
	struct sockaddr_in &si_other,
	const std::string &message) {
	// save client ip address
	const std::string ipAddress = inet_ntoa(si_other.sin_addr);

	// @TODO we should probably listen to only one message
	// check do we hear our echo, same UUID
	const std::string usn = StringConverter::getHeaderFieldParameter(message, "USN:");
	if (!usn.empty() && usn.substr(5, _properties.getUUID().size()) == _properties.getUUID()) {
		return;
	}
	// get method from message
	const std::string method = StringConverter::getMethod(message);
	if (!method.empty()) {
		const std::string deviceID = StringConverter::getHeaderFieldParameter(message, "DEVICEID.SES.COM:");
		const std::string st = StringConverter::getHeaderFieldParameter(message, "ST:");
		if (method == "NOTIFY") {
			if (!deviceID.empty() && usn.find("SatIPServer:1") != std::string::npos) {
				// check server found with clashing DEVICEID? we should defend it!!
				// get device id of other
				const unsigned int otherDeviceID = std::stoi(deviceID);
				checkDefendDeviceID(otherDeviceID, ipAddress);
			}
		} else if (method == "M-SEARCH") {
			if (!deviceID.empty()) {
				// someone contacted us, so this should mean we have the same DEVICEID
				sendGiveUpDeviceID(si_other, ipAddress);
			} else if (!st.empty()) {
				if (st == "urn:ses-com:device:SatIPServer:1") {
					sendSATIPClientDiscoverResponse(si_other, ipAddress);
				} else if (st == "upnp:rootdevice") {
					sendRootDeviceDiscoverResponse(si_other, ipAddress);
				}
			}
		}
	}
}

void Server::checkDefendDeviceID(
		unsigned int otherDeviceID,
		const std::string &ip_addr) {
	// check server found with clashing DEVICEID? we should defend it!!
	if (_deviceID == otherDeviceID) {
		SI_LOG_INFO("Found SAT>IP Server %s: with clashing DEVICEID %d defending", ip_addr.c_str(), otherDeviceID);
		SocketClient udpSend;
		initUDPSocket(udpSend, ip_addr, SSDP_PORT);
		// send message back
		const char *UPNP_M_SEARCH =
				"M-SEARCH * HTTP/1.1\r\n" \
				"HOST: %1:%2\r\n" \
				"MAN: \"ssdp:discover\"\r\n" \
				"ST: urn:ses-com:device:SatIPServer:1\r\n" \
				"USER-AGENT: Linux/1.0 UPnP/1.1 SatPI/%3\r\n" \
				"DEVICEID.SES.COM: %4\r\n" \
				"\r\n";
		const std::string msg = StringConverter::stringFormat(UPNP_M_SEARCH,
			_bindIPAddress,
			SSDP_PORT,
			_properties.getSoftwareVersion(),
			_deviceID);
		if (!udpSend.sendDataTo(msg.c_str(), msg.size(), 0)) {
			SI_LOG_ERROR("SSDP M_SEARCH data send failed");
		}
	} else if (servers.find(otherDeviceID) == servers.end()) {
		SI_LOG_INFO("Found SAT>IP Server %s: with DEVICEID %d", ip_addr.c_str(), otherDeviceID);
		servers[otherDeviceID] = ip_addr;
	}
}

void Server::sendGiveUpDeviceID(
		sockaddr_in &si_other,
		const std::string &ip_addr) {
	SI_LOG_INFO("SAT>IP Server %s: contacted us because of clashing DEVICEID %d", ip_addr.c_str(), _deviceID);

	// send message back
	const char *UPNP_M_SEARCH_OK =
			"HTTP/1.1 200 OK\r\n" \
			"CACHE-CONTROL: max-age=%1\r\n" \
			"EXT:\r\n" \
			"LOCATION: %2\r\n" \
			"SERVER: Linux/1.0 UPnP/1.1 SatPI/%3\r\n" \
			"ST: urn:ses-com:device:SatIPServer:1\r\n" \
			"USN: uuid:%4::urn:ses-com:device:SatIPServer:1\r\n" \
			"BOOTID.UPNP.ORG: %5\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"DEVICEID.SES.COM: %6\r\n" \
			"\r\n";

	const std::string msg = StringConverter::stringFormat(UPNP_M_SEARCH_OK,
		_announceTimeSec,
		_location,
		_properties.getSoftwareVersion(),
		_properties.getUUID(),
		_bootID,
		_deviceID);

	if (sendto(_udpMultiSend.getFD(), msg.c_str(), msg.size(), 0,
		reinterpret_cast<sockaddr *>(&si_other), sizeof(si_other)) == -1) {
		PERROR("send");
	}
	// we should increment DEVICEID and send bye bye
	incrementDeviceID();
	sendByeBye(_deviceID, _properties.getUUID().c_str());

	// now increment bootID
	incrementBootID();
}

void Server::sendSATIPClientDiscoverResponse(
		sockaddr_in &si_other,
		const std::string &ip_addr) {
	SI_LOG_INFO("SAT>IP Client %s : tries to discover the network, sending reply back", ip_addr.c_str());

	sendDiscoverResponse("urn:ses-com:device:SatIPServer:1", si_other);
}

void Server::sendRootDeviceDiscoverResponse(
		sockaddr_in &si_other,
		const std::string &ip_addr) {
	SI_LOG_INFO("Root Device Client %s : tries to discover the network, sending reply back", ip_addr.c_str());

	sendDiscoverResponse("upnp:rootdevice", si_other);
}

void Server::sendDiscoverResponse(
		const std::string &searchTarget,
		struct sockaddr_in &si_other) {
	// send message back
	const char *UPNP_M_SEARCH_OK =
			"HTTP/1.1 200 OK\r\n" \
			"CACHE-CONTROL: max-age=%1\r\n" \
			"EXT:\r\n" \
			"LOCATION: %2\r\n" \
			"SERVER: Linux/1.0 UPnP/1.1 SatPI/%3\r\n" \
			"ST: %4\r\n" \
			"USN: uuid:%5::urn:ses-com:device:SatIPServer:1\r\n" \
			"BOOTID.UPNP.ORG: %6\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"\r\n";

	const std::string msg = StringConverter::stringFormat(UPNP_M_SEARCH_OK,
		_announceTimeSec,
		_location,
		_properties.getSoftwareVersion(),
		searchTarget,
		_properties.getUUID(),
		_bootID);

	if (sendto(_udpMultiSend.getFD(), msg.c_str(), msg.size(), 0,
		reinterpret_cast<sockaddr *>(&si_other), sizeof(si_other)) == -1) {
		PERROR("send");
	}
}

void Server::sendAnnounce() {

	// broadcast message
	const char *UPNP_ROOTDEVICE =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"CACHE-CONTROL: max-age=%1\r\n" \
			"LOCATION: %2\r\n" \
			"NT: upnp:rootdevice\r\n" \
			"NTS: ssdp:alive\r\n" \
			"SERVER: Linux/1.0 UPnP/1.1 SatPI/%3\r\n" \
			"USN: uuid:%4::upnp:rootdevice\r\n" \
			"BOOTID.UPNP.ORG: %5\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"DEVICEID.SES.COM: %6\r\n" \
			"\r\n";
	const std::string msgRoot = StringConverter::stringFormat(UPNP_ROOTDEVICE,
		_announceTimeSec,
		_location,
		_properties.getSoftwareVersion(),
		_properties.getUUID(),
		_bootID,
		_deviceID);
	if (!_udpMultiSend.sendDataTo(msgRoot.c_str(), msgRoot.size(), 0)) {
		SI_LOG_ERROR("SSDP UPNP_ROOTDEVICE data send failed");
	}

	std::this_thread::sleep_for(std::chrono::microseconds(UTIME_DEL));

	// broadcast message
	const char *UPNP_ALIVE =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"CACHE-CONTROL: max-age=%1\r\n" \
			"LOCATION: %2\r\n" \
			"NT: uuid:%3\r\n" \
			"NTS: ssdp:alive\r\n" \
			"SERVER: Linux/1.0 UPnP/1.1 SatPI/%4\r\n" \
			"USN: uuid:%3\r\n" \
			"BOOTID.UPNP.ORG: %5\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"DEVICEID.SES.COM: %6\r\n" \
			"\r\n";
	const std::string msgAlive = StringConverter::stringFormat(UPNP_ALIVE,
		_announceTimeSec,
		_location,
		_properties.getUUID(),
		_properties.getSoftwareVersion(),
		_bootID,
		_deviceID);
	if (!_udpMultiSend.sendDataTo(msgAlive.c_str(), msgAlive.size(), 0)) {
		SI_LOG_ERROR("SSDP UPNP_ALIVE data send failed");
	}

	std::this_thread::sleep_for(std::chrono::microseconds(UTIME_DEL));

	// broadcast message
	const char *UPNP_DEVICE =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"CACHE-CONTROL: max-age=%1\r\n" \
			"LOCATION: %2\r\n" \
			"NT: urn:ses-com:device:SatIPServer:1\r\n" \
			"NTS: ssdp:alive\r\n" \
			"SERVER: Linux/1.0 UPnP/1.1 SatPI/%3\r\n" \
			"USN: uuid:%4::urn:ses-com:device:SatIPServer:1\r\n" \
			"BOOTID.UPNP.ORG: %5\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"DEVICEID.SES.COM: %6\r\n" \
			"\r\n";
	const std::string msgDevice = StringConverter::stringFormat(UPNP_DEVICE,
		_announceTimeSec,
		_location,
		_properties.getSoftwareVersion(),
		_properties.getUUID(),
		_bootID,
		_deviceID);
	if (!_udpMultiSend.sendDataTo(msgDevice.c_str(), msgDevice.size(), 0)) {
		SI_LOG_ERROR("SSDP UPNP_DEVICE data send failed");
	}

}

bool Server::sendByeBye(
		unsigned int bootId,
		const char *uuid) {
	// broadcast message
	const char *UPNP_ROOTDEVICE_BB =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"NT: upnp:rootdevice\r\n" \
			"NTS: ssdp:byebye\r\n" \
			"USN: uuid:%1::upnp:rootdevice\r\n" \
			"BOOTID.UPNP.ORG: %2\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"\r\n";
	const std::string msgRoot = StringConverter::stringFormat(UPNP_ROOTDEVICE_BB, uuid, bootId);
	if (!_udpMultiSend.sendDataTo(msgRoot.c_str(), msgRoot.size(), 0)) {
		SI_LOG_ERROR("SSDP UPNP_ROOTDEVICE_BB data send failed");
		return false;
	}

	std::this_thread::sleep_for(std::chrono::microseconds(UTIME_DEL));

	// broadcast message
	const char *UPNP_BYEBYE =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"NT: uuid:%1\r\n" \
			"NTS: ssdp:byebye\r\n" \
			"USN: uuid:%1\r\n" \
			"BOOTID.UPNP.ORG: %2\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"\r\n";
	const std::string msgByeBye = StringConverter::stringFormat(UPNP_BYEBYE, uuid, bootId);
	if (!_udpMultiSend.sendDataTo(msgByeBye.c_str(), msgByeBye.size(), 0)) {
		SI_LOG_ERROR("SSDP BYEBYE data send failed");
		return false;
	}

	std::this_thread::sleep_for(std::chrono::microseconds(UTIME_DEL));

	// broadcast message
	const char *UPNP_DEVICE_BB =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"NT: urn:ses-com:device:SatIPServer:1\r\n" \
			"NTS: ssdp:byebye\r\n" \
			"USN: uuid:%1::urn:ses-com:device:SatIPServer:1\r\n" \
			"BOOTID.UPNP.ORG: %2\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"\r\n";
	const std::string msgDevice = StringConverter::stringFormat(UPNP_DEVICE_BB, uuid, bootId);
	if (!_udpMultiSend.sendDataTo(msgDevice.c_str(), msgDevice.size(), 0)) {
		SI_LOG_ERROR("SSDP UPNP_DEVICE_BB data send failed");
		return false;
	}

	std::this_thread::sleep_for(std::chrono::microseconds(UTIME_DEL));
	return true;
}

void Server::incrementDeviceID() {
	++_deviceID;
	notifyChanges();
}

void Server::incrementBootID() {
	++_bootID;
	notifyChanges();
}

void Server::constructLocation() {
	_xmlDeviceDescriptionFile = _properties.getXMLDeviceDescriptionFile();
	_location = StringConverter::stringFormat("http://%1:%2/%3",
		_bindIPAddress,
		_properties.getHttpPort(),
		_xmlDeviceDescriptionFile);
}

} // namespace ssdp
} // namespace upnp
