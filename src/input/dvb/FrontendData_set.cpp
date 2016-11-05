/* FrontendData_set.cpp

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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
#include <input/dvb/FrontendData.h>

namespace input {
namespace dvb {

	void FrontendData::resetFrontendDataChanged() {
		base::MutexLock lock(_mutex);
		_changed = false;
	}

	void FrontendData::addPIDData(int pid, uint8_t cc) {
		base::MutexLock lock(_mutex);
		_pidTable.addPIDData(pid, cc);
	}

	void FrontendData::resetPidData(int pid) {
		base::MutexLock lock(_mutex);
		_pidTable.resetPidData(pid);
	}

	void FrontendData::setDMXFileDescriptor(int pid, int fd) {
		base::MutexLock lock(_mutex);
		_pidTable.setDMXFileDescriptor(pid, fd);
	}

	void FrontendData::closeDMXFileDescriptor(int pid) {
		base::MutexLock lock(_mutex);
		_pidTable.closeDMXFileDescriptor(pid);
	}

	void FrontendData::resetPIDTableChanged() {
		base::MutexLock lock(_mutex);
		_pidTable.resetPIDTableChanged();
	}

	void FrontendData::setPID(int pid, bool val) {
		base::MutexLock lock(_mutex);
		_pidTable.setPID(pid, val);
	}

	void FrontendData::setAllPID(bool val) {
		base::MutexLock lock(_mutex);
		_pidTable.setAllPID(val);
	}

	void FrontendData::setFrequency(uint32_t freq) {
		base::MutexLock lock(_mutex);
		_freq = freq;
		_changed = true;
	}

	void FrontendData::setSymbolRate(int srate) {
		base::MutexLock lock(_mutex);
		_srate = srate;
	}

	void FrontendData::setDeliverySystem(input::InputSystem delsys) {
		base::MutexLock lock(_mutex);
		_delsys = delsys;
	}

	void FrontendData::setModulationType(int modtype) {
		base::MutexLock lock(_mutex);
		_modtype = modtype;
	}

	void FrontendData::setPolarization(int pol) {
		base::MutexLock lock(_mutex);
		_pol_v = pol;
	}

	void FrontendData::setDiSEqcSource(int source) {
		base::MutexLock lock(_mutex);
		_src = source;
	}

	void FrontendData::setRollOff(int rolloff) {
		base::MutexLock lock(_mutex);
		_rolloff = rolloff;
	}

	void FrontendData::setFEC(int fec) {
		base::MutexLock lock(_mutex);
		_fec = fec;
	}

	void FrontendData::setPilotTones(int pilot) {
		base::MutexLock lock(_mutex);
		_pilot = pilot;
	}

	void FrontendData::setSpectralInversion(int specinv) {
		base::MutexLock lock(_mutex);
		_inversion = specinv;
	}

	void FrontendData::setHierarchy(int hierarchy) {
		base::MutexLock lock(_mutex);
		_hierarchy = hierarchy;
	}

	void FrontendData::setBandwidthHz(int bandwidth) {
		base::MutexLock lock(_mutex);
		_bandwidthHz = bandwidth;
	}

	void FrontendData::setTransmissionMode(int transmission) {
		base::MutexLock lock(_mutex);
		_transmission = transmission;
	}

	void FrontendData::setGuardInverval(int guard) {
		base::MutexLock lock(_mutex);
		_guard = guard;
	}

	void FrontendData::setUniqueIDPlp(int plp) {
		base::MutexLock lock(_mutex);
		_plp_id = plp;
	}

	void FrontendData::setUniqueIDT2(int id) {
		base::MutexLock lock(_mutex);
		_t2_system_id = id;
	}

	void FrontendData::setSISOMISO(int sm) {
		base::MutexLock lock(_mutex);
		_siso_miso = sm;
	}

	void FrontendData::setDataSlice(int slice) {
		base::MutexLock lock(_mutex);
		_data_slice = slice;
	}

	void FrontendData::setC2TuningFrequencyType(int c2tft) {
		base::MutexLock lock(_mutex);
		_c2tft = c2tft;
	}

} // namespace dvb
} // namespace input
