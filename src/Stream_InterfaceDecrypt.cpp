/* Stream_InterfaceDecrypt.cpp

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
#include <Stream.h>

int Stream::getBatchCount() const {
	return _properties.getBatchCount();
}

int Stream::getBatchParity() const {
	return _properties.getBatchParity();
}

int Stream::getMaximumBatchSize() const {
	return _properties.getMaximumBatchSize();
}

void Stream::decryptBatch(bool final) {
	return _properties.decryptBatch(final);
}

void Stream::setBatchData(unsigned char *ptr, int len, int parity, unsigned char *originalPtr) {
	_properties.setBatchData(ptr, len, parity, originalPtr);
}

const dvbcsa_bs_key_s *Stream::getKey(int parity) const {
	return _properties.getKey(parity);
}

bool Stream::isTableCollected(int tableID) const {
	return _properties.isTableCollected(tableID);
}

void Stream::setTableCollected(int tableID, bool collected) {
	_properties.setTableCollected(tableID, collected);
}

const unsigned char *Stream::getTableData(int tableID) const {
	return _properties.getTableData(tableID);
}

void Stream::collectTableData(int streamID, int tableID, const unsigned char *data) {
	_properties.collectTableData(streamID, tableID, data);
}

int  Stream::getTableDataSize(int tableID) const {
	return _properties.getTableDataSize(tableID);
}

void Stream::setPMT(int pid, bool set) {
	_properties.setPMT(pid, set);
}

bool Stream::isPMT(int pid) const {
	return _properties.isPMT(pid);
}

void Stream::setECMFilterData(int demux, int filter, int pid, bool set) {
	_properties.setECMFilterData(demux, filter, pid, set);
}

void Stream::getECMFilterData(int &demux, int &filter, int pid) const {
	_properties.getECMFilterData(demux, filter, pid);
}

bool Stream::getActiveECMFilterData(int &demux, int &filter, int &pid) const {
	return _properties.getActiveECMFilterData(demux, filter, pid);
}

bool Stream::isECM(int pid) const {
	return _properties.isECM(pid);
}

void Stream::setKey(const unsigned char *cw, int parity, int index) {
	_properties.setKey(cw, parity, index);
}

void Stream::freeKeys() {
	_properties.freeKeys();
}

void Stream::setKeyParity(int pid, int parity) {
	_properties.setKeyParity(pid, parity);
}

int Stream::getKeyParity(int pid) const {
	return _properties.getKeyParity(pid);
}

void Stream::setECMInfo(int pid, int serviceID, int caID, int provID, int emcTime,
	const std::string &cardSystem, const std::string &readerName,
	const std::string &sourceName, const std::string &protocolName,
	int hops) {
	_properties.setECMInfo(pid, serviceID, caID, provID, emcTime,
		cardSystem, readerName, sourceName, protocolName, hops);
}

bool Stream::updateInputDevice() {
	base::MutexLock lock(_mutex);

	if (_properties.hasPIDTableChanged()) {
		if (_frontend->isTuned()) {
			_frontend->update(_properties);
		} else {
			SI_LOG_INFO("Stream: %d, Updating PID filters requested but frontend not tuned!",
			            _properties.getStreamID());
		}
	}
	return true;
}
