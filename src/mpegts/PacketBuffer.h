/* PacketBuffer.h

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
#ifndef MPEGTS_PACKET_BUFFER_H_INCLUDE
#define MPEGTS_PACKET_BUFFER_H_INCLUDE MPEGTS_PACKET_BUFFER_H_INCLUDE

#include <cstdint>
#include <cstddef>

namespace mpegts {

	#define MTU                     1500
	#define RTP_HEADER_LEN            12
	#define TS_PACKET_SIZE           188
	#define NUMBER_OF_TS_PACKETS       7
	#define MTU_MAX_TS_PACKET_SIZE  TS_PACKET_SIZE * NUMBER_OF_TS_PACKETS

	class PacketBuffer {
		public:
			// =======================================================================
			// Constructors and destructor
			// =======================================================================
			PacketBuffer();
			virtual ~PacketBuffer();

			// =======================================================================
			// Other functions
			// =======================================================================

			/// Initialize this TS packet
			void initialize(uint32_t ssrc, long timestamp);

			/// Reset this TS packet
			void reset() {
				_decryptPending = false;
				_writeIndex = RTP_HEADER_LEN;
			}

			/// Check if these packets are in sync
			bool isSynced() const {
				return _buffer[RTP_HEADER_LEN + (TS_PACKET_SIZE * 0)] == 0x47 &&
					   _buffer[RTP_HEADER_LEN + (TS_PACKET_SIZE * 1)] == 0x47 &&
					   _buffer[RTP_HEADER_LEN + (TS_PACKET_SIZE * 2)] == 0x47;
			}

			/// try to sync this buffer
			bool trySyncing();

			/// This function will return the number of TS Packets that are
			/// in this TS Packet
			std::size_t getNumberOfTSPackets() const {
				return NUMBER_OF_TS_PACKETS;
			}

			/// get the amount of data that can be written to this TS packet
			std::size_t getBufferSize() const {
				return MTU_MAX_TS_PACKET_SIZE;
			}

			/// This will return the amount of bytes that (still) need to be written
			std::size_t getAmountOfBytesToWrite() const {
				return (MTU_MAX_TS_PACKET_SIZE + RTP_HEADER_LEN) - _writeIndex;
			}

			/// Add the amount of bytes written. So increment write index
			/// @param index specifies the amount written to TS packet
			void addAmountOfBytesWritten(std::size_t index) {
				_writeIndex += index;
			}

			/// Check if we have written all the buffer
			bool full() const {
				return (MTU_MAX_TS_PACKET_SIZE + RTP_HEADER_LEN) == _writeIndex;
			}

			/// Get the write buffer pointer for this TS packet
			unsigned char *getWriteBufferPtr() {
				return &_buffer[_writeIndex];
			}

			/// This function will return the begin of this RTP packet
			unsigned char *getReadBufferPtr() {
				return _buffer;
			}

			/// This function will return the begin of this TS packets
			unsigned char *getTSReadBufferPtr() {
				return &_buffer[RTP_HEADER_LEN];
			}

			/// Get the TS packet pointer for packets 0 up until NUMBER_OF_TS_PACKETS
			/// @param packetNumber a value from 0 up until NUMBER_OF_TS_PACKETS
			unsigned char *getTSPacketPtr(std::size_t packetNumber) {
				const std::size_t index = (packetNumber * TS_PACKET_SIZE) + RTP_HEADER_LEN;
				return &_buffer[index];
			}
			const unsigned char *getTSPacketPtr(std::size_t packetNumber) const {
				const std::size_t index = (packetNumber * TS_PACKET_SIZE) + RTP_HEADER_LEN;
				return &_buffer[index];
			}

			/// Set the decrypt pending flag, so we should check scramble flag if this
			/// buffer is ready for sending
			void setDecryptPending() {
				_decryptPending = true;
			}

			/// This function checks if this TS packet is ready to be send.
			/// When the pending decrypt flag was set, all scramble flags should be cleared from all TS packets.
			bool isReadyToSend() const {
				// can only be ready when buffer is full, so start from there
				bool ready = full();
				if (_decryptPending && ready) {
					for (std::size_t i = 0; i < NUMBER_OF_TS_PACKETS; ++i) {
						const unsigned char *ts = getTSPacketPtr(i);
						ready &= ((ts[3] & 0x80) != 0x80);
					}
				}
				return ready;
			}

			// ================================================================
			//  -- Data members -----------------------------------------------
			// ================================================================

		protected:
			unsigned char _buffer[MTU];
			std::size_t   _writeIndex;
			bool          _initialized;
			bool          _decryptPending;

	};

} // namespace mpegts

#endif // MPEGTS_PACKET_BUFFER_H_INCLUDE
