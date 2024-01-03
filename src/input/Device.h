/* Device.h

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
#ifndef INPUT_DEVICE_H_INCLUDE
#define INPUT_DEVICE_H_INCLUDE INPUT_DEVICE_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <base/XMLSupport.h>
#include <input/InputSystem.h>
#include <mpegts/Filter.h>

#include <string>
#include <utility>

FW_DECL_NS0(TransportParamVector);
FW_DECL_NS1(mpegts, PacketBuffer);

FW_DECL_SP_NS1(input, Device);

namespace input {

/// The class @c Device is an interface to some input device.
/// For example an frontend with DVB-S2 or File input etc.
class Device :
	public base::XMLSupport {
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		Device(FeIndex index) : _index(index), _feID(index.getID() + 1), _streamID(index.getID() + 100) {}

		virtual ~Device() = default;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		/// Add the amount of delivery systems of this device to the
		/// specified parameters
		virtual void addDeliverySystemCount(
			std::size_t &dvbs2,
			std::size_t &dvbt,
			std::size_t &dvbt2,
			std::size_t &dvbc,
			std::size_t &dvbc2) = 0;

		/// Check if there is data to be red from this device
		virtual bool isDataAvailable() = 0;

		/// Read the available data from this device
		/// @param buffer this is the buffer were to wirite to
		virtual bool readTSPackets(mpegts::PacketBuffer& buffer) = 0;

		/// Check the capability of this device
		/// @param system specifies the input system that this device is capable of
		virtual bool capableOf(input::InputSystem system) const = 0;

		/// Check if this device can be shared
		/// @param params
		virtual bool capableToShare(const TransportParamVector& params) const = 0;

		/// Check if this device can transform the reguest according to an M3U file
		/// This function should be called after function @c capableOf
		/// @param params
		virtual bool capableToTransform(const TransportParamVector& params) const = 0;

		/// Check if this device is already claimed/opened by an other process.
		/// @return true meaning the device is opened by an other process
		virtual bool isLockedByOtherProcess() const = 0;

		/// Monitor signal of this device
		/// @return true meaning there is a Signal Lock
		virtual bool monitorSignal(bool showStatus) = 0;

		/// This indicates that the frequency (or pol) has changed, so some actions
		/// are required
		/// @return true meaning the frequency (or pol) has changed
		virtual bool hasDeviceFrequencyChanged() const = 0;

		/// Parse the input/request string from client.
		///   For example:
		///   rtsp://ip.of.your.box/?fe=3&freq=170&sr=6900&msys=dvbc&mtype=256qam&fec=35&addpids=0,1,16,17
		/// @param params
		virtual void parseStreamString(const TransportParamVector& params) = 0;

		/// Update the Channel and PID. Will close DVR and reopen it if channel did change
		virtual bool update() = 0;

		/// Teardown/Stop this device
		virtual bool teardown() = 0;

		///
		virtual std::string attributeDescribeString() const = 0;

		///
		virtual mpegts::Filter &getFilter() = 0;

		/// Generic pid filtering Update function
		virtual void updatePIDFilters() {
			getFilter().updatePIDFilters(_feID,
				// openPid lambda function
				[&](const int) {
					return true;
				},
				// closePid lambda function
				[&](const int) {
					return true;
				});
		}

		/// Generic pid filtering Close function
		virtual void closeActivePIDFilters() {
			getFilter().closeActivePIDFilters(_feID,
				// closePid lambda function
				[&](const int) {
					return true;
				});
		}

		///
		FeID getFeID() const {
			return _feID;
		}

		///
		FeIndex getFeIndex() const {
			return _index;
		}

		///
		StreamID getStreamID() const {
			return _streamID;
		}

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	protected:

		FeIndex _index;
		FeID _feID;
		StreamID _streamID;
};

}

#endif // INPUT_DEVICE_H_INCLUDE
