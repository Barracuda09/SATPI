/* FrontendData.h

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
#ifndef INPUT_DVB_FRONTEND_DATA_H_INCLUDE
#define INPUT_DVB_FRONTEND_DATA_H_INCLUDE INPUT_DVB_FRONTEND_DATA_H_INCLUDE

#include <input/DeviceData.h>
#include <input/dvb/delivery/Lnb.h>

#include <cstdint>
#include <string>

namespace input::dvb {

/// The class @c FrontendData carries all the data/information for tuning a frontend
class FrontendData :
	public DeviceData {
		// =========================================================================
		// -- Static Data members (Multistream) ------------------------------------
		// =========================================================================
	public:
		static constexpr int NO_STREAM_ID = NO_STREAM_ID_FILTER;
		static constexpr int DEFAULT_GOLD_CODE = 0;
		static constexpr int DEFAULT_ROOT_CODE = 1;
		enum class PlsMode {
			Root, Gold, Combo, Unknown
		};

		// =========================================================================
		// -- Constructors and destructor ------------------------------------------
		// =========================================================================
	public:

		FrontendData();

		virtual ~FrontendData() = default;

		FrontendData(const FrontendData&) = delete;

		FrontendData& operator=(const FrontendData&) = delete;

		// =========================================================================
		// -- input::DeviceData ----------------------------------------------------
		// =========================================================================
	private:

		/// @see DeviceData
		virtual void doNextAddToXML(std::string &xml) const final;

		/// @see DeviceData
		virtual void doNextFromXML(const std::string &xml) final;

		/// @see DeviceData
		virtual void doInitialize() final;

		/// @see DeviceData
		virtual void doParseStreamString(FeID id, const TransportParamVector& params) final;

		/// @see DeviceData
		virtual std::string doAttributeDescribeString(FeID id) const final;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		/// Get the frequency in Mhz
		uint32_t getFrequency() const;

		///
		int getSymbolRate() const;

		/// Get modulation type
		int getModulationType() const;

		/// Set modulation type
		void setModulationType(int modtype);

		///
		int getRollOff() const;

		///
		int getFEC() const;

		///
		int getPilotTones() const;

		/// Get the LNB polarizaion
		input::dvb::delivery::Lnb::Polarization getPolarization() const;

		/// Get the LNB polarizaion as char
		char getPolarizationChar() const;

		/// Get the DiSEqc source
		int getDiSEqcSource() const;

		int getSpectralInversion() const;

		int getBandwidthHz() const;

		int getTransmissionMode() const;

		int getGuardInverval() const;

		int getHierarchy() const;

		int getUniqueIDPlp() const;

		int getUniqueIDT2() const;

		int getSISOMISO() const;

		int getDataSlice() const;

		int getC2TuningFrequencyType() const;

		int getInputStreamIdentifier() const;

		PlsMode getPhysicalLayerSignallingMode() const;

		int getPhysicalLayerSignallingCode() const;

	private:

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

		uint32_t _freq;          /// frequency in MHZ
		int _modtype;            /// modulation type i.e. (QPSK/PSK_8)
		int _srate;              /// symbol rate in kSymb/s
		int _fec;                /// forward error control i.e. (FEC_1_2 / FEC_2_3)
		int _rolloff;            /// roll-off
		int _inversion;          ///

		// =========================================================================
		// -- DVB-S(2) Data members ------------------------------------------------
		// =========================================================================
		int _pilot;              /// pilot tones (on/off)
		int _src;                /// Source (1-4) => DiSEqC switch position (0-3)
		input::dvb::delivery::Lnb::Polarization _pol;
		PlsMode _plsMode;        /// Physical Layer Signalling Mode
		int _isId;               /// Input Stream Identifier
		int _plsCode;            /// Physical Layer Signalling Code

		// =========================================================================
		// -- DVB-C2 Data members --------------------------------------------------
		// =========================================================================
		int _c2tft;
		int _data_slice;

		// =========================================================================
		// -- DVB-T(2) Data members ------------------------------------------------
		// =========================================================================
		int _transmission;
		int _guard;
		int _hierarchy;
		uint32_t _bandwidthHz;
		int _plpId;
		int _t2SystemId;
		int _siso_miso;

};

}

#endif // INPUT_DVB_FRONTEND_DATA_H_INCLUDE
