/* Filter.h

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
#ifndef MPEGTS_FILTER_H_INCLUDE
#define MPEGTS_FILTER_H_INCLUDE MPEGTS_FILTER_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <Log.h>
#include <base/Mutex.h>
#include <base/XMLSupport.h>
#include <mpegts/NIT.h>
#include <mpegts/PAT.h>
#include <mpegts/PCR.h>
#include <mpegts/PidTable.h>
#include <mpegts/PMT.h>
#include <mpegts/SDT.h>

#include <unordered_map>

FW_DECL_NS1(mpegts, PacketBuffer);

namespace mpegts {

/// The class @c Filter carries the PID Tables
class Filter :
	public base::XMLSupport {
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		Filter();

		virtual ~Filter() = default;

		// =========================================================================
		// -- base::XMLSupport -----------------------------------------------------
		// =========================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		/// Clear ONLY @see PidTable and MPEG Tables (Does not close PIDS)
		void clear();

		/// Parse the CSV PID string with requested PIDs and update @see PidTable
		/// @param id
		/// @param reqPids specifies the requested PIDs
		/// @param add specifies if true to open all the PIDs or false to close
		void parsePIDString(FeID id, const std::string &reqPids, bool add);

		/// Add the filter data to MPEG Tables and
		/// optionally purge TS packets from unused pids if filter is true
		/// @param feID specifies the frontend ID
		/// @param buffer specifies the mpegts buffer from the frontend
		/// @param filter enables the software pid filtering
		void filterData(FeID id, mpegts::PacketBuffer &buffer, bool filter);

		/// This will return true if the requested pid is the active/current one
		/// accoording to the PCR that is open.
		/// @param pid specifies the PID to check if it is the current one
		bool isMarkedAsActivePMT(int pid) const {
			base::MutexLock lock(_mutex);
			if (_pat->isMarkedAsPMT(pid) && _pmtMap.find(pid) != _pmtMap.end()) {
				const int pcrPID = _pmtMap[pid]->getPCRPid();
				if (_pidTable.isPIDOpened(pcrPID) && _pidTable.getPacketCounter(pcrPID) > 0) {
					return true;
				}
			}
			return false;
		}

		/// This will return the requested PMT for the specified pid
		/// @param pid specifies the PID to retrieve if it does not exists it will
		/// return an empty PMT. When set to 0 it will try to return the current PMT
		/// accoording the PCR that is open
		mpegts::SpPMT getPMTData(int pid) const {
			base::MutexLock lock(_mutex);
			if (pid == 0) {
				// Try to find current PMT based on open PCR
				for (const auto& [_, pmt] : _pmtMap) {
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

		///
		mpegts::SpPCR getPCRData() const {
			base::MutexLock lock(_mutex);
			return _pcr;
		}

		///
		mpegts::SpPAT getPATData() const {
			base::MutexLock lock(_mutex);
			return _pat;
		}

		///
		mpegts::SpSDT getSDTData() const {
			base::MutexLock lock(_mutex);
			return _sdt;
		}

		///
		mpegts::SpNIT getNITData() const {
			base::MutexLock lock(_mutex);
			return _nit;
		}

		// =========================================================================
		// =========================================================================

		/// Get the total amount of Continuity Counter Error
		uint32_t getTotalCCErrors() const {
			base::MutexLock lock(_mutex);
			return _pidTable.getTotalCCErrors();
		}

		/// Get the CSV of all the requested PID
		std::string getPidCSV() const {
			base::MutexLock lock(_mutex);
			return _pidTable.getPidCSV();
		}

		/// Set pid used or not
		void setPID(int pid, bool val) {
			base::MutexLock lock(_mutex);
			_pidTable.setPID(pid, val);
		}

		/// Close all active PID filter
		/// @param feID specifies the frontend ID
		/// @param closePid specifies the lambda function to use to close the PIDs
		template<typename CLOSE_FUNC>
		void closeActivePIDFilters(const FeID feID, CLOSE_FUNC closePid) {
			base::MutexLock lock(_mutex);
			SI_LOG_INFO("Frontend: @#1, Closing all active PID filters...", feID);
			for (int pid = 0; pid < mpegts::PidTable::MAX_PIDS; ++pid) {
				_pidTable.setPID(pid, false);
				if (_pidTable.shouldPIDClose(pid)) {
					closePIDFilter_L(feID, pid, closePid);
				}
			}
		}

		/// Update all PID filters set/reset in @see PidTable
		/// @param feID specifies the frontend ID
		/// @param openPid specifies the lambda function to use to open the PIDs
		/// @param closePid specifies the lambda function to use to close the PIDs
		template<typename OPEN_FUNC, typename CLOSE_FUNC>
		void updatePIDFilters(const FeID feID, OPEN_FUNC openPid, CLOSE_FUNC closePid) {
			base::MutexLock lock(_mutex);
			if (!_pidTable.hasPIDTableChanged()) {
				return;
			}
			_pidTable.resetPIDTableChanged();
			SI_LOG_INFO("Frontend: @#1, Updating PID filters...", feID);
			for (int pid = 0; pid < mpegts::PidTable::MAX_PIDS; ++pid) {
				// Check should we close PIDs first then open again
				if (_pidTable.shouldPIDClose(pid)) {
					closePIDFilter_L(feID, pid, closePid);
				}
				if (_pidTable.shouldPIDOpen(pid)) {
					openPIDFilter_L(feID, pid, openPid);
				}
			}
		}

	private:

		/// Open requesed PID filter
		/// @param feID specifies the frontend ID
		/// @param pid specifies the PID to open with openPid
		/// @param openPid specifies the lambda function to use to open the PID with
		template<typename OPEN_FUNC>
		void openPIDFilter_L(const FeID feID, const int pid, OPEN_FUNC openPid) {
			_mutex.unlock();
			const bool done = openPid(pid);
			_mutex.tryLock(15000);
			if (done) {
				_pidTable.setPIDOpened(pid);
				SI_LOG_DEBUG("Frontend: @#1, Set filter PID: @#2@#3",
					feID, PID(pid),
					_pat->isMarkedAsPMT(pid) ? " - PMT" : "");
			}
		}

		/// Close requesed PID filter
		/// @param feID specifies the frontend ID
		/// @param pid specifies the PID to close with closePid
		/// @param closePid specifies the lambda function to use to close the PID with
		template<typename CLOSE_FUNC>
		void closePIDFilter_L(const FeID feID, const int pid, CLOSE_FUNC closePid) {
			_mutex.unlock();
			const bool done = closePid(pid);
			_mutex.tryLock(15000);
			if (done) {
				SI_LOG_DEBUG("Frontend: @#1, Remove filter PID: @#2 - Packet Count: @#3:@#4@#5",
					feID, PID(pid),
					DIGIT(_pidTable.getPacketCounter(pid), 9),
					DIGIT(_pidTable.getCCErrors(pid), 6),
					_pat->isMarkedAsPMT(pid) ? " - PMT" : "");
				// Clear stats
				_pidTable.setPIDClosed(pid);
				// Need to clear the PID Tables as well?
				if (pid == 0) {
					_pat = std::make_shared<PAT>();
				} else if (pid == 17) {
					_sdt = std::make_shared<SDT>();
				} else if (_pmtMap.find(pid) != _pmtMap.end()) {
					_pmtMap.erase(pid);
				} else {
					// Did we close the PCR Pid
					for (const auto& [_, pmt] : _pmtMap) {
						const int pcrPID = pmt->getPCRPid();
						if (pcrPID > 0 && pcrPID == pid) {
							const int pmtPID = pmt->getAssociatedPID();
							SI_LOG_DEBUG("Frontend: @#1, Remove filter PID: @#2 - PCR Changed for PMT: @#3 - Clearing tables", feID, PID(pid), PID(pmtPID));
							_pcr = std::make_shared<PCR>();
							break;
						}
					}
				}
			}
		}

		// =========================================================================
		//  -- Data members --------------------------------------------------------
		// =========================================================================
	private:

		mutable base::Mutex _mutex;

		using PMTMap = std::unordered_map<int, mpegts::SpPMT>;

		mutable PMTMap _pmtMap;

		mutable mpegts::PidTable _pidTable;
		mutable mpegts::SpNIT _nit;
		mutable mpegts::SpPAT _pat;
		mutable mpegts::SpPCR _pcr;
		mutable mpegts::SpSDT _sdt;
		bool _filterPCR = false;
		std::string _userPids;
};

}

#endif // MPEGTS_FILTER_H_INCLUDE
