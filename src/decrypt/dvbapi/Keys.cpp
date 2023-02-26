/* Keys.cpp

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
#include <decrypt/dvbapi/Keys.h>

#include <Unused.h>

extern "C" {
	#include <dvbcsa/dvbcsa.h>
#ifdef ICAM
	void dvbcsa_bs_key_set_ecm(unsigned char ecm, const dvbcsa_cw_t cw, struct dvbcsa_bs_key_s *key);
#endif
}

namespace decrypt::dvbapi {

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

void Keys::set(const unsigned char *cw, int parity, int UNUSED(index)) {
	dvbcsa_bs_key_s *k = dvbcsa_bs_key_alloc();
#ifdef ICAM
	const unsigned char icamECM = (_icam[parity].size() > 0) ? _icam[parity].back() : 0;
	dvbcsa_bs_key_set_ecm(icamECM, cw, k);
#else
	dvbcsa_bs_key_set(cw, k);
#endif
	_key[parity].push(std::make_tuple(base::TimeCounter::getTicks(), k));
	while (_key[parity].size() > 1) {
		remove(parity);
	}
}

void Keys::setICAM(const unsigned char ecm, int parity) {
	_icam[parity].push(ecm);
	while (_icam[parity].size() > 1) {
		_icam[parity].pop();
	}
}

const dvbcsa_bs_key_s * Keys::get(int parity) const {
	if (!_key[parity].empty()) {
		const KeyTuple tup = _key[parity].back();
//		const long duration = base::TimeCounter::getTicks() - pair.first;
		return std::get<1>(tup);
	} else {
		return nullptr;
	}
}

void Keys::freeKeys() {
	while (!_key[0].empty()) {
		remove(0);
	}
	while (!_key[1].empty()) {
		remove(1);
	}
	while (_icam[0].size() > 1) {
		_icam[0].pop();
	}
	while (_icam[1].size() > 1) {
		_icam[1].pop();
	}
}

void Keys::remove(int parity) {
	const KeyTuple tup = _key[parity].front();
	dvbcsa_bs_key_free(std::get<1>(tup));
	_key[parity].pop();
}

}
