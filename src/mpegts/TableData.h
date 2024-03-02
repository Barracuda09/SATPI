/* TableData.h

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
#ifndef MPEGTS_TABLE_DATA_H_INCLUDE
#define MPEGTS_TABLE_DATA_H_INCLUDE MPEGTS_TABLE_DATA_H_INCLUDE

#include <Defs.h>

#include <cstdint>
#include <string>
#include <map>

namespace mpegts {

using TSData = std::basic_string<unsigned char>;

class TableData {
		// =========================================================================
		// -- Forward declaration --------------------------------------------------
		// =========================================================================
	public:

		struct Data;

		// =========================================================================
		// -- Defines --------------------------------------------------------------
		// =========================================================================
		#define CRC(data, sectionLength) \
			(data[sectionLength - 4 + 8] << 24) |     \
			(data[sectionLength - 4 + 8 + 1] << 16) | \
			(data[sectionLength - 4 + 8 + 2] <<  8) | \
			 data[sectionLength - 4 + 8 + 3]

		static constexpr int PAT_ID       = 0x00;
		static constexpr int CAT_ID       = 0x01;
		static constexpr int PMT_ID       = 0x02;
		static constexpr int NIT_ID       = 0x40;
		static constexpr int NIT_OTHER_ID = 0x41;
		static constexpr int SDT_ID       = 0x42;
		static constexpr int EIT1_ID      = 0x4E;
		static constexpr int EIT2_ID      = 0x4F;
		static constexpr int ECM0_ID      = 0x80;
		static constexpr int ECM1_ID      = 0x81;
		static constexpr int EMM1_ID      = 0x82;
		static constexpr int EMM2_ID      = 0x83;
		static constexpr int EMM3_ID      = 0x84;

		// =========================================================================
		// -- Constructors and destructor ------------------------------------------
		// =========================================================================
	public:

		TableData() = default;

		virtual ~TableData() = default;

		// =========================================================================
		//  -- Static member functions ---------------------------------------------
		// =========================================================================
	public:

		static uint32_t calculateCRC32(const unsigned char* data, std::size_t len) noexcept;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		/// Clear the collected table and data
		virtual void clear() noexcept;

		/// Get an copy of the requested table data
		/// @param secNr
		/// @param data
		bool getDataForSectionNumber(size_t secNr, TableData::Data &data) const noexcept;

		/// Collect Table data for tableID
		void collectData(FeID id, int tableID, const unsigned char* data, bool trace) {
			collectData(id, tableID, data, trace, false);
		}

		/// Collect Raw Table data for tableID
		void collectRawData(FeID id, int tableID, const unsigned char* data, bool trace) {
			collectData(id, tableID, data, trace, true);
		}

		/// Get the collected Table Data
		TSData getData(size_t secNr) const;

		/// Check if Table is collected
		bool isCollected() const noexcept {
			if (_collectingFinished) {
				return true;
			}
			return checkAllCollected();
		}

		/// Get the associated PID of this table
		int getAssociatedPID() const;

	protected:

		/// Add Table data that was collected
		bool addData(int tableID, const unsigned char* data, int length, int pid, int cc);

		/// Set if the current Table has been collected
		void setCollected() noexcept;

		///
		const char* getTableTXT(int tableID) const noexcept;

		///
		uint8_t getByte(size_t &i, const unsigned char* buf) const noexcept {
			uint8_t d = buf[i];
			++i;
			return d;
		}

		///
		uint16_t getWord(size_t &i, const unsigned char* buf) const noexcept {
			uint16_t d = (buf[i] << 8) | buf[i + 1];
			i += 2;
			return d;
		}

		uint32_t get24Bits(size_t &i, const unsigned char* buf) const noexcept {
			uint32_t d = (buf[i] << 16) | (buf[i + 1] << 8) | buf[i + 2];
			i += 3;
			return d;
		}

		uint32_t getDWord(size_t &i, const unsigned char* buf) const noexcept {
			uint32_t d = (buf[i] << 24) | (buf[i + 1] << 16) | (buf[i + 2] << 8) | buf[i + 3];
			i += 4;
			return d;
		}

	private:

		/// Collect Table data for tableID
		void collectData(FeID id, int tableID, const unsigned char* data, bool trace, bool raw);

		/// Check if all sections are collected
		bool checkAllCollected() const noexcept;

		// =========================================================================
		//  -- Data members --------------------------------------------------------
		// =========================================================================
	public:

		struct Data {
			int tableID;
			std::size_t sectionLength;
			int version;
			int nextIndicator;
			int secNr;
			int lastSecNr;
			uint32_t crc;
			TSData data;
			int cc;
			int pid;
			bool collected;
		};

	protected:

		std::size_t _numberOfSections = 0;

	private:

		std::size_t _currentSectionNumber = 0;
		mutable bool _collectingFinished = false;
		std::map<int, Data> _dataTable;

};

}

#endif // MPEGTS_TABLE_DATA_H_INCLUDE
