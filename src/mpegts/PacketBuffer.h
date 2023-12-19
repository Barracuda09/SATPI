/* PacketBuffer.h

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
#ifndef MPEGTS_PACKET_BUFFER_H_INCLUDE
#define MPEGTS_PACKET_BUFFER_H_INCLUDE MPEGTS_PACKET_BUFFER_H_INCLUDE

#include <cstdint>
#include <cstddef>

namespace mpegts {

class PacketBuffer {
		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:
		PacketBuffer() = default;

		virtual ~PacketBuffer() = default;

		// =====================================================================
		// -- Other functions --------------------------------------------------
		// =====================================================================
	public:

		/// Initialize this TS buffer
		void initialize(uint32_t ssrc, long timestamp) noexcept;

		/// get the amount of data that CAN be written to this TS buffer
		static constexpr std::size_t getMaxBufferSize() noexcept {
			return MTU_MAX_TS_PACKET_SIZE;
		}

		/// Check if we have written all of the TS Packets
		bool full() const noexcept {
			return (MTU_MAX_TS_PACKET_SIZE + RTP_HEADER_LEN) == _writeIndex;
		}

		/// Check if we have written all of the TS Packets
		bool empty() const noexcept {
			return RTP_HEADER_LEN == _writeIndex;
		}

		/// Reset this TS buffer
		void reset() noexcept {
			_decryptPending = false;
			_purgePending = 0;
			_writeIndex = RTP_HEADER_LEN;
			_processedIndex = RTP_HEADER_LEN;
		}

		/// try to sync this buffer
		bool trySyncing() noexcept;

		/// Mark one TS packet for purging (remove) by setting 0xFF _after_ the first SYNC Byte.
		/// @param packetNumber a value from 0 up until NUMBER_OF_TS_PACKETS
		void markTSForPurging(std::size_t packetNumber) noexcept;

		/// Purge (remove) marked filtered TS packets
		void purge() noexcept;

		/// This will tag the RTP header with sequence number and timestamp
		void tagRTPHeaderWith(uint32_t ssrc, uint16_t cseq, long timestamp) noexcept;

		/// This function will return the maximum number of TS Packets that will fit
		/// in this TS buffer
		static constexpr std::size_t getMaxNumberOfTSPackets() noexcept {
			return NUMBER_OF_TS_PACKETS;
		}

		/// This function will return the number of completed TS Packets that are
		/// in this TS buffer
		std::size_t getNumberOfCompletedPackets() const noexcept {
			if (full()) {
				return NUMBER_OF_TS_PACKETS;
			}
			return (_writeIndex - RTP_HEADER_LEN) / TS_PACKET_SIZE;
		}

		/// This function will return the first un-filtered TS Packets AND RESETS to
		/// be processed.
		std::size_t getBeginOfUnFilteredPackets() const noexcept {
			if (_processedIndex == RTP_HEADER_LEN) {
				return 0;
			}
			return (_processedIndex - RTP_HEADER_LEN) / TS_PACKET_SIZE;
		}

		/// get the amount of data that is in this TS buffer
		std::size_t getCurrentBufferSize() const noexcept {
			return _writeIndex - RTP_HEADER_LEN;
		}

		/// This will return the amount of bytes that can still be written to
		/// this TS buffer
		std::size_t getAmountOfBytesToWrite() const noexcept {
			return (MTU_MAX_TS_PACKET_SIZE + RTP_HEADER_LEN) - _writeIndex;
		}

		/// Add the amount of bytes written, by increment the write index
		/// @param index specifies the amount written to TS packet
		void addAmountOfBytesWritten(std::size_t index) noexcept {
			_writeIndex += index;
		}

		/// Get the write pointer for this TS buffer
		unsigned char* getWriteBufferPtr() noexcept {
			return &_buffer[_writeIndex];
		}

		/// This function will return the begin of this RTP packet
		unsigned char *getReadBufferPtr() noexcept {
			return _buffer;
		}

		/// This function will return the begin of the first TS packet in this TS buffer
		/// so without RTP header
		unsigned char* getTSReadBufferPtr() noexcept {
			return &_buffer[RTP_HEADER_LEN];
		}

		/// Get the TS packet pointer for packets 0 up until NUMBER_OF_TS_PACKETS
		/// @param packetNumber a value from 0 up until NUMBER_OF_TS_PACKETS
		unsigned char* getTSPacketPtr(std::size_t packetNumber) noexcept {
			const std::size_t index = (packetNumber * TS_PACKET_SIZE) + RTP_HEADER_LEN;
			return &_buffer[index];
		}
		const unsigned char* getTSPacketPtr(std::size_t packetNumber) const noexcept {
			const std::size_t index = (packetNumber * TS_PACKET_SIZE) + RTP_HEADER_LEN;
			return &_buffer[index];
		}

		/// Set the decrypt pending flag, so we should check scramble flag if this
		/// buffer is ready for sending
		void setDecryptPending() noexcept {
			_decryptPending = true;
		}

		/// This function checks if this TS buffer is ready to be send.
		/// There should be something in the buffer, in TS_PACKET_SIZE chucks.
		/// When the pending decrypt flag was set, all scramble flags should
		/// be cleared from all TS packets.
		bool isReadyToSend() const noexcept {
			// ready to send, when there is something in the buffer in TS_PACKET_SIZE chucks
			// and if all scramble flags are cleared
//			bool ready = (getCurrentBufferSize() % TS_PACKET_SIZE) == 0;
			bool ready = full();
			if (_decryptPending && ready) {
				const std::size_t size = getNumberOfCompletedPackets();
				for (std::size_t i = 0; i < size; ++i) {
					const unsigned char* ts = getTSPacketPtr(i);
					ready &= ((ts[3] & 0x80) != 0x80);
				}
			}
			return ready;
		}

	protected:

		/// Check if the first three TS packets are in sync
		bool isSynced() const noexcept {
			return
				_buffer[RTP_HEADER_LEN + (TS_PACKET_SIZE * 0)] == 0x47 &&
				_buffer[RTP_HEADER_LEN + (TS_PACKET_SIZE * 1)] == 0x47 &&
				_buffer[RTP_HEADER_LEN + (TS_PACKET_SIZE * 2)] == 0x47;
		}

		// =====================================================================
		//  -- Data members ----------------------------------------------------
		// =====================================================================
	public:

		static constexpr size_t MTU                    = 1500;
		static constexpr size_t RTP_HEADER_LEN         =   12;
		static constexpr size_t TS_PACKET_SIZE         =  188;
		static constexpr size_t NUMBER_OF_TS_PACKETS   =    7;
		static constexpr size_t MTU_MAX_TS_PACKET_SIZE = TS_PACKET_SIZE * NUMBER_OF_TS_PACKETS;

	protected:

		unsigned char       _buffer[MTU];
		std::size_t         _writeIndex = RTP_HEADER_LEN;
		mutable std::size_t _processedIndex = RTP_HEADER_LEN;
		bool                _decryptPending = false;
		std::size_t         _purgePending = 0;

};

}

#endif // MPEGTS_PACKET_BUFFER_H_INCLUDE
