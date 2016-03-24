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

#include <fcntl.h>
#include <sys/ioctl.h>

namespace input {
namespace dvb {
namespace delivery {

	// =======================================================================
	//  -- Constructors and destructor ---------------------------------------
	// =======================================================================
	DVBS::DVBS() :
		_diseqcRepeat(true) {
		for (std::size_t i = 0; i < MAX_LNB; ++i) {
			_diseqc.LNB[i].type        = Universal;
			_diseqc.LNB[i].lofStandard = DEFAULT_LOF_STANDARD;
			_diseqc.LNB[i].switchlof   = DEFAULT_SWITCH_LOF;
			_diseqc.LNB[i].lofLow      = DEFAULT_LOF_LOW_UNIVERSAL;
			_diseqc.LNB[i].lofHigh     = DEFAULT_LOF_HIGH_UNIVERSAL;
		}
	}

	DVBS::~DVBS() {}

	// =======================================================================
	//  -- base::XMLSupport --------------------------------------------------
	// =======================================================================

	void DVBS::addToXML(std::string &xml) const {
		base::MutexLock lock(_mutex);
		for (std::size_t i = 0; i < MAX_LNB; ++i) {
			StringConverter::addFormattedString(xml, "<lnb%zu>", i);
			ADD_CONFIG_NUMBER(xml, "lnbtype", _diseqc.LNB[i].type);
			ADD_CONFIG_NUMBER(xml, "lofStandard", _diseqc.LNB[i].lofStandard);
			ADD_CONFIG_NUMBER(xml, "switchlof", _diseqc.LNB[i].switchlof);
			ADD_CONFIG_NUMBER(xml, "lofLow", _diseqc.LNB[i].lofLow);
			ADD_CONFIG_NUMBER(xml, "lofHigh", _diseqc.LNB[i].lofHigh);
			StringConverter::addFormattedString(xml, "</lnb%zu>", i);
		}
		ADD_CONFIG_CHECKBOX(xml, "diseqc_repeat", (_diseqcRepeat ? "true" : "false"));
	}

	void DVBS::fromXML(const std::string &xml) {
		base::MutexLock lock(_mutex);
		std::string element;
		if (findXMLElement(xml, "diseqc_repeat.value", element)) {
			_diseqcRepeat = (element == "true") ? true : false;
		}
	}

	// =======================================================================
	// -- input::dvb::delivery::System ---------------------------------------
	// =======================================================================

	bool DVBS::tune(int streamID, int feFD, const input::dvb::FrontendData &frontendData) {
		SI_LOG_INFO("Stream: %d, Start tuning process for DVB-S(2)...", streamID);

		uint32_t ifreq = 0;
		// DiSEqC switch position differs from src
		const int src = frontendData.getDiSEqcSource() - 1;
		Lnb_t &lnb = _diseqc.LNB[src];
		_diseqc.src = src;
		_diseqc.pol_v = frontendData.getPolarization();
		_diseqc.hiband = 0;

		const uint32_t freq = frontendData.getFrequency();
		if (lnb.lofHigh) {
			if (lnb.switchlof) {
				// Voltage controlled switch
				if (freq >= lnb.switchlof) {
					_diseqc.hiband = 1;
				}

				if (_diseqc.hiband) {
					ifreq = abs(freq - lnb.lofHigh);
				} else {
					ifreq = abs(freq - lnb.lofLow);
				}
			} else {
				// C-Band Multi-point LNB
				ifreq = abs(freq - (_diseqc.pol_v == POL_V ? lnb.lofLow : lnb.lofHigh));
			}
		} else {
			// Mono-point LNB without switch
			ifreq = abs(freq - lnb.lofLow);
		}
		// send diseqc
		if (!sendDiseqc(feFD, streamID)) {
			return false;
		}

		// Now tune by setting properties
		if (!setProperties(feFD, ifreq, frontendData)) {
			return false;
		}
		return true;
	}

	// =======================================================================
	//  -- Other member functions --------------------------------------------
	// =======================================================================

