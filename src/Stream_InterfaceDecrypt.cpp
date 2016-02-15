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

#include <input/dvb/Frontend.h>
#include <input/dvb/FrontendData.h>

int Stream::getBatchCount() const {
	return _frontendData->getBatchCount();
}

int Stream::getBatchParity() const {
	return _frontendData->getBatchParity();
}

int Stream::getMaximumBatchSize() const {
	return _frontendData->getMaximumBatchSize();
}

void Stream::decryptBatch(bool final) {
	return _frontendData->decryptBatch(final);
}

void Stream::setBatchData(unsigned char *ptr, int len, int parity, unsigned char *originalPtr) {
	_frontendData->setBatchData(ptr, len, parity, originalPtr);
}

const dvbcsa_bs_key_s *Stream::getKey(int parity) const {
	return _frontendData->getKey(parity);
}

bool Stream::isTableCollected(int tableID) const {
	return _frontendData->isTableCollected(tableID);
}

void Stream::setTableCollected(int tableID, bool collected) {
	_frontendData->setTableCollected(tableID, collected);
}

const unsigned char *Stream::getTableData(int tableID) const {
	return _frontendData->getTableData(tableID);
}

void Stream::collectTableData(int streamID, int tableID, const unsigned char *data) {
	_frontendData->collectTableData(streamID, tableID, data);
}

int  Stream::getTableDataSize(int tableID) const {
	return _frontendData->getTableDataSize(tableID);
}

void Stream::setPMT(int pid, bool set) {
	_frontendData->setPMT(pid, set);
}

bool Stream::isPMT(int pid) const {
	return _frontendData->isPMT(pid);
}

void Stream::setECMFilterData(int demux, int filter, int pid, bool set) {
	_frontendData->setECMFilterData(demux, filter, pid, set);
}

void Stream::getECMFilterData(int &demux, int &filter, int pid) const {
	_frontendData->getECMFilterData(demux, filter, pid);
}

bool Stream::getActiveECMFilterData(int &demux, int &filter, int &pid) const {
	return _frontendData->getActiveECMFilterData(demux, filter, pid);
}

bool Stream::isECM(int pid) const {
	return _frontendData->isECM(pid);
}

void Stream::setKey(const unsigned char *cw, int parity, int index) {
	_frontendData->setKey(cw, parity, index);
}

void Stream::freeKeys() {
	_frontendData->freeKeys();
}

void Stream::setKeyParity(int pid, int parity) {
	_frontendData->setKeyParity(pid, parity);
}

int Stream::getKeyParity(int pid) const {
	return _frontendData->getKeyParity(pid);
}

void Stream::setECMInfo(int pid, int serviceID, int caID, int provID, int emcTime,
	const std::string &cardSystem, const std::string &readerName,
	const std::string &sourceName, const std::string &protocolName,
	int hops) {
	_frontendData->setECMInfo(pid, serviceID, caID, provID, emcTime,
		cardSystem, readerName, sourceName, protocolName, hops);
}

bool Stream::updateInputDevice() {
	base::MutexLock lock(_mutex);
	if (_frontendData->hasPIDTableChanged()) {
		if (_device->isTuned()) {
			_device->update();
		} else {
			SI_LOG_INFO("Stream: %d, Updating PID filters requested but frontend not tuned!",
						_streamID);
		}
	}
	return true;
}
