/* TSPacketBuffer.cpp

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
#include "TSPacketBuffer.h"

#include <assert.h>

static_assert(MTU_MAX_TS_PACKET_SIZE < MTU, "TS Packet size bigger then MTU");

TSPacketBuffer::TSPacketBuffer() :
		_writeIndex(0),
		_initialized(false),
		_decryptPending(false) {
}

TSPacketBuffer::~TSPacketBuffer() {}

void TSPacketBuffer::initialize(uint32_t ssrc, long timestamp) {
	// initialize RTP header
	_buffer[0]  = 0x80;                         // version: 2, padding: 0, extension: 0, CSRC: 0
	_buffer[1]  = 33;                           // marker: 0, payload type: 33 (MP2T)
	_buffer[2]  = (0 >> 8) & 0xff;              // sequence number
	_buffer[3]  = (0 >> 0) & 0xff;              // sequence number
	_buffer[4]  = (timestamp >> 24) & 0xff;     // timestamp
	_buffer[5]  = (timestamp >> 16) & 0xff;     // timestamp
	_buffer[6]  = (timestamp >>  8) & 0xff;     // timestamp
	_buffer[7]  = (timestamp >>  0) & 0xff;     // timestamp
	_buffer[8]  = (ssrc >> 24) & 0xff;          // synchronization source
	_buffer[9]  = (ssrc >> 16) & 0xff;          // synchronization source
	_buffer[10] = (ssrc >>  8) & 0xff;          // synchronization source
	_buffer[11] = (ssrc >>  0) & 0xff;          // synchronization source

	_initialized = true;
}
