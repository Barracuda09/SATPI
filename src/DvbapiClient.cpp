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
#include "Configure.h"
#include "TSPacketBuffer.h"

#include <stdio.h>
#include <stdlib.h>

extern "C" {
	#include <dvbcsa/dvbcsa.h>
}

#include <poll.h>

#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

//constants used in socket communication:
#define DVBAPI_PROTOCOL_VERSION         2

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
#define DVBAPI_ECM_INFO        0xFFFF0003

#define LIST_ONLY              0x03
#define LIST_ONLY_UPDATE       0x05

static uint32_t crc32Table[] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static uint32_t calculateCRC32(const unsigned char *data, int len) {
    uint32_t crc = 0xffffffff;
    for(int i = 0; i < len; ++i) {
        crc = (crc << 8) ^ crc32Table[((crc >> 24) ^ (data[i] & 0xff)) & 0xff];
    }
	return crc;
}

#define CRC(data, sectionLength) (data[sectionLength - 4 + 8]     << 24) | (data[sectionLength - 4 + 8 + 1] << 16) | \
                                 (data[sectionLength - 4 + 8 + 2] <<  8) |  data[sectionLength - 4 + 8 + 3]

DvbapiClient::DvbapiClient(const Functor1Ret<StreamProperties &, int> &getStreamProperties,
			const Functor1Ret<bool, int> &updateFrontend) :
		ThreadBase("DvbapiClient"),
		_connected(false),
		_enabled(false),
		_rewritePMT(false),
		_serverPort(15011),
		_serverIpAddr("127.0.0.1"),
		_serverName("Not connected"),
		_getStreamProperties(getStreamProperties),
		_updateFrontend(updateFrontend) {
	startThread();
}

DvbapiClient::~DvbapiClient() {
	cancelThread();
	joinThread();
}

void DvbapiClient::decrypt(int streamID, TSPacketBuffer &buffer) {
	if (_connected && _enabled) {
		StreamProperties &properties = _getStreamProperties(streamID);
		const size_t size = buffer.getNumberOfTSPackets();
		for (size_t i = 0; i < size; ++i) {
			// Get TS packet from the buffer
			unsigned char *data = buffer.getTSPacketPtr(i);

			// Check is this the beginning of the TS and no Transport error indicator
			if (data[0] == 0x47 && (data[1] & 0x80) != 0x80) {
				// get PID from TS
				const int pid = ((data[1] & 0x1f) << 8) | data[2];

				// this packet scrambled and no NULL packet
				if (data[3] & 0x80 && pid < 0x1FFF) {

					// scrambled TS packet with even(0) or odd(1) key?
					const int parity = (data[3] & 0x40) > 0;

					// get batch parity and count
					const int parityBatch = properties.getBatchParity();
					const int countBatch  = properties.getBatchCount();

					// check if the parity changed in this batch (but should not be the begin of the batch)
					// or check if this batch full, then decrypt this batch
					if (countBatch != 0 && (parity != parityBatch || countBatch >= properties.getMaximumBatchSize())) {
						//
						SI_LOG_COND_DEBUG(parity != parityBatch, "Stream: %d, Parity changed from %d to %d, decrypting batch size %d",
						   streamID, parityBatch, parity, countBatch);

						// decrypt this batch
						properties.decryptBatch();
					}

					// Can we add this packet to the batch
					if (properties.getKey(parity) != nullptr) {
						// check is there an adaptation field we should skip, then add it to batch
						int skip = 4;
						if((data[3] & 0x20) && (data[4] < 183)) {
							skip += data[4] + 1;
						}
						properties.setBatchData(data + skip, 188 - skip, parity, data);

						// set pending decrypt for this buffer
						buffer.setDecryptPending();
					} else {
						// set decrypt failed by setting NULL packet ID..
						data[1] |= 0x1F;
						data[2] |= 0xFF;

						// clear scramble flag, so we can send it.
						data[3] &= 0x3F;
					}
				} else {
					if (pid == 0) {
						// Check PAT pid and start indicator and table ID
						collectPAT(properties, data);
					} else if (properties.isPMT(pid)) {
						collectPMT(properties, data);
						// Do we need to clean PMT
						if (_rewritePMT) {
							cleanPMT(properties, data);
						}
					} else if (properties.isECM(pid)) {
						collectECM(properties, data);
					}
				}
			}
		}
	}
}

