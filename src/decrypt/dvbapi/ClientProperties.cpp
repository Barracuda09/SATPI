/* ClientProperties.cpp

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
#include <decrypt/dvbapi/ClientProperties.h>

#include <Utils.h>
#include <Unused.h>

#include <dlfcn.h>

namespace decrypt::dvbapi {

// ===========================================================================
// -- Constructors and destructor --------------------------------------------
// ===========================================================================

ClientProperties::ClientProperties() {
	_batchSizeMax = dvbcsa_bs_batch_size();
	_batchSize = _batchSizeMax;
	_batch = new dvbcsa_bs_batch_s[_batchSizeMax + 1];
	_ts = new dvbcsa_bs_batch_s[_batchSizeMax + 1];
	_batchCount = 0;
	_parity = 0;
	void* handle = dlopen("libdvbcsa.so.1", RTLD_LAZY | RTLD_NODELETE);
	if (handle != nullptr) {
		if (dlsym(handle, "dvbcsa_bs_key_set_ecm") == nullptr) {
			SI_LOG_INFO("  ICAM support in libdvbcsa.so: No");
			_icamEnabled = false;
		} else {
			SI_LOG_INFO("  ICAM support in libdvbcsa.so: Yes");
			_icamEnabled = true;
		}
		dlclose(handle);
	}
}

ClientProperties::~ClientProperties() {
	DELETE_ARRAY(_batch);
	DELETE_ARRAY(_ts);
	_keys.freeKeys();
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void ClientProperties::doAddToXML(std::string& xml) const {
	ADD_XML_NUMBER_INPUT(xml, "dvbcsa_bs_batch_size", _batchSize, 0, _batchSizeMax);
	ADD_XML_ELEMENT(xml, "icamEnabled", _icamEnabled ? "Yes" : "No");
}

void ClientProperties::doFromXML(const std::string& UNUSED(xml)) {
}

// ===========================================================================
// -- Other member functions -------------------------------------------------
// ===========================================================================

void ClientProperties::stopOSCamFilters(FeID id) {
	SI_LOG_INFO("Frontend: @#1, Clearing OSCam filters and Keys...", id);
	// free keys
	_keys.freeKeys();
	_batchCount = 0;
	_parity = 0;
	_filter.clear();
}

void ClientProperties::decryptBatch() noexcept {
	const auto key = _keys.get(_parity);
	if (key != nullptr) {
		// terminate batch buffer
		setBatchData(nullptr, 0, _parity, nullptr);
		// decrypt it
		dvbcsa_bs_decrypt(key, _batch, 184);

		// clear scramble flags, so we can send it.
		for (unsigned int i = 0; _ts[i].data != nullptr; ++i) {
			_ts[i].data[3] &= 0x3F;
		}
	} else {
		for (unsigned int i = 0; _ts[i].data != nullptr; ++i) {
			// set decrypt failed by setting NULL packet ID..
			_ts[i].data[1] |= 0x1F;
			_ts[i].data[2] |= 0xFF;

			// clear scramble flag, so we can send it.
			_ts[i].data[3] &= 0x3F;
		}
	}
	// decrypted this batch reset counter
	_batchCount = 0;
}

void ClientProperties::setECMInfo(
	int UNUSED(pid),
	int UNUSED(serviceID),
	int UNUSED(caID),
	int UNUSED(provID),
	int UNUSED(emcTime),
	const std::string& UNUSED(cardSystem),
	const std::string& UNUSED(readerName),
	const std::string& UNUSED(sourceName),
	const std::string& UNUSED(protocolName),
	int UNUSED(hops)) {
}

}
