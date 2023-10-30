/* DVBS.cpp

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
#include <input/dvb/delivery/DVBS.h>

#include <Log.h>
#include <StringConverter.h>
#include <Utils.h>
#include <base/Tokenizer.h>
#include <input/dvb/FrontendData.h>
#include <input/dvb/delivery/DiSEqcEN50494.h>
#include <input/dvb/delivery/DiSEqcEN50607.h>
#include <input/dvb/delivery/DiSEqcLnb.h>
#include <input/dvb/delivery/DiSEqcSwitch.h>

#include <cstdlib>
#include <fstream>

#include <fcntl.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <dirent.h>

namespace input::dvb::delivery {

	// Copyright (C) 2017 Marcus Metzler <mocm@metzlerbros.de>
	//                    Ralph Metzler <rjkm@metzlerbros.de>
	//
	// https://github.com/DigitalDevices/dddvb/blob/master/apps/pls.c
	static uint32_t root2gold(uint32_t root) {
		uint32_t x = 1;
		uint32_t g = 0;
		for (; g < 0x3ffff; ++g) {
			if (root == x) {
				return g;
			}
			x = (((x ^ (x >> 7)) & 1) << 17) | (x >> 1);
		}
		return 0x3ffff;
	}

	// =========================================================================
	//  -- Constructors and destructor -----------------------------------------
	// =========================================================================

	DVBS::DVBS(const FeIndex index, const FeID id, const std::string &fePath, unsigned int dvbVersion) :
		input::dvb::delivery::System(index, id, fePath, dvbVersion),
		_diseqcType(DiseqcType::Lnb),
		_diseqc(new DiSEqcLnb),
		_turnoffLnbVoltage(false),
		_higherLnbVoltage(false) {

		// Only DVB-S2 have special handling for FBC tuners
		_fbcSetID = readProcData(index, "fbc_set_id");
		_fbcTuner = _fbcSetID >= 0;
		_fbcConnect = 0;
		_fbcLinked = false;
		_fbcRoot = false;
		_sendDiSEqcViaRootTuner = false;
		if (_fbcTuner) {
			// Read 'settings' from system, add offset to address FBC Slots A and B
			readConnectionChoices(index, _fbcSetID * 8);
			_fbcRoot = _choices.find(index.getID()) != _choices.end();
			_fbcLinked = readProcData(index, "fbc_link");
			_fbcConnect = readProcData(index, "fbc_connect");
		}
	}

	// =========================================================================
	//  -- base::XMLSupport ----------------------------------------------------
	// =========================================================================

	void DVBS::doAddToXML(std::string &xml) const {
		if (_fbcTuner) {
			const std::string typeStr = StringConverter::stringFormat("DVB-S(2) FBC@#1 (Slot @#2)",
				(_fbcRoot ? "-Root" : ""), ((_fbcSetID == 0) ? "A" : "B"));
			ADD_XML_ELEMENT(xml, "type", typeStr);
			ADD_XML_BEGIN_ELEMENT(xml, "fbc");
				ADD_XML_BEGIN_ELEMENT(xml, "fbcConnection");
					ADD_XML_ELEMENT(xml, "inputtype", "selectionlist");
					ADD_XML_ELEMENT(xml, "value", _fbcConnect - (_fbcSetID * 8));
					ADD_XML_BEGIN_ELEMENT(xml, "list");
					for (const auto& choice : _choices) {
						ADD_XML_ELEMENT(xml, StringConverter::stringFormat("option@#1", choice.first), choice.second);
					}
					ADD_XML_END_ELEMENT(xml, "list");
				ADD_XML_END_ELEMENT(xml, "fbcConnection");
				ADD_XML_CHECKBOX(xml, "fbcLinked", (_fbcLinked ? "true" : "false"));
				ADD_XML_CHECKBOX(xml, "sendDiSEqcViaRootTuner", (_sendDiSEqcViaRootTuner ? "true" : "false"));
			ADD_XML_END_ELEMENT(xml, "fbc");
		} else {
			ADD_XML_ELEMENT(xml, "type", "DVB-S(2)");
		}

		ADD_XML_CHECKBOX(xml, "turnoffLNBPower", (_turnoffLnbVoltage ? "true" : "false"));
		ADD_XML_CHECKBOX(xml, "higherLnbVoltage", (_higherLnbVoltage ? "true" : "false"));

		ADD_XML_BEGIN_ELEMENT(xml, "diseqcType");
			ADD_XML_ELEMENT(xml, "inputtype", "selectionlist");
			ADD_XML_ELEMENT(xml, "value", asInteger(_diseqcType));
			ADD_XML_BEGIN_ELEMENT(xml, "list");
			ADD_XML_ELEMENT(xml, "option0", "DiSEqc Switch");
			ADD_XML_ELEMENT(xml, "option1", "Unicable (EN50494)");
			ADD_XML_ELEMENT(xml, "option2", "Jess/Unicable 2 (EN50607)");
			ADD_XML_ELEMENT(xml, "option3", "Lnb");
			ADD_XML_END_ELEMENT(xml, "list");
		ADD_XML_END_ELEMENT(xml, "diseqcType");

		if (_diseqc != nullptr) {
			ADD_XML_ELEMENT(xml, "diseqc", _diseqc->toXML());
		}
	}

	void DVBS::doFromXML(const std::string &xml) {
		std::string element;
		if (findXMLElement(xml, "turnoffLNBPower.value", element)) {
			_turnoffLnbVoltage = (element == "true") ? true : false;
		}
		if (findXMLElement(xml, "higherLnbVoltage.value", element)) {
			_higherLnbVoltage = (element == "true") ? true : false;
		}
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
				case DiseqcType::Lnb:
					_diseqcType = DiseqcType::Lnb;
					_diseqc.reset(new DiSEqcLnb);
					break;
				default:
					SI_LOG_ERROR("Frontend: @#1, Wrong DiSEqc type requested, not changing", _feID);
			}
		}
		if (_diseqc != nullptr) {
			// This after creating _diseqc!
			if (findXMLElement(xml, "diseqc", element)) {
				_diseqc->fromXML(element);
			}
		}
		if (findXMLElement(xml, "fbcLinked.value", element)) {
			_fbcLinked = (element == "true") ? true : false;
			writeProcData(_index, "fbc_link", _fbcLinked ? 1 : 0);
		}
		if (findXMLElement(xml, "fbcConnection.value", element)) {
			_fbcConnect = std::stoi(element) + (_fbcSetID * 8);
			writeProcData(_index, "fbc_connect", _fbcConnect);
		}
		if (findXMLElement(xml, "sendDiSEqcViaRootTuner.value", element)) {
			_sendDiSEqcViaRootTuner =  (element == "true") ? true : false;
		}
	}

	// =========================================================================
	// -- input::dvb::delivery::System -----------------------------------------
	// =========================================================================

	bool DVBS::tune(const int feFD, const input::dvb::FrontendData &frontendData) {
		SI_LOG_INFO("Frontend: @#1, Start tuning process for DVB-S(2)...", _feID);

		std::string fePathDiseqc(_fePath);
		int feFDDiseqc = feFD;
		if (_fbcTuner && _fbcLinked && _sendDiSEqcViaRootTuner) {
			// Replace Frontend number with Root Frontend Number
			fePathDiseqc.replace(fePathDiseqc.end()-1, fePathDiseqc.end(), std::to_string(_fbcConnect));
			feFDDiseqc = ::open(fePathDiseqc.data(), O_RDWR | O_NONBLOCK);
			if (feFDDiseqc  < 0) {
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
		SI_LOG_INFO("Frontend: @#1, Opened @#2 for Writing DiSEqC command with fd: @#3",
				_feID, fePathDiseqc, feFDDiseqc);

		_diseqc->enableHigherLnbVoltage(feFDDiseqc, _higherLnbVoltage);

		uint32_t freq = frontendData.getFrequency();
		// send diseqc ('src' differs from 'DiSEqC switch position' so adjust with -1)
		if (_diseqc != nullptr &&
			!_diseqc->sendDiseqc(feFDDiseqc, _feID, freq, frontendData.getDiSEqcSource() - 1, frontendData.getPolarization())) {
			return false;
		}

		if (_fbcTuner && _fbcLinked && _sendDiSEqcViaRootTuner) {
			SI_LOG_INFO("Frontend: @#1, Closing @#2 with fd: @#3", _feID, fePathDiseqc, feFDDiseqc);
			::close(feFDDiseqc);
		}

		// Now tune by setting properties
		if (!setProperties(feFD, freq, frontendData)) {
			return false;
		}
		return true;
	}

	void DVBS::teardown(int feFD) const {
		if (_turnoffLnbVoltage) {
			SI_LOG_INFO("Frontend: @#1, Turning off LNB Power", _feID);
			_diseqc->turnOffLNBPower(feFD);
		}
	}

	// =========================================================================
	//  -- Other member functions ----------------------------------------------
	// =========================================================================

	bool DVBS::setProperties(const int feFD, const uint32_t freq,
			const input::dvb::FrontendData &frontendData) {
		struct dtv_property p[15];
		int size = 0;

		SI_LOG_DEBUG("Frontend: @#1, Set Properties: Frequency @#2", _feID, freq);

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

		auto plsMode = frontendData.getPhysicalLayerSignallingMode();
		int plsCode = frontendData.getPhysicalLayerSignallingCode();
		int isId = frontendData.getInputStreamIdentifier();
		if (isId != FrontendData::NO_STREAM_ID) {
			if (_dvbVersion >= 0x0500 && _dvbVersion <= 0x050A) {
				isId = (asInteger(plsMode) << 26) | (plsCode << 8) | isId;
			}
			SI_LOG_DEBUG("Frontend: @#1, Set Properties: StreamID @#2", _feID, isId);
			FILL_PROP(DTV_STREAM_ID,     isId);
		}

		if (_dvbVersion >= 0x050B &&
				frontendData.getPhysicalLayerSignallingMode() != FrontendData::PlsMode::Unknown) {
			if (plsMode == FrontendData::PlsMode::Root) {
					plsCode = root2gold(plsCode);
			}
			SI_LOG_DEBUG("Frontend: @#1, Set Properties: PLS Code @#2", _feID, plsCode);
			FILL_PROP(DTV_SCRAMBLING_SEQUENCE_INDEX, plsCode);
		}
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
			SI_LOG_PERROR("FE_SET_PROPERTY failed");
			return false;
		}
		return true;
	}

	// =========================================================================
	//  -- FBC member functions ------------------------------------------------
	// =========================================================================

	int DVBS::readProcData(const FeIndex index, const std::string &procEntry) const {
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

	void DVBS::writeProcData(const FeIndex index, const std::string &procEntry, int value) {
		const std::string filePath = StringConverter::stringFormat(
			"/proc/stb/frontend/@#1/@#2", index.getID(), procEntry);
		std::ofstream file(filePath);
		if (file.is_open()) {
			file << value;
		}
	}

	void DVBS::readConnectionChoices(const FeIndex index, int offset) {
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
			if (tokenizerChoice.isNextToken(idx) &&
			    tokenizerChoice.isNextToken(name)) {
				_choices[std::stoi(idx) + offset] = name;
			}
		}
	}

}
