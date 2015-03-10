/* dvbfix.h

   Copyright (C) 2014 Marc Postema (m.a.postema -at- alice.nl)

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
#define DVB_FIX_H_INCLUDE

#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>

#define FULL_DVB_API_VERSION (DVB_API_VERSION << 8 | DVB_API_VERSION_MINOR)

#if FULL_DVB_API_VERSION < 0x0500
#error Not correct DVB_API_VERSION
#endif

#if FULL_DVB_API_VERSION < 0x0505
#define DTV_ENUM_DELSYS     44
#define SYS_DVBC_ANNEX_A    SYS_DVBC_ANNEX_AC
#define SYS_DVBC_ANNEX_C    18
#define NOT_PREFERRED_DVB_API 1
#endif

#endif // DVB_FIX_H_INCLUDE