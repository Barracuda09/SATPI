/* XMLSupport.h

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
#ifndef BASE_XML_SUPPORT_H_INCLUDE
#define BASE_XML_SUPPORT_H_INCLUDE BASE_XML_SUPPORT_H_INCLUDE

#include <base/Mutex.h>
#include <StringConverter.h>
#include <Unused.h>

#include <functional>
#include <string>
#include <sstream>

namespace base {

class XMLString : private std::string {
		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:

		XMLString(std::string &str) : std::string(str) {}

		virtual ~XMLString() {}

		// =====================================================================
		// -- Other member functions -------------------------------------------
		// =====================================================================
	public:

		///
		std::string getString() const {
		    return substr(0);
		}
};

/// The class @c XMLSupport has some basic functions to handle XML strings
class XMLSupport {
		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:

		XMLSupport() {}

		virtual ~XMLSupport() {}

		// =====================================================================
		// -- Other static member functions ------------------------------------
		// =====================================================================
	public:

		///
		static std::string makeXMLString(const XMLString &str) {
			return str.getString();
		}

		///
		template <typename Type>
		static std::string makeXMLString(const Type &type) {
			std::ostringstream stream;
			stream.setf(std::ios::fixed);
			stream.precision(4);
			stream << type;
			std::string xml;
			for (const char c : stream.str()) {
				if (c == '&') {
					xml += "&amp;";
				} else if (c == '"') {
					xml += "&quot;";
				} else if (c == '>') {
					xml += "&gt;";
				} else if (c == '<') {
					xml += "&lt;";
				} else {
					xml += c;
				}
			}
			return xml;
		}

		// =====================================================================
		// -- Other member functions -------------------------------------------
		// =====================================================================
	public:

		/// Add data to an XML for storing or web interface
		XMLString toXML() const {
			std::string xml;
			addToXML(xml);
			return XMLString(xml);
		}

		/// Add data to an XML for storing or web interface
		void addToXML(std::string &xml) const {
			base::MutexLock lock(_mutex);
			doAddToXML(xml);
		}

		/// Get data from an XML for restoring or web interface
		void fromXML(const std::string &xml) {
			base::MutexLock lock(_mutex);
			doFromXML(xml);
		}

		using FunctionNotifyChanges = std::function<bool()>;
		void setFunctionNotifyChanges(FunctionNotifyChanges notifyChanges) {
			base::MutexLock lock(_mutex);
			_notifyChanges = notifyChanges;
		}

	private:

		/// Specialization for @see addToXML
		virtual void doAddToXML(std::string &UNUSED(xml)) const {}

		/// Specialization for @see fromXML
		virtual void doFromXML(const std::string &UNUSED(xml)) {}

	protected:

		virtual bool notifyChanges() const;

		///
		bool findXMLElement(const std::string &xml, const std::string &elementToFind,
			std::string &element);

	private:

		/// Very basic/simple recursive XML parser
		bool parseXML(const std::string &xml, const std::string &elementToFind,
			bool &found, std::string &element, std::string::const_iterator &it,
			std::string &tagEnd, std::string::const_iterator &itEndElement);

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		base::Mutex _mutex;
		FunctionNotifyChanges _notifyChanges;
};

} // namespace base

#define ADD_XML_BEGIN_ELEMENT(XML, ELEMENTNAME) \
	XML += StringConverter::stringFormat("<%1>", ELEMENTNAME)

#define ADD_XML_END_ELEMENT(XML, ELEMENTNAME) \
	XML += StringConverter::stringFormat("</%1>", ELEMENTNAME)

#define ADD_XML_ELEMENT(XML, ELEMENTNAME, VALUE) \
	XML += StringConverter::stringFormat("<%1>%2</%1>", ELEMENTNAME, base::XMLSupport::makeXMLString(VALUE))

#define ADD_XML_N_ELEMENT(XML, ELEMENTNAME, N, VALUE) \
	XML += StringConverter::stringFormat("<%1%2>%3</%1%2>", ELEMENTNAME, N, base::XMLSupport::makeXMLString(VALUE))

#define ADD_XML_CHECKBOX(XML, VARNAME, VALUE) \
	XML += StringConverter::stringFormat("<%1><inputtype>checkbox</inputtype><value>%2</value></%1>", VARNAME, base::XMLSupport::makeXMLString(VALUE))

#define ADD_XML_NUMBER_INPUT(XML, VARNAME, VALUE, MIN, MAX) \
	XML += StringConverter::stringFormat("<%1><inputtype>number</inputtype><value>%2</value><minvalue>%3</minvalue><maxvalue>%4</maxvalue></%1>", VARNAME, base::XMLSupport::makeXMLString(VALUE), MIN, MAX)

#define ADD_XML_TEXT_INPUT(XML, VARNAME, VALUE) \
	XML += StringConverter::stringFormat("<%1><inputtype>text</inputtype><value>%2</value></%1>", VARNAME, base::XMLSupport::makeXMLString(VALUE))

#define ADD_XML_IP_INPUT(XML, VARNAME, VALUE) \
	XML += StringConverter::stringFormat("<%1><inputtype>ip</inputtype><value>%2</value></%1>", VARNAME, base::XMLSupport::makeXMLString(VALUE))

#endif // BASE_XML_SUPPORT_H_INCLUDE
