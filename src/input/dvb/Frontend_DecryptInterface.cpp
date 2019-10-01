/* Frontend_DecryptInterface.cpp

   Copyright (C) 2014 - 2018 Marc Postema (mpostema09 -at- gmail.com)

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
#include <input/dvb/Frontend.h>

#include <input/dvb/FrontendData.h>

namespace input {
namespace dvb {

	int Frontend::getStreamID() const {
		return _streamID;
	}

	int Frontend::getBatchCount() const {
		return _dvbapiData.getBatchCount();
	}

	int Frontend::getBatchParity() const {
		return _dvbapiData.getBatchParity();
	}

	int Frontend::getMaximumBatchSize() const {
		return _dvbapiData.getMaximumBatchSize();
	}

	void Frontend::decryptBatch(bool final) {
		return _dvbapiData.decryptBatch(final);
	}

	void Frontend::setBatchData(unsigned char *ptr, int len, int parity, unsigned char *originalPtr) {
		_dvbapiData.setBatchData(ptr, len, parity, originalPtr);
	}

	const dvbcsa_bs_key_s *Frontend::getKey(int parity) const {
		return _dvbapiData.getKey(parity);
	}

	void Frontend::setKey(const unsigned char *cw, int parity, int index) {
		_dvbapiData.setKey(cw, parity, index);
	}

	void Frontend::startOSCamFilterData(const int pid, const int demux, const int filter,
		const unsigned char *filterData, const unsigned char *filterMask) {
		SI_LOG_INFO("Stream: %d, Start filter PID: %04d  demux: %d  filter: %d (data %02x mask %02x %02x)",
			_streamID, pid, demux, filter, filterData[0], filterMask[0], filterMask[1]);
		_dvbapiData.startOSCamFilterData(pid, demux, filter, filterData, filterMask);
		_frontendData.setPID(pid, true);
		// now update frontend, PID list has changed
		updatePIDFilters();
   }

	void Frontend::stopOSCamFilterData(int pid, int demux, int filter) {
		SI_LOG_INFO("Stream: %d, Stop filter PID: %04d  demux: %d  filter: %d", _streamID, pid, demux, filter);
		_dvbapiData.stopOSCamFilterData(demux, filter);
		// Do not remove the PID!
	}

	bool Frontend::findOSCamFilterData(const int streamID, const int pid, const unsigned char *tsPacket,
		int &tableID, int &filter, int &demux, mpegts::TSData &filterData) {
		return _dvbapiData.findOSCamFilterData(streamID, pid, tsPacket, tableID, filter, demux, filterData);
	}

	void Frontend::stopOSCamFilters(int streamID) {
		_dvbapiData.stopOSCamFilters(streamID);
	}

	void Frontend::setECMInfo(int pid, int serviceID, int caID, int provID, int emcTime,
		const std::string &cardSystem, const std::string &readerName,
		const std::string &sourceName, const std::string &protocolName,
		int hops) {
		_dvbapiData.setECMInfo(pid, serviceID, caID, provID, emcTime,
			cardSystem, readerName, sourceName, protocolName, hops);
	}

	bool Frontend::isMarkedAsPMT(int pid) const {
		return _filter.isMarkedAsPMT(pid);
	}

	mpegts::SpPMT Frontend::getPMTData() const {
		return _filter.getPMTData();
	}

} // namespace dvb
} // namespace input
