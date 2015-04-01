/* Streams.cpp

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
#include "Streams.h"
#include "Stream.h"
#include "Log.h"
#include "Utils.h"
#include "Frontend.h"
#include "StreamClient.h"
#include "SocketClient.h"
#include "StringConverter.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DMX       "/dev/dvb/adapter%d/demux%d"
#define DVR       "/dev/dvb/adapter%d/dvr%d"
#define FRONTEND  "/dev/dvb/adapter%d/frontend%d"

#define FE_PATH_LEN 255

Streams::Streams() :
		_stream(NULL),
		_maxStreams(0) {;}

Streams::~Streams() {
	delete[] _stream;
}

int Streams::getAttachedFrontendCount(const std::string &path, int count) {
#if SIMU
	UNUSED(path)
	count = 1;
	if (_maxStreams && _stream) {
		char fe_path[FE_PATH_LEN];
		char dvr_path[FE_PATH_LEN];
		char dmx_path[FE_PATH_LEN];
		snprintf(fe_path,  FE_PATH_LEN, FRONTEND, 0, 0);
		snprintf(dvr_path, FE_PATH_LEN, DVR, 0, 0);
		snprintf(dmx_path, FE_PATH_LEN, DMX, 0, 0);
		_stream[0].addFrontendPaths(fe_path, dvr_path, dmx_path);
	}
#else
	struct dirent **file_list;
	const int n = scandir(path.c_str(), &file_list, NULL, alphasort);
	if (n > 0) {
		int i;
		for (i = 0; i < n; ++i) {
			char full_path[FE_PATH_LEN];
			snprintf(full_path, FE_PATH_LEN, "%s/%s", path.c_str(), file_list[i]->d_name);
			struct stat stat_buf;
			if (stat(full_path, &stat_buf) == 0) {
				switch (stat_buf.st_mode & S_IFMT) {
					case S_IFCHR: // character device
						if (strstr(file_list[i]->d_name, "frontend") != NULL) {
							// check if we have an array we can fill in
							if (_maxStreams && _stream) {
								int fe_nr;
								sscanf(file_list[i]->d_name, "frontend%d", &fe_nr);
								int adapt_nr;
								sscanf(path.c_str(), "/dev/dvb/adapter%d", &adapt_nr);

								// make paths and 'save' them
								char fe_path[FE_PATH_LEN];
								char dvr_path[FE_PATH_LEN];
								char dmx_path[FE_PATH_LEN];
								snprintf(fe_path,  FE_PATH_LEN, FRONTEND, adapt_nr, fe_nr);
								snprintf(dvr_path, FE_PATH_LEN, DVR, adapt_nr, fe_nr);
								snprintf(dmx_path, FE_PATH_LEN, DMX, adapt_nr, fe_nr);
								_stream[count].addFrontendPaths(fe_path, dvr_path, dmx_path);
							}
							++count;
						}
					    break;
					case S_IFDIR:
						// do not use dir '.' an '..'
						if (strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
							count = getAttachedFrontendCount(full_path, count);
						}
						break;
				}
			}
			free(file_list[i]);
		}
	}
#endif
	return count;
}

int Streams::enumerateFrontends(const std::string &path) {
#ifdef NOT_PREFERRED_DVB_API
	SI_LOG_DEBUG("Not the preferred DVB API version, for correct function it should be 5.5 or higher");
#endif
	SI_LOG_DEBUG("Current DVB_API_VERSION: %d.%d", DVB_API_VERSION, DVB_API_VERSION_MINOR);

	SI_LOG_INFO("Detecting frontends in: %s", path.c_str());
	_maxStreams = 0;
	_nr_dvb_s2 = 0;
	_nr_dvb_t = 0;
	_nr_dvb_t2 = 0;
	_nr_dvb_c = 0;
#if FULL_DVB_API_VERSION >= 0x0505
	_nr_dvb_c2 = 0;
#endif
	_maxStreams = getAttachedFrontendCount(path, 0);
	_stream = new Stream[_maxStreams+1];
	if (_stream) {
		for (int i = 0; i < _maxStreams; ++i) {
			_stream[i].setStreamID(i);
		}
	} else {
		_maxStreams = 0;
	}
	SI_LOG_INFO("Frontends found: %zu", _maxStreams);
	if (_maxStreams) {
		getAttachedFrontendCount(path, 0);
		for (size_t i = 0; i < static_cast<size_t>(_maxStreams); ++i) {
			_stream[i].setFrontendInfo();
			int dvb_s2 = 0;
			int dvb_t  = 0;
			int dvb_t2 = 0;
			int dvb_c  = 0;
			//
			for (size_t j = 0; j < _stream[i].getDeliverySystemSize(); j++) {
				const fe_delivery_system_t *del_sys = _stream[i].getDeliverySystem();
				switch (del_sys[j]) {
					// only count DVBS2
					case SYS_DVBS2:
						++dvb_s2;
						break;
					case SYS_DVBT:
						++dvb_t;
						break;
					case SYS_DVBT2:
						++dvb_t2;
						break;
#if FULL_DVB_API_VERSION >= 0x0505
					case SYS_DVBC_ANNEX_A:
					case SYS_DVBC_ANNEX_B:
					case SYS_DVBC_ANNEX_C:
						if (!dvb_c) {
							++dvb_c;
						}
						break;
#else
					case SYS_DVBC_ANNEX_AC:
					case SYS_DVBC_ANNEX_B:
						if (!dvb_c) {
							++dvb_c;
						}
						break;
#endif
					default:
						// Not supported
						break;
				}
			}
			_nr_dvb_s2 += dvb_s2;
			_nr_dvb_t  += dvb_t;
			_nr_dvb_t2 += dvb_t2;
			_nr_dvb_c  += dvb_c;
		}
		// make xml delivery system string
#if FULL_DVB_API_VERSION >= 0x0505
		StringConverter::addFormattedString(_del_sys_str, "DVBS2-%zu,DVBT-%zu,DVBT2-%zu,DVBC-%zu,DVBC2-%zu",
				   _nr_dvb_s2, _nr_dvb_t, _nr_dvb_t2, _nr_dvb_c, _nr_dvb_c2);
#else
		StringConverter::addFormattedString(_del_sys_str, "DVBS2-%zu,DVBT-%zu,DVBT2-%zu,DVBC-%zu",
				   _nr_dvb_s2, _nr_dvb_t, _nr_dvb_t2, _nr_dvb_c);
#endif
	} else {
		_del_sys_str = "Noting Found";
	}
	return _maxStreams;
}

Stream *Streams::findStreamAndClientIDFor(SocketClient &socketClient, int &clientID) {
	// Here we need to find the correct Stream and StreamClient
	assert(_stream);
	const std::string &msg = socketClient.getMessage();
	std::string method;
	StringConverter::getMethod(msg, method);

	int streamID = StringConverter::getIntParameter(msg, method, "stream=");
	const int fe_nr = StringConverter::getIntParameter(msg, method, "fe=");
	if (streamID == -1 && fe_nr >= 1 && fe_nr <= static_cast<int>(_maxStreams)) {
		streamID = fe_nr - 1;
	}

	std::string sessionID;
	bool foundSessionID = StringConverter::getHeaderFieldParameter(msg, "Session:", sessionID);
	bool newSession = false;
	clientID = 0;

	// if no sessionID then..
	if (!foundSessionID) {
		// Does the SocketClient have an sessionID
		if (socketClient.getSessionID().size() > 2) {
			foundSessionID = true;
			sessionID = socketClient.getSessionID();
			SI_LOG_INFO("Found SessionID %s by SocketClient", sessionID.c_str());
		// Do we need to make a new sessionID (only if there are transport parameters
		} else if (StringConverter::hasTransportParameters(socketClient.getMessage())) {
			static unsigned int seedp = 0xBEEF;
			char newSessionID[50];
			snprintf(newSessionID, sizeof(newSessionID), "%010d", rand_r(&seedp) % 0xffffffff);
			sessionID = newSessionID;
			newSession = true;
		// None of the above.. so it is just an outside session give an temporary StreamClient
		} else {
			SI_LOG_INFO("Found message outside session");
			socketClient.setSessionID("-1");
			_dummyStream.copySocketClientAttr(socketClient);
			return &_dummyStream;
		}
	}

	// if no streamID we need to find the streamID
	if (streamID == -1) {
		if (foundSessionID) {
			SI_LOG_INFO("Found StreamID x - SessionID: %s", sessionID.c_str());
			for (streamID = 0; streamID < (int)_maxStreams; ++streamID) {
				if (_stream[streamID].findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
					socketClient.setSessionID(sessionID);
					return &_stream[streamID];
				}
			}
		} else {
			SI_LOG_INFO("Found StreamID x - SessionID x - Creating new SessionID: %s", sessionID.c_str());
			for (streamID = 0; streamID < (int)_maxStreams; ++streamID) {
				if (!_stream[streamID].streamInUse()) {
					if (_stream[streamID].findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
						socketClient.setSessionID(sessionID);
						return &_stream[streamID];
					}
				}
			}
		}
	} else {
// @TODO Check is this the owner of this stream??
		SI_LOG_INFO("Found StreamID %d - SessionID %s", streamID, sessionID.c_str());
		// Did we find the StreamClient? else try to search in other Streams
		if (!_stream[streamID].findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
			SI_LOG_ERROR("SessionID %s not found int StreamID %d", sessionID.c_str(), streamID);
			for (streamID = 0; streamID < (int)_maxStreams; ++streamID) {
				if (_stream[streamID].findClientIDFor(socketClient, newSession, sessionID, method, clientID)) {
					socketClient.setSessionID(sessionID);
					return &_stream[streamID];
				}
			}
		} else {
			socketClient.setSessionID(sessionID);
			return &_stream[streamID];
		}
	}
	// Did not find anything
	SI_LOG_ERROR("Found no Stream/Client of interest!");
	return NULL;
}

void Streams::checkStreamClientsWithTimeout() {
	assert(_stream);
	for (size_t streamID = 0; streamID < static_cast<size_t>(_maxStreams); ++streamID) {
		if (_stream[streamID].streamInUse()) {
			_stream[streamID].checkStreamClientsWithTimeout();
		}
	}
}

void Streams::setDVRBufferSize(int streamID, unsigned long size) {
	assert(_stream);
	if (streamID < _maxStreams) {
		_stream[streamID].setDVRBufferSize(size);
	}
}

unsigned long Streams::getDVRBufferSize(int streamID) const {
	assert(_stream);
	if (streamID < _maxStreams) {
		return _stream[streamID].getDVRBufferSize();
	}
	return 0;
}

std::string Streams::attribute_describe_string(unsigned int stream, bool &active) const {
	assert(_stream);
	return _stream[stream].attribute_describe_string(active);
}

void Streams::make_streams_xml(std::string &xml) const {
	assert(_stream);
	// make data xml
	xml  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
	xml += "<data>\r\n";
	
	// application data
	xml += "<streams>";
	for (size_t i = 0; i < static_cast<size_t>(_maxStreams); ++i) {
		StringConverter::addFormattedString(xml, "<stream>");
		_stream[i].addToXML(xml);
		StringConverter::addFormattedString(xml, "</stream>");
	}
	xml += "</streams>";

	xml += "</data>\r\n";
}

