/* TSReader.cpp

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
#include <input/file/TSReader.h>

#include <Log.h>
#include <Utils.h>
#include <Stream.h>
#include <StringConverter.h>
#include <mpegts/PacketBuffer.h>

#include <thread>

namespace input {
namespace file {

	TSReader::TSReader(int streamID) :
		_streamID(streamID),
		_filePath("None") {}

	TSReader::~TSReader() {}

	// =======================================================================
	//  -- Static member functions -------------------------------------------
	// =======================================================================

	void TSReader::enumerate(StreamVector &streamVector, const std::string &path) {
		SI_LOG_INFO("Setting up TS Reader using path: %s", path.c_str());
		const StreamVector::size_type size = streamVector.size();
		input::file::SpTSReader tsreader = std::make_shared<input::file::TSReader>(size);
		streamVector.push_back(std::make_shared<Stream>(size, tsreader, nullptr));
	}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void TSReader::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		StringConverter::addFormattedString(xml, "<frontendname>%s</frontendname>", "TS Reader");
		StringConverter::addFormattedString(xml, "<pathname>%s</pathname>", _filePath.c_str());
	}

	void TSReader::fromXML(const std::string &UNUSED(xml)) {
		base::MutexLock lock(_mutex);
	}

	// =======================================================================
	//  -- input::Device -----------------------------------------------------
	// =======================================================================

	void TSReader::addDeliverySystemCount(
		std::size_t &dvbs2,
		std::size_t &dvbt,
		std::size_t &dvbt2,
		std::size_t &dvbc,
		std::size_t &dvbc2) {
		dvbs2 += 0;
		dvbt  += 0;
		dvbt2 += 0;
		dvbc  += 0;
		dvbc2 += 0;
	}

	bool TSReader::isDataAvailable() {
		if (_pcrDelta != 0) {
			_t2 = _t1;
			_t1 = std::chrono::steady_clock::now();

			const long interval = _pcrDelta - std::chrono::duration_cast<std::chrono::microseconds>(_t1 - _t2).count();
			if (interval > 0) {
				std::this_thread::sleep_for(std::chrono::microseconds(interval));
			}
			_t1 = std::chrono::steady_clock::now();
			_pcrDelta = 0;
		} else {
			std::this_thread::sleep_for(std::chrono::microseconds(1000));
		}
		return true;
	}

	bool TSReader::readFullTSPacket(mpegts::PacketBuffer &buffer) {
		if (_file.is_open()) {
			const auto size = buffer.getAmountOfBytesToWrite();
			_file.read(reinterpret_cast<char *>(buffer.getWriteBufferPtr()), size);
			buffer.addAmountOfBytesWritten(size);
			buffer.trySyncing();

///////////////////////////////////////////////////////////////////////////////
// @todo Should be shared with decrypt/dvbapi/Client.cpp
			if (buffer.full()) {
				const std::size_t size = buffer.getNumberOfTSPackets();
				for (std::size_t i = 0; i < size; ++i) {
					// Get TS packet from the buffer
					unsigned char *data = buffer.getTSPacketPtr(i);

					// Check is this the beginning of the TS and no Transport error indicator
					if (data[0] == 0x47 && (data[1] & 0x80) != 0x80) {
						// get PID from TS
						const int pid = ((data[1] & 0x1f) << 8) | data[2];

						if (pid == 0 && !_pat.isCollected()) {
							// collect PAT data
							_pat.collectData(_streamID, PAT_TABLE_ID, data);

							// Did we finish collecting PAT
							if (_pat.isCollected()) {
								const unsigned char *cData = _pat.getData();

								// Parse PAT Data
								const int sectionLength = ((cData[ 6] & 0x0F) << 8) | cData[ 7];
								const int tid           =  (cData[ 8] << 8) | cData[ 9];
								const int version       =   cData[10];
								const int secNr         =   cData[11];
								const int lastSecNr     =   cData[12];
								const uint32_t crc      =  CRC(cData, sectionLength);

								const uint32_t calccrc = mpegts::TableData::calculateCRC32(&cData[5], sectionLength - 4 + 3);
								if (calccrc == crc) {
									SI_LOG_INFO("Stream: %d, PAT: Section Length: %d  TID: %d  Version: %d  secNr: %d lastSecNr: %d  CRC: %04X",
												_streamID, sectionLength, tid, version, secNr, lastSecNr, crc);

									// 4 = CRC  6 = PAT Table begin from section length
									const int len = sectionLength - 4 - 6;

									// skip to Table begin and iterate over entries
									const unsigned char *ptr = &cData[13];
									for (int i = 0; i < len; i += 4) {
										// Get PAT entry
										const uint16_t prognr =  (ptr[i + 0] << 8) | ptr[i + 1];
										const uint16_t pid    = ((ptr[i + 2] & 0x1F) << 8) | ptr[i + 3];
										if (prognr == 0) {
											SI_LOG_INFO("Stream: %d, PAT: Prog NR: %d  NIT PID: %04d", _streamID, prognr, pid);
										} else {
											SI_LOG_INFO("Stream: %d, PAT: Prog NR: %d  PMT PID: %04d", _streamID, prognr, pid);
											_pmtPIDS[pid] = true;
										}
									}
								} else {
									SI_LOG_ERROR("Stream: %d, PAT: CRC Error! Calc CRC32: %04X - Msg CRC32: %04X  Retrying to collect data...", _streamID, calccrc, crc);
									_pat.setCollected(false);
								}
							}
						} else if (_pmtPIDS[pid] && !_pmt.isCollected()) {
							// collect PAT data
							_pmt.collectData(_streamID, PMT_TABLE_ID, data);

							// Did we finish collecting PAT
							if (_pmt.isCollected()) {
								const unsigned char *cData = _pmt.getData();

								// Parse PMT Data
								const int sectionLength = ((cData[ 6] & 0x0F) << 8) | cData[ 7];
								const int programNumber = ((cData[ 8]       ) << 8) | cData[ 9];
								const int version       =   cData[10];
								const int secNr         =   cData[11];
								const int lastSecNr     =   cData[12];
								const int pcrPID        = ((cData[13] & 0x1F) << 8) | cData[14];
								const int prgLength     = ((cData[15] & 0x0F) << 8) | cData[16];
								const uint32_t crc      =  CRC(cData, sectionLength);

								const uint32_t calccrc = mpegts::TableData::calculateCRC32(&cData[5], sectionLength - 4 + 3);
								if (calccrc == crc) {
									_pcrPID = pcrPID;
									SI_LOG_INFO("Stream: %d, PMT - Section Length: %d  Prog NR: %d  Version: %d  secNr: %d  lastSecNr: %d  PCR-PID: %04d  Program Length: %d  CRC: %04X",
												_streamID, sectionLength, programNumber, version, secNr, lastSecNr, pcrPID, prgLength, crc);
									SI_LOG_INFO("Stream: %d, PMT Found: %04d with PCR: %04d", _streamID, pid, _pcrPID);
								} else {
									SI_LOG_ERROR("Stream: %d, PMT: CRC Error! Calc CRC32: %04X - Msg CRC32: %04X  Retrying to collect data...", _streamID, calccrc, crc);
									_pmt.setCollected(false);
								}
							}
						} else if (pid == _pcrPID && (data[3] & 0x20) == 0x20 && (data[5] & 0x10) == 0x10) {
							// Check for 'adaptation field flag' and 'PCR field present'

							//        4           3          2          1          0
							// 76543210 98765432 10987654 32109876 54321098 76543210
							// xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xRRRRRRx xxxxxxxx
							// 21098765 43210987 65432109 87654321 0      8 76543210
							//   3          2          1           0               0
							//  b6       b7       b8       b9       b10      b11
							// PCR Base runs with 90KHz and PCR Ext runs with 27MHz
							#define PCR_BASE(DATA) static_cast<uint64_t>(DATA[6] << 24 | DATA[7] << 16 | DATA[8] << 8 | DATA[9]) << 1
							#define PCR_EXT(DATA)  static_cast<uint64_t>((DATA[10] & 0x01 << 8) | DATA[11])

							uint64_t pcrCur = PCR_BASE(data);

							_pcrDelta = pcrCur - _pcrPrev;
							_pcrPrev = pcrCur;
							if (_pcrDelta < 0) {
								_pcrDelta = 1;
							}
							if (_pcrDelta > 75000) {
								_pcrDelta = 75000;
							}
							_pcrDelta *= 11;  // 11 -> PCR_Base runs with 90KHz
							#undef PCR_BASE
							#undef PCR_EXT
						}
					}
				}
			}
///////////////////////////////////////////////////////////////////////////////
			return buffer.full();
		}
		return false;
	}

	bool TSReader::capableOf(const input::InputSystem system) const {
		return system == input::InputSystem::FILE;
	}

	bool TSReader::capableToTranslate(const std::string &UNUSED(msg),
			const std::string &UNUSED(method)) const {
		return false;
	}

	void TSReader::monitorSignal(bool UNUSED(showStatus)) {}

	bool TSReader::hasDeviceDataChanged() const {
		return false;
	}

	void TSReader::parseStreamString(const std::string &msg, const std::string &method) {
		std::string file;
		if (StringConverter::getStringParameter(msg, method, "uri=", file) == true) {
			if (!_file.is_open()) {
				_filePath = file;
				_file.open(_filePath, std::ifstream::binary | std::ifstream::in);
				if (_file.is_open()) {
///////////////////////////////////////////////////////////////////////////////
					_pcrPID = -1;
					_pcrPrev = 0;
					_pcrDelta = 0;
					_pmt.setCollected(false);
					_pat.setCollected(false);
					_t1 = std::chrono::steady_clock::now();
					_t2 = _t1;
					for (size_t i = 0; i < 8193; ++i) {
						_pmtPIDS[i] = false;
					}
///////////////////////////////////////////////////////////////////////////////
					SI_LOG_INFO("Stream: %d, TS Reader using path: %s", _streamID, _filePath.c_str());
				} else {
					SI_LOG_ERROR("Stream: %d, TS Reader unable to open path: %s", _streamID, _filePath.c_str());
				}
			}
		}
	}

	bool TSReader::update() {
		return true;
	}

	bool TSReader::teardown() {
		_file.close();
		_filePath = "None";
		return true;
	}

	std::string TSReader::attributeDescribeString() const {
		std::string desc;
		if (_file.is_open()) {
			// ver=1.5;tuner=<feID>;file=<file>
			StringConverter::addFormattedString(desc, "ver=1.5;tuner=%d;file=%s",
					_streamID + 1, _filePath.c_str());
		} else {
			desc = "";
		}
		return desc;
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

} // namespace file
} // namespace input
