/* TableData.h

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
#ifndef MPEGTS_TABLE_DATA_H_INCLUDE
#define MPEGTS_TABLE_DATA_H_INCLUDE MPEGTS_TABLE_DATA_H_INCLUDE

#include <string>
#include <map>

namespace mpegts {

	using TSData = std::basic_string<unsigned char>;

	class TableData {
		public:
			// ================================================================
			// -- Forward declaration -----------------------------------------
			// ================================================================
			struct Data;

			// ================================================================
			// -- Defines -----------------------------------------------------
			// ================================================================
			#define CRC(data, sectionLength) \
				(data[sectionLength - 4 + 8] << 24) |     \
				(data[sectionLength - 4 + 8 + 1] << 16) | \
				(data[sectionLength - 4 + 8 + 2] <<  8) | \
				 data[sectionLength - 4 + 8 + 3]

			#define PAT_TABLE_ID           0x00
			#define CAT_TABLE_ID           0x01
			#define PMT_TABLE_ID           0x02
			#define SDT_TABLE_ID           0x42
			#define EIT1_TABLE_ID          0x4E
			#define EIT2_TABLE_ID          0x4F
			#define ECM0_TABLE_ID          0x80
			#define ECM1_TABLE_ID          0x81
			#define EMM1_TABLE_ID          0x82
			#define EMM2_TABLE_ID          0x83
			#define EMM3_TABLE_ID          0x84

			// ================================================================
			// -- Constructors and destructor ---------------------------------
			// ================================================================

			TableData();

			virtual ~TableData();

			// ================================================================
			//  -- Static member functions ------------------------------------
			// ================================================================

		public:

			static uint32_t calculateCRC32(const unsigned char *data, std::size_t len);

			// ================================================================
			//  -- Other member functions -------------------------------------
			// ================================================================

		public:

			///
			virtual void clear();

			/// Get an copy of the requested table data
			/// @param secNr
			/// @param data
			bool getDataForSectionNumber(size_t secNr, TableData::Data &data) const;

			/// Collect Table data for tableID
			void collectData(int streamID, int tableID, const unsigned char *data, bool raw);

			/// Get the collected Table Data
			TSData getData(size_t secNr) const;

			/// Check if Table is collected
			bool isCollected() const;

		protected:

			/// Add Table data that was collected
			bool addData(int tableID, const unsigned char *data, int length, int pid, int cc);

			/// Set if the current Table has been collected
			void setCollected();

			///
			const char* getTableTXT(int tableID) const;

			// ================================================================
			//  -- Data members -----------------------------------------------
			// ================================================================

		public:

			struct Data {
				int tableID;
				std::size_t sectionLength;
				int version;
				int secNr;
				int lastSecNr;
				uint32_t crc;
				TSData data;
				int cc;
				int pid;
				bool collected;
			};

		protected:

			std::size_t _numberOfSections;

		private:

			std::size_t _currentSectionNumber;
			std::map<int, Data> _dataTable;
			mutable bool _collectingFinished;

	};

} // namespace mpegts

#endif // MPEGTS_TABLE_DATA_H_INCLUDE
