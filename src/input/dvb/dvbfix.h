/* dvbfix.h

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
#ifndef DVB_FIX_H_INCLUDE
#define DVB_FIX_H_INCLUDE DVB_FIX_H_INCLUDE

#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>
#include <linux/dvb/dmx.h>

#define FULL_DVB_API_VERSION ((DVB_API_VERSION << 8) | DVB_API_VERSION_MINOR)

#if FULL_DVB_API_VERSION < 0x0500
#error Not correct DVB_API_VERSION should be >= 5.0
#endif

#if FULL_DVB_API_VERSION < 0x0505
#define DTV_ENUM_DELSYS       44
#define NOT_PREFERRED_DVB_API 1
#endif

#ifdef  DEFINE_SYS_DVBS2X
#define SYS_DVBS2X            static_cast<fe_delivery_system_t>(21)
#endif

#ifndef DTV_STREAM_ID
#define DTV_STREAM_ID         42
#endif

#ifndef DTV_SCRAMBLING_SEQUENCE_INDEX
#define DTV_SCRAMBLING_SEQUENCE_INDEX  70
#endif

// DMX_SET_SOURCE ioclt and dmx_source enum are removed from Kernel 4.14 and onwards
// Check commit 13adefbe9e566c6db91579e4ce17f1e5193d6f2c
#ifndef DMX_SET_SOURCE
enum dmx_source {
	DMX_SOURCE_FRONT0 = 0,
	DMX_SOURCE_FRONT1,
	DMX_SOURCE_FRONT2,
	DMX_SOURCE_FRONT3,
	DMX_SOURCE_DVR0   = 16,
	DMX_SOURCE_DVR1,
	DMX_SOURCE_DVR2,
	DMX_SOURCE_DVR3
};
#define DMX_SET_SOURCE _IOW('o', 49, enum dmx_source)
#endif

#endif // DVB_FIX_H_INCLUDE
