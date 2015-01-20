/* tune.c

   Copyright (C) 2014 Marc Postema

    Parts of this code comes from szap-s2 by Igor M. Liplianin

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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>


#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>

#include "tune.h"
#include "utils.h"
#include "applog.h"

#define FULL_DVB_API_VERSION (DVB_API_VERSION << 8 | DVB_API_VERSION_MINOR)

#if FULL_DVB_API_VERSION < 0x0500
#error Not correct DVB_API_VERSION
#endif

#if FULL_DVB_API_VERSION < 0x0505
#define DTV_ENUM_DELSYS     44
#define SYS_DVBC_ANNEX_A    SYS_DVBC_ANNEX_AC
#define NOT_PREFERRED_DVB_API 1
#endif

#define DMX       "/dev/dvb/adapter%d/demux%d"
#define DVR       "/dev/dvb/adapter%d/dvr%d"
#define FRONTEND  "/dev/dvb/adapter%d/frontend%d"

/*
 *
 */
static size_t get_attached_frontend_count(const char *path, size_t count, Frontend_t **fe_array) {
	struct dirent **file_list;
	const int n = scandir(path, &file_list, NULL, alphasort);
	if (n > 0) {
		int i;
		for (i = 0; i < n; ++i) {
			char full_path[FE_PATH_LEN];
			snprintf(full_path, FE_PATH_LEN, "%s/%s", path, file_list[i]->d_name);
			struct stat stat_buf;
			if (stat(full_path, &stat_buf) == 0) {
				switch (stat_buf.st_mode & S_IFMT) {
					case S_IFCHR: // character device
						if (strstr(file_list[i]->d_name, "frontend") != NULL) {
							// check if we have an array we can fill in
							if (fe_array && fe_array[count]) {
								int fe_nr;
								sscanf(file_list[i]->d_name, "frontend%d", &fe_nr);
								int adapt_nr;
								sscanf(path, "/dev/dvb/adapter%d", &adapt_nr);

								//
								snprintf(fe_array[count]->path_to_fe,  FE_PATH_LEN, FRONTEND, adapt_nr, fe_nr);
								snprintf(fe_array[count]->path_to_dvr, FE_PATH_LEN, DVR, adapt_nr, fe_nr);
								snprintf(fe_array[count]->path_to_dmx, FE_PATH_LEN, DMX, adapt_nr, fe_nr);
							}

							++count;
						}
					    break;
					case S_IFDIR:
						// do not use dir '.' an '..'
						if (strcmp(file_list[i]->d_name, ".") != 0 && strcmp(file_list[i]->d_name, "..") != 0) {
							count = get_attached_frontend_count(full_path, count, fe_array);
						}
						break;
				}
			}
			free(file_list[i]);
		}
	}
	return count;
}

/*
 *
 */
