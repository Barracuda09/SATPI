/* XMLSupport.h

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
#ifndef XML_SUPPORT_H_INCLUDE
#define XML_SUPPORT_H_INCLUDE XML_SUPPORT_H_INCLUDE

#include <string>

/// The class @c XMLSupport has some basic functions to handle XML strings
class XMLSupport {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		XMLSupport();
		virtual ~XMLSupport();

		/// Add data to an XML for storing or web interface
		virtual void addToXML(std::string &xml) const = 0;

		/// Get data from an XML for restoring or web interface
		virtual void fromXML(const std::string &xml) = 0;

	protected:
		///
		bool findXMLElement(const std::string &xml, const std::string &elementToFind, std::string &element);

		///
		std::string makeXMLString(const std::string &msg);

	private:

		/// Very basic/simple XML parser
		bool parseXML(const std::string &xml, const std::string &elementToFind, bool &found, std::string &element,
			std::string::const_iterator &it, std::string &tagEnd, std::string::const_iterator &itEndElement);

}; // class XMLSupport

#endif // XML_SUPPORT_H_INCLUDE
