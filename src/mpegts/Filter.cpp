/* Filter.cpp

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
#include <mpegts/Filter.h>

#include <Log.h>
#include <Utils.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace mpegts {

// =============================================================================
// -- Constructors and destructor ----------------------------------------------
// =============================================================================

Filter::Filter() {
	_nit = std::make_shared<NIT>();
	_pat = std::make_shared<PAT>();
	_pcr = std::make_shared<PCR>();
	_sdt = std::make_shared<SDT>();
	_userPids = "0,1,16,17,18";
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void Filter::doAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "pidcsv", getPidCSV());
	ADD_XML_ELEMENT(xml, "totalCCErrors", getTotalCCErrors());
	ADD_XML_CHECKBOX(xml, "filterPCR", (_filterPCR ? "true" : "false"));
	ADD_XML_TEXT_INPUT(xml, "addUserPids", _userPids);

	const SDT::Data sdtData = getSDTData()->getSDTDataFor(
			getPMTData(0)->getProgramNumber());
	ADD_XML_ELEMENT(xml, "channelname", sdtData.channelNameUTF8);
	ADD_XML_ELEMENT(xml, "networkname", sdtData.networkNameUTF8);

	ADD_XML_ELEMENT(xml, "pat", getPATData()->toXML());
	for (const auto &[pid, pmt] : _pmtMap) {
		ADD_XML_ELEMENT(xml, "pmt_" + DIGIT(pid, 4), pmt->toXML());
	}
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
	_pmtMap.clear();
	_pidTable.clear();
}

void Filter::parsePIDString(const std::string &reqPids, const bool add) {
	base::MutexLock lock(_mutex);
	if (reqPids.find("all") != std::string::npos ||
		reqPids.find("none") != std::string::npos) {
		// all/none pids requested then 'remove' all used PIDS first
		_pidTable.clear();
		if (reqPids.find("all") != std::string::npos) {
			_pidTable.setAllPID(add);
		}
	} else {
		const std::string pidlist = reqPids + ((add) ? ("," + _userPids) : "");
		StringVector pids = StringConverter::split(pidlist, ",");
		for (const std::string &pid : pids) {
			if (std::isdigit(pid[0]) != 0) {
				_pidTable.setPID(std::stoi(pid), add);
			}
		}
	}
}

void Filter::filterData(const FeID id, mpegts::PacketBuffer &buffer, const bool filter) {
//	base::MutexLock lock(_mutex);
	const std::size_t size = buffer.getNumberOfCompletedPackets();
	const std::size_t begin = buffer.getBeginOfUnFilteredPackets();

	for (std::size_t i = begin; i < size; ++i) {
		const unsigned char *ptr = buffer.getTSPacketPtr(i);
		// Check is this the beginning of the TS and no Transport error indicator
		if (ptr[0] != 0x47 || (ptr[1] & 0x80) == 0x80) {
			if (filter && !_pidTable.isAllPID()) {
				buffer.markTSForPurging(i);
			}
			continue;
		}
		// get PID and CC from TS
		const uint16_t pid = ((ptr[1] & 0x1f) << 8) | ptr[2];
		// If pid was not opened, skip this one (and perhaps purge it)
		if (!_pidTable.isPIDOpened(pid)) {
			if (filter && !_pidTable.isAllPID()) {
				buffer.markTSForPurging(i);
			}
			continue;
		}
		const uint8_t cc = ptr[3] & 0x0f;
		_pidTable.addPIDData(pid, cc);

		switch (pid) {
			case 0:
				if (!_pat->isCollected()) {
					// collect PAT data
					_pat->collectData(id, TableData::PAT_ID, ptr, false);
					// Did we finish collecting PAT
					if (_pat->isCollected()) {
						_pat->parse(id);
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
					// Did we finish collecting PMT
					_pmtMap.try_emplace(pid, std::make_shared<PMT>());
					if (!_pmtMap[pid]->isCollected()) {
						// collect PMT data
						_pmtMap[pid]->collectData(id, TableData::PMT_ID, ptr, false);
						if (_pmtMap[pid]->isCollected()) {
							_pmtMap[pid]->parse(id);
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
					for (const auto &[_, pmt] : _pmtMap) {
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

bool Filter::isMarkedAsActivePMT(const int pid) const {
	// Do not use lock here, its used for decrypt (Uses to much time)
	if (_pat->isMarkedAsPMT(pid) && _pmtMap.find(pid) != _pmtMap.end()) {
		const int pcrPID = _pmtMap[pid]->getPCRPid();
		if (_pidTable.isPIDOpened(pcrPID) && _pidTable.getPacketCounter(pcrPID) > 0) {
			return true;
		}
	}
	return false;
}

mpegts::SpPMT Filter::getPMTData(const int pid) const {
	base::MutexLock lock(_mutex);
	if (pid == 0) {
		// Try to find current PMT based on open PCR
		for (const auto &[_, pmt] : _pmtMap) {
			const int pcrPID = pmt->getPCRPid();
			if (_pidTable.isPIDOpened(pcrPID) && _pidTable.getPacketCounter(pcrPID) > 0) {
				return pmt;
			}
		}
	}
	if (_pmtMap.find(pid) != _pmtMap.end()) {
		return _pmtMap[pid];
	}
	return std::make_shared<PMT>();
}

uint32_t Filter::getTotalCCErrors() const {
	base::MutexLock lock(_mutex);
	return _pidTable.getTotalCCErrors();
}

std::string Filter::getPidCSV() const {
	base::MutexLock lock(_mutex);
	return _pidTable.getPidCSV();
}

void Filter::setPID(const int pid, const bool val) {
	base::MutexLock lock(_mutex);
	_pidTable.setPID(pid, val);
}

}