static int get_fe_info(Frontend_t *frontend) {
	struct dtv_properties dtvProperties;
	struct dtv_property dtvProperty;
	int fd_fe;

	// open frondend in readonly mode
	if((fd_fe = open_fe(frontend->path_to_fe, 1)) < 0){
		snprintf(frontend->fe_info.name, sizeof(frontend->fe_info.name), "Not Found");
		PERROR("open_fe");
		return -1;
	}

	if ( ioctl(fd_fe, FE_GET_INFO, &frontend->fe_info) != 0){
		snprintf(frontend->fe_info.name, sizeof(frontend->fe_info.name), "Not Set");
		PERROR("FE_GET_INFO");
		CLOSE_FD(fd_fe);
		return -1;
	}
	SI_LOG_INFO("Frontend Name: %s", frontend->fe_info.name);

	dtvProperty.cmd = DTV_ENUM_DELSYS;
	dtvProperty.u.data = DTV_UNDEFINED;

	dtvProperties.num = 1; // size
	dtvProperties.props = &dtvProperty;
	if (ioctl(fd_fe, FE_GET_PROPERTY, &dtvProperties ) != 0) {
		PERROR("ioctl FE_GET_PROPERTY");
		CLOSE_FD(fd_fe);
		return -1;
	}

	size_t i;
	frontend->del_sys_size = dtvProperty.u.buffer.len;
	for (i = 0; i < dtvProperty.u.buffer.len; i++) {
		switch (dtvProperty.u.buffer.data[i]) {
			case SYS_DVBS:
				frontend->info_del_sys[i] = SYS_DVBS;
				SI_LOG_DEBUG("Frontend Type: Satellite (DVB-S)");
				break;
			case SYS_DVBS2:
				frontend->info_del_sys[i] = SYS_DVBS2;
				SI_LOG_DEBUG("Frontend Type: Satellite (DVB-S2)");
				break;
			case SYS_DVBT:
				frontend->info_del_sys[i] = SYS_DVBT;
				SI_LOG_DEBUG("Frontend Type: Terrestrial (DVB-T)");
				break;
			case SYS_DVBT2:
				frontend->info_del_sys[i] = SYS_DVBT2;
				SI_LOG_DEBUG("Frontend Type: Terrestrial (DVB-T2)");
				break;
			case SYS_DVBC_ANNEX_A:
				frontend->info_del_sys[i] = SYS_DVBC_ANNEX_A;
				SI_LOG_DEBUG("Frontend Type: Cable (Annex A)");
				break;
			case SYS_DVBC_ANNEX_B:
				frontend->info_del_sys[i] = SYS_DVBC_ANNEX_B;
				SI_LOG_DEBUG("Frontend Type: Cable (Annex B)");
				break;
#if FULL_DVB_API_VERSION >= 0x0505
			case SYS_DVBC_ANNEX_C:
				frontend->info_del_sys[i] = SYS_DVBC_ANNEX_C;
				SI_LOG_DEBUG("Frontend Type: Cable (Annex C)");
				break;
#endif
			default:
				SI_LOG_DEBUG("Frontend Type Unknown: %d", dtvProperty.u.buffer.data[i]);
				break;
		}
	}

	SI_LOG_DEBUG("Frontend Freq: %d Hz to %d Hz", frontend->fe_info.frequency_min, frontend->fe_info.frequency_max);
	SI_LOG_DEBUG("Frontend srat: %d symbols/s to %d symbols/s", frontend->fe_info.symbol_rate_min, frontend->fe_info.symbol_rate_max);

	CLOSE_FD(fd_fe);
	return 1;
}

/*
 *
 */
static int open_dvr(const char *pah_to_dvr) {
	int fd;
	if((fd = open(pah_to_dvr, O_RDONLY | O_NONBLOCK)) < 0) {
		PERROR("DVR DEVICE");
	}
	return fd;
}

/*
 *
 */
static int open_dmx(const char *path_to_dmx) {
	int fd;
	if((fd = open(path_to_dmx, O_RDWR | O_NONBLOCK)) < 0) {
		PERROR("DMX DEVICE");
	}
	return fd;
}

/*
 *
 */
static int set_demux_filter(int fd, uint16_t pid) {
	struct dmx_pes_filter_params pesFilter;

	pesFilter.pid      = pid;
	pesFilter.input    = DMX_IN_FRONTEND;
	pesFilter.output   = DMX_OUT_TS_TAP;
	pesFilter.pes_type = DMX_PES_OTHER;
	pesFilter.flags    = DMX_IMMEDIATE_START;

	if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilter) != 0) {
		PERROR("DMX_SET_PES_FILTER");
		return -1;
	}
	SI_LOG_DEBUG("Set filter PID %d - fd %d", pid, fd);
	return 1;
}

struct diseqc_cmd {
	struct dvb_diseqc_master_cmd cmd;
	uint32_t wait;
};

static int diseqc_send_msg(int fd_fe, fe_sec_voltage_t v, struct diseqc_cmd *cmd,
	fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b) {
	if (ioctl(fd_fe, FE_SET_TONE, SEC_TONE_OFF) == -1) {
		PERROR("FE_SET_TONE failed");
		return -1;
	}
	if (ioctl(fd_fe, FE_SET_VOLTAGE, v) == -1) {
		PERROR("FE_SET_VOLTAGE failed");
		return -1;
	}
	usleep(15 * 1000);
	if (ioctl(fd_fe, FE_DISEQC_SEND_MASTER_CMD, &cmd->cmd) == -1) {
		PERROR("FE_DISEQC_SEND_MASTER_CMD failed");
		return -1;
	}
	usleep(cmd->wait * 1000);
	usleep(15 * 1000);
	if (ioctl(fd_fe, FE_DISEQC_SEND_BURST, b) == -1) {
		PERROR("FE_DISEQC_SEND_BURST failed");
		return -1;
	}
	usleep(15 * 1000);
	if (ioctl(fd_fe, FE_SET_TONE, t) == -1) {
		PERROR("FE_SET_TONE failed");
		return -1;
	}
	usleep(15 * 1000);
	
	return 1;
}

