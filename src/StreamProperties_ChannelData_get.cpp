/* StreamProperties_ChannelData_get.cpp

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#include "StreamProperties.h"

bool StreamProperties::hasChannelDataChanged() const {
	MutexLock lock(_mutex);
	return _channelData.changed;
}

fe_delivery_system_t StreamProperties::getDeliverySystem() const {
	MutexLock lock(_mutex);
	return _channelData.delsys;
}

uint32_t StreamProperties::getIntermediateFrequency() const {
	MutexLock lock(_mutex);
	return _channelData.ifreq;
}

int StreamProperties::getDiSEqcSource() const {
	MutexLock lock(_mutex);
	return _channelData.src;
}

int StreamProperties::getPolarization() const {
	MutexLock lock(_mutex);
	return _channelData.pol_v;
}

uint32_t StreamProperties::getFrequency() const {
	MutexLock lock(_mutex);
	return _channelData.freq;
}

bool StreamProperties::isPIDUsed(int pid) const {
	MutexLock lock(_mutex);
	return _channelData._pidTable.isPIDUsed(pid);
}

uint32_t StreamProperties::getPacketCounter(int pid) const {
	MutexLock lock(_mutex);
	return _channelData._pidTable.getPacketCounter(pid);
}

int StreamProperties::getDMXFileDescriptor(int pid) const {
	MutexLock lock(_mutex);
	return _channelData._pidTable.getDMXFileDescriptor(pid);
}

bool StreamProperties::hasPIDTableChanged() const {
	MutexLock lock(_mutex);
	return _channelData._pidTable.hasPIDTableChanged();
}

std::string StreamProperties::getPidCSV() const {
	MutexLock lock(_mutex);
	return _channelData._pidTable.getPidCSV();
}

int StreamProperties::getModulationType() const {
	MutexLock lock(_mutex);
	return _channelData.modtype;
}

int StreamProperties::getSymbolRate() const {
	MutexLock lock(_mutex);
	return _channelData.srate;
}

int StreamProperties::getFEC() const {
	MutexLock lock(_mutex);
	return _channelData.fec;
}

int StreamProperties::getRollOff() const {
	MutexLock lock(_mutex);
	return _channelData.rolloff;
}

int StreamProperties::getPilotTones() const {
	MutexLock lock(_mutex);
	return _channelData.pilot;
}

int StreamProperties::getSpectralInversion() const {
	MutexLock lock(_mutex);
	return _channelData.inversion;
}

int StreamProperties::getBandwidthHz() const {
	MutexLock lock(_mutex);
	return _channelData.bandwidthHz;
}

int StreamProperties::getTransmissionMode() const {
	MutexLock lock(_mutex);
	return _channelData.transmission;
}

int StreamProperties::getHierarchy() const {
	MutexLock lock(_mutex);
	return _channelData.hierarchy;
}

int StreamProperties::getGuardInverval() const {
	MutexLock lock(_mutex);
	return _channelData.guard;
}

int StreamProperties::getUniqueIDPlp() const {
	MutexLock lock(_mutex);
	return _channelData.plp_id;
}