bool DvbapiClient::stopDecrypt(int streamID) {
	if (_connected) {
		int demux = streamID; // demux index
		int filter = 0;
		int pid = 0;
		unsigned char caPMT[8];

		StreamProperties &properties = _getStreamProperties(streamID);
		if (!properties.getActiveECMFilterData(demux, filter, pid)) {
			SI_LOG_DEBUG("Stream: %d, Strange, did not find any active ECM filter!", streamID);
		}
		// Stop 9F 80 3f 04 83 02 00 <demux index>
		caPMT[0] = (DVBAPI_AOT_CA_STOP >> 24) & 0xFF;
		caPMT[1] = (DVBAPI_AOT_CA_STOP >> 16) & 0xFF;
		caPMT[2] = (DVBAPI_AOT_CA_STOP >>  8) & 0xFF;
		caPMT[3] =  DVBAPI_AOT_CA_STOP & 0xFF;
		caPMT[4] = 0x83;
		caPMT[5] = 0x02;
		caPMT[6] = 0x00;
		caPMT[7] = streamID;  // demux
//		caPMT[7] = demux;

		SI_LOG_BIN_DEBUG(caPMT, sizeof(caPMT), "Stream: %d, Stop CA Decrypt DMX: %d  PID: %d", streamID, demux, pid);

		// cleaning tables
		SI_LOG_DEBUG("Stream: %d, Clearing PAT/PMT Tables...", streamID);
		properties.setTableCollected(PAT_TABLE_ID, false);
		properties.setTableCollected(PMT_TABLE_ID, false);
		properties.freeKeys();

		if (send(_client.getFD(), caPMT, sizeof(caPMT), MSG_DONTWAIT) == -1) {
			SI_LOG_ERROR("Stream: %d, PMT Stop DMX - send data to server failed", streamID);
			return false;
		}
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
		client.closeFD();
		return false;
	}
	if (connect(client.getFD(), (struct sockaddr *)&client._addr, sizeof(client._addr)) == -1) {
		if (errno != ECONNREFUSED && errno != EINPROGRESS) {
			PERROR("connect");
		}
		client.closeFD();
		return false;
	}
	return true;
}

void DvbapiClient::sendClientInfo() {
	extern const char *satpi_version;
	std::string name = "SatPI ";
	name += satpi_version;

	const int len = name.size() - 1; // ignoring null termination
	unsigned char buff[7 + len];

	buff[0] = (DVBAPI_CLIENT_INFO >> 24) & 0xFF;
	buff[1] = (DVBAPI_CLIENT_INFO >> 16) & 0xFF;
	buff[2] = (DVBAPI_CLIENT_INFO >>  8) & 0xFF;
	buff[3] =  DVBAPI_CLIENT_INFO & 0xFF;
	buff[4] = (DVBAPI_PROTOCOL_VERSION >>  8) & 0xFF;
	buff[5] =  DVBAPI_PROTOCOL_VERSION & 0xFF;
	buff[6] = len;
	memcpy(&buff[7], name.c_str(), len);

	if (write(_client.getFD(), buff, sizeof(buff)) == -1) {
		SI_LOG_ERROR("write failed");
	}
}

void DvbapiClient::collectPAT(StreamProperties &properties, const unsigned char *data) {
	if (!properties.isTableCollected(PAT_TABLE_ID)) {
		const int streamID = properties.getStreamID();
		// collect PAT data
		properties.collectTableData(streamID, PAT_TABLE_ID, data);

		// Did we finish collecting PAT
		if (properties.isTableCollected(PAT_TABLE_ID)) {
			const unsigned char *cData = properties.getTableData(PAT_TABLE_ID);
			// Parse PAT Data
			const int sectionLength = ((cData[ 6] & 0x0F) << 8) | cData[ 7];
			const int tid           =  (cData[ 8]         << 8) | cData[ 9];
			const int version       =   cData[10];
			const int secNr         =   cData[11];
			const int lastSecNr     =   cData[12];
			const int nLoop         =   cData[13];
			const uint32_t crc      =  CRC(cData, sectionLength);

			SI_LOG_INFO("Stream: %d, PAT: Section Length: %d  TID: %d  Version: %d  secNr: %d lastSecNr: %d  nLoop: %d  CRC: %04X",
						 streamID, sectionLength, tid, version, secNr, lastSecNr, nLoop, crc);

			const int len = sectionLength - 4 - 6; // 4 = CRC  6 = PAT Table begin from section length

//			SI_LOG_BIN_DEBUG(cData, properties.getTableDataSize(PAT_TABLE_ID), "Stream: %d, PAT data", streamID);

			const uint32_t calccrc = calculateCRC32(&cData[5], sectionLength - 4 + 3);
			SI_LOG_INFO("Stream: %d, PAT: Calc CRC32: %04X   Msg CRC32: %04X", streamID, calccrc, crc);

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
					properties.setPMT(pid, true);
				}
			}
		}
	}
}

