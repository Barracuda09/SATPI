/* DVBC.cpp

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
#include <input/dvb/delivery/DVBC.h>

#include <Log.h>
#include <StringConverter.h>
#include <Unused.h>
#include <input/dvb/FrontendData.h>


#include <fcntl.h>
#include <sys/ioctl.h>

namespace input::dvb::delivery {

	// =======================================================================
	//  -- Constructors and destructor ---------------------------------------
	// =======================================================================
	DVBC::DVBC(const FeIndex index, const FeID id, const std::string &fePath, unsigned int dvbVersion) :
		input::dvb::delivery::System(index, id, fePath, dvbVersion),
		_fbc(index, id, "DVB-C(2)", false) {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void DVBC::doAddToXML(std::string &xml) const {
		if (_fbc.isFBCTuner()) {
			_fbc.addToXML(xml);
		} else {
			ADD_XML_ELEMENT(xml, "type", "DVB-C(2)");
		}
	}

	void DVBC::doFromXML(const std::string& xml) {
		if (_fbc.isFBCTuner()) {
			_fbc.fromXML(xml);
		}
	}

	// =======================================================================
	// -- input::dvb::delivery::System ---------------------------------------
	// =======================================================================

	bool DVBC::tune(const int feFD, const input::dvb::FrontendData &frontendData) {
		SI_LOG_INFO("Frontend: @#1, Start tuning process for DVB-C(2)...", _feID);

		// Now tune by setting properties
		if (!setProperties(feFD, frontendData)) {
			return false;
		}
		return true;
	}

	bool DVBC::isCapableOf(input::InputSystem system) const {
		return system == input::InputSystem::DVBC;
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool DVBC::setProperties(int feFD, const input::dvb::FrontendData &frontendData) {
		struct dtv_property p[15];
		int size = 0;
		const uint32_t freq = frontendData.getFrequency() * 1000;
		SI_LOG_DEBUG("Frontend: @#1, Set Properties: Frequency @#2", _feID, freq);

		#define FILL_PROP(CMD, DATA) { p[size].cmd = CMD; p[size].u.data = DATA; ++size; }

		FILL_PROP(DTV_CLEAR, DTV_UNDEFINED);
		switch (frontendData.getDeliverySystem()) {
			case input::InputSystem::DVBC:
				FILL_PROP(DTV_BANDWIDTH_HZ,    frontendData.getBandwidthHz());
				FILL_PROP(DTV_DELIVERY_SYSTEM, frontendData.convertDeliverySystem());
				FILL_PROP(DTV_FREQUENCY,       freq);
				FILL_PROP(DTV_INVERSION,       frontendData.getSpectralInversion());
				FILL_PROP(DTV_MODULATION,      frontendData.getModulationType());
				FILL_PROP(DTV_SYMBOL_RATE,     frontendData.getSymbolRate());
				FILL_PROP(DTV_INNER_FEC,       frontendData.getFEC());
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
			SI_LOG_PERROR("FE_SET_PROPERTY failed");
			return false;
		}
		return true;
	}

}
