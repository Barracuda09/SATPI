/* DiSEqc.h

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
#ifndef INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE
#define INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE

#include <Defs.h>
#include <base/XMLSupport.h>
#include <Unused.h>
#include <input/dvb/dvbfix.h>
#include <input/dvb/delivery/Lnb.h>

namespace input::dvb::delivery {

	/// The class @c DiSEqc specifies an interface to an connected DiSEqc device
	class DiSEqc :
		public base::XMLSupport {
			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
		public:

			DiSEqc() = default;

			virtual ~DiSEqc() = default;

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================
		private:

			/// @see XMLSupport
			virtual void doAddToXML(std::string &xml) const final;

			/// @see XMLSupport
			virtual void doFromXML(const std::string &xml) final;

			// =======================================================================
			// -- Other member functions ---------------------------------------------
			// =======================================================================
		public:

			///
			/// @param feFD
			/// @param id
			/// @param freq
			/// @param src specifies the DiSEqc src starting from 0
			/// @param pol
			virtual bool sendDiseqc(int feFD, FeID id, uint32_t &freq,
				int src, Lnb::Polarization pol) = 0;

			/// This will turn off the power to the LNB
			/// @param feFD specifies the file descriptor for the frontend
			virtual void turnOffLNBPower(int feFD) const;

			/// This will enable an slightly higher voltages instead of 13/18V,
			/// in order to compensate for long antenna cables.
			/// @param feFD specifies the file descriptor for the frontend
			/// @param higherVoltage when <code>true</code> the LNB voltage will be slightly higher
			virtual void enableHigherLnbVoltage(int feFD, bool higherVoltage) const;

		protected:

			///
			enum class MiniDiSEqCSwitch {
				DoNotSend,
				MiniA,
				MiniB
			};

			/// This will send the DiSEqc 'Reset' command
			/// @param feFD the file descriptor the command should be send to
			/// @param id specifies which frontend id the command is send to
			void sendDiseqcResetCommand(int feFD, FeID id);

			/// This will send the DiSEqc 'Peripherial Power On' command
			/// @param feFD the file descriptor the command should be send to
			/// @param id specifies which frontend id the command is send to
			void sendDiseqcPeripherialPowerOnCommand(int feFD, FeID id);

			/// This will send the DiSEqc master command
			/// @param feFD the file descriptor the command should be send to
			/// @param id specifies which frontend id the command is send to
			/// @param cmd the command that should be send
			/// @param sasbBurst specifies the SA/SB tone burst
			/// @param repeatCmd the number of times this command should be repeated
			bool sendDiseqcMasterCommand(int feFD, FeID id, dvb_diseqc_master_cmd &cmd,
				MiniDiSEqCSwitch sw, unsigned int repeatCmd);

		private:

			/// Specialization for @see doAddToXML
			virtual void doNextAddToXML(std::string &UNUSED(xml)) const {}

			/// Specialization for @see doFromXML
			virtual void doNextFromXML(const std::string &UNUSED(xml)) {}

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================
		public:

			static constexpr size_t MAX_LNB = 4;

		protected:

			unsigned int _diseqcRepeat = 0;
			unsigned int _delayBeforeWrite = 35;
			unsigned int _delayAfterWrite = 40;
	};

}

#endif // INPUT_DVB_DELIVERY_DISEQC_H_INCLUDE
