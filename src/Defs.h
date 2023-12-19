/* Defs.h

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
#ifndef DEFS_H_INCLUDE
#define DEFS_H_INCLUDE DEFS_H_INCLUDE

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using StringVector = std::vector<std::string>;

using PacketPtr = std::unique_ptr<uint8_t[]>;

class TypeID {
	public:
		TypeID(int id) : _id(id) {}
		TypeID &operator=(int id) {
			_id = id;
			return *this;
		}

		int getID() const {
			return _id;
		}

		operator int() const {
			return _id;
		}

		bool operator>=(const int id) const {
			return _id >= id;
		}

		bool operator<=(const int id) const {
			return _id <= id;
		}

		bool operator==(const int id) const {
			return _id == id;
		}

		bool operator!=(const int id) const {
			return _id != id;
		}

		bool operator==(const TypeID& rhs) const {
			return _id == rhs._id;
		}

	private:
		int _id;
};

class FeID : public TypeID {
	public:
		FeID(int id=-1) : TypeID(id) {}
};

class FeIndex : public TypeID {
	public:
		FeIndex(int index=-1) : TypeID(index) {}
};

class StreamID : public TypeID {
	public:
		StreamID(int id=-1) : TypeID(id) {}
};

#endif // DEFS_H_INCLUDE
