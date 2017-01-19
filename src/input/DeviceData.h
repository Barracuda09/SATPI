/* DeviceData.h

   Copyright (C) 2015 - 2017 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef INPUT_DEVICE_DATA_H_INCLUDE
#define INPUT_DEVICE_DATA_H_INCLUDE INPUT_DEVICE_DATA_H_INCLUDE

#include <base/Mutex.h>
#include <input/InputSystem.h>
#include <input/dvb/dvbfix.h>
#include <mpegts/PidTable.h>

namespace input {

	/// The class @c DeviceData. carries all the data/information for a device
	class DeviceData {
		public:
			// =======================================================================
			// -- Constructors and destructor ----------------------------------------
			// =======================================================================
			DeviceData();
			virtual ~DeviceData();

			// =======================================================================
			//  -- Other member functions --------------------------------------------
			// =======================================================================

		public:

			///
			virtual void initialize();

			/// Set DMX file descriptor
			void setDMXFileDescriptor(int pid, int fd);

			/// Get DMX file descriptor
			int getDMXFileDescriptor(int pid) const;

			/// Close DMX file descriptor
			void closeDMXFileDescriptor(int pid);

			///
			void setKeyParity(int pid, int parity);

			///
			int getKeyParity(int pid) const;

			/// Reset the pid data like counters etc. (Not DMX File Descriptor)
			void resetPidData(int pid);

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

			/// Check if PID is used
			bool isPIDUsed(int pid) const;

			/// Check if pid is an PMT PID
			bool isPMT(int pid) const;

			///
			void setPMT(int pid, bool set);

			/// Set all PID
			void setAllPID(bool val);

			/// Get the frequency in Mhz
			uint32_t getFrequency() const;

			/// Set the frequency in Mhz
			void setFrequency(uint32_t freq);

			/// Get the current Delivery System
			input::InputSystem getDeliverySystem() const;

			/// Set the current Delivery System
			void setDeliverySystem(input::InputSystem system);

			fe_delivery_system convertDeliverySystem() const;

			///
			int  getSymbolRate() const;

			///
			void setSymbolRate(int srate);

			/// Get modulation type
			int  getModulationType() const;

			/// Set modulation type
			void setModulationType(int modtype);

			///
			void setRollOff(int rolloff);

			///
			int  getRollOff() const;

			///
			void setFEC(int fec);

			///
			int  getFEC() const;

			///
			void setPilotTones(int pilot);

			///
			int  getPilotTones() const;

			/// Check the 'Channel Data changed' flag. If set we should update
			bool hasDeviceDataChanged() const;

			/// Reset/clear the 'Channel Data changed' flag
			void resetDeviceDataChanged();

			/// Get/Set the LNB polarizaion
			int getPolarization() const;
			char getPolarizationChar() const;
			void setPolarization(int pol);

			/// Get/Set the DiSEqc source
			int getDiSEqcSource() const;
			void setDiSEqcSource(int source);

			void setSpectralInversion(int specinv);
			int  getSpectralInversion() const;
			void setBandwidthHz(int bandwidth);
			int  getBandwidthHz() const;
			void setTransmissionMode(int transmission);
			int  getTransmissionMode() const;
			void setGuardInverval(int guard);
			int  getGuardInverval() const;
			void setHierarchy(int hierarchy);
			int  getHierarchy() const;
			void setUniqueIDPlp(int plp);
			int  getUniqueIDPlp() const;
			void setUniqueIDT2(int id);
			int  getUniqueIDT2() const;
			void setSISOMISO(int sm);
			int  getSISOMISO() const;

			void setDataSlice(int slice);
			int  getDataSlice() const;

			void setC2TuningFrequencyType(int c2tft);
			int  getC2TuningFrequencyType() const;

			void setMonitorData(fe_status_t status,
					uint16_t strength,
					uint16_t snr,
					uint32_t ber,
					uint32_t ublocks);

			int hasLock() const;

			fe_status_t getSignalStatus() const;

			uint16_t getSignalStrength() const;

			uint16_t getSignalToNoiseRatio() const;

			uint32_t getBitErrorRate() const;

			uint32_t getUncorrectedBlocks() const;

			///
			std::string attributeDescribeString(int streamID) const;

			// =======================================================================
			// -- Data members -------------------------------------------------------
			// =======================================================================

		protected:
			base::Mutex _mutex;

			bool _changed;             ///
			uint32_t _freq;            /// frequency in MHZ
			input::InputSystem _delsys;/// modulation system i.e. (DVBS/DVBS2)
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

			// =======================================================================
			// -- Monitor Data members -----------------------------------------------
			// =======================================================================
			fe_status_t _status;
			uint16_t _strength;
			uint16_t _snr;
			uint32_t _ber;
			uint32_t _ublocks;
	};

} // namespace input

#endif // INPUT_DEVICE_DATA_H_INCLUDE
