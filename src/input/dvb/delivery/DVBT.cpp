/* DVBT.cpp

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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
#include <input/dvb/delivery/DVBT.h>

#include <Log.h>
#include <StringConverter.h>
#include <input/dvb/FrontendData.h>

#include <fcntl.h>
#include <sys/ioctl.h>

namespace input {
namespace dvb {
namespace delivery {

	// =======================================================================
	//  -- Constructors and destructor ---------------------------------------
	// =======================================================================
	DVBT::DVBT(int streamID) :
		input::dvb::delivery::System(streamID),
		_lna(1) {}

	DVBT::~DVBT() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void DVBT::doAddToXML(std::string &xml) const {
		ADD_XML_ELEMENT(xml, "type", "DVB-T(2)");
		ADD_XML_NUMBER_INPUT(xml, "lna", _lna, 0, 5);
	}

	void DVBT::doFromXML(const std::string &xml) {
		std::string element;
		if (findXMLElement(xml, "lna.value", element)) {
			_lna = std::stoi(element);
		}
	}

	// =======================================================================
	// -- input::dvb::delivery::System ---------------------------------------
	// =======================================================================

	bool DVBT::tune(const int feFD, const input::dvb::FrontendData &frontendData) {
		SI_LOG_INFO("Stream: %d, Start tuning process for DVB-T(2)...", _streamID);

		// Now tune by setting properties
		if (!setProperties(feFD, frontendData)) {
			return false;
		}
		return true;
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool DVBT::setProperties(int feFD, const input::dvb::FrontendData &frontendData) {
		struct dtv_property p[15];
		int size = 0;

		#define FILL_PROP(CMD, DATA) { p[size].cmd = CMD; p[size].u.data = DATA; ++size; }

		FILL_PROP(DTV_CLEAR, DTV_UNDEFINED);
		switch (frontendData.getDeliverySystem()) {
			case input::InputSystem::DVBT2:
				FILL_PROP(DTV_STREAM_ID,         frontendData.getUniqueIDPlp());
			// Fall-through
			case input::InputSystem::DVBT:
				FILL_PROP(DTV_DELIVERY_SYSTEM,   frontendData.convertDeliverySystem());
				FILL_PROP(DTV_FREQUENCY,         frontendData.getFrequency() * 1000UL);
				FILL_PROP(DTV_MODULATION,        frontendData.getModulationType());
				FILL_PROP(DTV_INVERSION,         frontendData.getSpectralInversion());
				FILL_PROP(DTV_BANDWIDTH_HZ,      frontendData.getBandwidthHz());
				FILL_PROP(DTV_CODE_RATE_HP,      frontendData.getFEC());
				FILL_PROP(DTV_CODE_RATE_LP,      frontendData.getFEC());
				FILL_PROP(DTV_TRANSMISSION_MODE, frontendData.getTransmissionMode());
				FILL_PROP(DTV_GUARD_INTERVAL,    frontendData.getGuardInverval());
				FILL_PROP(DTV_HIERARCHY,         frontendData.getHierarchy());
#if FULL_DVB_API_VERSION >= 0x0509
				FILL_PROP(DTV_LNA,               _lna);
#endif
				break;
			default:
				return false;
		}
		FILL_PROP(DTV_TUNE, DTV_UNDEFINED);

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
