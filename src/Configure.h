/* Configure.h

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
#ifndef CONFIGURE_H_INCLUDE
#define CONFIGURE_H_INCLUDE

#include "StringConverter.h"

#define ADD_CONFIG_TEXT(XML, VARNAME, VALUE) \
  StringConverter::addFormattedString(XML, "<" VARNAME ">%s</" VARNAME ">", VALUE)

#define ADD_CONFIG_NUMBER(XML, VARNAME, VALUE) \
  StringConverter::addFormattedString(XML, "<" VARNAME ">%d</" VARNAME ">", VALUE)

#define ADD_CONFIG_CHECKBOX(XML, VARNAME, VALUE) \
  StringConverter::addFormattedString(XML, "<" VARNAME "><inputtype>checkbox</inputtype><value>%s</value></" VARNAME ">", VALUE)

#define ADD_CONFIG_NUMBER_INPUT(XML, VARNAME, VALUE, MIN, MAX) \
  StringConverter::addFormattedString(XML, "<" VARNAME "><inputtype>number</inputtype><value>%lu</value><minvalue>%lu</minvalue><maxvalue>%lu</maxvalue></" VARNAME ">", VALUE, MIN, MAX)

#define ADD_CONFIG_TEXT_INPUT(XML, VARNAME, VALUE) \
  StringConverter::addFormattedString(XML, "<" VARNAME "><inputtype>text</inputtype><value>%s</value></" VARNAME ">", VALUE)

#define ADD_CONFIG_IP_INPUT(XML, VARNAME, VALUE) \
  StringConverter::addFormattedString(XML, "<" VARNAME "><inputtype>ip</inputtype><value>%s</value></" VARNAME ">", VALUE)

#endif // CONFIGURE_H_INCLUDE