void DvbapiClient::cleanPMT(StreamProperties &UNUSED(properties), unsigned char *data) {
	const unsigned char options = (data[1] & 0xE0);
	if (options == 0x40 && data[5] == PMT_TABLE_ID) {
//		const int streamID = properties.getStreamID();

//		const int pid           = ((data[1] & 0x1f) << 8) | data[2];
//		const int cc            =   data[3] & 0x0f;
		const int sectionLength = ((data[6] & 0x0F) << 8) | data[7];
		const int prgLength     = ((data[15] & 0x0F) << 8) | data[16];

		std::string pmt;
		// Copy first part to new PMT buffer
		pmt.append((const char *)&data[0], 17);

		// Clear sectionLength
		pmt[6]  &= 0xF0;
		pmt[7]   = 0x00;

		//
		pmt[10] ^= 0x3F;

		// Clear prgLength
		pmt[15] &= 0xF0;
		pmt[16]  = 0x00;

		const int len = sectionLength - 4 - 9 - prgLength; // 4 = CRC   9 = PMT Header from section length
		// skip to ES Table begin and iterate over entries
		const unsigned char *ptr = &data[17 + prgLength];
		for (int i = 0; i < len; ) {
//			const int streamType    =   ptr[i + 0];
//			const int elementaryPID = ((ptr[i + 1] & 0x1F) << 8) | ptr[i + 2];
			const int esInfoLength  = ((ptr[i + 3] & 0x0F) << 8) | ptr[i + 4];
			// Append
			pmt.append((const char *)&ptr[i + 0], 5);
			pmt[pmt.size() - 2] &= 0xF0;  // Clear esInfoLength
			pmt[pmt.size() - 1]  = 0x00;

			// goto next ES entry
			i += esInfoLength + 5;

		}
		// adjust section length
		const int newSectionLength = pmt.size() - 6 - 2 + 4; // 6 = PMT Header  2 = section Length  4 = CRC
		pmt[6] |= ((newSectionLength >> 8) & 0xFF);
		pmt[7]  =  (newSectionLength & 0xFF);

		// append calculated CRC
		const uint32_t crc = calculateCRC32(reinterpret_cast<const unsigned char *>(pmt.c_str() + 5), pmt.size() - 5);
		pmt += ((crc >> 24) & 0xFF);
		pmt += ((crc >> 16) & 0xFF);
		pmt += ((crc >>  8) & 0xFF);
		pmt += ((crc >>  0) & 0xFF);

		// clear rest of packet
		for (int i = pmt.size(); i < 188; ++i) {
			pmt += 0xFF;
		}
		// copy new PMT to buffer
		memcpy(data, pmt.c_str(), 188);

//		SI_LOG_BIN_DEBUG(data, 188, "Stream: %d, NEW PMT data", streamID);

	} else {
//		SI_LOG_BIN_DEBUG(data, 188, "Stream: %d, Not handled Cleaning PMT data!", streamID);
		// Clear PID to NULL packet
		data[1] = 0x1F;
		data[2] = 0xFF;
	}
}

