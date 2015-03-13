/* Channel.h

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#ifndef CHANNEL_DATA_H_INCLUDE
#define CHANNEL_DATA_H_INCLUDE

#include <stdint.h>

#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>

#define MAX_PIDS 8193

#define ALL_PIDS 8192

// PID and DMX file descriptor
typedef struct {
	int      fd_dmx;         // used DMX file descriptor for PID
	uint8_t  used;           // used pid (0 = not used, 1 = in use)
	uint8_t  cc;             // continuity counter (0 - 15) of this PID
	uint32_t cc_error;       // cc error count
	uint32_t count;          // the number of times this pid occurred
} PidData_t;

typedef struct {
	bool      changed;       // if something changed to 'pid' array
    PidData_t data[MAX_PIDS];// used pids
} Pid_t;

/// The class @c ChannelData carries all the data/information for tuning a frontend
class ChannelData  {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		ChannelData();
		virtual ~ChannelData();

		void initialize();

//	protected:

//	private:
		// =======================================================================
		// Data members
		// =======================================================================
		bool changed;            //
		fe_delivery_system_t delsys; // modulation system i.e. (SYS_DVBS/SYS_DVBS2)
		uint32_t freq;           // frequency in MHZ
		uint32_t ifreq;          // intermediate frequency in kHZ
		int modtype;             // modulation type i.e. (QPSK/PSK_8)
		int srate;               // symbol rate in kSymb/s
		int fec;                 // forward error control i.e. (FEC_1_2 / FEC_2_3)
		int rolloff;             // roll-off
		int inversion;           //
		Pid_t pid;               //

		// =======================================================================
		// DVB-S(2) Data members
		// =======================================================================
		int pilot;               // pilot tones (on/off)
		int src;                 // Source (1-4) => DiSEqC switch position (0-3)
		int pol_v;               // polarisation (1 = vertical/circular right, 0 = horizontal/circular left)

		// =======================================================================
		// DVB-C2 Data members
		// =======================================================================
		int c2tft;               // DVB-C2
		int data_slice;          // DVB-C2

		// =======================================================================
		// DVB-T(2) Data members
		// =======================================================================
		int transmission;        // DVB-T(2)
		int guard;               // DVB-T(2)
		int hierarchy;           // DVB-T(2)
		int bandwidth;           // DVB-T(2)/C2
		int plp_id;              // DVB-T2/C2
		int t2_system_id;        // DVB-T2
		int siso_miso;           // DVB-T2
}; // class ChannelData

#endif // CHANNEL_DATA_H_INCLUDE