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
		_fbc(index, id, "DVB-S(2)", true),
		_diseqcType(DiseqcType::Lnb),
		_diseqc(new DiSEqcLnb),
		_turnoffLnbVoltage(false),
		_higherLnbVoltage(false) {
	}

	// =========================================================================
	//  -- base::XMLSupport ----------------------------------------------------
	// =========================================================================

	void DVBS::doAddToXML(std::string &xml) const {
		if (_fbc.isFBCTuner()) {
			_fbc.addToXML(xml);
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
		if (_fbc.isFBCTuner()) {
			_fbc.fromXML(xml);
		}
	}

	// =========================================================================
	// -- input::dvb::delivery::System -----------------------------------------
	// =========================================================================

	bool DVBS::tune(const int feFD, const input::dvb::FrontendData &frontendData) {
		SI_LOG_INFO("Frontend: @#1, Start tuning process for DVB-S(2)...", _feID);

		std::string fePathDiseqc(_fePath);
		int feFDDiseqc = feFD;
		if (_fbc.doSendDiSEqcViaRootTuner()) {
			feFDDiseqc = _fbc.getFileDescriptorOfRootTuner(fePathDiseqc);
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

		if (_fbc.doSendDiSEqcViaRootTuner()) {
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

}
