/* DiSEqcSwitch.cpp

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
#include <input/dvb/delivery/DiSEqcSwitch.h>

#include <Log.h>
#include <StringConverter.h>
#include <input/dvb/FrontendData.h>

#include <chrono>
#include <thread>

#include <fcntl.h>
#include <sys/ioctl.h>

namespace input::dvb::delivery {

	// =======================================================================
	// -- input::dvb::delivery::DiSEqc ---------------------------------------
	// =======================================================================

	bool DiSEqcSwitch::sendDiseqc(const int feFD, const FeID id, uint32_t &freq,
			const int src, const Lnb::Polarization pol) {
		bool hiband = false;
		_lnb.getIntermediateFrequency(freq, hiband, pol);

		// Framing 0xe0: Command from Master, No reply required, First transmission
		// -------------------------------------------------------------------------
		// Address 0x10: Any LNB, Switcher or SMATV (Master to all...)
		// Address 0x11: LNB
		// Address 0x12: LNB with Loop-through switching
		// Address 0x14: Switcher (d.c. blocking)
		// Address 0x15: Switcher with d.c. Loop-through
		// -------------------------------------------------------------------------
		// Command 0x38: Write to Port group 0 (Committed switches)
		// Command 0x39: Write to Port group 1 (Uncommitted switches)
		// -------------------------------------------------------------------------
		// Data 1  0xf0: see below
		// Data 2  0x00: not used
		// Data 3  0x00: not used
		// -------------------------------------------------------------------------
		// size    0x04: send x bytes
		dvb_diseqc_master_cmd cmd = {{0xe0, _addressByte, _commandByte, 0xf0}, 4};
		switch (_addressByte) {
			default:
				cmd.msg[1] = 0x10;
				cmd.msg[2] = 0x38;
				[[fallthrough]];
			case 0x10:
				switch (_switchType) {
					default:
						// default to committed switch
					case SwitchType::COMMITTED: {
						// high nibble: reset bits
						//  low nibble:   set bits  (option, position, polarizaion, band)
						cmd.msg[3] |= (src << 2) & 0x0f;
						cmd.msg[3] |= pol == Lnb::Polarization::Horizontal ? 0x2 : 0x0;
						cmd.msg[3] |= hiband ? 0x1 : 0x0;
						const auto minisw = (src % 2) ? MiniDiSEqCSwitch::MiniB : MiniDiSEqCSwitch::MiniA;
						sendDiseqcCommand(feFD, id, cmd, minisw, src, _diseqcRepeat);
						break;
					}
					case SwitchType::UNCOMMITTED: {
						cmd.msg[3] |= src & 0x0f;
						const auto minisw = (src % 2) ? MiniDiSEqCSwitch::MiniB : MiniDiSEqCSwitch::MiniA;
						sendDiseqcCommand(feFD, id, cmd, minisw, src, _diseqcRepeat);
						break;
					}
					case SwitchType::CASCADE: {
						const int srcCommitted = src & 0x03;
						const int srcUncommitted = (src >> 2) & 0x0F;
						const bool uncommittedFirst = (src & 0x40) == 0x40;
						const auto minisw = ((src & 0x80) == 0x80) ? MiniDiSEqCSwitch::MiniB : MiniDiSEqCSwitch::MiniA;
						if (uncommittedFirst) {
							cmd.msg[2] = 0x39;
							cmd.msg[3] = 0xf0 | srcUncommitted;
							sendDiseqcCommand(feFD, id, cmd, MiniDiSEqCSwitch::DoNotSend, src, 0);
							cmd.msg[2] = 0x38;
							cmd.msg[3] = 0xf0 | srcCommitted;
							sendDiseqcCommand(feFD, id, cmd, minisw, src, 0);
						} else {
							cmd.msg[2] = 0x38;
							cmd.msg[3] = 0xf0 | srcCommitted;
							sendDiseqcCommand(feFD, id, cmd, MiniDiSEqCSwitch::DoNotSend, src, 0);
							cmd.msg[2] = 0x39;
							cmd.msg[3] = 0xf0 | srcUncommitted;
							sendDiseqcCommand(feFD, id, cmd, minisw, src, 0);
						}
						break;
					}
				}
				break;
			case 0x14:
			case 0x15:
				cmd.msg[3]  = 0xf0;
				cmd.msg[3] |= src & 0x0f;
				break;
		}

		// Setup LNB
		const auto v = (pol == Lnb::Polarization::Vertical || pol == Lnb::Polarization::CircularRight) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
		if (ioctl(feFD, FE_SET_VOLTAGE, v) == -1) {
			SI_LOG_PERROR("FE_SET_VOLTAGE failed");
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		const auto tone = hiband ? SEC_TONE_ON : SEC_TONE_OFF;
		if (ioctl(feFD, FE_SET_TONE, tone) == -1) {
			SI_LOG_PERROR("FE_SET_TONE failed");
			return false;
		}
		return true;
	}

	void DiSEqcSwitch::doNextAddToXML(std::string &xml) const {
		ADD_XML_BEGIN_ELEMENT(xml, "switchType");
			ADD_XML_ELEMENT(xml, "inputtype", "selectionlist");
			ADD_XML_ELEMENT(xml, "value", asInteger(_switchType));
			ADD_XML_BEGIN_ELEMENT(xml, "list");
			ADD_XML_ELEMENT(xml, "option0", "Committed");
			ADD_XML_ELEMENT(xml, "option1", "Uncommitted");
			ADD_XML_ELEMENT(xml, "option2", "Cascade");
			ADD_XML_END_ELEMENT(xml, "list");
		ADD_XML_END_ELEMENT(xml, "switchType");
		ADD_XML_N_ELEMENT(xml, "lnb", 0, _lnb.toXML());
	}

	void DiSEqcSwitch::doNextFromXML(const std::string &xml) {
		std::string element;
		if (findXMLElement(xml, "switchType.value", element)) {
			const auto type = integerToEnum<SwitchType>(std::stoi(element));
			switch (type) {
				case SwitchType::COMMITTED:
					_switchType = SwitchType::COMMITTED;
					_commandByte = 0x38;
					break;
				case SwitchType::UNCOMMITTED:
					_switchType = SwitchType::UNCOMMITTED;
					_commandByte = 0x39;
					break;
				case SwitchType::CASCADE:
					_switchType = SwitchType::CASCADE;
					_commandByte = 0x39;
					break;
				default:
					SI_LOG_ERROR("Frontend: x, Wrong Switch Type requested, not changing");
			}
		}
		if (findXMLElement(xml, "lnb0", element)) {
			_lnb.fromXML(element);
		}
	}

	// ===========================================================================
	// -- Other member functions -------------------------------------------------
	// ===========================================================================

	bool DiSEqcSwitch::sendDiseqcCommand(int feFD, FeID id, dvb_diseqc_master_cmd &cmd,
			MiniDiSEqCSwitch sw, const int src, unsigned int repeatCmd) {
		SI_LOG_INFO("Frontend: @#1, Sending DiSEqC: [@#2] [@#3] [@#4] [@#5] - DiSEqC Src: @#6",
			id, HEX(cmd.msg[0], 2), HEX(cmd.msg[1], 2), HEX(cmd.msg[2], 2), HEX(cmd.msg[3], 2), src);

		return sendDiseqcMasterCommand(feFD, id, cmd, sw, repeatCmd);
	}

}
