/* DvbapiClient.cpp

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
#include "DvbapiClient.h"
#include "Log.h"
#include "SocketClient.h"
#include "StringConverter.h"
#include "StreamProperties.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef LIBDVBCSA
extern "C" {
	#include <dvbcsa/dvbcsa.h>
}
#endif

#include <poll.h>

#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

//constants used in socket communication:
#define DVBAPI_PROTOCOL_VERSION         1

#define DVBAPI_CA_SET_PID      0x40086f87
#define DVBAPI_CA_SET_DESCR    0x40106f86
#define DVBAPI_DMX_SET_FILTER  0x403c6f2b
#define DVBAPI_DMX_STOP        0x00006f2a

#define DVBAPI_AOT_CA          0x9F803000
#define DVBAPI_AOT_CA_PMT      0x9F803200  //least significant byte is length (ignored)
#define DVBAPI_AOT_CA_STOP     0x9F803F04
#define DVBAPI_FILTER_DATA     0xFFFF0000
#define DVBAPI_CLIENT_INFO     0xFFFF0001
#define DVBAPI_SERVER_INFO     0xFFFF0002

#define PAT_TABLE_ID           0x00
#define PMT_TABLE_ID           0x02

DvbapiClient::DvbapiClient() :
		ThreadBase("DvbapiClient") {
	startThread();
}

DvbapiClient::~DvbapiClient() {
	cancelThread();
	joinThread();
}

bool DvbapiClient::decrypt(StreamProperties &properties, unsigned char *data, int len) {
	MutexLock lock(_mutex);

	unsigned char *ptr = data;
	for (int i = 0; i < len; i += 188) {
		// Check is the the begin of TS
		if (ptr[0] == 0x47) {
			// get PID from TS
			const int pid = ((ptr[1] & 0x1f) << 8) | ptr[2];

			// Check PAT pid and start indicator and table ID
			if (pid == 0) {
				collectPAT(properties, ptr, 188);
			} else if (properties.isPMTPID(pid)) {
				collectPMT(properties, ptr, 188);
			}
		}
		// goto next TS packet
		ptr += 188;
	}

	return true;
}

bool DvbapiClient::initClientSocket(SocketClient &client, int port, in_addr_t s_addr) {
	// fill in the socket structure with host information
	memset(&client._addr, 0, sizeof(client._addr));
	client._addr.sin_family      = AF_INET;
	client._addr.sin_addr.s_addr = s_addr;
	client._addr.sin_port        = htons(port);

	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM /*| SOCK_NONBLOCK*/, 0)) == -1) {
		PERROR("socket send");
		return false;
	}
	client.setFD(fd);

	int val = 1;
	if (setsockopt(client.getFD(), SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		PERROR("setsockopt SO_REUSEADDR");
		return false;
	}

	if (connect(client.getFD(), (struct sockaddr *)&client._addr, sizeof(client._addr)) == -1) {
		PERROR("connect");
		return false;
	}
	return true;
}

void DvbapiClient::sendClientInfo() {
	extern const char *satpi_version;
	std::string name = "SatIP ";
	name += satpi_version;

	int len = name.size() - 1; // ignoring null termination
	unsigned char buff[7 + len];
	uint32_t req = htonl(DVBAPI_CLIENT_INFO);
	memcpy(&buff[0], &req, 4);
	int16_t proto_version = htons(DVBAPI_PROTOCOL_VERSION);
	memcpy(&buff[4], &proto_version, 2);
	buff[6] = len;
	memcpy(&buff[7], name.c_str(), len);

	if (send(_client.getFD(), buff, sizeof(buff), MSG_DONTWAIT) == -1) {
		SI_LOG_ERROR("write failed");
	}
}