void DvbapiClient::collectPMT(StreamProperties &properties, const unsigned char *data) {
	if (!properties.isTableCollected(PMT_TABLE_ID)) {
		const int streamID = properties.getStreamID();
		// collect PMT data
		properties.collectTableData(streamID, PMT_TABLE_ID, data);

		// Did we finish collecting PMT
		if (properties.isTableCollected(PMT_TABLE_ID)) {
			const unsigned char *cData = properties.getTableData(PMT_TABLE_ID);

			SI_LOG_BIN_DEBUG(cData, properties.getTableDataSize(PMT_TABLE_ID), "Stream: %d, PMT data", streamID);

			// Parse PMT Data
			const int sectionLength = ((cData[ 6] & 0x0F) << 8) | cData[ 7];
			const int programNumber = ((cData[ 8]       ) << 8) | cData[ 9];
			const int version       =   cData[10];
//			const int secNr         =   cData[11];  // always 0x00
//			const int lastSecNr     =   cData[12];  // always 0x00
			const int pcrPID        = ((cData[13] & 0x1F) << 8) | cData[14];
			const int prgLength     = ((cData[15] & 0x0F) << 8) | cData[16];
			const uint32_t crc      =  CRC(cData, sectionLength);

			SI_LOG_INFO("Stream: %d, PMT - Section Length: %d  Prog NR: %d  Version: %d  PCR-PID: %d  Program Length: %d  CRC: %04X",
						streamID, sectionLength, programNumber, version, pcrPID, prgLength, crc);

			// To save the Program Info
			std::string progInfo;
			if (prgLength > 0) {
				progInfo.append((const char *)&cData[17], prgLength);
			}

			const int len = sectionLength - 4 - 9 - prgLength; // 4 = CRC   9 = PMT Header from section length
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

//			SI_LOG_BIN_DEBUG(reinterpret_cast<const unsigned char *>(progInfo.c_str()), progInfo.size(), "Stream: %d, PROG INFO data", streamID);

			// Send it here !!
			unsigned char caPMT[2048];
			int cpyLength = progInfo.size();
			int piLenght  = cpyLength + 1 + 4;
			int totLength = piLenght + 6;

// HACK HACK (Add some STREAM PID of video)
			{
				const char tmp[5] = { 0x00, 0x00, 0x0B, 0x00, 0x06 };
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
			caPMT[ 6] = LIST_ONLY_UPDATE;            // send LIST_ONLY_UPDATE
//			caPMT[ 6] = LIST_ONLY;                   // send LIST_ONLY
			caPMT[ 7] = (programNumber >> 8) & 0xFF; // Program ID
			caPMT[ 8] =  programNumber & 0xFF;       // Program ID
			caPMT[ 9] = DVBAPI_PROTOCOL_VERSION;     // Version
			caPMT[10] = (piLenght >> 8) & 0xFF;      // Prog Info Length
			caPMT[11] =  piLenght & 0xFF;            // Prog Info Length
			caPMT[12] = 0x01;                        // ca_pmt_cmd_id = CAPMT_CMD_OK_DESCRAMBLING
			caPMT[13] = 0x82;                        // CAPMT_DESC_DEMUX
			caPMT[14] = 0x02;                        // Length
			caPMT[15] = (char) streamID;             // Demux ID
			caPMT[16] = (char) streamID;             // streamID
			memcpy(&caPMT[17], progInfo.c_str(), cpyLength); // copy Prog Info data

			SI_LOG_BIN_DEBUG(caPMT, totLength + 6, "Stream: %d, PMT data to OSCam", streamID);

			if (send(_client.getFD(), caPMT, totLength + 6, MSG_DONTWAIT) == -1) {
				SI_LOG_ERROR("Stream: %d, PMT - send data to server failed", streamID);
			}
		}
	}
}

void DvbapiClient::collectECM(StreamProperties &properties, const unsigned char *data) {
	// Check Table ID of ECM
	const uint8_t tableID = data[5];
	if (tableID == 0x81 || tableID == 0x80) {
		const int streamID = properties.getStreamID();
		const int pid = ((data[1] & 0x1f) << 8) | data[2];
		int demux;
		int filter;
		properties.getECMFilterData(demux, filter, pid);
		// Do we have and filter and did the parity change?
		if (filter != -1 && demux != -1 && properties.getKeyParity(pid) != tableID) {
//			const int cc            =    data[3] & 0x0f;
			const int sectionLength = (((data[6] & 0x0F) << 8) | data[7]) + 3; // 3 = tableID + length field

			properties.setKeyParity(pid, tableID);

//			SI_LOG_BIN_DEBUG(data, 188, "Stream: %d, ECM sectionlenght: %d  data", streamID, sectionLength);

			unsigned char caECM[2048];
			caECM[0] = (DVBAPI_FILTER_DATA >> 24) & 0xFF;
			caECM[1] = (DVBAPI_FILTER_DATA >> 16) & 0xFF;
			caECM[2] = (DVBAPI_FILTER_DATA >>  8) & 0xFF;
			caECM[3] =  DVBAPI_FILTER_DATA & 0xFF;
			caECM[4] =  demux;
			caECM[5] =  filter;
			memcpy(&caECM[6], &data[5], sectionLength); // copy ECM data
			const int length = sectionLength + 6; // 6 = caECM header

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
	time_t retryTime = time(nullptr) + 2;

	for (;;) {
		// try to connect to server
		if (!_connected && _enabled) {
			const time_t currTime = time(nullptr);
			if (retryTime < currTime) {
				MutexLock lock(_mutex);
				if (initClientSocket(_client, _serverPort, inet_addr(_serverIpAddr.c_str()))) {
					sendClientInfo();
					pfd[0].fd = _client.getFD();
				} else {
					retryTime = currTime + 5;
				}
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
					SI_LOG_DEBUG("Stream: %d, Receive data  len: %d  cmd: %X", buf[4], size, cmd);

					if (cmd == DVBAPI_SERVER_INFO) {
						_serverName.assign(&buf[7], buf[6]);
						SI_LOG_INFO("Connected to %s", _serverName.c_str());
						_connected = true;
					} else if (cmd == DVBAPI_DMX_SET_FILTER) {
						const int adapter =  buf[4];
						const int demux   =  buf[5];
						const int filter  =  buf[6];
						const int pid     = (buf[7] << 8) | buf[8];

						StreamProperties &properties = _getStreamProperties(adapter);
						properties.setECMFilterData(demux, filter, pid, true);
						// now update frontend, PID list has changed
						_updateFrontend(adapter);
					} else if (cmd == DVBAPI_DMX_STOP) {
						const int adapter =  buf[4];
						const int demux   =  buf[5];
						const int filter  =  buf[6];
						const int pid     = (buf[7] << 8) | buf[8];

						StreamProperties &properties = _getStreamProperties(adapter);
						properties.setECMFilterData(demux, filter, pid, false);
						// now update frontend, PID list has changed
						_updateFrontend(adapter);
					} else if (cmd == DVBAPI_CA_SET_DESCR) {
						const int adapter =  buf[4];
						const int index   = (buf[5] << 24) | (buf[ 6] << 16) | (buf[ 7] << 8) | buf[ 8];
						const int parity  = (buf[9] << 24) | (buf[10] << 16) | (buf[11] << 8) | buf[12];
						const unsigned char *cw = reinterpret_cast<unsigned char *>(&buf[13]);

						StreamProperties &properties = _getStreamProperties(adapter);
						properties.setKey(cw, parity, index);
						SI_LOG_DEBUG("Stream: %d, Received %s(%02X) CW: %02X %02X %02X %02X %02X %02X %02X %02X  index: %d",
						             adapter, (parity == 0) ? "even" : "odd", parity, cw[0], cw[1], cw[2], cw[3], cw[4], cw[5], cw[6], cw[7], index);
					} else if (cmd == DVBAPI_ECM_INFO) {
						const int adapter =  buf[4];
//						StreamProperties &properties = _getStreamProperties(adapter);
						SI_LOG_DEBUG("Stream: %d, Receive ECM Info", adapter);
					} else {
						SI_LOG_BIN_DEBUG(reinterpret_cast<unsigned char *>(buf), size, "Stream: %d, Receive unexpected data", 0);
					}
				} else {
					// connection closed, try to reconnect
					SI_LOG_INFO("Connected lost with %s", _serverName.c_str());
					_serverName = "Not connected";
					_client.closeFD();
					pfd[0].fd = -1;
					_connected = false;
				}
			}
		}
	}
}

void DvbapiClient::fromXML(const std::string &xml) {
	MutexLock lock(_mutex);

	std::string element;
	if (findXMLElement(xml, "data.configdata.OSCamIP.value", element)) {
		_serverIpAddr = element;
	}
	if (findXMLElement(xml, "data.configdata.OSCamPORT.value", element)) {
		_serverPort = atoi(element.c_str());
	}
	if (findXMLElement(xml, "data.configdata.OSCamEnabled.value", element)) {
		_enabled = (element == "true") ? true : false;
	}
	if (findXMLElement(xml, "data.configdata.RewritePMT.value", element)) {
		_rewritePMT = (element == "true") ? true : false;
	}
}

void DvbapiClient::addToXML(std::string &xml) const {
	MutexLock lock(_mutex);

	ADD_CONFIG_CHECKBOX(xml, "OSCamEnabled", (_enabled ? "true" : "false"));
	ADD_CONFIG_CHECKBOX(xml, "RewritePMT", (_rewritePMT ? "true" : "false"));
	ADD_CONFIG_IP_INPUT(xml, "OSCamIP", _serverIpAddr.c_str());
	ADD_CONFIG_NUMBER_INPUT(xml, "OSCamPORT", _serverPort.load(), 0, 65535);
	ADD_CONFIG_TEXT(xml, "OSCamServerName", _serverName.c_str());
}
