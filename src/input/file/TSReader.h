/* TSReader.h

   Copyright (C) 2014 - 2019 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef INPUT_FILE_TSREADER_H_INCLUDE
#define INPUT_FILE_TSREADER_H_INCLUDE INPUT_FILE_TSREADER_H_INCLUDE

#include <FwDecl.h>
#include <input/Device.h>
#include <input/Transformation.h>
#include <input/file/TSReaderData.h>

#include <string>
#include <chrono>
#include <fstream>

FW_DECL_SP_NS2(input, file, TSReader);

FW_DECL_VECTOR_OF_SP_NS0(Stream);

namespace input {
namespace file {

/// The class @c TSReader is for reading from an TS files as input device
/// Some example for opening a TS file:
/// http://ip.of.your.box:8875/?msys=file&uri="test.ts"
class TSReader :
	public input::Device {
		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================

	public:

		TSReader(
			int streamID,
			const std::string &appDataPath);

		virtual ~TSReader();

		// =====================================================================
		//  -- Static member functions -----------------------------------------
		// =====================================================================

	public:

		///
		static void enumerate(
			StreamSpVector &streamVector,
			const std::string &appDataPath);

		// =====================================================================
		// -- base::XMLSupport -------------------------------------------------
		// =====================================================================

	public:
		///
		virtual void addToXML(std::string &xml) const override;

		///
		virtual void fromXML(const std::string &xml) override;


		// =====================================================================
		//  -- input::Device----------------------------------------------------
		// =====================================================================

	public:

		virtual void addDeliverySystemCount(
			std::size_t &dvbs2,
			std::size_t &dvbt,
			std::size_t &dvbt2,
			std::size_t &dvbc,
			std::size_t &dvbc2) override;

		virtual bool isDataAvailable() override;

		virtual bool readFullTSPacket(mpegts::PacketBuffer &buffer) override;

		virtual bool capableOf(input::InputSystem msys) const override;

		virtual bool capableToTransform(const std::string &msg, const std::string &method) const override;

		virtual void monitorSignal(bool showStatus) override;

		virtual bool hasDeviceDataChanged() const override;

		virtual void parseStreamString(const std::string &msg, const std::string &method) override;

		virtual bool update() override;

		virtual bool teardown() override;

		virtual std::string attributeDescribeString() const override;

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================

	protected:

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================

	private:
		std::ifstream _file;
		TSReaderData _deviceData;
		TSReaderData _transformDeviceData;
		input::Transformation _transform;

		std::chrono::steady_clock::time_point _t1;
		std::chrono::steady_clock::time_point _t2;
};

} // namespace file
} // namespace input

#endif // INPUT_FILE_TSREADER_H_INCLUDE
