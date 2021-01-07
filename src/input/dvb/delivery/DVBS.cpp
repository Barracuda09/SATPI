/* DVBS.cpp

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
#include <input/dvb/delivery/DVBS.h>

#include <Log.h>
#include <StringConverter.h>
#include <Utils.h>
#include <input/dvb/FrontendData.h>
#include <input/dvb/delivery/DiSEqcSwitch.h>
#include <input/dvb/delivery/DiSEqcEN50494.h>
#include <input/dvb/delivery/DiSEqcEN50607.h>
#include <input/dvb/delivery/Lnb.h>

#include <base/Tokenizer.h>

#include <cstdlib>
#include <fstream>

#include <fcntl.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <dirent.h>

namespace input {
namespace dvb {
namespace delivery {

	// =========================================================================
	//  -- Constructors and destructor -----------------------------------------
	// =========================================================================

	DVBS::DVBS(int streamID, const std::string &fePath) :
		input::dvb::delivery::System(streamID, fePath),
		_diseqcType(DiseqcType::Switch),
		_diseqc(new DiSEqcSwitch) {

		// Only DVB-S2 have special handling for FBC tuners
		_fbcSetID = readProcData(streamID, "fbc_set_id");
		_fbcTuner = _fbcSetID >= 0;
		_fbcConnect = 0;
		_fbcLinked = false;
		_fbcRoot = false;
		_sendDiSEqcViaRootTuner = false;
		if (_fbcTuner) {
			readConnectionChoices(streamID);
			_fbcRoot = _choices.find(streamID) != _choices.end();
			_fbcConnect = _fbcRoot ? streamID : 0;
		}
	}

	DVBS::~DVBS() {}

	// =========================================================================
	//  -- base::XMLSupport ----------------------------------------------------
	// =========================================================================

	void DVBS::doAddToXML(std::string &xml) const {

		if (_fbcTuner) {
			if (_fbcRoot) {
				ADD_XML_ELEMENT(xml, "type", "DVB-S(2) FBC-Root");
			} else {
				ADD_XML_ELEMENT(xml, "type", "DVB-S(2) FBC");
			}
			ADD_XML_BEGIN_ELEMENT(xml, "fbc");
				ADD_XML_BEGIN_ELEMENT(xml, "fbcConnection");
					ADD_XML_ELEMENT(xml, "inputtype", "selectionlist");
					ADD_XML_ELEMENT(xml, "value", _fbcConnect);
					ADD_XML_BEGIN_ELEMENT(xml, "list");
					for (const auto &choice : _choices) {
						ADD_XML_ELEMENT(xml, "option" + choice.first, choice.second);
					}
					ADD_XML_END_ELEMENT(xml, "list");
				ADD_XML_END_ELEMENT(xml, "fbcConnection");
				ADD_XML_CHECKBOX(xml, "fbcLinked", (_fbcLinked ? "true" : "false"));
				ADD_XML_CHECKBOX(xml, "sendDiSEqcViaRootTuner", (_sendDiSEqcViaRootTuner ? "true" : "false"));
			ADD_XML_END_ELEMENT(xml, "fbc");
		} else {
			ADD_XML_ELEMENT(xml, "type", "DVB-S(2)");
		}

		ADD_XML_BEGIN_ELEMENT(xml, "diseqcType");
			ADD_XML_ELEMENT(xml, "inputtype", "selectionlist");
			ADD_XML_ELEMENT(xml, "value", asInteger(_diseqcType));
			ADD_XML_BEGIN_ELEMENT(xml, "list");
			ADD_XML_ELEMENT(xml, "option0", "DiSEqc Switch");
			ADD_XML_ELEMENT(xml, "option1", "Unicable (EN50494)");
			ADD_XML_ELEMENT(xml, "option2", "Jess/Unicable 2 (EN50607)");
//			ADD_XML_ELEMENT(xml, "option3", "None");
			ADD_XML_END_ELEMENT(xml, "list");
		ADD_XML_END_ELEMENT(xml, "diseqcType");

		ADD_XML_ELEMENT(xml, "diseqc", _diseqc->toXML());
	}

	void DVBS::doFromXML(const std::string &xml) {
		std::string element;
		if (findXMLElement(xml, "diseqcType.value", element)) {
			const DiseqcType type = static_cast<DiseqcType>(std::stoi(element));
			switch (type) {
				case DiseqcType::Switch:
					_diseqcType = DiseqcType::Switch;
					_diseqc.reset(new DiSEqcSwitch);
					break;
				case DiseqcType::EN50494:
					_diseqcType = DiseqcType::EN50494;
					_diseqc.reset(new DiSEqcEN50494);
					break;
				case DiseqcType::EN50607:
					_diseqcType = DiseqcType::EN50607;
					_diseqc.reset(new DiSEqcEN50607);
					break;
				case DiseqcType::None:
					_diseqcType = DiseqcType::None;
					_diseqc.reset(nullptr);
					break;
				default:
					SI_LOG_ERROR("Stream: %d, Wrong DiSEqc type requested, not changing", _streamID);
			}
		}
		// This after creating _diseqc!
		if (findXMLElement(xml, "diseqc", element)) {
			_diseqc->fromXML(element);
		}

		if (findXMLElement(xml, "fbcLinked.value", element)) {
			_fbcLinked = (element == "true") ? true : false;
			writeProcData(_streamID, "fbc_link", _fbcLinked ? 1 : 0);
		}
		if (findXMLElement(xml, "fbcConnection.value", element)) {
			_fbcConnect = std::stoi(element);
			writeProcData(_streamID, "fbc_connect", _fbcConnect);
		}
		if (findXMLElement(xml, "sendDiSEqcViaRootTuner.value", element)) {
			_sendDiSEqcViaRootTuner =  (element == "true") ? true : false;
		}
	}

	// =========================================================================
	// -- input::dvb::delivery::System -----------------------------------------
	// =========================================================================

	bool DVBS::tune(const int feFD, const input::dvb::FrontendData &frontendData) {
		SI_LOG_INFO("Stream: %d, Start tuning process for DVB-S(2)...", _streamID);

		// DiSEqC switch position differs from src and adjust to MAX_LNB
		const int src = (frontendData.getDiSEqcSource() - 1) % DiSEqc::MAX_LNB;
		const Lnb::Polarization pol = frontendData.getPolarization();
		uint32_t freq = frontendData.getFrequency();

		std::string fePathDiseqc(_fePath);
		int feFDDiseqc = feFD;
		if (_fbcTuner && _fbcLinked && _sendDiSEqcViaRootTuner) {
			// Replace Frontend number with Root Frontend Number
			fePathDiseqc.replace(fePathDiseqc.end()-1, fePathDiseqc.end(), std::to_string(_fbcConnect));
			feFDDiseqc = ::open(fePathDiseqc.c_str(), O_RDWR | O_NONBLOCK);
			if (feFDDiseqc  < 0) {
				// Probably already open, try to find it and duplicate fd
				dirent **fileList;
				const std::string procSelfFD("/proc/self/fd");
				const int n = scandir(procSelfFD.c_str(), &fileList, nullptr, versionsort);
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
						const auto s = readlink(procSelfFDNr.c_str(), buf, sizeof(buf));
						if (s > 0) {
							buf[s] = 0;
							if (fePathDiseqc == buf) {
								// Found it so duplicate and stop;
								feFDDiseqc = ::dup(fd);
								break;
							}
						}
					}
					free(fileList);
				}
			}
		}
		SI_LOG_INFO("Stream: %d, Opened %s for Writing DiSEqC command with fd: %d", _streamID, fePathDiseqc.c_str(), feFDDiseqc);

		// send diseqc
		if (!_diseqc || !_diseqc->sendDiseqc(feFDDiseqc, _streamID, freq, src, pol)) {
			return false;
		}

		if (_fbcTuner && _fbcLinked && _sendDiSEqcViaRootTuner) {
			SI_LOG_INFO("Stream: %d, Closing %s with fd: %d", _streamID, fePathDiseqc.c_str(), feFDDiseqc);
			::close(feFDDiseqc);
		}

		// Now tune by setting properties
		if (!setProperties(feFD, freq, frontendData)) {
			return false;
		}
		return true;
	}

	// =========================================================================
	//  -- Other member functions ----------------------------------------------
	// =========================================================================

	bool DVBS::setProperties(int feFD, const uint32_t freq, const input::dvb::FrontendData &frontendData) {
		struct dtv_property p[15];
		int size = 0;

		SI_LOG_DEBUG("Stream: %d, Set Properties: Frequency %d", _streamID, freq);

		#define FILL_PROP(CMD, DATA) { p[size].cmd = CMD; p[size].u.data = DATA; ++size; }

		FILL_PROP(DTV_CLEAR,           DTV_UNDEFINED);
		FILL_PROP(DTV_DELIVERY_SYSTEM, frontendData.convertDeliverySystem());
		FILL_PROP(DTV_FREQUENCY,       freq);
		FILL_PROP(DTV_MODULATION,      frontendData.getModulationType());
		FILL_PROP(DTV_SYMBOL_RATE,     frontendData.getSymbolRate());
		FILL_PROP(DTV_INNER_FEC,       frontendData.getFEC());
		FILL_PROP(DTV_INVERSION,       INVERSION_AUTO);
		FILL_PROP(DTV_ROLLOFF,         frontendData.getRollOff());
		FILL_PROP(DTV_PILOT,           frontendData.getPilotTones());
		FILL_PROP(DTV_TUNE,            DTV_UNDEFINED);

		#undef FILL_PROP

		struct dtv_properties cmdseq;
		cmdseq.num = size;
		cmdseq.props = p;
		// get all pending events to clear the POLLPRI status
		for (;; ) {
			struct dvb_frontend_event dfe;
			if (ioctl(feFD, FE_GET_EVENT, &dfe) == -1) {
				break;
			}
		}
		// set the tuning properties
		if ((ioctl(feFD, FE_SET_PROPERTY, &cmdseq)) == -1) {
			PERROR("FE_SET_PROPERTY failed");
			return false;
		}
		return true;
	}

	// =========================================================================
	//  -- FBC member functions ------------------------------------------------
	// =========================================================================

	int DVBS::readProcData(int streamID, const std::string &procEntry) const {
		const std::string filePath = StringConverter::stringFormat(
			"/proc/stb/frontend/%1/%2", streamID, procEntry);
		std::ifstream file(filePath);
		if (file.is_open()) {
			int value;
			file >> value;
			return value;
		}
		return -1;
	}

	void DVBS::writeProcData(int streamID, const std::string &procEntry, int value) {
		const std::string filePath = StringConverter::stringFormat(
			"/proc/stb/frontend/%1/%2", streamID, procEntry);
		std::ofstream file(filePath);
		if (file.is_open()) {
			file << value;
		}
	}

	void DVBS::readConnectionChoices(int streamID) {
		// File looks something like: 0=A, 1=B
		const std::string filePath = StringConverter::stringFormat(
			"/proc/stb/frontend/%1/fbc_connect_choices", streamID);
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
			std::string index;
			std::string name;
			if (tokenizerChoice.isNextToken(index) &&
			    tokenizerChoice.isNextToken(name)) {
				_choices[std::stoi(index)] = name;
			}
		}
	}

} // namespace delivery
} // namespace dvb
} // namespace input
