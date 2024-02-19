/* Keys.h

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
#ifndef DECRYPT_DVBAPI_KEYS_H_INCLUDE
#define DECRYPT_DVBAPI_KEYS_H_INCLUDE DECRYPT_DVBAPI_KEYS_H_INCLUDE

#include <FwDecl.h>
#include <base/TimeCounter.h>
#include <Log.h>

#include <utility>
#include <queue>
#include <tuple>

FW_DECL_NS0(dvbcsa_bs_key_s);

namespace decrypt::dvbapi {

///
class Keys {
	public:
		using KeyTuple = std::tuple<long, dvbcsa_bs_key_s *>;
		using KeyQueue = std::queue<KeyTuple>;
		using ICAMQueue = std::queue<unsigned char>;

		// =========================================================================
		//  -- Constructors and destructor -----------------------------------------
		// =========================================================================
	public:

		Keys() = default;

		virtual ~Keys() = default;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		void set(const unsigned char* cw, unsigned int parity, int index, bool icamEnabled);

		void setICAM(const unsigned char ecm, unsigned int parity);

		const dvbcsa_bs_key_s *get(unsigned int parity) const;

		void freeKeys();

	private:

		void remove(unsigned int parity);

		// =====================================================================
		//  -- Data members ----------------------------------------------------
		// =====================================================================
	private:

		KeyQueue _key[2];
		ICAMQueue _icam[2];
};

}

#endif // DECRYPT_DVBAPI_KEYS_H_INCLUDE
