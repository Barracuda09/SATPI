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
#define DVBAPI_AOT_CA_PMT      0x9F803282
#define DVBAPI_AOT_CA_STOP     0x9F803F04
#define DVBAPI_FILTER_DATA     0xFFFF0000
#define DVBAPI_CLIENT_INFO     0xFFFF0001
#define DVBAPI_SERVER_INFO     0xFFFF0002

#define LIST_ONLY              0x03

#define PAT_TABLE_ID           0x00
#define PMT_TABLE_ID           0x02

DvbapiClient::DvbapiClient() :
		ThreadBase("DvbapiClient"),
		_connected(false) {
	_key[0] = NULL;
	_key[1] = NULL;
	startThread();
}

DvbapiClient::~DvbapiClient() {
	dvbcsa_key_free(_key[0]);
	dvbcsa_key_free(_key[1]);
	cancelThread();
	joinThread();
}

bool DvbapiClient::decrypt(StreamProperties &properties, unsigned char *data, int len) {
	MutexLock lock(_mutex);
	if (_connected) {
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
				} else if (properties.isECMPID(pid)) {
					collectECM(properties, ptr, 188);
				} else if (ptr[3] & 0x80) {
					// scrambled TS packet with even(0) or odd(1) key
					const int parity = (ptr[3] & 0x40) > 0;

					if (_key[parity]) {
						// check is there an adaptation field we should skip
						int skip = 4;
						if((ptr[3] & 0x20) && (ptr[4] < 183)) {
							skip += ptr[4] + 1;
						}
						dvbcsa_decrypt(_key[parity], ptr + skip, 188 - skip);

						// clear scramble flag
						ptr[3] &= 0x3F;
					}
				}
			}
			// goto next TS packet
			ptr += 188;
		}
	}
	return true;
}

bool DvbapiClient::stopDecrypt(StreamProperties &properties) {
	unsigned char caPMT[8];
	const int streamID = properties.getStreamID();

	dvbcsa_key_free(_key[0]);
	dvbcsa_key_free(_key[1]);
	_key[0] = NULL;
	_key[1] = NULL;

	// Stop 9F 80 3f 04 83 02 00 <demux index>
	caPMT[0] = (DVBAPI_AOT_CA_STOP >> 24) & 0xFF;
	caPMT[1] = (DVBAPI_AOT_CA_STOP >> 16) & 0xFF;
	caPMT[2] = (DVBAPI_AOT_CA_STOP >>  8) & 0xFF;
	caPMT[3] =  DVBAPI_AOT_CA_STOP & 0xFF;
	caPMT[4] = 0x83;
	caPMT[5] = 0x02;
	caPMT[6] = 0x00;
	caPMT[7] = 0xFF;  // demux index wildcard

	SI_LOG_BIN_DEBUG(caPMT, sizeof(caPMT), "Stream: %d, PMT Stop DMX", streamID);

	if (send(_client.getFD(), caPMT, sizeof(caPMT), MSG_DONTWAIT) == -1) {
		SI_LOG_ERROR("Stream: %d, PMT - send data to server failed", streamID);
		return false;
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
	std::string name = "SatPI ";
	name += satpi_version;

	int len = name.size() - 1; // ignoring null termination
	unsigned char buff[7 + len];
	uint32_t req = htonl(DVBAPI_CLIENT_INFO);
	memcpy(&buff[0], &req, 4);
	int16_t proto_version = htons(DVBAPI_PROTOCOL_VERSION);
	memcpy(&buff[4], &proto_version, 2);
	buff[6] = len;
	memcpy(&buff[7], name.c_str(), len);

	if (write(_client.getFD(), buff, sizeof(buff)) == -1) {
		SI_LOG_ERROR("write failed");
	}
}

void DvbapiClient::collectPAT(StreamProperties &properties, const unsigned char *data, int len) {
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
			if (properties.addPATData(&data[4], 188 - 4, pid, cc)) { // 4 = TS Header
				const int patDataSize = properties.getPATDataSize();
				SI_LOG_DEBUG("Stream: %d, PAT: sectionLength: %d  patDataSize: %d", streamID, sectionLength, patDataSize);
				// Check did we finish collecting PAT Data
				if (sectionLength <= (patDataSize - 9)) { // 9 = Untill PAT Section Length
					properties.setPMTCollected(true);
				}
			} else {
				SI_LOG_ERROR("Stream: %d, PAT: Unable to add data!", streamID);
			}
		}
		// Did we finish collecting PAT
		if (properties.isPATCollected()) {
			const unsigned char *cData = properties.getPATData();
			// Parse PAT Data
			const int sectionLength = ((cData[ 6] & 0x0F) << 8) | cData[ 7];
			const int tid           =  (cData[ 8]         << 8) | cData[ 9];
			const int version       =   cData[10];
			const int secNr         =   cData[11];
			const int lastSecNr     =   cData[12];
			const int nLoop         =   cData[13];
			SI_LOG_INFO("Stream: %d, PAT: Section Length: %d  TID: %d  Version: %d  secNr: %d lastSecNr: %d  nLoop: %d",
						 streamID, sectionLength, tid, version, secNr, lastSecNr, nLoop);

			len = sectionLength - 4 - 6; // 4 = CRC  6 = PAT Table begin from section length
			// skip to Table begin and iterate over entries
			const unsigned char *ptr = &cData[13];
			for (int i = 0; i < len; i += 4) {
				// Get PAT entry
				const uint16_t prognr =  (ptr[i + 0] << 8) | ptr[i + 1];
				const uint16_t pid    = ((ptr[i + 2] & 0x1F) << 8) | ptr[i + 3];
				if (prognr == 0) {
					SI_LOG_INFO("Stream: %d, PAT: Prog NR: %d  NIT: %d", streamID, prognr, pid);
				} else {
					SI_LOG_INFO("Stream: %d, PAT: Prog NR: %d  PMT: %d", streamID, prognr, pid);
					properties.setPMTPID(true, pid);
				}
			}
		}
	}
}

