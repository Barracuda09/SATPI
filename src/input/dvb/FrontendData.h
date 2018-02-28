/* FrontendData.h

   Copyright (C) 2014 - 2018 Marc Postema (mpostema09 -at- gmail.com)

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
#include <mpegts/PidTable.h>

#include <stdint.h>
#include <string>

namespace input {
namespace dvb {

	/// The class @c FrontendData carries all the data/information for tuning a frontend
	class FrontendData :
		public DeviceData {
		public:
			// =======================================================================
			// Constructors and destructor
			// =======================================================================
			FrontendData();

			virtual ~FrontendData();

			FrontendData(const FrontendData&) = delete;

			FrontendData& operator=(const FrontendData&) = delete;

			// =======================================================================
			// -- base::XMLSupport ---------------------------------------------------
			// =======================================================================

		public:
			///
			virtual void addToXML(std::string &xml) const override;

			///
			virtual void fromXML(const std::string &xml) override;

			// =======================================================================
			// -- input::DeviceData --------------------------------------------------
			// =======================================================================

		public:

			///
			virtual void initialize() override;

			///
			virtual void parseStreamString(int streamID, const std::string &msg, const std::string &method) override;

			///
			virtual std::string attributeDescribeString(int streamID) const override;

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		public:

			/// Set DMX file descriptor
			void setDMXFileDescriptor(int pid, int fd);

			/// Get DMX file descriptor
			int getDMXFileDescriptor(int pid) const;

			/// Close DMX file descriptor and reset data, but keep used flag
			void closeDMXFileDescriptor(int pid);

			/// Reset 'PID has changed' flag
			void resetPIDTableChanged();

			/// Check the 'PID has changed' flag
			bool hasPIDTableChanged() const;

			/// Get the amount of packet that were received of this pid
			uint32_t getPacketCounter(int pid) const;

			/// Get the CSV of all the requested PID
			std::string getPidCSV() const;

			/// Set the continuity counter for pid
			void addPIDData(int pid, uint8_t cc);

			/// Set pid used or not
			void setPID(int pid, bool val);

			/// Check if this pid should be closed
			bool shouldPIDClose(int pid) const;

			/// Check if PID is used
			bool isPIDUsed(int pid) const;

			/// Set all PID
			void setAllPID(bool val);

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
			int getPolarization() const;
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

		private:

			///
			void parsePIDString(const std::string &pids, bool clearPidsFirst, bool add);

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		private:

			uint32_t _freq;            /// frequency in MHZ
			int _modtype;              /// modulation type i.e. (QPSK/PSK_8)
			int _srate;                /// symbol rate in kSymb/s
			int _fec;                  /// forward error control i.e. (FEC_1_2 / FEC_2_3)
			int _rolloff;              /// roll-off
			int _inversion;            ///
			mpegts::PidTable _pidTable;///

			// =======================================================================
			// -- DVB-S(2) Data members ----------------------------------------------
			// =======================================================================
			int _pilot;               // pilot tones (on/off)
			int _src;                 // Source (1-4) => DiSEqC switch position (0-3)
			int _pol_v;               // polarisation (1 = vertical/circular right, 0 = horizontal/circular left)

			// =======================================================================
			// -- DVB-C2 Data members ------------------------------------------------
			// =======================================================================
			int _c2tft;               // DVB-C2
			int _data_slice;          // DVB-C2

			// =======================================================================
			// -- DVB-T(2) Data members ----------------------------------------------
			// =======================================================================
			int _transmission;       // DVB-T(2)
			int _guard;              // DVB-T(2)
			int _hierarchy;          // DVB-T(2)
			uint32_t _bandwidthHz;   // DVB-T(2)/C2
			int _plp_id;             // DVB-T2/C2
			int _t2_system_id;       // DVB-T2
			int _siso_miso;          // DVB-T2
	};

} // namespace dvb
} // namespace input

#endif // INPUT_DVB_FRONTEND_DATA_H_INCLUDE
