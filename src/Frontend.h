/* Frontend.h

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef FRONTEND_H_INCLUDE
#define FRONTEND_H_INCLUDE

#include "dvbfix.h"

#include <stdint.h>
#include <string>

// Forward declarations
class StreamProperties;

#define LNB_UNIVERSAL 0
#define LNB_STANDARD  1

// slof: switch frequency of LNB
#define DEFAULT_SWITCH_LOF (11700*1000UL)

// lofLow: local frequency of lower LNB band
#define DEFAULT_LOF_LOW_UNIVERSAL (9750*1000UL)

// lofHigh: local frequency of upper LNB band
#define DEFAULT_LOF_HIGH_UNIVERSAL (10600*1000UL)

// Lnb standard Local oscillator frequency
#define DEFAULT_LOF_STANDARD (10750*1000UL)

// LNB properties
typedef struct {
	uint8_t  type;           // LNB  (0: UNIVERSAL , 1: STANDARD)
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
	int src;                 // Source (1-4) => DiSEqC switch position (0-3)
	int pol_v;               // polarisation (1 = vertical/circular right, 0 = horizontal/circular left)
	int hiband;              //
	Lnb_t LNB[MAX_LNB];      // LNB properties
} DiSEqc_t;

#define MAX_DELSYS       5

// Forward declarations

/// The class @c Frontend carries all the data/information of an frontend
/// and to tune it
class Frontend  {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		Frontend();
		virtual ~Frontend();

		///
		void addFrontendPaths(const std::string &fe,
		                      const std::string &dvr,
							  const std::string &dmx) {
			_path_to_fe = fe;
			_path_to_dvr = dvr;
			_path_to_dmx = dmx;
		}

		///
		void addToXML(std::string &xml) const;

		///
		bool setFrontendInfo();

		///
		int get_dvr_fd() const { return _fd_dvr; }

		/// caller should release fd
		int get_monitor_fd() const { return open_fe(_path_to_fe, true); }

		/// Check if this frontend is tuned
		bool isTuned() const {return (_fd_dvr != -1) && _tuned; }

		///
		bool capableOf(fe_delivery_system_t msys);

		/// Update the Channel and PID. Will close DVR and reopen it if channel did change
		bool update(StreamProperties &properties);

		///
		bool teardown(StreamProperties &properties);

		///
		size_t getDeliverySystemSize() const { return _del_sys_size; }

		const fe_delivery_system_t *getDeliverySystem() const { return _info_del_sys; }

	protected:
		///
		int open_fe(const std::string &path, bool readonly) const;

		///
		int open_dvr(const std::string &path);

		///
		int open_dmx(const std::string &path);

		///
		bool set_demux_filter(int fd, uint16_t pid);

		///
		bool tune(const StreamProperties &properties);

		///
		bool diseqcSendMsg(fe_sec_voltage_t v, struct diseqc_cmd *cmd,
		                     fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b, bool repeatDiseqc);

		///
		bool sendDiseqc(int streamID, bool repeatDiseqc);

		///
		bool tune_it(StreamProperties &properties);

		///
		bool updatePIDFilters(StreamProperties &properties);

		///
		bool setupAndTune(StreamProperties &properties);

	private:
		// =======================================================================
		// Data members
		// =======================================================================
		bool                     _tuned;
		int                      _fd_fe;
		int                      _fd_dvr;
		std::string              _path_to_fe;
		std::string              _path_to_dvr;
		std::string              _path_to_dmx;
		struct dvb_frontend_info _fe_info;
		fe_delivery_system_t     _info_del_sys[MAX_DELSYS];
		size_t                   _del_sys_size;
		// =======================================================================
		//
		// =======================================================================
		Lnb_t    _lnb[MAX_LNB];   // lnb that can be connected to this frontend
		DiSEqc_t _diseqc;         //

}; // class Frontend

#endif // FRONTEND_H_INCLUDE