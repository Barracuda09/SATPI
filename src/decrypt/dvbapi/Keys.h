/* Keys.h

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
#ifndef DECRYPT_DVBAPI_KEYS_H_INCLUDE
#define DECRYPT_DVBAPI_KEYS_H_INCLUDE DECRYPT_DVBAPI_KEYS_H_INCLUDE

#include <FwDecl.h>
#include <base/TimeCounter.h>
#include <Log.h>

#include <utility>
#include <queue>

FW_DECL_NS0(dvbcsa_bs_key_s);

namespace decrypt::dvbapi {

///
class Keys {
	public:
		using KeyPair = std::pair<long, dvbcsa_bs_key_s *>;
		using KeyQueue = std::queue<KeyPair>;

		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:

		Keys() = default;

		virtual ~Keys() = default;

		// =====================================================================
		//  -- Other member functions ------------------------------------------
		// =====================================================================
	public:

		void set(const unsigned char *cw, int parity, int index);

		const dvbcsa_bs_key_s *get(int parity) const;

		void freeKeys();

	private:

		void remove(int parity);

		// =====================================================================
		//  -- Data members ----------------------------------------------------
		// =====================================================================
	private:

		KeyQueue _key[2];
};

}

#endif // DECRYPT_DVBAPI_KEYS_H_INCLUDE
