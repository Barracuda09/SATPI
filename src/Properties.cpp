/* Properties.cpp

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
#include "Properties.h"
#include "Log.h"
#include "Utils.h"
#include "Configure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

extern const char *satpi_version;

Properties::Properties(const std::string &uuid, const std::string &delsysString, const std::string &startPath) :
		_delsysString(delsysString),
		_uuid(uuid),
		_versionString(satpi_version),
		_startPath(startPath),
		_bootID(1),
		_deviceID(1),
		_ssdpAnnounceTimeSec(60),
		_appStartTime(time(NULL)) {;}

Properties::~Properties() {;}

void Properties::fromXML(const std::string &xml) {
	
	std::string element;
	if (findXMLElement(xml, "data.configdata.input1.value", element)) {
		_ssdpAnnounceTimeSec = atoi(element.c_str());
		SI_LOG_INFO("Setting SSDP annouce interval to: %d Sec", _ssdpAnnounceTimeSec);
	}
	if (findXMLElement(xml, "data.configdata.input1.value", element)) {
	}
}

void Properties::addToXML(std::string &xml) const {
	// application data
	xml += "<configdata>\r\n";

	ADD_CONFIG_NUMBER(xml, "input1", _ssdpAnnounceTimeSec, 0, 1800);
	
	ADD_CONFIG_IP(xml, "OSCamIP", "127.0.0.1");
	ADD_CONFIG_NUMBER(xml, "OSCamPORT", 15011, 0, 65535);

	xml += "</configdata>\r\n";
}
