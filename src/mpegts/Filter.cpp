/* Filter.cpp

   Copyright (C) 2014 - 2026 Marc Postema (mpostema09 -at- gmail.com)

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
#include <mpegts/Filter.h>

#include <Log.h>
#include <Utils.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>

#include <fstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace mpegts {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

Filter::Filter() :
		_enigmaTuner(std::ifstream("/proc/stb/info/version").is_open()) {
	_nit = std::make_shared<NIT>();
	_pat = std::make_shared<PAT>();
	_pcr = std::make_shared<PCR>();
	_sdt = std::make_shared<SDT>();
	_userPids = "0,1,16,17,18";

	// NEXUS driver crashes (NULL deref in NEXUS_Recpump_AddPidChannel) on pid >= 0x2000
	// Never send it to the driver; the demux still passes the full TS once any
	// filter is open (verified empirically on Vu+ Duo 4K SE by Logic Encoder).
	// So force Emulate 'pids=all' for Enigma Tuners (Not adjustible for Enigma tuners)
	_emulatePIDSAll = _enigmaTuner;
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void Filter::doAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "pidcsv", getPidCSV());
	ADD_XML_ELEMENT(xml, "totalCCErrors", getTotalCCErrors());
	ADD_XML_CHECKBOX(xml, "filterPCR", (_filterPCR ? "true" : "false"));
	ADD_XML_TEXT_INPUT(xml, "addUserPids", _userPids);
	if (!_enigmaTuner) {
		ADD_XML_CHECKBOX(xml, "emulatePIDSAll", (_emulatePIDSAll ? "true" : "false"));
		ADD_XML_NUMBER_INPUT(xml, "emuTotalPIDBuget", _emuTotalPIDBuget, 5, 50);
	}
	ADD_XML_NUMBER_INPUT(xml, "emuPMTBudget", _emuPMTBudget, 1, 8);

	const SDT::Data sdtData = getSDTData()->getSDTDataFor(
			getPMTData(0)->getProgramNumber());
	ADD_XML_ELEMENT(xml, "channelname", sdtData.channelNameUTF8);
	ADD_XML_ELEMENT(xml, "networkname", sdtData.networkNameUTF8);

	ADD_XML_ELEMENT(xml, "pat", getPATData()->toXML());
	ADD_XML_BEGIN_ELEMENT(xml, "pmtlist");
		for (const auto& [pid, pmt] : _pmtMap) {
			ADD_XML_ELEMENT(xml, "pmt", pmt->toXML());
		}
	ADD_XML_END_ELEMENT(xml, "pmtlist");
	ADD_XML_ELEMENT(xml, "sdt", getSDTData()->toXML());
	ADD_XML_ELEMENT(xml, "nit", getNITData()->toXML());
}

void Filter::doFromXML(const std::string &xml) {
	std::string element;
	if (findXMLElement(xml, "addUserPids.value", element)) {
		if (element.size() > 0) {
			if (element[0] == ',') {
				element.erase(0, 1);
			}
			const auto s = element.size();
			if (s > 0 && element[s - 1] == ',') {
				element.erase(s - 1, 1);
			}
		}
		_userPids = element;
	}
	if (findXMLElement(xml, "filterPCR.value", element)) {
		_filterPCR = (element == "true") ? true : false;
	}
	if (!_enigmaTuner) {
		if (findXMLElement(xml, "emulatePIDSAll.value", element)) {
			_emulatePIDSAll = (element == "true") ? true : false;
		}
		if (findXMLElement(xml, "emuTotalPIDBuget.value", element)) {
			_emuTotalPIDBuget = std::stoi(element);
		}
	}
	if (findXMLElement(xml, "emuPMTBudget.value", element)) {
		_emuPMTBudget = std::stoi(element);
	}
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

void Filter::clear() {
	base::MutexLock lock(_mutex);
	_nit = std::make_shared<NIT>();
	_pat = std::make_shared<PAT>();
	_pcr = std::make_shared<PCR>();
	_sdt = std::make_shared<SDT>();
	_emuPidCount = 0;
	_emuPmtCount = 0;
	_pmtMap.clear();
	_pidTable.clear();
}

void Filter::parsePIDString(const FeID id, const std::string &reqPids, const bool add) {
	base::MutexLock lock(_mutex);
	if (reqPids.find("all") != std::string::npos ||
		reqPids.find("none") != std::string::npos) {
		// all/none pids requested then 'remove' all used PIDS first
		_pidTable.clear();
		if (reqPids.find("all") != std::string::npos) {
			_pidTable.setAllPID(add);
			if (_emulatePIDSAll) {
				// Always open user/table pids too so PAT/SDT/NIT still flow for service discovery.
				const StringVector userPidList = StringConverter::split(_userPids, ",");
				for (const std::string& pid : userPidList) {
					try {
						_pidTable.setPID(std::stoi(pid), add);
					} catch (const std::invalid_argument &) {
						SI_LOG_ERROR("Frontend: @#1, Error, skipping PID: @#2", id, pid);
					}
				}
			}
		}
	} else {
		const StringVector reqPidList = StringConverter::split(reqPids, ",");
		for (const std::string& pid : reqPidList) {
			try {
				if (const auto p = std::stoi(pid); p > 18 || add) {
					_pidTable.setPID(p, add);
				}
			} catch (const std::invalid_argument &) {
				SI_LOG_ERROR("Frontend: @#1, Error, skipping PID: @#2", id, pid);
			}
		}
		if (add) {
			const StringVector userPidList = StringConverter::split(_userPids, ",");
			for (const std::string& pid : userPidList) {
				try {
					_pidTable.setPID(std::stoi(pid), add);
				} catch (const std::invalid_argument &) {
					SI_LOG_ERROR("Frontend: @#1, Error, skipping PID: @#2", id, pid);
				}
			}
		}
	}
}

void Filter::filterData(const FeID id, mpegts::PacketBuffer &buffer, const bool filter) {
	base::MutexLock lock(_mutex);
	const std::size_t begin = buffer.getBeginOfUnFilteredPackets();
	const std::size_t size = buffer.getNumberOfCompletedPackets();

	for (std::size_t i = begin; i < size; ++i) {
		const unsigned char* ptr = buffer.getTSPacketPtr(i);
		const uint16_t pid = ((ptr[1] & 0x1f) << 8) | ptr[2];
		// Check is this the beginning of the TS and no Transport error indicator and not a NULL packet
		if (ptr[0] != 0x47 || (ptr[1] & 0x80) == 0x80 || pid == 0x1FFF || !_pidTable.isPIDOpened(pid)) {
			if (filter && !_pidTable.isAllPID()) {
				buffer.markTSForPurging(i);
			}
			continue;
		}
		_pidTable.addPIDData(pid, ptr[3]);
		switch (pid) {
			case 0:
				if (!_pat->isCollected()) {
					// collect PAT data
					_pat->collectData(id, TableData::PAT_ID, ptr, false);
					// Did we finish collecting PAT
					if (_pat->isCollected()) {
						_pat->parse(id);
						// Emulate 'pids=all' walk PAT -> PMT -> ES and open each discovered pid instead.
						if (_emulatePIDSAll && _pidTable.isAllPID()) {
							for (const auto& [pmtPid, _] : _pat->getPMTPidTable()) {
								if (_emuPmtCount >= _emuPMTBudget || _emuPidCount >= _emuTotalPIDBuget) {
									break;
								}
								_pidTable.setPID(pmtPid, true);
								++_emuPidCount;
								++_emuPmtCount;
							}
						}
					}
				}
				break;
			case 1:
				// Empty
				break;
			case 16:
				if (!_nit->isCollected()) {
					// collect NIT data
					_nit->collectData(id, TableData::NIT_ID, ptr, false);

					// Did we finish collecting SDT
					if (_nit->isCollected()) {
						_nit->parse(id);
					}
				}
				break;
			case 17:
				if (!_sdt->isCollected()) {
					// collect SDT data
					_sdt->collectData(id, TableData::SDT_ID, ptr, false);
					// Did we finish collecting SDT
					if (_sdt->isCollected()) {
						_sdt->parse(id);
					}
				}
				break;
			case 18:
				// Empty
				break;
			case 20: {
				const unsigned int tableID = ptr[5];
				const unsigned int mjd = (ptr[8] << 8) | (ptr[9]);
				const unsigned int y1 = static_cast<unsigned int>((mjd - 15078.2) / 365.25);
				const unsigned int m1 = static_cast<unsigned int>((mjd - 14956.1 - static_cast<unsigned int>(y1 * 365.25)) / 30.6001);
				const unsigned int d = static_cast<unsigned int>(mjd - 14956.0 - static_cast<unsigned int>(y1 * 365.25) - static_cast<unsigned int>(m1 * 30.6001 ));
				const unsigned int k = (m1 == 14 || m1 ==15) ? 1 : 0;
				const unsigned int y = y1 + k + 1900;
				const unsigned int m = m1 - 1 - (k * 12);
				const unsigned int h = ptr[10];
				const unsigned int mi = ptr[11];
				const unsigned int s = ptr[12];

				SI_LOG_INFO("Frontend: @#1, TDT - Table ID: @#2  Date: @#3-@#4-@#5  Time: @#6:@#7.@#8  MJD: @#9",
					id, HEX(tableID, 2), y, m, d, DIGIT(h, 2), DIGIT(mi, 2), DIGIT(s, 2), HEX(mjd, 4));
#ifdef ADDDVBCA
				const char fileFIFO[] = "/tmp/fifo";
				int fd = ::open(fileFIFO, O_WRONLY | O_NONBLOCK);
				if (fd > 0) {
					::write(fd, ptr, 188);
					::close(fd);
				}
#endif
				}
				break;
			case 21:
				// Empty
				break;
			default:
				if (_pat->isMarkedAsPMT(pid)) {
					// Did we finish collecting PMT, we always get a valid PMT (empty or filled)
					mpegts::SpPMT pmt = _pmtMap.try_emplace(pid, std::make_shared<PMT>()).first->second;
					if (!pmt->isCollected()) {
						// collect PMT data
						pmt->collectData(id, TableData::PMT_ID, ptr, false);
						if (pmt->isCollected()) {
							pmt->parse(id);
							// Emulate 'pids=all': open every ES + PCR pid of
							// this service so the full TS flows progressively
							if (_emulatePIDSAll && _pidTable.isAllPID()) {
								for (const auto& es : pmt->getESPIDs()) {
									if (_emuPidCount >= _emuTotalPIDBuget) {
										break;
									}
									switch (es.streamType) {
										// Video
										case 0x01: // MPEG-1 Video
										case 0x02: // MPEG-2 Video
										case 0x10: // MPEG-4 Visual
										case 0x1B: // H.264 / AVC
										case 0x24: // H.265 / HEVC
										case 0x42: // AVS
										case 0xEA: // VC-1
											SI_LOG_INFO("Frontend: @#1, Emulate: Add Video PID @#2", id, PID(es.pid));
											_pidTable.setPID(es.pid, true);
											++_emuPidCount;
											break;
										// Audio
										case 0x03: // MPEG-1 Audio
										case 0x04: // MPEG-2 Audio
										case 0x0F: // AAC ADTS
										case 0x11: // AAC LATM/LOAS
										case 0x1C: // MPEG-4 Audio
											SI_LOG_INFO("Frontend: @#1, Emulate: Add Audio PID @#2", id, PID(es.pid));
											_pidTable.setPID(es.pid, true);
											++_emuPidCount;
											break;
										default:
											// Do not add
											break;
									}
								}
								for (const auto& ecm : pmt->getECMPIDs()) {
									if (ecm.ecmpid > 0 &&
											!_pidTable.shouldPIDOpen(ecm.ecmpid) &&
											!_pidTable.isPIDOpened(ecm.ecmpid) &&
											_emuPidCount < _emuTotalPIDBuget) {
										SI_LOG_INFO("Frontend: @#1, Emulate: Add ECM PID @#2", id, PID(ecm.ecmpid));
										_pidTable.setPID(ecm.ecmpid, true);
										++_emuPidCount;
										// @TODO: We should probably only add the supported caID (for now we add first)
										break;
									}
								}
								const int pcrPid = pmt->getPCRPid();
								if (pcrPid > 0 &&
										!_pidTable.shouldPIDOpen(pcrPid) &&
										!_pidTable.isPIDOpened(pcrPid) &&
										_emuPidCount < _emuTotalPIDBuget) {
									SI_LOG_INFO("Frontend: @#1, Emulate: Add PCR PID @#2", id, PID(pcrPid));
									_pidTable.setPID(pcrPid, true);
									++_emuPidCount;
								}
							}
						}
#ifdef ADDDVBCA
						const char fileFIFO[] = "/tmp/fifo";
						int fd = ::open(fileFIFO, O_WRONLY | O_NONBLOCK);
						if (fd > 0) {
							::write(fd, ptr, 188);
							::close(fd);
						}
#endif
					}
				} else if (_filterPCR && PCR::isPCRTableData(ptr)) {
					for (const auto& [_, pmt] : _pmtMap) {
						const int pcrPID = pmt->getPCRPid();
						if (pid == pcrPID && _pidTable.isPIDOpened(pcrPID) && _pidTable.getPacketCounter(pcrPID) > 0) {
							_pcr->collectData(id, ptr);
						}
					}
				}
				break;
		}
	}
	if (filter) {
		buffer.purge();
	}
}

}
