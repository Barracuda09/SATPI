/* DVBS.cpp

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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
#include <Utils.h>
#include <StringConverter.h>
#include <input/dvb/FrontendData.h>
#include <input/dvb/delivery/DiSEqcSwitch.h>
#include <input/dvb/delivery/DiSEqcEN50494.h>
#include <input/dvb/delivery/DiSEqcEN50607.h>

#include <cstdlib>

#include <fcntl.h>
#include <sys/ioctl.h>

namespace input {
namespace dvb {
namespace delivery {

	// =======================================================================
	//  -- Constructors and destructor ---------------------------------------
	// =======================================================================
	DVBS::DVBS(int streamID) :
		input::dvb::delivery::System(streamID),
		_diseqcType(DiseqcType::Switch),
		_diseqc(new DiSEqcSwitch) {}

	DVBS::~DVBS() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void DVBS::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);

		ADD_XML_BEGIN_ELEMENT(xml, "diseqcType");
			ADD_CONFIG_TEXT(xml, "inputtype", "selectionlist");
			ADD_CONFIG_NUMBER(xml, "value", _diseqcType);
			ADD_XML_BEGIN_ELEMENT(xml, "list");
			ADD_CONFIG_TEXT(xml, "option0", "DiSEqc Switch");
			ADD_CONFIG_TEXT(xml, "option1", "Unicable (EN50494)");
			ADD_CONFIG_TEXT(xml, "option2", "Jess/Unicable 2 (EN50607)");
			ADD_XML_END_ELEMENT(xml, "list");
		ADD_XML_END_ELEMENT(xml, "diseqcType");

		ADD_XML_BEGIN_ELEMENT(xml, "diseqc");
		_diseqc->addToXML(xml);
		ADD_XML_END_ELEMENT(xml, "diseqc");
	}

	void DVBS::fromXML(const std::string &xml) {
		base::MutexLock lock(_mutex);
		std::string element;
		if (findXMLElement(xml, "diseqc", element)) {
			_diseqc->fromXML(element);
		}
		if (findXMLElement(xml, "diseqcType.value", element)) {
			if (element.compare("DiSEqc Switch") == 0) {
				_diseqcType = DiseqcType::Switch;
				_diseqc.reset(new DiSEqcSwitch);
			} else if (element.compare("Unicable (EN50494)") == 0) {
				_diseqcType = DiseqcType::EN50494;
				_diseqc.reset(new DiSEqcEN50494);
			} else if (element.compare("Jess/Unicable 2 (EN50607)") == 0) {
				_diseqcType = DiseqcType::EN50607;
				_diseqc.reset(new DiSEqcEN50607);
			}
		}
	}

	// =======================================================================
	// -- input::dvb::delivery::System ---------------------------------------
	// =======================================================================

	bool DVBS::tune(const int feFD, const input::dvb::FrontendData &frontendData) {
		SI_LOG_INFO("Stream: %d, Start tuning process for DVB-S(2)...", _streamID);

		// DiSEqC switch position differs from src
		const int src = frontendData.getDiSEqcSource() - 1;
		const int pol_v = frontendData.getPolarization();
		uint32_t freq = frontendData.getFrequency();

		{
			// send diseqc
			base::MutexLock lock(_mutex);
			if (!_diseqc || !_diseqc->sendDiseqc(feFD, _streamID, freq, src, pol_v)) {
				return false;
			}

		}

		// Now tune by setting properties
		if (!setProperties(feFD, freq, frontendData)) {
			return false;
		}
		return true;
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool DVBS::setProperties(int feFD, const uint32_t freq, const input::dvb::FrontendData &frontendData) {
		struct dtv_property p[15];
		int size = 0;

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

} // namespace delivery
} // namespace dvb
} // namespace input
