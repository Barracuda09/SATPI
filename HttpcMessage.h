/* HttpcMessage.h

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
#ifndef HTTPC_MESSAGE_H_INCLUDE
#define HTTPC_MESSAGE_H_INCLUDE

#include <string>

/// Socket attributes
class HttpcMessage {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		HttpcMessage();
		virtual ~HttpcMessage();
		
		///
		void clear() {
			_msg.clear();
			_nextline = 0;
		}
		///
		void add(const std::string &msg) {
			_msg += msg;
		}
		
		///
		bool endOfMessage();
		
		///
		bool isRootFile() const {
			return _msg.find("/ ") != std::string::npos;
		}
		
		///
		bool getRequestedFile(std::string &file) const;

		///
		const char *c_str() const {
			return _msg.c_str();
		}
		
		///
		bool getHeaderFieldParameter(const char *header_field, std::string &parameter) const;
		
		///
		bool getMethod(std::string &method) const;
		
	protected:
		// =======================================================================
		// Data members
		// =======================================================================
		std::string _msg;
		mutable std::string::size_type _nextline;

}; // class HttpcMessage

#endif // HTTPC_MESSAGE_H_INCLUDE