/* StreamProperties_ChannelData_set.cpp

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
#include <StreamProperties.h>

void StreamProperties::resetChannelDataChanged() {
	base::MutexLock lock(_mutex);
	_channelData.changed = false;
}

void StreamProperties::setIntermediateFrequency(uint32_t frequency) {
	base::MutexLock lock(_mutex);
	_channelData.ifreq = frequency;
}

void StreamProperties::addPIDData(int pid, uint8_t cc) {
	base::MutexLock lock(_mutex);
	_channelData.addPIDData(pid, cc);
}

void StreamProperties::resetPid(int pid) {
	base::MutexLock lock(_mutex);
	_channelData._pidTable.resetPid(pid);
}

void StreamProperties::setDMXFileDescriptor(int pid, int fd) {
	base::MutexLock lock(_mutex);
	_channelData._pidTable.setDMXFileDescriptor(pid, fd);
}

void StreamProperties::closeDMXFileDescriptor(int pid) {
	base::MutexLock lock(_mutex);
	_channelData._pidTable.closeDMXFileDescriptor(pid);
}

void StreamProperties::resetPIDTableChanged() {
	base::MutexLock lock(_mutex);
	_channelData._pidTable.resetPIDTableChanged();
}

void StreamProperties::setPID(int pid, bool val) {
	base::MutexLock lock(_mutex);
	_channelData._pidTable.setPID(pid, val);
}

void StreamProperties::setAllPID(bool val) {
	base::MutexLock lock(_mutex);
	_channelData._pidTable.setAllPID(val);
}

void StreamProperties::initializeChannelData() {
	base::MutexLock lock(_mutex);
	_channelData.initialize();
}

void StreamProperties::setFrequency(uint32_t freq) {
	base::MutexLock lock(_mutex);
	_channelData.freq = freq;
	_channelData.changed = true;
}

void StreamProperties::setSymbolRate(int srate) {
	base::MutexLock lock(_mutex);
	_channelData.srate = srate;
}

void StreamProperties::setDeliverySystem(fe_delivery_system_t delsys) {
	base::MutexLock lock(_mutex);
	_channelData.delsys = delsys;
}

void StreamProperties::setModulationType(int modtype) {
	base::MutexLock lock(_mutex);
	_channelData.modtype = modtype;
}

void StreamProperties::setPolarization(int pol) {
	base::MutexLock lock(_mutex);
	_channelData.pol_v = pol;
}

void StreamProperties::setDiSEqcSource(int source) {
	base::MutexLock lock(_mutex);
	_channelData.src = source;
}

void StreamProperties::setRollOff(int rolloff) {
	base::MutexLock lock(_mutex);
	_channelData.rolloff = rolloff;
}

void StreamProperties::setFEC(int fec) {
	base::MutexLock lock(_mutex);
	_channelData.fec = fec;
}

void StreamProperties::setPilotTones(int pilot) {
	base::MutexLock lock(_mutex);
	_channelData.pilot = pilot;
}

void StreamProperties::setSpectralInversion(int specinv) {
	base::MutexLock lock(_mutex);
	_channelData.inversion = specinv;
}

void StreamProperties::setHierarchy(int hierarchy) {
	base::MutexLock lock(_mutex);
	_channelData.hierarchy = hierarchy;
}

void StreamProperties::setBandwidthHz(int bandwidth) {
	base::MutexLock lock(_mutex);
	_channelData.bandwidthHz = bandwidth;
}

void StreamProperties::setTransmissionMode(int transmission) {
	base::MutexLock lock(_mutex);
	_channelData.transmission = transmission;
}

void StreamProperties::setGuardInverval(int guard) {
	base::MutexLock lock(_mutex);
	_channelData.guard = guard;
}

void StreamProperties::setUniqueIDPlp(int plp) {
	base::MutexLock lock(_mutex);
	_channelData.plp_id = plp;
}

void StreamProperties::setUniqueIDT2(int id) {
	base::MutexLock lock(_mutex);
	_channelData.t2_system_id = id;
}

void StreamProperties::setSISOMISO(int sm) {
	base::MutexLock lock(_mutex);
	_channelData.siso_miso = sm;
}
