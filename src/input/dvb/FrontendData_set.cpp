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
		_changed = false;
	}

	void FrontendData::addPIDData(int pid, uint8_t cc) {
		_pidTable.addPIDData(pid, cc);
	}

	void FrontendData::resetPid(int pid) {
		_pidTable.resetPid(pid);
	}

	void FrontendData::setDMXFileDescriptor(int pid, int fd) {
		_pidTable.setDMXFileDescriptor(pid, fd);
	}

	void FrontendData::closeDMXFileDescriptor(int pid) {
		_pidTable.closeDMXFileDescriptor(pid);
	}

	void FrontendData::resetPIDTableChanged() {
		_pidTable.resetPIDTableChanged();
	}

	void FrontendData::setPID(int pid, bool val) {
		_pidTable.setPID(pid, val);
	}

	void FrontendData::setAllPID(bool val) {
		_pidTable.setAllPID(val);
	}

	void FrontendData::setFrequency(uint32_t freq) {
		_freq = freq;
		_changed = true;
	}

	void FrontendData::setSymbolRate(int srate) {
		_srate = srate;
	}

	void FrontendData::setDeliverySystem(fe_delivery_system_t delsys) {
		_delsys = delsys;
	}

	void FrontendData::setModulationType(int modtype) {
		_modtype = modtype;
	}

	void FrontendData::setPolarization(int pol) {
		_pol_v = pol;
	}

	void FrontendData::setDiSEqcSource(int source) {
		_src = source;
	}

	void FrontendData::setRollOff(int rolloff) {
		_rolloff = rolloff;
	}

	void FrontendData::setFEC(int fec) {
		_fec = fec;
	}

	void FrontendData::setPilotTones(int pilot) {
		_pilot = pilot;
	}

	void FrontendData::setSpectralInversion(int specinv) {
		_inversion = specinv;
	}

	void FrontendData::setHierarchy(int hierarchy) {
		_hierarchy = hierarchy;
	}

	void FrontendData::setBandwidthHz(int bandwidth) {
		_bandwidthHz = bandwidth;
	}

	void FrontendData::setTransmissionMode(int transmission) {
		_transmission = transmission;
	}

	void FrontendData::setGuardInverval(int guard) {
		_guard = guard;
	}

	void FrontendData::setUniqueIDPlp(int plp) {
		_plp_id = plp;
	}

	void FrontendData::setUniqueIDT2(int id) {
		_t2_system_id = id;
	}

	void FrontendData::setSISOMISO(int sm) {
		_siso_miso = sm;
	}

	void FrontendData::setDataSlice(int slice) {
		_data_slice = slice;
	}

	void FrontendData::setC2TuningFrequencyType(int c2tft) {
		_c2tft = c2tft;

	}

} // namespace dvb
} // namespace input