void DvbapiClient::collectPAT(StreamProperties &properties, unsigned char *data, int len) {
	if (!properties.isPATCollected()) {
		const int streamID = properties.getStreamID();
		const unsigned char options = (data[1] & 0xE0);
		if (options == 0x40 && data[5] == PAT_TABLE_ID) {
			const int pid           = ((data[1] & 0x1f) << 8) | data[2];
			const int cc            =   data[3] & 0x0f;
			const int sectionLength = ((data[6] & 0x0F) << 8) | data[7];
			// Add PAT Data
			if (properties.addPATData(data, 188, pid, cc)) {
				// Check did we finish collecting PAT Data
				if (sectionLength <= 183) {
					properties.setPATCollected(true);
				}
			} else {
				SI_LOG_ERROR("Stream: %d, PAT: Unable to add data!", streamID);
			}
		} else {
			const unsigned char *cData = properties.getPATData();
			const int sectionLength    = ((cData[6] & 0x0F) << 8) | cData[7];

			const int pid = ((data[1] & 0x1f) << 8) | data[2];
			const int cc  =   data[3] & 0x0f;

			// Add PAT Data without TS Header
			if (properties.addPATData(data + 4, 188 - 4, pid, cc)) { // 4 = TS Header
				const int patDataSize = properties.getPATDataSize();
				SI_LOG_DEBUG("Stream: %d, PAT: sectionLength: %d  patDataSize: %d", streamID, sectionLength, patDataSize);
				// Check did we finish collecting PAT Data
				if (sectionLength == (patDataSize - 9)) { // 9 = Untill PAT Section Length
					properties.setPMTCollected(true);
				}
			} else {
				SI_LOG_ERROR("Stream: %d, PAT: Unable to add data!", streamID);
			}
		}
		// Did we finish collecting PAT
		if (properties.isPATCollected()) {
			const unsigned char *data = properties.getPATData();
			const int sectionLength = ((data[ 6] & 0x0F) << 8) | data[ 7];
			const int tid           =  (data[ 8]         << 8) | data[ 9];
			const int version       =   data[10];
			const int secNr         =   data[11];
			const int lastSecNr     =   data[12];
			const int nLoop         =   data[13];
			SI_LOG_INFO("Stream: %d, PAT: Section Length: %d  TID: %d  Version: %d  secNr: %d lastSecNr: %d  nLoop: %d",
						 streamID, sectionLength, tid, version, secNr, lastSecNr, nLoop);

			len = sectionLength - 4 - 6; // 4 = CRC  6 = PAT Table begin from section length
			// skip to Table begin and iterate over entries
			data += 13;
			for (int i = 0; i < len; i += 4) {
				// Get PAT entry
				const uint16_t prognr =  (data[0] << 8) | data[1];
				const uint16_t pid    = ((data[2] & 0x1F) << 8) | data[3];
				if (prognr == 0) {
					SI_LOG_INFO("Stream: %d, PAT: Prog NR: %d  NIT: %d", streamID, prognr, pid);
				} else {
					SI_LOG_INFO("Stream: %d, PAT: Prog NR: %d  PMT: %d", streamID, prognr, pid);
					properties.setPMTPID(true, pid);
				}
				// goto next PAT entry
				data += 4;
			}
		}
	}
}

