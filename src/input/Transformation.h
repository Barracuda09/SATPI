/* Transformation.h

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef INPUT_TRANSFORMATION_H_INCLUDE
#define INPUT_TRANSFORMATION_H_INCLUDE INPUT_TRANSFORMATION_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <base/Mutex.h>
#include <base/M3UParser.h>
#include <base/XMLSupport.h>
#include <input/InputSystem.h>
#include <input/dvb/FrontendData.h>

#include <string>

FW_DECL_NS0(TransportParamVector);
FW_DECL_NS1(input, DeviceData);

namespace input {

/// The class @c Transformation is an interface for transform
/// an input request to an different request red from an M3U file.
class Transformation :
	public base::XMLSupport  {
		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:

		Transformation(const std::string &appDataPath) :
			Transformation(appDataPath, input::InputSystem::UNDEFINED) {}

		Transformation(const std::string &appDataPath, input::InputSystem ownInputSystem);

		virtual ~Transformation() = default;

		// =====================================================================
		// -- base::XMLSupport -------------------------------------------------
		// =====================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	public:

		/// Check if transformation is enabled
		bool isEnabled() const;

		/// Check if we advertise this 'Frontend' as DVB-S2 tuner
		bool advertiseAsDVBS2() const;

		/// Check if we advertise this 'Frontend' as DVB-C tuner
		bool advertiseAsDVBC() const;

		///
		void resetTransformFlag();

		/// This function may return the input system for the
		/// requested input frequency and src input.
		input::InputSystem getTransformationSystemFor(const TransportParamVector& params) const;

		/// This function may return the transformed input message
		TransportParamVector transformStreamString(FeID id, const TransportParamVector& params);

		/// This function may return the transformed Device Data
		const DeviceData &transformDeviceData(const DeviceData &deviceData) const;

	private:

		/// This function will check if transformation is possible and returns
		/// the URI to use for the transformation
		std::string transformStreamPossible(FeID id,
			const TransportParamVector& params);

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		enum class AdvertiseAs {
			NONE,
			DVB_S2,
			DVB_C,
			DVB_T
		};
		base::Mutex _mutex;
		bool _enabled;
		bool _transform;
		bool _fileParsed;
		AdvertiseAs _advertiseAs;
		input::InputSystem _ownInputSystem;
		base::M3UParser _m3u;
		std::string _appDataPath;
		std::string _transformFileM3U;
		mutable input::dvb::FrontendData _transformedDeviceData;
		uint32_t _transformFreq;
		uint32_t _generatePSIFreq;
};

} // namespace input

#endif // INPUT_TRANSFORMATION_H_INCLUDE
