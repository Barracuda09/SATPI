/* Filter.cpp

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
	_pat = std::make_shared<PAT>();
	_pcr = std::make_shared<PCR>();
	_pmt = std::make_shared<PMT>();
	_sdt = std::make_shared<SDT>();
}

Filter::~Filter() {}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

void Filter::clear() {
	base::MutexLock lock(_mutex);
	_pat = std::make_shared<PAT>();
	_pcr = std::make_shared<PCR>();
	_pmt = std::make_shared<PMT>();
	_sdt = std::make_shared<SDT>();
	_pidTable.clear();
}

void Filter::addData(const FeID id, const mpegts::PacketBuffer &buffer) {
	base::MutexLock lock(_mutex);
	static constexpr std::size_t size = mpegts::PacketBuffer::getNumberOfTSPackets();
	for (std::size_t i = 0; i < size; ++i) {
		const unsigned char *ptr = buffer.getTSPacketPtr(i);
		// Check is this the beginning of the TS and no Transport error indicator
		if (!(ptr[0] == 0x47 && (ptr[1] & 0x80) != 0x80)) {
			continue;
		}

		// get PID and CC from TS
		const uint16_t pid = ((ptr[1] & 0x1f) << 8) | ptr[2];
		const uint8_t  cc  =   ptr[3] & 0x0f;
		_pidTable.addPIDData(pid, cc);

		if (pid == 0) {
			if (!_pat->isCollected()) {
				// collect PAT data
				_pat->collectData(id.getID(), PAT_TABLE_ID, ptr, false);

				// Did we finish collecting PAT
				if (_pat->isCollected()) {
					_pat->parse(id.getID());
				}
			}
		} else if (_pat->isMarkedAsPMT(pid) /*&& _pidTable.isPIDUsed(pid)*/) {
			if (!_pmt->isCollected()) {
#ifdef ADDDVBCA
				{
					const char fileFIFO[] = "/tmp/fifo";
					int fd = ::open(fileFIFO, O_WRONLY | O_NONBLOCK);
					if (fd > 0) {
						::write(fd, ptr, 188);
						::close(fd);
					}
				}
#endif
				// collect PMT data
				_pmt->collectData(id.getID(), PMT_TABLE_ID, ptr, true);

				// Did we finish collecting PMT
				if (_pmt->isCollected()) {
					// First check if this is PMT we need. So first get PCR Pid
					const int pcrPID = _pmt->parsePCRPid();
					if (!_pidTable.isPIDOpened(pcrPID) && _pidTable.getPacketCounter(pcrPID) == 0) {
						// Probably not the correct PMT, so clear it and try again
						_pmt = std::make_shared<PMT>();
						SI_LOG_INFO("Frontend: @#1, Found PMT, but probably not the correct PMT: @#2  PCR-PID: @#3, Retrying...",
							id, DIGIT(pid, 4), DIGIT(pcrPID, 4));
					} else {
						// Yes, the correct one, then parse it
						_pmt->parse(id.getID());
					}
				}
			}
		} else if (pid == 17) {
			if (!_sdt->isCollected()) {
				// collect SDT data
				_sdt->collectData(id.getID(), SDT_TABLE_ID, ptr, false);

				// Did we finish collecting SDT
				if (_sdt->isCollected()) {
					_sdt->parse(id.getID());
				}
			}
		} else if (pid == 20) {
#ifdef ADDDVBCA
			{
				const char fileFIFO[] = "/tmp/fifo";
				int fd = ::open(fileFIFO, O_WRONLY | O_NONBLOCK);
				if (fd > 0) {
					::write(fd, ptr, 188);
					::close(fd);
				}
			}
#endif
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
				id, HEX(tableID, 2), y, m, d, HEX(h, 2), HEX(mi, 2), HEX(s, 2), HEX(mjd, 4));
//				SI_LOG_BIN_DEBUG(ptr, 188, "Frontend: @#1, TDT - ", _feID);
		} else if (pid == _pmt->getPCRPid()) {
			_pcr->collectData(id.getID(), ptr);
		}
	}
}

bool Filter::isMarkedAsActivePMT(const int pid) const {
	if (isMarkedAsPMT(pid)) {
		const int pcrPID = _pmt->getPCRPid();
		if (_pidTable.isPIDOpened(pcrPID) || _pidTable.getPacketCounter(pcrPID) != 0) {
			return true;
		}
		// Probably not the correct PMT, so clear it and try again
		_pmt = std::make_shared<PMT>();
	}
	return false;
}

void Filter::resetPIDTableChanged() {
	base::MutexLock lock(_mutex);
	_pidTable.resetPIDTableChanged();
}

bool Filter::hasPIDTableChanged() const {
	base::MutexLock lock(_mutex);
	return _pidTable.hasPIDTableChanged();
}

uint32_t Filter::getPacketCounter(const int pid) const {
	base::MutexLock lock(_mutex);
	return _pidTable.getPacketCounter(pid);
}

uint32_t Filter::getCCErrors(int pid) const {
	base::MutexLock lock(_mutex);
	return _pidTable.getCCErrors(pid);
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

bool Filter::shouldPIDClose(const int pid) const {
	base::MutexLock lock(_mutex);
	return _pidTable.shouldPIDClose(pid);
}

void Filter::setPIDClosed(const int pid) {
	base::MutexLock lock(_mutex);
	_pidTable.setPIDClosed(pid);
	if (pid == 0) {
//			_pat = std::make_shared<PAT>();
	} else if (_pat->isMarkedAsPMT(pid)) {
		_pmt = std::make_shared<PMT>();
		_pcr = std::make_shared<PCR>();
	} else if (pid == 17) {
		_sdt = std::make_shared<SDT>();
	}
}

bool Filter::shouldPIDOpen(const int pid) const {
	base::MutexLock lock(_mutex);
	return _pidTable.shouldPIDOpen(pid);
}

void Filter::setPIDOpened(const int pid) {
	base::MutexLock lock(_mutex);
	_pidTable.setPIDOpened(pid);
}

void Filter::setAllPID(const bool val) {
	base::MutexLock lock(_mutex);
	_pidTable.setAllPID(val);
}

} // namespace mpegts
