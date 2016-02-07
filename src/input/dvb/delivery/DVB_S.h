/* DVB_S.h

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
#ifndef INPUT_DVB_DELIVERY_DVB_S_H_INCLUDE
#define INPUT_DVB_DELIVERY_DVB_S_H_INCLUDE INPUT_DVB_DELIVERY_DVB_S_H_INCLUDE

#include <FwDecl.h>
#include <dvbfix.h>
#include <input/dvb/delivery/System.h>

#include <stdlib.h>
#include <stdint.h>
#include <string>

FW_DECL_NS0(StreamProperties);

namespace input {
namespace dvb {
namespace delivery {

	enum LNBType {
		Universal,
		Standard
	};

	// slof: switch frequency of LNB
	#define DEFAULT_SWITCH_LOF (11700 * 1000UL)

	// lofLow: local frequency of lower LNB band
	#define DEFAULT_LOF_LOW_UNIVERSAL (9750 * 1000UL)

	// lofHigh: local frequency of upper LNB band
	#define DEFAULT_LOF_HIGH_UNIVERSAL (10600 * 1000UL)

	// Lnb standard Local oscillator frequency
	#define DEFAULT_LOF_STANDARD (10750 * 1000UL)

	// LNB properties
	typedef struct {
		LNBType type;
		uint32_t lofStandard;
		uint32_t switchlof;
		uint32_t lofLow;
		uint32_t lofHigh;
	} Lnb_t;

	// DiSEqc properties
	typedef struct {
		#define MAX_LNB 4
		#define POL_H   0
		#define POL_V   1
		int src;             // Source (1-4) => DiSEqC switch position (0-3)
		int pol_v;           // polarisation (1 = vertical/circular right, 0 = horizontal/circular left)
		int hiband;          //
		Lnb_t LNB[MAX_LNB];  // LNB properties
	} DiSEqc_t;

	/// The class @c DVB_S specifies DVB-S/S2 delivery system
	class DVB_S :
		public input::dvb::delivery::System {
		public:

			// =======================================================================
			//  -- Constructors and destructor ---------------------------------------
			// =======================================================================
			DVB_S();
			virtual ~DVB_S();

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================

		public:

			///
			virtual void addToXML(std::string &xml) const;

			///
			virtual void fromXML(const std::string &xml);

			// =======================================================================
			// -- input::dvb::delivery::System ---------------------------------------
			// =======================================================================

		public:

			///
			virtual bool tune(int feFD, const StreamProperties &properties);


			// =======================================================================
			// -- Other member functions ---------------------------------------------
			// =======================================================================

		private:
			///
			bool setProperties(int feFD, uint32_t ifreq, const StreamProperties &properties);

			///
			bool diseqcSendMsg(int feFD, fe_sec_voltage_t v, struct diseqc_cmd *cmd,
				fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b, bool repeatDiseqc);

			///
			bool sendDiseqc(int feFD, int streamID, bool repeatDiseqc);

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:

			Lnb_t _lnb[MAX_LNB];  // lnb that can be connected to this frontend
			DiSEqc_t _diseqc;     //

	};

} // namespace delivery
} // namespace dvb
} // namespace input

#endif // INPUT_DVB_DELIVERY_DVB_S_H_INCLUDE
