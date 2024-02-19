/* Client.cpp

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
#include <decrypt/dvbapi/Client.h>

#include <Log.h>
#include <Unused.h>
#include <StreamManager.h>
#include <socket/SocketClient.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>
#include <mpegts/TableData.h>
#include <mpegts/PAT.h>
#include <mpegts/PMT.h>
#include <mpegts/SDT.h>
#include <input/dvb/FrontendDecryptInterface.h>

#include <cstring>

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

namespace decrypt::dvbapi {

	//constants used in socket communication:
	#define DVBAPI_PROTOCOL_VERSION 2

	#define DVBAPI_CA_SET_DESCR     0x40106f86
	#define DVBAPI_CA_SET_PID       0x40086f87
	#define DVBAPI_DMX_SET_FILTER   0x403c6f2b
	#define DVBAPI_DMX_STOP         0x00006f2a

	#define DVBAPI_AOT_CA           0x9F803000
	#define DVBAPI_AOT_CA_PMT       0x9F803282
	#define DVBAPI_AOT_CA_STOP      0x9F803F04
	#define DVBAPI_FILTER_DATA      0xFFFF0000
	#define DVBAPI_CLIENT_INFO      0xFFFF0001
	#define DVBAPI_SERVER_INFO      0xFFFF0002
	#define DVBAPI_ECM_INFO         0xFFFF0003

	#define LIST_MORE               0x00 // append 'MORE' CAPMT object the list and start receiving the next object
	#define LIST_FIRST              0x01 // clear the list when 'FIRST' CAPMT object is received, and start receiving the next object
	#define LIST_LAST               0x02 // append 'LAST' CAPMT object to the list, and start working with the list
	#define LIST_ONLY               0x03 // clear the list when 'ONLY' CAPMT object is received, and start working with the object
	#define LIST_ADD                0x04 // append 'ADD' CAPMT object to the current list, and start working with the updated list
	#define LIST_UPDATE             0x05 // replace entry in the list with 'UPDATE' CAPMT object, and start working with the updated list

	Client::Client(StreamManager &streamManager) :
		ThreadBase("DvbApiClient"),
		XMLSupport(),
		_connected(false),
		_enabled(false),
		_rewritePMT(false),
		_serverPort(15011),
		_adapterOffset(0),
		_serverIPAddr("127.0.0.1"),
		_serverName("Not connected"),
		_streamManager(streamManager) {
		startThread();
	}

	Client::~Client() {
		cancelThread();
		joinThread();
	}

	void Client::decrypt(const FeIndex index, const FeID id, mpegts::PacketBuffer &buffer) {
		if (_connected && _enabled) {
			const input::dvb::SpFrontendDecryptInterface frontend = _streamManager.getFrontendDecryptInterface(index);
			const unsigned int maxBatchSize = frontend->getMaximumBatchSize();
			const std::size_t size = buffer.getNumberOfCompletedPackets();
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
						const unsigned int parity = (data[3] & 0x40) > 0;

						// get batch parity and count
						const unsigned int parityBatch = frontend->getBatchParity();
						const unsigned int countBatch  = frontend->getBatchCount();

						// check if the parity changed in this batch (but should not be the begin of the batch)
						// or check if this batch full, then decrypt this batch
						if (countBatch != 0 && (parity != parityBatch || countBatch >= maxBatchSize)) {
							//
							SI_LOG_COND_DEBUG(parity != parityBatch, "Frontend: @#1, PID @#2 Parity changed from @#3 to @#4, decrypting batch size @#5",
								id, PID(pid), parityBatch, parity, countBatch);

							// decrypt this batch
							frontend->decryptBatch();
						}

						// Can we add this packet to the batch
						if (frontend->getKey(parity) != nullptr) {
							// check is there an adaptation field we should skip.
							unsigned int skip = 4;
							if((data[3] & 0x20) && (data[4] < 183)) {
								skip += data[4] + 1;
							}
							// Add it to batch.
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
						// Need to filter this packet to OSCam
						unsigned int demux = 0;
						unsigned int filter = 0;
						unsigned int tableID = data[5];
						mpegts::TSData filterData;
						if (frontend->findOSCamFilterData(pid, data, tableID, filter, demux, filterData)) {
							// Don't send PAT or PMT before we have an active
							if (pid == 0 || frontend->isMarkedAsActivePMT(pid)) {
							} else {
								const unsigned char* tableData = filterData.data();
								const int sectionLength = (((tableData[6] & 0x0F) << 8) | tableData[7]) + 3; // 3 = tableID + length field
								// Check for ICAM in ECM
								if ((tableID == mpegts::TableData::ECM0_ID ||	tableID == mpegts::TableData::ECM1_ID)) {
										frontend->setICAM(((tableData[7] - tableData[9]) == 4) ?
											tableData[26] : 0, ((tableID & 0x01) > 0));
								}
								std::unique_ptr<unsigned char[]> clientData(new unsigned char[sectionLength + 25]);
								const uint32_t request = htonl(DVBAPI_FILTER_DATA);
								std::memcpy(&clientData[0], &request, 4);
								clientData[4] =  demux;
								clientData[5] =  filter;
								std::memcpy(&clientData[6], &tableData[5], sectionLength); // copy Table data
								const int length = sectionLength + 6; // 6 = clientData header

								SI_LOG_DEBUG("Frontend: @#1, Send Filter Data with size @#2 for demux: @#3  filter: @#4 PID @#5 TableID @#6 @#7 @#8 @#9 @#10",
									id, length, demux, filter, PID(pid),
									HEX2(tableData[5]), HEX2(tableData[6]), HEX2(tableData[7]), HEX2(tableData[8]), HEX2(tableData[9]));

								if (!_client.sendData(clientData.get(), length, MSG_DONTWAIT)) {
									SI_LOG_ERROR("Frontend: @#1, Filter - send data to server failed", id);
								}
							}
						}

						if (frontend->isMarkedAsActivePMT(pid)) {
							sendPMT(index, id, *frontend->getSDTData(), *frontend->getPMTData(pid));
							if (_rewritePMT) {
								mpegts::PMT::cleanPI(data);
							}
						}
					}
				}
			}
		}
	}

	bool Client::stopDecrypt(const FeIndex index, const FeID id) {
		const input::dvb::SpFrontendDecryptInterface frontend = _streamManager.getFrontendDecryptInterface(index);
		if (_connected) {
			// Stop 9F 80 3f 04 83 02 00 <demux index>
			const int demux = index.getID() + _adapterOffset;
			const uint32_t request = htonl(DVBAPI_AOT_CA_STOP);
			unsigned char buff[8];
			std::memcpy(&buff[0], &request, 4);
			buff[4] = 0x83;
			buff[5] = 0x02;
			buff[6] = 0x00;
			buff[7] = demux;
			SI_LOG_DEBUG("Frontend: @#1, Stop CA Decrypt with demux index @#2", id, demux);
			if (!_client.sendData(buff, sizeof(buff), MSG_DONTWAIT)) {
				SI_LOG_ERROR("Frontend: @#1, Stop CA Decrypt with demux index @#2 - send data to server failed", id, demux);
				return false;
			}
		}
		// Remove this PMT from the list
		const auto it = _capmtMap.find(index.getID());
		if (it != _capmtMap.end()) {
			_capmtMap.erase(it);
		}
		// cleaning OSCam filters
		frontend->stopOSCamFilters(id);
		return true;
	}

	bool Client::initClientSocket(SocketClient &client, const std::string &ipAddr, int port) {

		client.setupSocketStructure(ipAddr, port, 0);

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
		std::unique_ptr<unsigned char[]> buff(new unsigned char[7 + len]);

		const uint32_t request = htonl(DVBAPI_CLIENT_INFO);
		std::memcpy(&buff[0], &request, 4);

		const uint16_t version = htons(DVBAPI_PROTOCOL_VERSION);
		std::memcpy(&buff[4], &version, 2);

		buff[6] = len;
		std::memcpy(&buff[7], name.data(), len);

		if (!_client.sendData(buff.get(), 7 + len, MSG_DONTWAIT)) {
			SI_LOG_ERROR("write failed");
		}
	}

	void Client::sendPMT(const FeIndex index, const FeID id, const mpegts::SDT &sdt, const mpegts::PMT &pmt) {
		// Did we collect SDT and PMT
		if (!sdt.isCollected()) {
			return;
		}
		// Did we send PMT already
		if (!pmt.isReadySend()) {
			return;
		}
		const int adapterIndex = index.getID() + _adapterOffset;
		const int demuxIndex = adapterIndex;

#if USE_DEPRECATED_DVBAPI
		unsigned char oscamDesc[5];
		oscamDesc[0] = 0x01;          // ca_pmt_cmd_id = CAPMT_CMD_OK_DESCRAMBLING

		oscamDesc[1] = 0x82;          // CAPMT_DESC_DEMUX
		oscamDesc[2] = 0x02;          // Length
		oscamDesc[3] = demuxIndex;    // Demux ID
		oscamDesc[4] = adapterIndex;  // streamID
#else
		const int pid = pmt.getAssociatedPID();
		const int tsid = sdt.getTransportStreamID();
		const int onid = sdt.getNetworkID();
		unsigned char oscamDesc[24];
		oscamDesc[0]  = 0x01;         // ca_pmt_cmd_id = CAPMT_CMD_OK_DESCRAMBLING

		oscamDesc[1]  = 0x81;         // enigma_namespace_descriptor
		oscamDesc[2]  = 0x08;         // descriptor_length
		oscamDesc[3]  = 0x00;         // enigma_namespace
		oscamDesc[4]  = 0x00;
		oscamDesc[5]  = 0x00;
		oscamDesc[6]  = 0x00;
		oscamDesc[7]  = (tsid >> 8);  // transport_stream_id
		oscamDesc[8]  = (tsid & 0xFF);
		oscamDesc[9]  = (onid >> 8);  // original_network_id
		oscamDesc[10] = (onid & 0xFF);

		oscamDesc[11] = 0x83;         // adapter_device_descriptor
		oscamDesc[12] = 0x01;         // descriptor_length
		oscamDesc[13] = adapterIndex; // adapter_device

		oscamDesc[14] = 0x84;         // pmt_pid_descriptor
		oscamDesc[15] = 0x02;         // descriptor_length
		oscamDesc[16] = (pid >> 8);   // pmt_pid (uimsbf)
		oscamDesc[17] = (pid & 0xFF); // pmt_pid (uimsbf)

		oscamDesc[18] = 0x86;         // demux_device_descriptor
		oscamDesc[19] = 0x01;         // descriptor_length
		oscamDesc[20] = demuxIndex;   // demux_device

		oscamDesc[21] = 0x87;         // ca_device_descriptor
		oscamDesc[22] = 0x01;         // descriptor_length
		oscamDesc[23] = demuxIndex;   // ca_device
#endif
		const mpegts::TSData progInfo = pmt.getProgramInfo();
		const int progInfoLen  = progInfo.size() + sizeof(oscamDesc);
		const int totLength = progInfoLen + 6;

		UCharPtr caPMT(new unsigned char[totLength + 6]);

		const uint32_t request = htonl(DVBAPI_AOT_CA_PMT);
		const uint16_t len = htons(totLength);
		const uint16_t prgNr = htons(pmt.getProgramNumber());
		const uint16_t pLen = htons(progInfoLen);

		std::memcpy(&caPMT[0], &request, 4);      // ca_pmt_tag
		std::memcpy(&caPMT[4], &len, 2);          // Total Length of caPMT
		caPMT[6] = LIST_FIRST;                    // ca_pmt_list_management
		std::memcpy(&caPMT[7], &prgNr, 2);        // Program ID
		caPMT[9] = DVBAPI_PROTOCOL_VERSION << 1;  // Version
		std::memcpy(&caPMT[10], &pLen, 2);        // Prog Info Length
		std::memcpy(&caPMT[12], oscamDesc, sizeof(oscamDesc));
		std::memcpy(&caPMT[12 + sizeof(oscamDesc)], progInfo.data(), progInfo.size());
		// Save new PMT so we can send it
		_capmtMap[index.getID()] = PMTEntry{ caPMT, totLength + 6 };

		// Send the collected PMT list
		unsigned int i = 0;
		for (const auto& [_, entry] : _capmtMap) {
			++i;
			if (i == 1) {
				if (_capmtMap.size() == 1) {
					entry.caPtr[6] = LIST_ONLY;
				} else {
					entry.caPtr[6] = LIST_FIRST;
				}
			} else if (i == _capmtMap.size()) {
				entry.caPtr[6] = LIST_LAST;
			} else {
				entry.caPtr[6] = LIST_MORE;
			}
			if (entry.caPtr[13] == 0x81) {
				SI_LOG_BIN_DEBUG(entry.caPtr.get(), entry.size, "Frontend: @#1, PMT data to OSCam with adapter: @#2  demux: @#3  list_management: @#4",
					id, static_cast<int>(entry.caPtr[25]), static_cast<int>(entry.caPtr[32]), HEX2(entry.caPtr[6]));
			} else {
				SI_LOG_BIN_DEBUG(entry.caPtr.get(), entry.size, "Frontend: @#1, PMT data to OSCam with adapter: @#2  demux: @#3  list_management: @#4",
					id, static_cast<int>(entry.caPtr[16]), static_cast<int>(entry.caPtr[15]), HEX2(entry.caPtr[6]));
			}

			if (!_client.sendData(entry.caPtr.get(), entry.size, MSG_DONTWAIT)) {
				SI_LOG_ERROR("Frontend: @#1, PMT - send data to server failed", id);
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
						if (initClientSocket(_client, _serverIPAddr, _serverPort)) {
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
					const unsigned char* buf = reinterpret_cast<unsigned char *>(&tmpData);
					if (size > 0) {
						while (i < size) {
							// get command
							const uint32_t cmd = (buf[i + 0] << 24) | (buf[i + 1] << 16) | (buf[i + 2] << 8) | buf[i + 3];
//							SI_LOG_DEBUG("Frontend: @#1, Receive data total size @#2 - cmd: @#3", buf[i + 4] - _adapterOffset, size, HEX2(cmd));

							switch (cmd) {
								case DVBAPI_SERVER_INFO: {
										_serverName.assign(reinterpret_cast<const char *>(&buf[i + 7]), buf[i + 6]);
										SI_LOG_INFO("Connected to @#1", _serverName);
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
										const unsigned char* filterData = &buf[i + 9];
										const unsigned char* filterMask = &buf[i + 25];

//										SI_LOG_BIN_DEBUG(&buf[i], 65, "Frontend: @#1, DVBAPI_DMX_SET_FILTER", adapter);

										const input::dvb::SpFrontendDecryptInterface frontend = _streamManager.getFrontendDecryptInterface(adapter);
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

										const input::dvb::SpFrontendDecryptInterface frontend = _streamManager.getFrontendDecryptInterface(adapter);
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

										const input::dvb::SpFrontendDecryptInterface frontend = _streamManager.getFrontendDecryptInterface(adapter);
										frontend->setKey(cw, parity, index);
										SI_LOG_DEBUG("Frontend: @#1, Received @#2(@#3) CW: @#4 @#5 @#6 @#7 @#8 @#9 @#10 @#11  index: @#12",
											frontend->getFeID(), (parity == 0) ? "even" : "odd", HEX2(parity),
											HEX2(cw[0]), HEX2(cw[1]), HEX2(cw[2]), HEX2(cw[3]),
											HEX2(cw[4]), HEX2(cw[5]), HEX2(cw[6]), HEX2(cw[7]), index);

										// Goto next cmd
										i += 21;
										break;
									}
								case DVBAPI_CA_SET_PID: {
//										const int adapter   =  buf[i +  4] - _adapterOffset;
//										SI_LOG_BIN_DEBUG(&buf[i], 65, "Frontend: @#1, DVBAPI_CA_SET_PID", adapter);

										// Goto next cmd
										i += 13;
										break;
									}
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

										const input::dvb::SpFrontendDecryptInterface frontend = _streamManager.getFrontendDecryptInterface(adapter);
										frontend->setECMInfo(pid, serviceID, caID, provID, emcTime,
														  cardSystem, readerName, sourceName, protocolName, hops);
										SI_LOG_DEBUG("Frontend: @#1, Receive ECM Info System: @#2  Reader: @#3  Source: @#4  Protocol: @#5  ECM Time: @#6",
											frontend->getFeID(), cardSystem, readerName, sourceName, protocolName, emcTime);
										break;
									}
								default:
									SI_LOG_BIN_DEBUG(buf, size, "Frontend: x, Receive unexpected data");
									i = size;
									break;
							}
						}
					} else {
						// connection closed, try to reconnect
						SI_LOG_INFO("Connection lost with @#1", _serverName);
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

	void Client::doFromXML(const std::string &xml) {
		std::string element;
		if (findXMLElement(xml, "OSCamIP.value", element)) {
			_serverIPAddr = element;
		}
		if (findXMLElement(xml, "OSCamPORT.value", element)) {
			_serverPort = std::stoi(element.data());
		}
		if (findXMLElement(xml, "AdapterOffset.value", element)) {
			_adapterOffset = std::stoi(element.data());
		}
		if (findXMLElement(xml, "OSCamEnabled.value", element)) {
			_enabled = (element == "true") ? true : false;
			if (!_enabled) {
				SI_LOG_INFO("Connection closed with @#1", _serverName);
				_serverName = "Not connected";
				_client.closeFD();
				_connected = false;
			}
		}
		if (findXMLElement(xml, "RewritePMT.value", element)) {
			_rewritePMT = (element == "true") ? true : false;
		}
	}

	void Client::doAddToXML(std::string &xml) const {
		ADD_XML_CHECKBOX(xml, "OSCamEnabled", (_enabled ? "true" : "false"));
		ADD_XML_CHECKBOX(xml, "RewritePMT", (_rewritePMT ? "true" : "false"));
		ADD_XML_IP_INPUT(xml, "OSCamIP", _serverIPAddr);
		ADD_XML_NUMBER_INPUT(xml, "OSCamPORT", _serverPort.load(), 0, 65535);
		ADD_XML_NUMBER_INPUT(xml, "AdapterOffset", _adapterOffset.load(), 0, 128);
		ADD_XML_ELEMENT(xml, "OSCamServerName", _serverName);
	}

}
