/* tune.h

   Copyright (C) 2014 Marc Postema

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


#ifndef _TUNE_H
#define _TUNE_H

#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>

#define FULL_DVB_API_VERSION (DVB_API_VERSION << 8 | DVB_API_VERSION_MINOR)

#if FULL_DVB_API_VERSION < 0x0500
#error Not correct DVB_API_VERSION
#endif

#if FULL_DVB_API_VERSION < 0x0505
#define DTV_ENUM_DELSYS     44
#define SYS_DVBC_ANNEX_A    SYS_DVBC_ANNEX_AC
#define NOT_PREFERRED_DVB_API 1
#endif

#define LNB_UNIVERSAL 0
#define LNB_STANDARD  1
// slof: switch frequency of LNB */
#define DEFAULT_SLOF (11700*1000UL)
// lofLow: local frequency of lower LNB band
#define DEFAULT_LOF1_UNIVERSAL (9750*1000UL)
// lofHigh: local frequency of upper LNB band */
#define DEFAULT_LOF2_UNIVERSAL (10600*1000UL)
// Lnb standard Local oscillator frequency*/
#define DEFAULT_LOF_STANDARD (10750*1000UL)

// LNB properties
typedef struct {
#define MAX_LNB 4
	uint8_t  type;           // LNB  (0: UNIVERSAL , 1: STANDARD)
	uint32_t lofStandard;
	uint32_t switchlof;
	uint32_t lofLow;
	uint32_t lofHigh;
} Lnb_t;

// DiSEqc properties
typedef struct {
    int src;                 // DiSEqC (1-4)
    int pol;                 // polarisation (1 = vertical/circular right, 0 = horizontal/circular left)
	int hiband;              //
    Lnb_t *LNB;              // LNB properties
} DiSEqc_t;

// Frontend Monitor
typedef struct {
	pthread_mutex_t mutex;    // monitor mutex
	int             fd_fe;    //
	fe_status_t     status;   // FE_HAS_LOCK | FE_HAS_SYNC | FE_HAS_SIGNAL
	uint16_t        strength; //
	uint16_t        snr;      //
	uint32_t        ber;      //
	uint32_t        ublocks;  //
} FE_Monitor_t;

// PID and DMX file descriptor
typedef struct {
	int      fd_dmx;         // used DMX file descriptor for PID
	uint8_t  used;           // used pid (0 = not used, 1 = in use)
	uint8_t  cc;             // continuity counter (0 - 15) of this PID
	uint32_t cc_error;       // cc error count
	uint32_t count;          // the number of times this pid occurred
} PidData_t;

// PID properties
typedef struct {
#define MAX_PIDS 8192
	int       changed;       // if something changed to 'pid' array
	int       all;           // if all pid are requested
    PidData_t data[MAX_PIDS];// used pids
} Pid_t;

// Channel data
typedef struct {
    fe_delivery_system_t delsys; // modulation system i.e. (SYS_DVBS/SYS_DVBS2)
    uint32_t freq;           // frequency in MHZ
    uint32_t ifreq;          // intermediate frequency in kHZ
    int modtype;             // modulation type i.e. (QPSK/PSK_8)
    int srate;               // symbol rate in kSymb/s
    int fec;                 // forward error control i.e. (FEC_1_2 / FEC_2_3)
    int rolloff;             // roll-off
    int pilot;               // pilot tones (on/off)
    int inversion;           //

	int c2tft;               // DVB-C2
	int data_slice;          // DVB-C2
	
	int transmission;        // DVB-T(2)
	int guard;               // DVB-T(2)
	int hierarchy;           // DVB-T(2)
	int bandwidth;           // DVB-T(2)/C2
	int plp_id;              // DVB-T2/C2
	int t2_system_id;        // DVB-T2
	int siso_miso;           // DVB-T2
} ChannelData_t;

// Frontend properties
typedef struct {
	int index;               // frontend number
	pthread_mutex_t mutex;   // frontend mutex
	int attached;            //
    int fd_fe;               // used frontend file descriptor
    int fd_dvr;              // used DVR file descriptor
	int tuned;               // if tuned to this channel (1 = tuned, 0 = not tuned yet)
	FE_Monitor_t monitor;    // frontend monitor
	ChannelData_t channel;   //
	DiSEqc_t diseqc;         //
    Pid_t pid;               // used pids
	Lnb_t lnb_array[MAX_LNB];// lnb that can be connected to this frontend
	
#define FE_PATH_LEN 255
#define MAX_DELSYS  5
	char path_to_fe[FE_PATH_LEN];
	char path_to_dvr[FE_PATH_LEN];
	char path_to_dmx[FE_PATH_LEN];
	struct dvb_frontend_info fe_info;
	fe_delivery_system_t     info_del_sys[MAX_DELSYS];
	size_t                   del_sys_size;
} Frontend_t;

// Frontend Array
typedef struct {
	Frontend_t       **array;         //
	size_t           max_fe;          //
	char             del_sys_str[50]; // delivery system string for xml
} FrontendArray_t;

// Detect the attached frontends and get frontend properties
size_t detect_attached_frontends(const char *path, FrontendArray_t *fe);

// open frontend
int open_fe(const char *path_to_fe, int readonly);

//
void reset_pid(PidData_t *pid);

//
int update_pid_filters(Frontend_t *frontend);

//
int clear_pid_filters(Frontend_t *frontend);

//
int setup_frontend_and_tune(Frontend_t *frontend);

//
void monitor_frontend(FE_Monitor_t *monitor, int showStatus);

//
char *attribute_describe_string(const Frontend_t *fe);

// Convert Functions
const char *fec_to_string(int fec);
const char *delsys_to_string(int delsys);
const char *modtype_to_sting(int modtype);
const char *rolloff_to_sting(int rolloff);
const char *transmode_to_string(int transmission_mode);
const char *guardinter_to_string(int guard_interval);
const char *bandwidth_to_string(int bandwidth);

#endif
