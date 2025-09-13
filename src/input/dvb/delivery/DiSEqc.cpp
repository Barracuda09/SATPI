/* DiSEqc.cpp

   Copyright (C) 2014 - 2026 Marc Postema (mpostema09 -at- gmail.com)

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
#include <input/dvb/delivery/DiSEqc.h>

#include <Log.h>
#include <StringConverter.h>

#include <cmath>
#include <string>

#include <fcntl.h>
#include <sys/ioctl.h>

namespace input::dvb::delivery {

	// ===========================================================================
	//  -- base::XMLSupport ------------------------------------------------------
	// ===========================================================================

	void DiSEqc::doAddToXML(std::string &xml) const {
		ADD_XML_NUMBER_INPUT(xml, "diseqc_repeat", _diseqcRepeat, 0, 10);
		ADD_XML_NUMBER_INPUT(xml, "delayBeforeWrite", _delayBeforeWrite, 10, 200);
		ADD_XML_NUMBER_INPUT(xml, "delayAfterWrite", _delayAfterWrite, 15, 180);
		doNextAddToXML(xml);
	}

	void DiSEqc::doFromXML(const std::string &xml) {
		std::string element;
		if (findXMLElement(xml, "diseqc_repeat.value", element)) {
			_diseqcRepeat = std::stoi(element);
		}
		if (findXMLElement(xml, "delayBeforeWrite.value", element)) {
			_delayBeforeWrite = std::stoi(element);
		}
		if (findXMLElement(xml, "delayAfterWrite.value", element)) {
			_delayAfterWrite = std::stoi(element);
		}
		doNextFromXML(xml);
	}

	// ===========================================================================
	//  -- Other member functions ------------------------------------------------
	// ===========================================================================

	void DiSEqc::turnOffLNBPower(int feFD) const {
		if (::ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_OFF) == -1) {
			SI_LOG_PERROR("FE_SET_VOLTAGE failed to switch off");
		}
	}

	void DiSEqc::enableHigherLnbVoltage(int feFD, bool higherVoltage) const {
		if (::ioctl(feFD, FE_ENABLE_HIGH_LNB_VOLTAGE, higherVoltage ? 1 : 0) == -1) {
			SI_LOG_PERROR("FE_ENABLE_HIGH_LNB_VOLTAGE failed to switch off");
		}
	}

	void DiSEqc::sendDiseqcResetCommand(int feFD, FeID id) {
		dvb_diseqc_master_cmd cmd = {{0xe0, 0x00, 0x00}, 3};

		SI_LOG_INFO("Frontend: @#1, Sending DiSEqC: [@#2] [@#3] [@#4] - DiSEqC 'Reset'",
			id, HEX(cmd.msg[0], 2), HEX(cmd.msg[1], 2), HEX(cmd.msg[2], 2));

		sendDiseqcMasterCommand(feFD, id, cmd, MiniDiSEqCSwitch::DoNotSend, 0);

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	void DiSEqc::sendDiseqcPeripherialPowerOnCommand(int feFD, FeID id) {
		dvb_diseqc_master_cmd cmd = {{0xe0, 0x00, 0x03}, 3};

		SI_LOG_INFO("Frontend: @#1, Sending DiSEqC: [@#2] [@#3] [@#4] - DiSEqC 'Peripherial Power On'",
			id, HEX(cmd.msg[0], 2), HEX(cmd.msg[1], 2), HEX(cmd.msg[2], 2));

		sendDiseqcMasterCommand(feFD, id, cmd, MiniDiSEqCSwitch::DoNotSend, 0);

		std::this_thread::sleep_for(std::chrono::milliseconds(250));
	}

	bool DiSEqc::sendDiseqcMasterCommand(int feFD, FeID id, dvb_diseqc_master_cmd &cmd,
			MiniDiSEqCSwitch sw, unsigned int repeatCmd) {
		while (1) {
			if (::ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_18) == -1) {
				SI_LOG_PERROR("FE_SET_VOLTAGE failed to 18V");
			}
			if (::ioctl(feFD, FE_SET_TONE, SEC_TONE_OFF) == -1) {
				SI_LOG_PERROR("FE_SET_TONE failed");
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(_delayBeforeWrite));
			if (::ioctl(feFD, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1) {
				SI_LOG_PERROR("FE_DISEQC_SEND_MASTER_CMD failed");
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(_delayAfterWrite));
			if (sw != MiniDiSEqCSwitch::DoNotSend) {
				fe_sec_mini_cmd_t sasbBurst = (sw == MiniDiSEqCSwitch::MiniA) ? SEC_MINI_A : SEC_MINI_B;
				if (::ioctl(feFD, FE_DISEQC_SEND_BURST, sasbBurst) == -1) {
					SI_LOG_PERROR("FE_DISEQC_SEND_BURST failed");
					return false;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}

			if (ioctl(feFD, FE_SET_VOLTAGE, SEC_VOLTAGE_13) == -1) {
				SI_LOG_PERROR("FE_SET_VOLTAGE failed to 13V");
			}
			// Should we repeat message
			if (repeatCmd > 0) {
				--repeatCmd;
				SI_LOG_INFO("Frontend: @#1, Sending DiSEqC: Command Repeated", id);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				// Framing 0xe1: Command from master, no reply required, repeated transmission
				cmd.msg[0] = 0xe1;
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
				break;
			}
		}
		return true;
	}

}