	bool DVBS::setProperties(int feFD, const uint32_t ifreq, const input::dvb::FrontendData &frontendData) {
		struct dtv_property p[15];
		int size = 0;

		#define FILL_PROP(CMD, DATA) { p[size].cmd = CMD; p[size].u.data = DATA; ++size; }

		FILL_PROP(DTV_CLEAR,           DTV_UNDEFINED);
		FILL_PROP(DTV_DELIVERY_SYSTEM, frontendData.convertDeliverySystem());
		FILL_PROP(DTV_FREQUENCY,       ifreq);
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

	struct diseqc_cmd {
		struct dvb_diseqc_master_cmd cmd;
		uint32_t wait;
	};

	bool DVBS::diseqcSendMsg(int feFD, fe_sec_voltage_t v, struct diseqc_cmd *cmd,
			fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b) {
		if (ioctl(feFD, FE_SET_TONE, SEC_TONE_OFF) == -1) {
			PERROR("FE_SET_TONE failed");
			return false;
		}
		if (ioctl(feFD, FE_SET_VOLTAGE, v) == -1) {
			PERROR("FE_SET_VOLTAGE failed");
			return false;
		}
		usleep(15 * 1000);
		if (ioctl(feFD, FE_DISEQC_SEND_MASTER_CMD, &cmd->cmd) == -1) {
			PERROR("FE_DISEQC_SEND_MASTER_CMD failed");
			return false;
		}
		usleep(cmd->wait * 1000);
		usleep(15 * 1000);
		{
			base::MutexLock lock(_mutex);
			if (_diseqcRepeat) {
				usleep(100 * 1000);
				// Framing 0xe1: Command from Master, No reply required, Repeated transmission
				cmd->cmd.msg[0] = 0xe1;
				if (ioctl(feFD, FE_DISEQC_SEND_MASTER_CMD, &cmd->cmd) == -1) {
					PERROR("Repeated FE_DISEQC_SEND_MASTER_CMD failed");
					return false;
				}
			}
		}
		if (ioctl(feFD, FE_DISEQC_SEND_BURST, b) == -1) {
			PERROR("FE_DISEQC_SEND_BURST failed");
			return false;
		}
		usleep(15 * 1000);
		if (ioctl(feFD, FE_SET_TONE, t) == -1) {
			PERROR("FE_SET_TONE failed");
			return false;
		}
		return true;
	}

	bool DVBS::sendDiseqc(int feFD, int streamID) {
		// Digital Satellite Equipment Control, specification is available from http://www.eutelsat.com/
		struct diseqc_cmd cmd = {
			{ {0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4}, 0
		};

		// Framing 0xe0: Command from Master, No reply required, First transmission(or repeated transmission)
		// Address 0x10: Any LNB, Switcher or SMATV (Master to all...)
		// Command 0x38: Write to Port group 0 (Committed switches)
		// Data 1  0xf0: see below
		// Data 2  0x00: not used
		// Data 3  0x00: not used
		// size    0x04: send x bytes

		// param: high nibble: reset bits, low nibble set bits,
		// bits are: option, position, polarizaion, band
		cmd.cmd.msg[3] =
		  0xf0 | (((_diseqc.src << 2) & 0x0f) | ((_diseqc.pol_v == POL_V) ? 0 : 2) | (_diseqc.hiband ? 1 : 0));

		SI_LOG_INFO("Stream: %d, Sending DiSEqC [%02x] [%02x] [%02x] [%02x]", streamID, cmd.cmd.msg[0],
				cmd.cmd.msg[1], cmd.cmd.msg[2], cmd.cmd.msg[3]);

		return diseqcSendMsg(feFD, (_diseqc.pol_v == POL_V) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18,
				&cmd, _diseqc.hiband ? SEC_TONE_ON : SEC_TONE_OFF, (_diseqc.src % 2) ? SEC_MINI_B : SEC_MINI_A);
	}

} // namespace delivery
} // namespace dvb
} // namespace input
