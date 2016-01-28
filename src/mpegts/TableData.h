/* TableData.h

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
#ifndef MPEGTS_TABLE_DATA_H_INCLUDE
#define MPEGTS_TABLE_DATA_H_INCLUDE MPEGTS_TABLE_DATA_H_INCLUDE

namespace mpegts {

class TableData {
#define PAT_TABLE_ID           0x00
#define PMT_TABLE_ID           0x02

	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		TableData() : _cc(-1), _pid(-1), _collected(false) {}
		virtual ~TableData() {}

		/// Add Table data that was collected
		bool addData(int tableID, const unsigned char *data, int length, int pid, int cc) {
			_tableID = tableID;
			if ((_cc  == -1 ||  cc == _cc + 1) &&
				(_pid == -1 || pid == _pid)) {
				if (_cc == -1 && _pid == -1) {
					_data.clear();
				}
				_data.append(reinterpret_cast<const char *>(data), length);
				_cc   = cc;
				_pid  = pid;
				return true;
			}
			return false;
		}

		/// Get the collected Table Data
		const unsigned char *getData() const {
			return reinterpret_cast<const unsigned char *>(_data.c_str());
		}

		/// Get the current size of the Table Packet
		int getDataSize() const {
			return _data.size();
		}

		/// Set if Table has been collected or not
		void setCollected(bool collected) {
			_collected = collected;
			if (!collected) {
				_cc  = -1;
				_pid = -1;
			}
		}

		/// Check if Table is collected
		bool isCollected() const {
			return _collected;
		}

	private:
		int _tableID;
		std::string _data;
		int _cc;
		int _pid;
		bool _collected;
};

} // namespace mpegts

#endif // MPEGTS_TABLE_DATA_H_INCLUDE