void DvbapiClient::collectPMT(StreamProperties &properties, const unsigned char *data, int len) {
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
			// Get current PID and CC
			const int pid = ((data[1] & 0x1f) << 8) | data[2];
			const int cc  =   data[3] & 0x0f;

			// Add PMT Data without TS Header
			if (properties.addPMTData(&data[4], 188 - 4, pid, cc)) { // 4 = TS Header
				const int pmtDataSize = properties.getPMTDataSize();
				SI_LOG_DEBUG("Stream: %d, PMT: sectionLength: %d  pmtDataSize: %d", streamID, sectionLength, pmtDataSize);
				// Check did we finish collecting PMT Data
				if (sectionLength <= (pmtDataSize - 9)) { // 9 = Untill PMT Section Length
					properties.setPMTCollected(true);
				}
			} else {
				SI_LOG_ERROR("Stream: %d, Unable to add PMT data!", streamID);
			}
		}
		// Did we finish collecting PMT
		if (properties.isPMTCollected()) {
			const unsigned char *cData = properties.getPMTData();

			SI_LOG_BIN_DEBUG(cData, properties.getPMTDataSize(), "Stream: %d, PMT data", streamID);

			// Parse PMT Data
			const int sectionLength = ((cData[ 6] & 0x0F) << 8) | cData[ 7];
			const int programNumber = ((cData[ 8]       ) << 8) | cData[ 9];
			const int version       =   cData[10];
//			const int secNr         =   cData[11];  // always 0x00
//			const int lastSecNr     =   cData[12];  // always 0x00
			const int pcrPID        = ((cData[13] & 0x1F) << 8) | cData[14];
			const int prgLength     = ((cData[15] & 0x0F) << 8) | cData[16];

			SI_LOG_INFO("Stream: %d, PMT - Section Length: %d  Prog NR: %d  Version: %d  PCR-PID: %d  Program Length: %d",
						streamID, sectionLength, programNumber, version, pcrPID, prgLength);

			// To save the Program Info
			std::string progInfo;
			if (prgLength > 0) {
				progInfo.append((const char *)&cData[17], prgLength);
			}

			len = sectionLength - 4 - 9 - prgLength; // 4 = CRC   9 = PMT Header from section length
			// skip to ES Table begin and iterate over entries
			const unsigned char *ptr = &cData[17 + prgLength];
			for (int i = 0; i < len; ) {
				const int streamType    =   ptr[i + 0];
				const int elementaryPID = ((ptr[i + 1] & 0x1F) << 8) | ptr[i + 2];
				const int esInfoLength  = ((ptr[i + 3] & 0x0F) << 8) | ptr[i + 4];

				SI_LOG_INFO("Stream: %d, PMT - Stream Type: %d  ES PID: %d  ES-Length: %d",
							streamID, streamType, elementaryPID, esInfoLength);
				for (int j = 0; j < esInfoLength; ) {
					const int subLength = ptr[j + i + 6];
					// Check for Conditional access system and EMM/ECM PID
					if (ptr[j + i + 5] == 9) {
						const int caid   =  (ptr[j + i +  7]         << 8) | ptr[j + i + 8];
						const int ecmpid = ((ptr[j + i +  9] & 0x1F) << 8) | ptr[j + i + 10];
						const int provid = ((ptr[j + i + 11] & 0x1F) << 8) | ptr[j + i + 12];
						SI_LOG_INFO("Stream: %d, ECM-PID - CAID: %04X  ECM-PID: %04X  PROVID: %04X ES-Length: %d",
									streamID, caid, ecmpid, provid, subLength);

						progInfo.append((const char *)&ptr[j + i + 5], subLength + 2);
					}
					// goto next ES Info
					j += subLength + 2;
				}

				// goto next ES entry
				i += esInfoLength + 5;
			}

			SI_LOG_BIN_DEBUG(reinterpret_cast<const unsigned char *>(progInfo.c_str()), progInfo.size(), "Stream: %d, PROG INFO data", streamID);

			// Send it here !!
			unsigned char caPMT[2048];
			int cpyLength = progInfo.size();
			int piLenght  = cpyLength + 1 + 4;
			int totLength = piLenght + 6;
// HACK HACK (Add some STREAM PID of video)
			{
				const char tmp[5] = { 0x01, 0x00, 0x0B, 0x00, 0x06 };
				progInfo.append(tmp, sizeof(tmp));
				cpyLength += sizeof(tmp);
				totLength += sizeof(tmp);
			}
// HACK HACK
			// DVBAPI_AOT_CA_PMT 0x9F 80 32 82
			caPMT[ 0] = (DVBAPI_AOT_CA_PMT >> 24) & 0xFF;
			caPMT[ 1] = (DVBAPI_AOT_CA_PMT >> 16) & 0xFF;
			caPMT[ 2] = (DVBAPI_AOT_CA_PMT >>  8) & 0xFF;
			caPMT[ 3] =  DVBAPI_AOT_CA_PMT & 0xFF;
			caPMT[ 4] = (totLength >> 8) & 0xFF;     // Total Length of caPMT
			caPMT[ 5] =  totLength & 0xFF;           // Total Length of caPMT
			caPMT[ 6] = LIST_ONLY;                   // send LIST_ONLY
			caPMT[ 7] = (programNumber >> 8) & 0xFF; // Program ID
			caPMT[ 8] =  programNumber & 0xFF;       //
			caPMT[ 9] = DVBAPI_PROTOCOL_VERSION;     // Version
			caPMT[10] = (piLenght >> 8) & 0xFF;      // Prog Info Length
			caPMT[11] =  piLenght & 0xFF;            // Prog Info Length
			caPMT[12] = 0x01;                        // ca_pmt_cmd_id = CAPMT_CMD_OK_DESCRAMBLING
			caPMT[13] = 0x82;                        // CAPMT_DESC_DEMUX
			caPMT[14] = 0x02;                        // Length
			caPMT[15] = 0x00;                        // Demux ID
			caPMT[16] = (char) streamID;             // streamID
			memcpy(&caPMT[17], progInfo.c_str(), cpyLength); // copy Prog Info data

			SI_LOG_BIN_DEBUG(caPMT, totLength + 6, "Stream: %d, PMT data", streamID);

			if (send(_client.getFD(), caPMT, totLength + 6, MSG_DONTWAIT) == -1) {
				SI_LOG_ERROR("Stream: %d, PMT - send data to server failed", streamID);
			}
		}
	}
}

