/* SocketAttr.h

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#ifndef SOCKET_ATTR_H_INCLUDE
#define SOCKET_ATTR_H_INCLUDE

#include <netinet/in.h>
#include <string>

/// Socket attributes
class SocketAttr {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		SocketAttr() {;}
		virtual ~SocketAttr() {;}

		// =======================================================================
		// Data members
		// =======================================================================
		struct sockaddr_in _addr;
		int _fd;
	protected:
		std::string _ip_addr;
}; // class SocketAttr

#endif // SOCKET_ATTR_H_INCLUDE