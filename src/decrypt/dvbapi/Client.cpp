/* Client.cpp

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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
#include <decrypt/dvbapi/Client.h>

#include <Log.h>
#include <Unused.h>
#include <socket/SocketClient.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>
#include <mpegts/TableData.h>
#include <input/dvb/FrontendDecryptInterface.h>

#include <stdio.h>
#include <stdlib.h>

#include <cstring>
#include <ctime>

extern "C" {
	#include <dvbcsa/dvbcsa.h>
}

#include <poll.h>

#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

extern const char *satpi_version;

namespace decrypt {
namespace dvbapi {

	//constants used in socket communication:
	#define DVBAPI_PROTOCOL_VERSION         2

	#define DVBAPI_CA_SET_DESCR    0x40106f86
	#define DVBAPI_CA_SET_PID      0x40086f87
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

	Client::Client(const std::string &xmlFilePath,
				   const base::Functor1Ret<input::dvb::SpFrontendDecryptInterface, int> getFrontendDecryptInterface) :
		ThreadBase("DvbApiClient"),
		XMLSupport(xmlFilePath),
		_connected(false),
		_enabled(false),
		_rewritePMT(false),
		_serverPort(15011),
		_adapterOffset(0),
		_serverIpAddr("127.0.0.1"),
		_serverName("Not connected"),
		_getFrontendDecryptInterface(getFrontendDecryptInterface) {
		restoreXML();
		startThread();
	}

	Client::~Client() {
		cancelThread();
		joinThread();
	}

	void Client::decrypt(const int streamID, mpegts::PacketBuffer &buffer) {
		if (_connected && _enabled) {
			const input::dvb::SpFrontendDecryptInterface frontend = _getFrontendDecryptInterface(streamID);
			const std::size_t size = buffer.getNumberOfTSPackets();
			for (std::size_t i = 0; i < size; ++i) {
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
						const int parityBatch = frontend->getBatchParity();
						const int countBatch  = frontend->getBatchCount();

						// check if the parity changed in this batch (but should not be the begin of the batch)
						// or check if this batch full, then decrypt this batch
						if (countBatch != 0 && (parity != parityBatch || countBatch >= frontend->getMaximumBatchSize())) {

							const bool final = parity != parityBatch;
							//
							SI_LOG_COND_DEBUG(final, "Stream: %d, Parity changed from %d to %d, decrypting batch size %d",
											  streamID, parityBatch, parity, countBatch);

							// decrypt this batch
							frontend->decryptBatch(final);
						}

						// Can we add this packet to the batch
						if (frontend->getKey(parity) != nullptr) {
							// check is there an adaptation field we should skip, then add it to batch
							int skip = 4;
							if((data[3] & 0x20) && (data[4] < 183)) {
								skip += data[4] + 1;
							}
							frontend->setBatchData(data + skip, 188 - skip, parity, data);

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

///////////////////////////////////////////////////////////////////
						int demux = 0;
						int filter = 0;
						int tableID = data[5];
						std::string filterData;
						if (frontend->findOSCamFilterData(pid, data, tableID, filter, demux, filterData)) {
							// Don't send PAT or PMT before we have an active
							if (pid == 0 || frontend->isPMT(pid)) {
							} else {
								const unsigned char *tableData = reinterpret_cast<const unsigned char *>(filterData.c_str());
								const int sectionLength = (((data[6] & 0x0F) << 8) | data[7]) + 3; // 3 = tableID + length field

								unsigned char clientData[sectionLength + 25];
								const uint32_t request = htonl(DVBAPI_FILTER_DATA);
								std::memcpy(&clientData[0], &request, 4);
								clientData[4] =  demux;
								clientData[5] =  filter;
								std::memcpy(&clientData[6], &tableData[5], sectionLength); // copy Table data
								const int length = sectionLength + 6; // 6 = clientData header

								SI_LOG_DEBUG("Stream: %d, Send Filter Data for demux %d  filter %d  PID %04d  TableID %04x %04x %04x",
									streamID, demux, filter, pid, tableID, tableData[8], tableData[9]);

								if (!_client.sendData(clientData, length, MSG_DONTWAIT)) {
									SI_LOG_ERROR("Stream: %d, Filter - send data to server failed", streamID);
								}
							}
						}
///////////////////////////////////////////////////////////////////

						if (pid == 0) {
							collectPAT(frontend, data);
						} else if (frontend->isPMT(pid)) {
							collectPMT(frontend, data);
							// Do we need to clean PMT
							if (_rewritePMT) {
								cleanPMT(frontend, data);
							}
						}
					}
				}
			}
		}
	}

	bool Client::stopDecrypt(int streamID) {
		if (_connected) {
			unsigned char buff[8];
			// Stop 9F 80 3f 04 83 02 00 <demux index>
			const uint32_t request = htonl(DVBAPI_AOT_CA_STOP);
			std::memcpy(&buff[0], &request, 4);
			buff[4] = 0x83;
			buff[5] = 0x02;
			buff[6] = 0x00;
			buff[7] = static_cast<char>(streamID + _adapterOffset);  // demux

			SI_LOG_BIN_DEBUG(buff, sizeof(buff), "Stream: %d, Stop CA Decrypt", streamID);

			// cleaning tables
			SI_LOG_DEBUG("Stream: %d, Clearing PAT/PMT Tables and Keys...", streamID);
			input::dvb::SpFrontendDecryptInterface frontend = _getFrontendDecryptInterface(streamID);
			frontend->setTableCollected(PAT_TABLE_ID, false);
			frontend->setTableCollected(PMT_TABLE_ID, false);
			frontend->freeKeys();

			frontend->clearOSCamFilters();

			if (!_client.sendData(buff, sizeof(buff), MSG_DONTWAIT)) {
				SI_LOG_ERROR("Stream: %d, Stop CA Decrypt - send data to server failed", streamID);
				return false;
			}
		}
		return true;
	}

	bool Client::initClientSocket(SocketClient &client, int port, const char *ip_addr) {

		client.setupSocketStructure(port, ip_addr);

		if (!client.setupSocketHandle(SOCK_STREAM /*| SOCK_NONBLOCK*/, 0)) {
			SI_LOG_ERROR("OSCam Server handle failed");
			return false;
		}

		client.setSocketTimeoutInSec(2);

		if (!client.connectTo()) {
			SI_LOG_ERROR("Connecting to OSCam Server failed");
			return false;
		}
		return true;
	}

	void Client::sendClientInfo() {
		std::string name = "SatPI ";
		name += satpi_version;

		const int len = name.size() - 1; // ignoring null termination
		unsigned char buff[7 + len];

		const uint32_t request = htonl(DVBAPI_CLIENT_INFO);
		std::memcpy(&buff[0], &request, 4);

		const uint16_t version = htons(DVBAPI_PROTOCOL_VERSION);
		std::memcpy(&buff[4], &version, 2);

		buff[6] = len;
		std::memcpy(&buff[7], name.c_str(), len);

		if (!_client.sendData(buff, sizeof(buff), MSG_DONTWAIT)) {
			SI_LOG_ERROR("write failed");
		}
	}

	void Client::collectPAT(input::dvb::SpFrontendDecryptInterface frontend, const unsigned char *data) {
		if (!frontend->isTableCollected(PAT_TABLE_ID)) {
			const int streamID = frontend->getStreamID();
			// collect PAT data
			frontend->collectTableData(streamID, PAT_TABLE_ID, data);

			// Did we finish collecting PAT
			if (frontend->isTableCollected(PAT_TABLE_ID)) {
				const unsigned char *cData = frontend->getTableData(PAT_TABLE_ID);
				// Parse PAT Data
				const int sectionLength = ((cData[ 6] & 0x0F) << 8) | cData[ 7];
				const int tid           =  (cData[ 8] << 8) | cData[ 9];
				const int version       =   cData[10];
				const int secNr         =   cData[11];
				const int lastSecNr     =   cData[12];
				const uint32_t crc      =  CRC(cData, sectionLength);

//				SI_LOG_BIN_DEBUG(cData, frontend->getTableDataSize(PAT_TABLE_ID), "Stream: %d, PAT data", streamID);

				const uint32_t calccrc = mpegts::TableData::calculateCRC32(&cData[5], sectionLength - 4 + 3);
				if (calccrc == crc) {
					SI_LOG_INFO("Stream: %d, PAT: Section Length: %d  TID: %d  Version: %d  secNr: %d lastSecNr: %d  CRC: %04X",
								streamID, sectionLength, tid, version, secNr, lastSecNr, crc);

					// 4 = CRC  6 = PAT Table begin from section length
					const int len = sectionLength - 4 - 6;

					// skip to Table begin and iterate over entries
					const unsigned char *ptr = &cData[13];
					for (int i = 0; i < len; i += 4) {
						// Get PAT entry
						const uint16_t prognr =  (ptr[i + 0] << 8) | ptr[i + 1];
						const uint16_t pid    = ((ptr[i + 2] & 0x1F) << 8) | ptr[i + 3];
						if (prognr == 0) {
							SI_LOG_INFO("Stream: %d, PAT: Prog NR: %d  NIT PID: %04d", streamID, prognr, pid);
						} else {
							SI_LOG_INFO("Stream: %d, PAT: Prog NR: %d  PMT PID: %04d", streamID, prognr, pid);
							frontend->setPMT(pid, true);
						}
					}
				} else {
					SI_LOG_ERROR("Stream: %d, PAT: CRC Error! Calc CRC32: %04X - Msg CRC32: %04X  Retrying to collect data...", streamID, calccrc, crc);
					frontend->setTableCollected(PAT_TABLE_ID, false);
				}
			}
		}
	}

	void Client::cleanPMT(input::dvb::SpFrontendDecryptInterface UNUSED(frontend), unsigned char *data) {
		const unsigned char options = (data[1] & 0xE0);
		if (options == 0x40 && data[5] == PMT_TABLE_ID) {
//			const int streamID = frontend->getStreamID();

//			const int pid           = ((data[1] & 0x1f) << 8) | data[2];
//			const int cc            =   data[3] & 0x0f;
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

			const std::size_t len = sectionLength - 4 - 9 - prgLength; // 4 = CRC   9 = PMT Header from section length
			// skip to ES Table begin and iterate over entries
			const unsigned char *ptr = &data[17 + prgLength];
			for (std::size_t i = 0; i < len; ) {
//				const int streamType    =   ptr[i + 0];
//				const int elementaryPID = ((ptr[i + 1] & 0x1F) << 8) | ptr[i + 2];
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
			const uint32_t crc = mpegts::TableData::calculateCRC32(reinterpret_cast<const unsigned char *>(pmt.c_str() + 5), pmt.size() - 5);
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

//			SI_LOG_BIN_DEBUG(data, 188, "Stream: %d, NEW PMT data", streamID);

		} else {
//			SI_LOG_BIN_DEBUG(data, 188, "Stream: %d, Not handled Cleaning PMT data!", streamID);
			// Clear PID to NULL packet
			data[1] = 0x1F;
			data[2] = 0xFF;
		}
	}

	void Client::collectPMT(input::dvb::SpFrontendDecryptInterface frontend, const unsigned char *data) {
		if (!frontend->isTableCollected(PMT_TABLE_ID)) {
			const int streamID = frontend->getStreamID();
			// collect PMT data
			frontend->collectTableData(streamID, PMT_TABLE_ID, data);

			// Did we finish collecting PMT
			if (frontend->isTableCollected(PMT_TABLE_ID)) {
				const unsigned char *cData = frontend->getTableData(PMT_TABLE_ID);
				// Parse PMT Data
				const int sectionLength = ((cData[ 6] & 0x0F) << 8) | cData[ 7];
				const int programNumber = ((cData[ 8]       ) << 8) | cData[ 9];
				const int version       =   cData[10];
				const int secNr         =   cData[11];
				const int lastSecNr     =   cData[12];
				const int pcrPID        = ((cData[13] & 0x1F) << 8) | cData[14];
				const int prgLength     = ((cData[15] & 0x0F) << 8) | cData[16];
				const uint32_t crc      =  CRC(cData, sectionLength);

				SI_LOG_BIN_DEBUG(cData, frontend->getTableDataSize(PMT_TABLE_ID), "Stream: %d, PMT data", streamID);

				const uint32_t calccrc = mpegts::TableData::calculateCRC32(&cData[5], sectionLength - 4 + 3);
				if (calccrc == crc) {
					SI_LOG_INFO("Stream: %d, PMT - Section Length: %d  Prog NR: %d  Version: %d  secNr: %d  lastSecNr: %d  PCR-PID: %04d  Program Length: %d  CRC: %04X",
								streamID, sectionLength, programNumber, version, secNr, lastSecNr, pcrPID, prgLength, crc);

					// To save the Program Info
					std::string progInfo;
					if (prgLength > 0) {
						progInfo.append(reinterpret_cast<const char *>(&cData[17]), prgLength);
					}

					// 4 = CRC   9 = PMT Header from section length
					const std::size_t len = sectionLength - 4 - 9 - prgLength;

					// skip to ES Table begin and iterate over entries
					const unsigned char *ptr = &cData[17 + prgLength];
					for (std::size_t i = 0; i < len; ) {
						const int streamType    =   ptr[i + 0];
						const int elementaryPID = ((ptr[i + 1] & 0x1F) << 8) | ptr[i + 2];
						const std::size_t esInfoLength  = ((ptr[i + 3] & 0x0F) << 8) | ptr[i + 4];

						SI_LOG_INFO("Stream: %d, PMT - Stream Type: %d  ES PID: %04d  ES-Length: %d",
									streamID, streamType, elementaryPID, esInfoLength);
						for (std::size_t j = 0; j < esInfoLength; ) {
							const std::size_t subLength = ptr[j + i + 6];
							// Check for Conditional access system and EMM/ECM PID
							if (ptr[j + i + 5] == 9) {
								const int caid   =  (ptr[j + i +  7] << 8) | ptr[j + i + 8];
								const int ecmpid = ((ptr[j + i +  9] & 0x1F) << 8) | ptr[j + i + 10];
								const int provid = ((ptr[j + i + 11] & 0x1F) << 8) | ptr[j + i + 12];
								SI_LOG_INFO("Stream: %d, ECM-PID - CAID: %04X  ECM-PID: %04d  PROVID: %04X ES-Length: %d",
											streamID, caid, ecmpid, provid, subLength);

								progInfo.append((const char *)&ptr[j + i + 5], subLength + 2);
							}
							// goto next ES Info
							j += subLength + 2;
						}

						// goto next ES entry
						i += esInfoLength + 5;
					}

					// Send it here !!
					const int cpyLength = progInfo.size();
					const int piLenght  = cpyLength + 1 + 4;
					const int totLength = piLenght + 6;
					unsigned char caPMT[totLength + 50];

					// DVBAPI_AOT_CA_PMT 0x9F 80 32 82
					const uint32_t request = htonl(DVBAPI_AOT_CA_PMT);
					std::memcpy(&caPMT[0], &request, 4);
					const uint16_t length = htons(totLength);        // Total Length of caPMT
					std::memcpy(&caPMT[4], &length, 2);
					caPMT[ 6] = LIST_ONLY_UPDATE;                    // send LIST_ONLY_UPDATE
//					caPMT[ 6] = LIST_ONLY;                           // send LIST_ONLY
					const uint16_t programNr = htons(programNumber); // Program ID
					std::memcpy(&caPMT[7], &programNr, 2);
					caPMT[ 9] = DVBAPI_PROTOCOL_VERSION;             // Version
					const uint16_t pLenght = htons(piLenght);        // Prog Info Length
					std::memcpy(&caPMT[10], &pLenght, 2);
					caPMT[12] = 0x01;                                // ca_pmt_cmd_id = CAPMT_CMD_OK_DESCRAMBLING
					caPMT[13] = 0x82;                                // CAPMT_DESC_DEMUX
					caPMT[14] = 0x02;                                // Length
					caPMT[15] = static_cast<char>(streamID + _adapterOffset); // Demux ID
					caPMT[16] = static_cast<char>(streamID + _adapterOffset); // streamID
					std::memcpy(&caPMT[17], progInfo.c_str(), cpyLength); // copy Prog Info data

					SI_LOG_BIN_DEBUG(caPMT, totLength + 6, "Stream: %d, PMT data to OSCam", streamID);

					if (!_client.sendData(caPMT, totLength + 6, MSG_DONTWAIT)) {
						SI_LOG_ERROR("Stream: %d, PMT - send data to server failed", streamID);
					}
				} else {
					SI_LOG_ERROR("Stream: %d, PMT: CRC Error! Calc CRC32: %04X - Msg CRC32: %04X  Retrying to collect data...", streamID, calccrc, crc);
					frontend->setTableCollected(PMT_TABLE_ID, false);
				}
			}
		}
	}

	void Client::threadEntry() {
		SI_LOG_INFO("Setting up DVBAPI client");

		struct pollfd pfd[1];

		// set time to try to connect
		std::time_t retryTime = std::time(nullptr) + 2;

		for (;; ) {
			// try to connect to server
			if (!_connected) {
				pfd[0].events  = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
				pfd[0].revents = 0;
				pfd[0].fd      = -1;
				if (_enabled) {
					const std::time_t currTime = std::time(nullptr);
					if (retryTime < currTime) {
						if (initClientSocket(_client, _serverPort, _serverIpAddr.c_str())) {
							sendClientInfo();
							pfd[0].fd = _client.getFD();
						} else {
							_client.closeFD();
							retryTime = currTime + 5;
						}
					}
				}
			}
			// call poll with a timeout of 500 ms
			const int pollRet = poll(pfd, 1, 500);
			if (pollRet > 0) {
				if (pfd[0].revents != 0) {
					char tmpData[2048];
					auto i = 0;
					const ssize_t size = _client.recvDatafrom(tmpData, sizeof(tmpData) - 1, MSG_DONTWAIT);
					const unsigned char *buf = reinterpret_cast<unsigned char *>(&tmpData);
					if (size > 0) {
						while (i < size) {
							// get command
							const uint32_t cmd = (buf[i + 0] << 24) | (buf[i + 1] << 16) | (buf[i + 2] << 8) | buf[i + 3];
							SI_LOG_DEBUG("Stream: %d, Receive data total size %zu - cmd: 0x%X", buf[i + 4] - _adapterOffset, size, cmd);

							switch (cmd) {
								case DVBAPI_SERVER_INFO: {
										_serverName.assign(reinterpret_cast<const char *>(&buf[i + 7]), buf[i + 6]);
										SI_LOG_INFO("Connected to %s", _serverName.c_str());
										_connected = true;

										// Goto next cmd
										i += 7 + _serverName.size();
										break;
									}
								case DVBAPI_DMX_SET_FILTER: {
										const int adapter =  buf[i + 4] - _adapterOffset;
										const int demux   =  buf[i + 5];
										const int filter  =  buf[i + 6];
										const int pid     = (buf[i + 7] << 8) | buf[i + 8];
										const unsigned char *filterData = &buf[i + 9];
										const unsigned char *filterMask = &buf[i + 25];

//										SI_LOG_BIN_DEBUG(&buf[i], 65, "Stream: %d, DVBAPI_DMX_SET_FILTER", adapter);

										const input::dvb::SpFrontendDecryptInterface frontend = _getFrontendDecryptInterface(adapter);
										frontend->startOSCamFilterData(pid, demux, filter, filterData, filterMask);

										// Goto next cmd
										i += 65;
										break;
									}
								case DVBAPI_DMX_STOP: {
										const int adapter =  buf[i + 4] - _adapterOffset;
										const int demux   =  buf[i + 5];
										const int filter  =  buf[i + 6];
										const int pid     = (buf[i + 7] << 8) | buf[i + 8];

										const input::dvb::SpFrontendDecryptInterface frontend = _getFrontendDecryptInterface(adapter);
										frontend->stopOSCamFilterData(pid, demux, filter);

										// Goto next cmd
										i += 9;
										break;
									}
								case DVBAPI_CA_SET_DESCR: {
										const int adapter =  buf[i + 4] - _adapterOffset;
										const int index   = (buf[i + 5] << 24) | (buf[i +  6] << 16) | (buf[i +  7] << 8) | buf[i +  8];
										const int parity  = (buf[i + 9] << 24) | (buf[i + 10] << 16) | (buf[i + 11] << 8) | buf[i + 12];
										unsigned char cw[9];
										memcpy(cw, &buf[i + 13], 8);
										cw[8] = 0;

										const input::dvb::SpFrontendDecryptInterface frontend = _getFrontendDecryptInterface(adapter);
										frontend->setKey(cw, parity, index);
										SI_LOG_DEBUG("Stream: %d, Received %s(%02X) CW: %02X %02X %02X %02X %02X %02X %02X %02X  index: %d",
													 adapter, (parity == 0) ? "even" : "odd", parity, cw[0], cw[1], cw[2], cw[3], cw[4], cw[5], cw[6], cw[7], index);

										// Goto next cmd
										i += 21;
										break;
									}
								case DVBAPI_CA_SET_PID:
									// Goto next cmd
									i += 13;
									break;
								case DVBAPI_ECM_INFO: {
										const int adapter   =  buf[i +  4] - _adapterOffset;
										const int serviceID = (buf[i +  5] <<  8) |  buf[i +  6];
										const int caID      = (buf[i +  7] <<  8) |  buf[i +  8];
										const int pid       = (buf[i +  9] <<  8) |  buf[i + 10];
										const int provID    = (buf[i + 11] << 24) | (buf[i + 12] << 16) | (buf[i + 13] << 8) | buf[i + 14];
										const int emcTime   = (buf[i + 15] << 24) | (buf[i + 16] << 16) | (buf[i + 17] << 8) | buf[i + 18];
										i += 19;
										std::string cardSystem;
										cardSystem.assign(reinterpret_cast<const char *>(&buf[i + 1]), buf[i + 0]);
										i += buf[i + 0] + 1;
										std::string readerName;
										readerName.assign(reinterpret_cast<const char *>(&buf[i + 1]), buf[i + 0]);
										i += buf[i + 0] + 1;
										std::string sourceName;
										sourceName.assign(reinterpret_cast<const char *>(&buf[i + 1]), buf[i + 0]);
										i += buf[i + 0] + 1;
										std::string protocolName;
										protocolName.assign(reinterpret_cast<const char *>(&buf[i + 1]), buf[i + 0]);
										i += buf[i + 0] + 1;
										const int hops = buf[i];
										++i;

										const input::dvb::SpFrontendDecryptInterface frontend = _getFrontendDecryptInterface(adapter);
										frontend->setECMInfo(pid, serviceID, caID, provID, emcTime,
														  cardSystem, readerName, sourceName, protocolName, hops);
										SI_LOG_DEBUG("Stream: %d, Receive ECM Info System: %s  Reader: %s  Source: %s  Protocol: %s  ECM Time: %d",
													 adapter, cardSystem.c_str(), readerName.c_str(), sourceName.c_str(), protocolName.c_str(), emcTime);
										break;
									}
								default:
									SI_LOG_BIN_DEBUG(buf, size, "Stream: %d, Receive unexpected data", 0);

									i = size;
									break;
							}
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

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void Client::fromXML(const std::string &xml) {
		base::MutexLock lock(_mutex);

		std::string element;
		if (findXMLElement(xml, "OSCamIP.value", element)) {
			_serverIpAddr = element;
		}
		if (findXMLElement(xml, "OSCamPORT.value", element)) {
			_serverPort = atoi(element.c_str());
		}
		if (findXMLElement(xml, "AdapterOffset.value", element)) {
			_adapterOffset = atoi(element.c_str());
		}
		if (findXMLElement(xml, "OSCamEnabled.value", element)) {
			_enabled = (element == "true") ? true : false;
			if (!_enabled) {
				SI_LOG_INFO("Connection closed with %s", _serverName.c_str());
				_serverName = "Not connected";
				_client.closeFD();
				_connected = false;
			}
		}
		if (findXMLElement(xml, "RewritePMT.value", element)) {
			_rewritePMT = (element == "true") ? true : false;
		}
		saveXML(xml);
	}

	void Client::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);

		// make data xml
		xml  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
		xml += "<data>\r\n";

		ADD_CONFIG_CHECKBOX(xml, "OSCamEnabled", (_enabled ? "true" : "false"));
		ADD_CONFIG_CHECKBOX(xml, "RewritePMT", (_rewritePMT ? "true" : "false"));
		ADD_CONFIG_IP_INPUT(xml, "OSCamIP", _serverIpAddr.c_str());
		ADD_CONFIG_NUMBER_INPUT(xml, "OSCamPORT", _serverPort.load(), 0, 65535);
		ADD_CONFIG_NUMBER_INPUT(xml, "AdapterOffset", _adapterOffset.load(), 0, 128);
		ADD_CONFIG_TEXT(xml, "OSCamServerName", _serverName.c_str());

		xml += "</data>\r\n";
	}

} // namespace dvbapi
} // namespace decrypt
