/* Server.cpp

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

namespace upnp::ssdp {

#define UTIME_DEL 200000
#define SSDP_PORT 1900


// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

Server::Server(
		const int ssdpTTL,
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
	_deviceID(1),
	_ttl(ssdpTTL) {}

Server::~Server() {
	cancelThread();
	joinThread();
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void Server::doAddToXML(std::string &xml) const {
	base::MutexLock lock(_mutex);
	ADD_XML_NUMBER_INPUT(xml, "annouceTime", _announceTimeSec, 0, 1800);
	ADD_XML_ELEMENT(xml, "bootID", _bootID);
	ADD_XML_ELEMENT(xml, "deviceID", _deviceID);
	ADD_XML_ELEMENT(xml, "ttl", _ttl);
	size_t i = 0;
	for (const auto& server : _servers) {
		ADD_XML_N_ELEMENT(xml, "satipserver", i, server.second);
		++i;
	}
}

void Server::doFromXML(const std::string &xml) {
	base::MutexLock lock(_mutex);
	std::string element;
	if (findXMLElement(xml, "annouceTime.value", element)) {
		_announceTimeSec = std::stoi(element.data());
	}
	if (findXMLElement(xml, "bootID", element)) {
		_bootID = std::stoi(element.data());
	}
	if (findXMLElement(xml, "deviceID", element)) {
		_deviceID = std::stoi(element.data());
	}
}

// =============================================================================
// -- Other member functions ---------------------------------------------------
// =============================================================================

void Server::threadEntry() {
	incrementBootID();
	SI_LOG_INFO("Setting up SSDP server with BOOTID: @#1  annouce interval: @#2 Sec  TTL: @#3",
			_bootID, _announceTimeSec, _ttl);

	// Get file and constuct new location
	constructLocation();

	SocketClient udpMultiSend;
	SocketClient udpMultiListen;
	initUDPSocket(udpMultiSend, "239.255.255.250", SSDP_PORT, _ttl);
	initMutlicastUDPSocket(udpMultiListen, "239.255.255.250", _bindIPAddress, SSDP_PORT, _ttl);

	std::time_t repeat_time = 0;
	struct pollfd pfd[1];
	pfd[0].fd = udpMultiListen.getFD();
	pfd[0].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
	pfd[0].revents = 0;

	for (;; ) {
		// call poll with a timeout of 500 ms
		const int pollRet = poll(pfd, 1, 500);

		base::MutexLock lock(_mutex);

		if (pollRet > 0 && pfd[0].revents != 0) {
			sockaddr_in si_other;
			socklen_t addrlen = sizeof(si_other);
			const ssize_t size = recvfromHttpcMessage(udpMultiListen, MSG_DONTWAIT, &si_other, &addrlen);
			if (size > 0) {
				checkReply(udpMultiListen, udpMultiSend, si_other);
			}
		}

		// Did the Device Description File change?
		if (_xmlDeviceDescriptionFile != _properties.getXMLDeviceDescriptionFile()) {
			// Get new file and constuct new location
			constructLocation();

			// we should send bye bye
			sendByeBye(udpMultiSend, _deviceID, _properties.getUUID());

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

			sendAnnounce(udpMultiSend);
		}
	}
	// we should send bye bye
	sendByeBye(udpMultiSend, _deviceID, _properties.getUUID().data());
}

void Server::checkReply(
	SocketClient& udpMultiListen,
	SocketClient& udpMultiSend,
	struct sockaddr_in& si_other) {
	// save client ip address
	const std::string ipAddress = inet_ntoa(si_other.sin_addr);

	HeaderVector headers = udpMultiListen.getHeaders();

	// @TODO we should probably listen to only one message
	// check do we hear our echo, same UUID
	const std::string usn = headers.getFieldParameter("USN");
	if (!usn.empty() && usn.substr(5, _properties.getUUID().size()) == _properties.getUUID()) {
		return;
	}

	// get method from message
	const std::string method = udpMultiListen.getMethod();
	if (!method.empty()) {
		const std::string deviceID = headers.getFieldParameter("DEVICEID.SES.COM");
		const std::string st = headers.getFieldParameter("ST");
		if (method == "NOTIFY") {
			if (!deviceID.empty() && usn.find("SatIPServer:1") != std::string::npos) {
				// check server found with clashing DEVICEID? we should defend it!!
				// get device id of other
				StringVector server = StringConverter::split(headers.getFieldParameter("SERVER"), " ");
				const unsigned int otherDeviceID = std::stoi(deviceID);
				checkDefendDeviceID(otherDeviceID, ipAddress, (server.size() >= 2) ? server[2] : "Unkown");
			}
		} else if (method == "M-SEARCH") {
			if (!deviceID.empty()) {
				// someone contacted us, so this should mean we have the same DEVICEID
				sendGiveUpDeviceID(udpMultiSend, si_other, ipAddress);
			} else if (!st.empty()) {
				StringVector userAgent = StringConverter::split(headers.getFieldParameter("USER-AGENT"), " ");
				if (st == "urn:ses-com:device:SatIPServer:1") {
					sendSATIPClientDiscoverResponse(
						udpMultiSend, si_other, ipAddress, (userAgent.size() >= 2) ? userAgent[2] : "Unkown");
				} else if (st == "upnp:rootdevice") {
					sendRootDeviceDiscoverResponse(
						udpMultiSend, si_other, ipAddress, (userAgent.size() >= 2) ? userAgent[2] : "Unkown");
				}
			}
		}
	}
}

void Server::checkDefendDeviceID(
		unsigned int otherDeviceID,
		const std::string_view ip_addr,
		const std::string& server) {
	// check server found with clashing DEVICEID? we should defend it!!
	if (_deviceID == otherDeviceID) {
		SI_LOG_INFO("Found SAT>IP Server @#1 [@#2]: with clashing DEVICEID @#3 defending", ip_addr, server, otherDeviceID);
		SocketClient udpSend;
		initUDPSocket(udpSend, ip_addr, SSDP_PORT, _ttl);
		// send message back
		const char *UPNP_M_SEARCH =
				"M-SEARCH * HTTP/1.1\r\n" \
				"HOST: @#1:@#2\r\n" \
				"MAN: \"ssdp:discover\"\r\n" \
				"ST: urn:ses-com:device:SatIPServer:1\r\n" \
				"USER-AGENT: Linux/1.0 UPnP/1.1 @#3\r\n" \
				"DEVICEID.SES.COM: @#4\r\n" \
				"\r\n";
		const std::string msg = StringConverter::stringFormat(UPNP_M_SEARCH,
			_bindIPAddress,
			SSDP_PORT,
			_properties.getUPnPVersion(),
			_deviceID);
		if (!udpSend.sendDataTo(msg.data(), msg.size(), 0)) {
			SI_LOG_ERROR("SSDP M_SEARCH data send failed");
		}
	} else if (_servers.find(otherDeviceID) == _servers.end()) {
		SI_LOG_INFO("Found SAT>IP Server @#1 [@#2]: with DEVICEID @#3", ip_addr, server, otherDeviceID);
		_servers[otherDeviceID] = StringConverter::stringFormat("@#1 [@#2]", ip_addr, server);
	}
}

void Server::sendGiveUpDeviceID(
		SocketClient& udpMultiSend,
		sockaddr_in &si_other,
		const std::string &ip_addr) {
	SI_LOG_INFO("SAT>IP Server @#1: contacted us because of clashing DEVICEID @#2", ip_addr, _deviceID);

	// send message back
	const char *UPNP_M_SEARCH_OK =
			"HTTP/1.1 200 OK\r\n" \
			"CACHE-CONTROL: max-age=@#1\r\n" \
			"EXT:\r\n" \
			"LOCATION: @#2\r\n" \
			"SERVER: Linux/1.0 UPnP/1.1 @#3\r\n" \
			"ST: urn:ses-com:device:SatIPServer:1\r\n" \
			"USN: uuid:@#4::urn:ses-com:device:SatIPServer:1\r\n" \
			"BOOTID.UPNP.ORG: @#5\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"DEVICEID.SES.COM: @#6\r\n" \
			"\r\n";

	const std::string msg = StringConverter::stringFormat(UPNP_M_SEARCH_OK,
		_announceTimeSec,
		_location,
		_properties.getUPnPVersion(),
		_properties.getUUID(),
		_bootID,
		_deviceID);

	if (::sendto(udpMultiSend.getFD(), msg.data(), msg.size(), 0,
		reinterpret_cast<sockaddr *>(&si_other), sizeof(si_other)) == -1) {
		SI_LOG_PERROR("send");
	}
	// we should increment DEVICEID and send bye bye
	incrementDeviceID();
	sendByeBye(udpMultiSend, _deviceID, _properties.getUUID());

	// now increment bootID
	incrementBootID();
}

void Server::sendSATIPClientDiscoverResponse(
		SocketClient& udpMultiSend,
		sockaddr_in &si_other,
		const std::string &ip_addr,
		const std::string& userAgent) {
	SI_LOG_INFO("SAT>IP Client @#1 [@#2]: tries to discover the network, sending reply back", ip_addr, userAgent);

	sendDiscoverResponse(udpMultiSend, "urn:ses-com:device:SatIPServer:1", si_other);
}

void Server::sendRootDeviceDiscoverResponse(
		SocketClient& udpMultiSend,
		sockaddr_in &si_other,
		const std::string &ip_addr,
		const std::string& userAgent) {
	SI_LOG_INFO("Root Device Client @#1 [@#2]: tries to discover the network, sending reply back", ip_addr, userAgent);

	sendDiscoverResponse(udpMultiSend, "upnp:rootdevice", si_other);
}

void Server::sendDiscoverResponse(
		SocketClient& udpMultiSend,
		const std::string &searchTarget,
		struct sockaddr_in &si_other) {
	// send message back
	const char *UPNP_M_SEARCH_OK =
			"HTTP/1.1 200 OK\r\n" \
			"CACHE-CONTROL: max-age=@#1\r\n" \
			"EXT:\r\n" \
			"LOCATION: @#2\r\n" \
			"SERVER: Linux/1.0 UPnP/1.1 @#3\r\n" \
			"ST: @#4\r\n" \
			"USN: uuid:@#5::urn:ses-com:device:SatIPServer:1\r\n" \
			"BOOTID.UPNP.ORG: @#6\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"\r\n";

	const std::string msg = StringConverter::stringFormat(UPNP_M_SEARCH_OK,
		_announceTimeSec,
		_location,
		_properties.getUPnPVersion(),
		searchTarget,
		_properties.getUUID(),
		_bootID);

	if (::sendto(udpMultiSend.getFD(), msg.data(), msg.size(), 0,
		reinterpret_cast<sockaddr *>(&si_other), sizeof(si_other)) == -1) {
		SI_LOG_PERROR("send");
	}
}

void Server::sendAnnounce(SocketClient& udpMultiSend) {
	// broadcast message
	const char *UPNP_ROOTDEVICE =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"CACHE-CONTROL: max-age=@#1\r\n" \
			"LOCATION: @#2\r\n" \
			"NT: upnp:rootdevice\r\n" \
			"NTS: ssdp:alive\r\n" \
			"SERVER: Linux/1.0 UPnP/1.1 @#3\r\n" \
			"USN: uuid:@#4::upnp:rootdevice\r\n" \
			"BOOTID.UPNP.ORG: @#5\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"DEVICEID.SES.COM: @#6\r\n" \
			"\r\n";
	const std::string msgRoot = StringConverter::stringFormat(UPNP_ROOTDEVICE,
		_announceTimeSec,
		_location,
		_properties.getUPnPVersion(),
		_properties.getUUID(),
		_bootID,
		_deviceID);
	if (!udpMultiSend.sendDataTo(msgRoot.data(), msgRoot.size(), 0)) {
		SI_LOG_ERROR("SSDP UPNP_ROOTDEVICE data send failed");
	}

	std::this_thread::sleep_for(std::chrono::microseconds(UTIME_DEL));

	// broadcast message
	const char *UPNP_ALIVE =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"CACHE-CONTROL: max-age=@#1\r\n" \
			"LOCATION: @#2\r\n" \
			"NT: uuid:@#3\r\n" \
			"NTS: ssdp:alive\r\n" \
			"SERVER: Linux/1.0 UPnP/1.1 @#4\r\n" \
			"USN: uuid:@#3\r\n" \
			"BOOTID.UPNP.ORG: @#5\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"DEVICEID.SES.COM: @#6\r\n" \
			"\r\n";
	const std::string msgAlive = StringConverter::stringFormat(UPNP_ALIVE,
		_announceTimeSec,
		_location,
		_properties.getUUID(),
		_properties.getUPnPVersion(),
		_bootID,
		_deviceID);
	if (!udpMultiSend.sendDataTo(msgAlive.data(), msgAlive.size(), 0)) {
		SI_LOG_ERROR("SSDP UPNP_ALIVE data send failed");
	}

	std::this_thread::sleep_for(std::chrono::microseconds(UTIME_DEL));

	// broadcast message
	const char *UPNP_DEVICE =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"CACHE-CONTROL: max-age=@#1\r\n" \
			"LOCATION: @#2\r\n" \
			"NT: urn:ses-com:device:SatIPServer:1\r\n" \
			"NTS: ssdp:alive\r\n" \
			"SERVER: Linux/1.0 UPnP/1.1 @#3\r\n" \
			"USN: uuid:@#4::urn:ses-com:device:SatIPServer:1\r\n" \
			"BOOTID.UPNP.ORG: @#5\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"DEVICEID.SES.COM: @#6\r\n" \
			"\r\n";
	const std::string msgDevice = StringConverter::stringFormat(UPNP_DEVICE,
		_announceTimeSec,
		_location,
		_properties.getUPnPVersion(),
		_properties.getUUID(),
		_bootID,
		_deviceID);
	if (!udpMultiSend.sendDataTo(msgDevice.data(), msgDevice.size(), 0)) {
		SI_LOG_ERROR("SSDP UPNP_DEVICE data send failed");
	}
}

bool Server::sendByeBye(
		SocketClient& udpMultiSend,
		unsigned int bootId,
		const std::string_view uuid) {
	// broadcast message
	const char *UPNP_ROOTDEVICE_BB =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"NT: upnp:rootdevice\r\n" \
			"NTS: ssdp:byebye\r\n" \
			"USN: uuid:@#1::upnp:rootdevice\r\n" \
			"BOOTID.UPNP.ORG: @#2\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"\r\n";
	const std::string msgRoot = StringConverter::stringFormat(UPNP_ROOTDEVICE_BB, uuid, bootId);
	if (!udpMultiSend.sendDataTo(msgRoot.data(), msgRoot.size(), 0)) {
		SI_LOG_ERROR("SSDP UPNP_ROOTDEVICE_BB data send failed");
		return false;
	}

	std::this_thread::sleep_for(std::chrono::microseconds(UTIME_DEL));

	// broadcast message
	const char *UPNP_BYEBYE =
			"NOTIFY * HTTP/1.1\r\n" \
			"HOST: 239.255.255.250:1900\r\n" \
			"NT: uuid:@#1\r\n" \
			"NTS: ssdp:byebye\r\n" \
			"USN: uuid:@#1\r\n" \
			"BOOTID.UPNP.ORG: @#2\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"\r\n";
	const std::string msgByeBye = StringConverter::stringFormat(UPNP_BYEBYE, uuid, bootId);
	if (!udpMultiSend.sendDataTo(msgByeBye.data(), msgByeBye.size(), 0)) {
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
			"USN: uuid:@#1::urn:ses-com:device:SatIPServer:1\r\n" \
			"BOOTID.UPNP.ORG: @#2\r\n" \
			"CONFIGID.UPNP.ORG: 0\r\n" \
			"\r\n";
	const std::string msgDevice = StringConverter::stringFormat(UPNP_DEVICE_BB, uuid, bootId);
	if (!udpMultiSend.sendDataTo(msgDevice.data(), msgDevice.size(), 0)) {
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
	_location = StringConverter::stringFormat("http://@#1:@#2/@#3",
		_bindIPAddress,
		_properties.getHttpPort(),
		_xmlDeviceDescriptionFile);
}

}
