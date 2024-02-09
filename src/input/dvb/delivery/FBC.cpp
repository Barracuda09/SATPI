/* FBC.cpp

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
#include <input/dvb/delivery/FBC.h>

#include <Log.h>
#include <StringConverter.h>
#include <Utils.h>
#include <base/Tokenizer.h>
#include <base/XMLSupport.h>

#include <cmath>
#include <fstream>

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

namespace input::dvb::delivery {

// =============================================================================
//  -- Constructors and destructor ---------------------------------------------
// =============================================================================

FBC::FBC(const FeIndex index, const FeID id, const std::string& name, const bool satTuner) :
		_index(index),
		_id(id),
		_name(name),
		_satTuner(satTuner) {
	_fbcSetID = readProcData(index, "fbc_set_id");
	_fbcTuner = _fbcSetID >= 0;
	_fbcConnect = 0;
	_fbcLinked = false;
	_fbcRoot = false;
	_sendDiSEqcViaRootTuner = false;
	_offset = _fbcSetID * 8;
	_tunerLetter = _index + 'A';
	if (_fbcTuner) {
		// Read 'settings' from system. Add offset to address FBC Slots A and B
		readConnectionChoices(_index, _offset);
		_fbcRoot = _choices.find(_index.getID()) != _choices.end();
		_fbcLinked = readProcData(_index, "fbc_link");
		_fbcConnect = readProcData(_index, "fbc_connect");
	}
	SI_LOG_INFO("Frontend  FBC: Frontend @#1  Root @#2  Index @#3(@#4)  Offset @#5  Connected @#6  Linked @#7",
		_id, _fbcRoot, _index, _tunerLetter, _offset, _fbcConnect, _fbcLinked);
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void FBC::doAddToXML(std::string& xml) const {
	const std::string typeStr = StringConverter::stringFormat("@#1 FBC@#2 (Slot @#3) (Tuner @#4)",
		_name, (_fbcRoot ? "-Root" : ""), ((_fbcSetID == 0) ? "A" : "B"), _tunerLetter);
	ADD_XML_ELEMENT(xml, "type", typeStr);
	if (_fbcTuner) {
		ADD_XML_BEGIN_ELEMENT(xml, "fbc");
			ADD_XML_BEGIN_ELEMENT(xml, "fbcConnection");
				ADD_XML_ELEMENT(xml, "inputtype", "selectionlist");
				ADD_XML_ELEMENT(xml, "value", std::abs(_fbcConnect - _offset));
				ADD_XML_BEGIN_ELEMENT(xml, "list");
				for (const auto& choice : _choices) {
					ADD_XML_ELEMENT(xml, StringConverter::stringFormat("option@#1", choice.first), choice.second);
				}
				ADD_XML_END_ELEMENT(xml, "list");
			ADD_XML_END_ELEMENT(xml, "fbcConnection");
			ADD_XML_ELEMENT(xml, "fbcConnectionRaw", _fbcConnect);
			ADD_XML_ELEMENT(xml, "fbcTunerSlotOffset", _offset);
			ADD_XML_CHECKBOX(xml, "fbcLinked", (_fbcLinked ? "true" : "false"));
			if (_satTuner) {
				ADD_XML_CHECKBOX(xml, "sendDiSEqcViaRootTuner", (_sendDiSEqcViaRootTuner ? "true" : "false"));
			}
		ADD_XML_END_ELEMENT(xml, "fbc");
	}
}

void FBC::doFromXML(const std::string &xml) {
	if (_fbcTuner) {
		std::string element;
		if (findXMLElement(xml, "fbcLinked.value", element)) {
			_fbcLinked = (element == "true") ? true : false;
			writeProcData(_index, "fbc_link", _fbcLinked ? 1 : 0);
		}
		if (findXMLElement(xml, "fbcConnection.value", element)) {
			_fbcConnect = std::stoi(element) + _offset;
			writeProcData(_index, "fbc_connect", _fbcConnect);
		}
		if (_satTuner && findXMLElement(xml, "sendDiSEqcViaRootTuner.value", element)) {
			_sendDiSEqcViaRootTuner =  (element == "true") ? true : false;
		}
	}
}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

int FBC::getFileDescriptorOfRootTuner(std::string& fePath) const {
	// Replace Frontend number with Root Frontend Number
	fePath.replace(fePath.end()-1, fePath.end(), std::to_string(_fbcConnect));
	int feFD = ::open(fePath.data(), O_RDWR | O_NONBLOCK);
	if (feFD  < 0) {
		// Probably already open, try to find it and duplicate fd
		dirent **fileList;
		const std::string procSelfFD("/proc/self/fd");
		const int n = scandir(procSelfFD.data(), &fileList, nullptr, versionsort);
		if (n > 0) {
			for (int i = 0; i < n; ++i) {
				// Check do we have a digit
				if (std::isdigit(fileList[i]->d_name[0]) == 0) {
					continue;
				}
				// Get the link the fd points to
				const int fd = std::atoi(fileList[i]->d_name);
				const std::string procSelfFDNr = procSelfFD + "/" + std::to_string(fd);
				char buf[255];
				const auto s = readlink(procSelfFDNr.data(), buf, sizeof(buf));
				if (s > 0) {
					buf[s] = 0;
					if (fePath == buf) {
						// Found it so duplicate and stop;
						feFD = ::dup(fd);
						break;
					}
				}
			}
			free(fileList);
		}
	}
	return feFD;
}

int FBC::readProcData(const FeIndex index, const std::string &procEntry) const {
	const std::string filePath = StringConverter::stringFormat(
		"/proc/stb/frontend/@#1/@#2", index.getID(), procEntry);
	std::ifstream file(filePath);
	if (file.is_open()) {
		int value;
		file >> value;
		return value;
	}
	return -1;
}

void FBC::writeProcData(const FeIndex index, const std::string &procEntry, int value) {
	const std::string filePath = StringConverter::stringFormat(
		"/proc/stb/frontend/@#1/@#2", index.getID(), procEntry);
	std::ofstream file(filePath);
	if (file.is_open()) {
		file << value;
	}
}

void FBC::readConnectionChoices(const FeIndex index, int offset) {
	// File looks something like: 0=A, 1=B
	const std::string filePath = StringConverter::stringFormat(
		"/proc/stb/frontend/@#1/fbc_connect_choices", index.getID());
	std::ifstream file(filePath);
	if (!file.is_open()) {
		return;
	}
	while(1) {
		std::string choice;
		file >> choice;
		if (choice.empty()) {
			break;
		}
		// Remove any unnecessary chars from choice
		const std::string::size_type pos = choice.find(',');
		if (pos != std::string::npos) {
			choice.erase(pos);
		}
		base::StringTokenizer tokenizerChoice(choice, "=");
		std::string idx;
		std::string name;
		if (tokenizerChoice.isNextToken(idx) && tokenizerChoice.isNextToken(name)) {
			_choices[std::stoi(idx) + offset] = name;
		}
	}
}

}