void DvbapiClient::collectECM(StreamProperties &properties, const unsigned char *data, int len) {
	// Check Table ID of ECM
	const uint8_t tableID = data[5];
	if (tableID == 0x81 || tableID == 0x80) {
		const int pid = ((data[1] & 0x1f) << 8) | data[2];
		int demux;
		int filter;
		properties.getECMFilterData(demux, filter, pid);
		// Do we have and filter and did the parity change?
		if (filter != -1 && demux != -1 && properties.getKeyParity(pid) != tableID) {
			const int streamID      = properties.getStreamID();
//			const int cc            =    data[3] & 0x0f;
			const int sectionLength = (((data[6] & 0x0F) << 8) | data[7]) + 3;
			const int length        = sectionLength + 6;

			properties.setKeyParity(pid, tableID);

//			SI_LOG_BIN_DEBUG(data, len, "Stream: %d, ECM sectionlenght: %d  data", streamID, sectionLength);

			unsigned char caECM[2048];
			caECM[0] = (DVBAPI_FILTER_DATA >> 24) & 0xFF;
			caECM[1] = (DVBAPI_FILTER_DATA >> 16) & 0xFF;
			caECM[2] = (DVBAPI_FILTER_DATA >>  8) & 0xFF;
			caECM[3] =  DVBAPI_FILTER_DATA & 0xFF;
			caECM[4] =  demux;
			caECM[5] =  filter;
			memcpy(&caECM[6], &data[5], sectionLength); // copy ECM data

//			SI_LOG_BIN_DEBUG(caECM, length, "Stream: %d, ECM data", streamID);
			SI_LOG_DEBUG("Stream: %d, Send ECM data with %02X", streamID, tableID);

			if (send(_client.getFD(), caECM, length, MSG_DONTWAIT) == -1) {
				SI_LOG_ERROR("Stream: %d, ECM - send data to server failed", streamID);
			}
		}
	}
}