void DvbapiClient::collectPMT(StreamProperties &properties, unsigned char *data, int len) {
	if (!properties.isPMTCollected()) {
		const int streamID = properties.getStreamID();
		const unsigned char options = (data[1] & 0xE0);
		// Check if this has the Payload unit start indicator
		if (options == 0x40 && data[5] == PMT_TABLE_ID) {
			const int pid           = ((data[1] & 0x1f) << 8) | data[2];
			const int cc            =   data[3] & 0x0f;
			const int sectionLength = ((data[6] & 0x0F) << 8) | data[7];
			// Add PMT Data
			if (properties.addPMTData(data, 188, pid, cc)) {
				// Check did we finish collecting PMT Data
				if (sectionLength <= 183) {
					properties.setPMTCollected(true);
				}
			} else {
				SI_LOG_ERROR("Stream: %d, Unable to add PMT data!", streamID);
			}
		} else {
			const unsigned char *cData = properties.getPMTData();
			const int sectionLength    = ((cData[6] & 0x0F) << 8) | cData[7];

			const int pid = ((data[1] & 0x1f) << 8) | data[2];
			const int cc  =   data[3] & 0x0f;

			// Add PMT Data without TS Header
			if (properties.addPMTData(data + 4, 188 - 4, pid, cc)) { // 4 = TS Header
				const int pmtDataSize = properties.getPMTDataSize();
				SI_LOG_DEBUG("Stream: %d, PMT: sectionLength: %d  pmtDataSize: %d", streamID, sectionLength, pmtDataSize);
				// Check did we finish collecting PMT Data
				if (sectionLength == (pmtDataSize - 9)) { // 9 = // 9 = Untill PMT Section Length
					properties.setPMTCollected(true);
				}
			} else {
				SI_LOG_ERROR("Stream: %d, Unable to add PMT data!", streamID);
			}
		}
		// Did we finish collecting PMT
		if (properties.isPMTCollected()) {
			const unsigned char *data = properties.getPMTData();

			// Parse PMT Data
			const int sectionLength = ((data[ 6] & 0x0F) << 8) | data[ 7];
			const int programNumber = ((data[ 8]       ) << 8) | data[ 9];
			const int version       =   data[10];
//			const int secNr         =   data[11];  // always 0x00
//			const int lastSecNr     =   data[12];  // always 0x00
			const int pcrPID        = ((data[13] & 0x1F) << 8) | data[14];
			const int programLength = ((data[15] & 0x0F) << 8) | data[16];

			//                                00      01        02       03     04         05      06        07      08        09      10       11        12      13        14      15        16       17       18       19        20      21      22
			SI_LOG_DEBUG("Stream: %d, PMT: [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X] [0x%02X]",
						 streamID, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15], data[16],
						 data[17], data[18], data[19], data[20], data[21], data[22]);

			SI_LOG_INFO("Stream: %d, PMT - Section Length: %d  Prog NR: %d  Version: %d  PCR-PID: %d  Program Length: %d",
						streamID, sectionLength, programNumber, version, pcrPID, programLength);

			len = sectionLength - 4 - 9 - programLength; // 4 = CRC   9 = PMT Header from section length
			// skip to ES Table begin and iterate over entries
			data += 17 + programLength;
			for (int i = 0; i < len; ) {
				const int streamType    =    data[0];
				const int elementaryPID = (((data[1] & 0x1F) << 8) | data[2]);
				const int esInfoLength  = (((data[3] & 0x0F) << 8) | data[4]);

				SI_LOG_INFO("Stream: %d, PMT - Stream Type: %d  ES PID: %d  ES-Length: %d",
							streamID, streamType, elementaryPID, esInfoLength);

				// goto next ES entry
				data += esInfoLength + 5;
				i += esInfoLength + 5;
			}
			// Send it here !!
		}
	}
}

void DvbapiClient::threadEntry() {
	SI_LOG_INFO("Setting up DVBAPI client");
/*
	dvbcsa_bs_key_s *key;
	key = dvbcsa_bs_key_alloc();
	SI_LOG_INFO("Key %p", key);
*/
	initClientSocket(_client, 15011, inet_addr("192.168.0.109"));

	struct pollfd pfd[1];
	pfd[0].fd = _client.getFD();
	pfd[0].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
	pfd[0].revents = 0;

	sendClientInfo();

	for (;;) {
		// call poll with a timeout of 500 ms
		const int pollRet = poll(pfd, 1, 500);
		if (pollRet > 0) {
			if (pfd[0].revents != 0) {
				char buf[1024];
				struct sockaddr_in si_other;
				socklen_t addrlen = sizeof(si_other);
				const ssize_t size = recvfrom(_client.getFD(), buf, sizeof(buf)-1, MSG_DONTWAIT, (struct sockaddr *)&si_other, &addrlen);
				if (size > 0) {
					// get command
					const unsigned int cmd = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
					if (cmd == DVBAPI_SERVER_INFO) {
						SI_LOG_INFO("Connected to %s", &buf[7]);
					} else {
						SI_LOG_INFO("Receive size %d", size);
						for (ssize_t i = 0; i <= size; ++i) {
							SI_LOG_INFO("Receive %02x", buf[i]);
						}
					}
				}
			}
		}
	}
}