/* Digital Satellite Equipment Control,
 * specification is available from http://www.eutelsat.com/
 */
static int send_diseqc(int fd_fe, DiSEqc_t *diseqc) {
	struct diseqc_cmd cmd = {
		{ {0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4}, 0
	};
	// Framing 0x0e: Command from Master, No reply required, First transmission
	// Address 0x10: Any LNB, Switcher or SMATV (Master to all...)
	// Command 0x38: Write to Port group 0 (Committed switches)
	// Data 1  0xf0: see below
	
	SI_LOG_DEBUG("Sending DiSEqC");

	// param: high nibble: reset bits, low nibble set bits,
	// bits are: option, position, polarizaion, band
	cmd.cmd.msg[3] =
		0xf0 | (((diseqc->src * 4) & 0x0f) | (diseqc->hiband ? 1 : 0) | (diseqc->pol ? 0 : 2));

	return diseqc_send_msg(fd_fe, diseqc->pol ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18,
		      &cmd, diseqc->hiband ? SEC_TONE_ON : SEC_TONE_OFF, ((diseqc->src / 4) % 2) ? SEC_MINI_B : SEC_MINI_A);
}

/*
 *
 */
static int tune(int fd_fe, const ChannelData_t *channel) {
	struct dtv_property p[13];
	int size = 0;

#define FILL_PROP(CMD, DATA) { p[size].cmd = CMD; p[size].u.data = DATA; ++size; }

	FILL_PROP(DTV_CLEAR, DTV_UNDEFINED);
	switch (channel->delsys) {
		case SYS_DVBS:
		case SYS_DVBS2:
			FILL_PROP(DTV_DELIVERY_SYSTEM, channel->delsys);
			FILL_PROP(DTV_FREQUENCY,       channel->ifreq);
			FILL_PROP(DTV_MODULATION,      channel->modtype);
			FILL_PROP(DTV_SYMBOL_RATE,     channel->srate);
			FILL_PROP(DTV_INNER_FEC,       channel->fec);
			FILL_PROP(DTV_INVERSION,       INVERSION_AUTO);
			FILL_PROP(DTV_ROLLOFF,         channel->rolloff);
			FILL_PROP(DTV_PILOT,           PILOT_AUTO);
			break;
		case SYS_DVBT:
			FILL_PROP(DTV_DELIVERY_SYSTEM,   channel->delsys);
			FILL_PROP(DTV_FREQUENCY,         channel->freq * 1000UL);
			FILL_PROP(DTV_MODULATION,        channel->modtype);
			FILL_PROP(DTV_INVERSION,         channel->inversion);
			FILL_PROP(DTV_BANDWIDTH_HZ,      channel->bandwidth);
			FILL_PROP(DTV_CODE_RATE_HP,      channel->fec);
			FILL_PROP(DTV_CODE_RATE_LP,      channel->fec);
			FILL_PROP(DTV_TRANSMISSION_MODE, channel->transmission);
			FILL_PROP(DTV_GUARD_INTERVAL,    channel->guard);
			FILL_PROP(DTV_HIERARCHY,         channel->hierarchy);
			break;
		case SYS_DVBC_ANNEX_A:
			FILL_PROP(DTV_DELIVERY_SYSTEM,   channel->delsys);
			FILL_PROP(DTV_FREQUENCY,         channel->freq * 1000UL);
			FILL_PROP(DTV_INVERSION,         channel->inversion);
			FILL_PROP(DTV_MODULATION,        channel->modtype);
			FILL_PROP(DTV_SYMBOL_RATE,       channel->srate);
			FILL_PROP(DTV_INNER_FEC,         channel->fec);
			break;

		default:
			return -1;
	}
	FILL_PROP(DTV_TUNE, DTV_UNDEFINED);
	struct dtv_properties cmdseq = {
		.num = size,
		.props = p
	};

	if ((ioctl(fd_fe, FE_SET_PROPERTY, &cmdseq)) == -1) {
		PERROR("FE_SET_PROPERTY failed");
		return -1;
	}
	return 1;
}

/*
 *
 */
static int tune_it(int fd, ChannelData_t *channel, DiSEqc_t *diseqc) {

	SI_LOG_DEBUG("Start tuning");

	if (channel->delsys == SYS_DVBS || channel->delsys == SYS_DVBS2) {
		diseqc->hiband = 0;
		if (diseqc->LNB->switchlof && diseqc->LNB->lofHigh && channel->freq >= diseqc->LNB->switchlof) {
			diseqc->hiband = 1;
		}

		if (diseqc->hiband) {
			channel->ifreq = channel->freq - diseqc->LNB->lofHigh;
		} else {
			if (channel->freq < diseqc->LNB->lofLow) {
				channel->ifreq = diseqc->LNB->lofLow - channel->freq;
			} else {
				channel->ifreq = channel->freq - diseqc->LNB->lofLow;
			}
		}
		// set diseqc
		if (send_diseqc(fd, diseqc) == -1) {
			return 0;
		}
	}
	// Now tune
	if (tune(fd, channel) == -1) {
		return 0;
	}
	return 1;
}

/*
 *
 */
size_t detect_attached_frontends(const char *path, FrontendArray_t *fe) {
#ifdef NOT_PREFERRED_DVB_API
	SI_LOG_ERROR("Not the preferred DVB API version, for correct function it should be 5.5 or higher");
	SI_LOG_ERROR("current DVB_API_VERSION: %d.%d", DVB_API_VERSION, DVB_API_VERSION_MINOR);
#endif
	SI_LOG_INFO("Detecting frontends in: %s", path);
	fe->max_fe = get_attached_frontend_count(path, 0, NULL);
	if (fe->max_fe != 0) {
		fe->array = malloc(sizeof(Frontend_t *) * fe->max_fe);
		size_t i;
		for (i = 0; i < fe->max_fe; ++i) {
			fe->array[i] = malloc(sizeof(Frontend_t));;
			fe->array[i]->index = i;
		}
		get_attached_frontend_count("/dev/dvb", 0, fe->array);
		SI_LOG_INFO("Frontends found: %zu", fe->max_fe);

		size_t nr_dvb_s2 = 0;
		size_t nr_dvb_t  = 0;
		size_t nr_dvb_t2 = 0;
		size_t nr_dvb_c  = 0;
		size_t nr_dvb_c2 = 0;

		// Get all Frontend properties
		for (i = 0; i < fe->max_fe; ++i) {
			Frontend_t *frontend = fe->array[i];
			if (get_fe_info(frontend) == 1) {
				size_t j;
				for (j = 0; j < frontend->del_sys_size; j++) {
					switch (frontend->info_del_sys[j]) {
						// only count DVBS2
						case SYS_DVBS2:
							++nr_dvb_s2;
							break;
						case SYS_DVBT:
							++nr_dvb_t;
							break;
						case SYS_DVBT2:
							++nr_dvb_t2;
							break;
						case SYS_DVBC_ANNEX_A:
						case SYS_DVBC_ANNEX_B:
							++nr_dvb_c;
							break;
#if FULL_DVB_API_VERSION >= 0x0505
						case SYS_DVBC_ANNEX_C:
							++nr_dvb_c2;
							break;
#endif
						default:
							// Not supported
							break;
					}
				}
			}
		}
		// make xml delivery system string
		snprintf(fe->del_sys_str, sizeof(fe->del_sys_str), "DVBS2-%zu,DVBT-%zu,DVBT2-%zu,DVBC-%zu,DVBC2-%zu",
		               nr_dvb_s2, nr_dvb_t, nr_dvb_t2, nr_dvb_c, nr_dvb_c2);
	} else {
		SI_LOG_ERROR("No Frontends found!!", fe->max_fe);
	}
	return fe->max_fe;
}

/*
 *
 */
int open_fe(const char *path_to_fe, int readonly) {
	int fd;
	if((fd = open(path_to_fe, (readonly ? O_RDONLY : O_RDWR) | O_NONBLOCK)) < 0){
		PERROR("FRONTEND DEVICE");
	}
	SI_LOG_DEBUG("Open FE fd: %d readonly %d", fd, readonly);
	return fd;
}

/*
 *
 */
void reset_pid(PidData_t *pid) {
	pid->used     = 0;
	pid->cc       = 0x80;
	pid->cc_error = 0;
	pid->count    = 0;
	CLOSE_FD(pid->fd_dmx);
}

/*
 *
 */
int update_pid_filters(Frontend_t *frontend) {
	if (frontend->pid.changed == 1) {
		frontend->pid.changed = 0;
		int i;
		for (i = 0; i < MAX_PIDS; ++i) {
			// check if PID is used or removed
			if (frontend->pid.data[i].used == 1) {
				// check if we have no DMX for this PID, then open one
				if (frontend->pid.data[i].fd_dmx == -1) {
					frontend->pid.data[i].fd_dmx = open_dmx(frontend->path_to_dmx);
					size_t timeout = 0;
					while (set_demux_filter(frontend->pid.data[i].fd_dmx, i) != 1) {
						usleep(350000);
						++timeout;
						if (timeout > 3) {
							return -1;
						}
					}
				}
			} else if (frontend->pid.data[i].fd_dmx != -1) {
				// We have a DMX but no PID anymore, so reset it
				SI_LOG_DEBUG("Remove filter PID %d - Packet Count: %d", i, frontend->pid.data[i].count);
				reset_pid(&frontend->pid.data[i]);
			}
		}
	}
	return 1;
}

/*
 *
 */
int setup_frontend_and_tune(Frontend_t *frontend) {
	if (frontend->tuned == 0) {
		// Check if have already opened a FE
		if (frontend->fd_fe == -1) {
			frontend->fd_fe = open_fe(frontend->path_to_fe, 0);
			SI_LOG_INFO("Frontend: %d, Opened FE fd: %d.", frontend->index, frontend->fd_fe);
		}
		// try tuning
		size_t timeout = 0;
		while (tune_it(frontend->fd_fe, &frontend->channel, &frontend->diseqc) != 1) {
			usleep(350000);
			++timeout;
			if (timeout > 3) {
				return -1;
			}
		}
		SI_LOG_INFO("Frontend: %d, Waiting on lock.", frontend->index);

		// check if frontend is locked, if not try a few times
		timeout = 0;
		while (timeout < 4) {
			fe_status_t status = 0;
			// first read status
			if (ioctl(frontend->fd_fe, FE_READ_STATUS, &status) == 0) {
				if ((status & FE_HAS_LOCK) && (status & FE_HAS_SIGNAL)) {
					// We are tuned now
					frontend->tuned = 1;
					SI_LOG_INFO("Frontend: %d, Tuned and locked (FE status %d).", frontend->index, status);
					break;
				} else {
					SI_LOG_INFO("Frontend: %d, Not locked yet (FE status %d)...", frontend->index, status);
				}
			}
			usleep(25000);
			++timeout;
		}
	}
	// Check if we have already a DVR open
	if (frontend->fd_dvr == -1) {
		// try opening DVR, try again if fails
		size_t timeout = 0;
		while ((frontend->fd_dvr = open_dvr(frontend->path_to_dvr)) == -1) {
			usleep(150000);
			++timeout;
			if (timeout > 3) {
				return -1;
			}
		}
		SI_LOG_INFO("Frontend: %d, Opened DVR fd: %d.", frontend->index, frontend->fd_dvr);
	}
	return 1;
}

/*
 *
 */
int clear_pid_filters(Frontend_t *frontend) {
	size_t i;
	for (i = 0; i < MAX_PIDS; ++i) {
		if (frontend->pid.data[i].used) {
			SI_LOG_DEBUG("Remove filter PID %d - Packet Count: %d", i, frontend->pid.data[i].count);
			reset_pid(&frontend->pid.data[i]);
		} else if (frontend->pid.data[i].fd_dmx != -1) {
			SI_LOG_ERROR("!! No PID %d but still open DMX !!", i);
			reset_pid(&frontend->pid.data[i]);
		}
	}
	return 1;
}

/*
 *
 */
void monitor_frontend(FE_Monitor_t *monitor, int showStatus) {
	fe_status_t status = 0;
	// first read status
	if (ioctl(monitor->fd_fe, FE_READ_STATUS, &status) == 0) {
		// check status OK, then read the rest (OR keep previous result)
		if ((status & FE_HAS_LOCK) && (status & FE_HAS_SIGNAL)) {
			pthread_mutex_lock(&monitor->mutex);

			// update monitor status with new one
			monitor->status = status;
			// some frontends might not support all these ioctls, thus we
			// avoid printing errors
			if (ioctl(monitor->fd_fe, FE_READ_SIGNAL_STRENGTH, &monitor->strength) != 0) {
				monitor->strength = 0;
			}
			if (ioctl(monitor->fd_fe, FE_READ_SNR, &monitor->snr) != 0) {
				monitor->snr = 0;
			}
			if (ioctl(monitor->fd_fe, FE_READ_BER, &monitor->ber) != 0) {
				monitor->ber = -2;
			}
			if (ioctl(monitor->fd_fe, FE_READ_UNCORRECTED_BLOCKS, &monitor->ublocks) != 0) {
				monitor->ublocks = -2;
			}
			monitor->strength = (monitor->strength * 100) / 0xffff;
			monitor->snr = (monitor->snr * 100) / 0xffff;

			pthread_mutex_unlock(&monitor->mutex);

			// Print Status
			if (showStatus) {
				SI_LOG_INFO("status %02x | signal %3u%% | snr %3u%% | ber %d | unc %d | Locked %d",
					monitor->status, monitor->strength, monitor->snr, monitor->ber, monitor->ublocks,
					(monitor->status & FE_HAS_LOCK) ? 1 : 0);
			}
		} else {
//			SI_LOG_INFO("Monitor Error - status %02x\r\n", status);
		}
	} else {
		PERROR("FE_READ_STATUS failed");
	}
}

/*
 *
 */
const char *fec_to_string(int fec) {
	switch (fec) {
		case FEC_1_2:
			return "FEC_1_2";
		case FEC_2_3:
			return "FEC_2_3";
		case FEC_3_4:
			return "FEC_3_4";
		case FEC_3_5:
			return "FEC_3_5";
		case FEC_4_5:
			return "FEC_4_5";
		case FEC_5_6:
			return "FEC_5_6";
		case FEC_6_7:
			return "FEC_6_7";
		case FEC_7_8:
			return "FEC_7_8";
		case FEC_8_9:
			return "FEC_8_9";
		case FEC_9_10:
			return "FEC_9_10";
		case FEC_AUTO:
			return "FEC_AUTO";
		case FEC_NONE:
			return "FEC_NONE";
		default:
			return "UNKNOWN FEC";
	}
}

/*
 *
 */
const char *delsys_to_string(int delsys) {
	switch (delsys) {
		case SYS_DVBS2:
			return "SYS_DVBS2";
		case SYS_DVBS:
			return "SYS_DVBS";
		case SYS_DVBT:
			return "SYS_DVBT";
		case SYS_DVBT2:
			return "SYS_DVBT2";
		case SYS_DVBC_ANNEX_A:
			return "SYS_DVBC_ANNEX_A";
		case SYS_DVBC_ANNEX_B:
			return "SYS_DVBC_ANNEX_B";
		default:
			return "UNKNOWN DELSYS";
	}
}

/*
 *
 */
const char *modtype_to_sting(int modtype) {
	switch (modtype) {
		case QAM_16:
			return "QAM_16";
		case QAM_32:
			return "QAM_32";
		case QAM_64:
			return "QAM_64";
		case QAM_128:
			return "QAM_128";
		case QAM_256:
			return "QAM_256";
		case QPSK:
			return "QPSK";
		case PSK_8:
			return "PSK_8";
		default:
			return "UNKNOWN MODTYPE";
	}
}

/*
 *
 */
const char *rolloff_to_sting(int rolloff) {
	switch (rolloff) {
		case ROLLOFF_35:
			return "0.35";
		case ROLLOFF_25:
			return "0.25";
		case ROLLOFF_20:
			return "0.20";
		case ROLLOFF_AUTO:
			return "auto";
		default:
			return "UNKNOWN ROLLOFF";
	}
}

