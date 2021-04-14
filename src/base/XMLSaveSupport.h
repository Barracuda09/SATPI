/* XMLSaveSupport.h

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef BASE_XML_SAVE_SUPPORT_H_INCLUDE
#define BASE_XML_SAVE_SUPPORT_H_INCLUDE BASE_XML_SAVE_SUPPORT_H_INCLUDE

#include <string>

namespace base {

/// The class @c XMLSaveSupport has some basic functions to handle XML files
class XMLSaveSupport {
		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:

		explicit XMLSaveSupport(const std::string &filePath = "");
		virtual ~XMLSaveSupport();

		// =====================================================================
		// -- Other member functions -------------------------------------------
		// =====================================================================
	public:

		virtual bool notifyChanges() const {
			return saveXML();
		}

		virtual bool saveXML() const = 0;

	protected:

		/// Get the file name for this XML
		std::string getFileName() const;

		/// Save XML file
		bool saveXML(const std::string &xml) const;

		/// Loads XML file
		bool restoreXML(std::string &xml);

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================

	private:

		std::string _filePath;
};

} // namespace base

#endif // BASE_XML_SAVE_SUPPORT_H_INCLUDE
