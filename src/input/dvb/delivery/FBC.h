/* FBC.h

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
#ifndef INPUT_DVB_DELIVERY_FBC_H_INCLUDE
#define INPUT_DVB_DELIVERY_FBC_H_INCLUDE INPUT_DVB_DELIVERY_FBC_H_INCLUDE

#include <Defs.h>
#include <FwDecl.h>
#include <base/XMLSupport.h>
#include <input/dvb/delivery/System.h>

#include <string>
#include <map>

namespace input::dvb::delivery {

/// The class @c FBC specifies tuners that support FBC delivery system
class FBC :
	public base::XMLSupport {
		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		FBC(const FeIndex index, const FeID id, const std::string& name, bool satTuner);
		virtual ~FBC() = default;

		// =========================================================================
		// -- base::XMLSupport -----------------------------------------------------
		// =========================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =========================================================================
		// -- Other member functions -----------------------------------------------
		// =========================================================================
	public:

		bool isFBCTuner() const noexcept {
			return _fbcTuner;
		}

		bool doSendDiSEqcViaRootTuner() const noexcept {
			return _fbcTuner && _fbcLinked && _satTuner && _sendDiSEqcViaRootTuner;
		}

		int getFileDescriptorOfRootTuner(std::string& fePath) const;

	private:

		///
		int readProcData(FeIndex index, const std::string& procEntry) const;

		///
		void writeProcData(FeIndex index, const std::string& procEntry, int value);

		///
		void readConnectionChoices(FeIndex index, int offset);

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

		FeIndex _index;
		FeID _id;
		std::string _name;
		bool _satTuner;
		using ConnectionChoices = std::map<int, std::string>;
		ConnectionChoices _choices;
		bool _fbcTuner;
		bool _fbcRoot;
		int _fbcSetID;
		int _fbcConnect;
		int _offset;
		char _tunerLetter;
		bool _fbcLinked;
		bool _sendDiSEqcViaRootTuner;

};

}

#endif