void DvbapiClient::threadEntry() {
	SI_LOG_INFO("Setting up DVBAPI client");

	struct pollfd pfd[1];
	pfd[0].events  = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
	pfd[0].revents = 0;
	pfd[0].fd      = -1;

	// set time to try to connect
	time_t retryTime = time(NULL) + 2;
	
	for (;;) {
		// try to connect to server
		if (!_connected) {
			const time_t currTime = time(NULL);
			if (retryTime < currTime && initClientSocket(_client, 15011, inet_addr("192.168.0.109"))) {
				sendClientInfo();
				pfd[0].fd = _client.getFD();
			} else {
				retryTime = currTime + 5;
			}
		}
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
					const uint32_t cmd = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
//					SI_LOG_DEBUG("Stream: %d, Receive data  len: %d  cmd: %X", buf[4], size, cmd);

					if (cmd == DVBAPI_SERVER_INFO) {
						SI_LOG_INFO("Connected to %s", &buf[7]);
						_connected = true;
					} else if (cmd == DVBAPI_DMX_SET_FILTER) {
						const int adapter = buf[4];
						const int demux   = buf[5];
						const int filter  = buf[6];
						const int pid     = (buf[7] << 8) | buf[8];
						setECMFilterData(adapter, demux, filter, pid, buf[9]);

						if(!_key[0])
							_key[0] = dvbcsa_key_alloc();
						if(!_key[1])
							_key[1] = dvbcsa_key_alloc();

//						SI_LOG_BIN_DEBUG(reinterpret_cast<unsigned char *>(buf), size, "Stream: %d, DMX Filter data", adapter);
					} else if (cmd == DVBAPI_DMX_STOP) {
						const int adapter = buf[4];
//						const int demux   = buf[5];
//						const int filter  = buf[6];
//						const int pid     = (buf[7] << 8) | buf[8];
						SI_LOG_BIN_DEBUG(reinterpret_cast<unsigned char *>(buf), size, "Stream: %d, Stop DMX Filter data", adapter);
//						setECMFilterData(adapter, demux, filter, pid);
					} else if (cmd == DVBAPI_CA_SET_DESCR) {
						const int adapter =  buf[ 4];
						const int index   = (buf[ 5] << 24) | (buf[ 6] << 16) | (buf[ 7] << 8) | buf[ 8];
						const int parity  = (buf[ 9] << 24) | (buf[10] << 16) | (buf[11] << 8) | buf[12];
						const unsigned char *cw = reinterpret_cast<unsigned char *>(&buf[13]);
						dvbcsa_key_set(cw, _key[parity]);
						SI_LOG_DEBUG("Stream: %d, Received %s(%02X) CW: %02X %02X %02X %02X %02X %02X %02X %02X  index: %d",
						                 adapter, (parity == 0) ? "even" : "odd", parity, cw[0], cw[1], cw[2], cw[3], cw[4], cw[5], cw[6], cw[7], index);
					} else {
						SI_LOG_BIN_DEBUG(reinterpret_cast<unsigned char *>(buf), size, "Stream: %d, Receive unknown data", 0);
					}
				}
			}
		}
	}
}
