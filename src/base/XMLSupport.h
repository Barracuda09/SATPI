/* XMLSupport.h

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef BASE_XML_SUPPORT_H_INCLUDE
#define BASE_XML_SUPPORT_H_INCLUDE BASE_XML_SUPPORT_H_INCLUDE

#include <base/Mutex.h>
#include <StringConverter.h>

#include <string>
#include <functional>

namespace base {

	/// The class @c XMLSupport has some basic functions to handle XML strings
	class XMLSupport {

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		public:

			XMLSupport() {}

			virtual ~XMLSupport() {}

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================

		public:

			/// Add data to an XML for storing or web interface
			virtual void addToXML(std::string &xml) const = 0;

			/// Get data from an XML for restoring or web interface
			virtual void fromXML(const std::string &xml) = 0;

			typedef std::function<bool()> FunctionNotifyChanges;
            void setFunctionNotifyChanges(FunctionNotifyChanges notifyChanges) {
                _notifyChanges = notifyChanges;
            }

		protected:

            virtual bool notifyChanges() const;

			///
			bool findXMLElement(const std::string &xml, const std::string &elementToFind,
				std::string &element);

		private:

			/// Very basic/simple recursive XML parser
			bool parseXML(const std::string &xml, const std::string &elementToFind, bool &found, std::string &element,
						  std::string::const_iterator &it, std::string &tagEnd, std::string::const_iterator &itEndElement);

			// =======================================================================
			// Data members
			// =======================================================================

		protected:

			base::Mutex _xmlMutex;

		private:

			FunctionNotifyChanges _notifyChanges;
	};

} // namespace base

#define ADD_XML_BEGIN_ELEMENT(XML, ELEMENTNAME) \
	XML += StringConverter::stringFormat("<%1>", ELEMENTNAME)

#define ADD_XML_END_ELEMENT(XML, ELEMENTNAME) \
	XML += StringConverter::stringFormat("</%1>", ELEMENTNAME)

#define ADD_CONFIG_TEXT(XML, VARNAME, VALUE) \
	XML += StringConverter::stringFormat("<%1>%2</%1>", VARNAME, VALUE)

#define ADD_CONFIG_NUMBER(XML, VARNAME, VALUE) \
	XML += StringConverter::stringFormat("<%1>%2</%1>", VARNAME, VALUE)

#define ADD_CONFIG_CHECKBOX(XML, VARNAME, VALUE) \
	XML += StringConverter::stringFormat("<%1><inputtype>checkbox</inputtype><value>%2</value></%1>", VARNAME, VALUE)

#define ADD_CONFIG_NUMBER_INPUT(XML, VARNAME, VALUE, MIN, MAX) \
	XML += StringConverter::stringFormat("<%1><inputtype>number</inputtype><value>%2</value><minvalue>%3</minvalue><maxvalue>%4</maxvalue></%1>", VARNAME, VALUE, MIN, MAX)

#define ADD_CONFIG_TEXT_INPUT(XML, VARNAME, VALUE) \
	XML += StringConverter::stringFormat("<%1><inputtype>text</inputtype><value>%2</value></%1>", VARNAME, VALUE)

#define ADD_CONFIG_IP_INPUT(XML, VARNAME, VALUE) \
	XML += StringConverter::stringFormat("<%1><inputtype>ip</inputtype><value>%2</value></%1>", VARNAME, VALUE)

#endif // BASE_XML_SUPPORT_H_INCLUDE
