/* TransportParamVector.h

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
#ifndef TRANSPORTPARAMVECTORS_H_INCLUDE
#define TRANSPORTPARAMVECTORS_H_INCLUDE TRANSPORTPARAMVECTORS_H_INCLUDE

#include <Defs.h>
#include <input/InputSystem.h>

#include <string_view>

class TransportParamVector {
		// =========================================================================
		// -- Constructors and destructor ------------------------------------------
		// =========================================================================
	public:

		explicit TransportParamVector(StringVector&& vector) : _vector(std::move(vector)) {}
		virtual ~TransportParamVector() = default;

		// =========================================================================
		//  -- Other member functions ----------------------------------------------
		// =========================================================================
	public:

		template< class... Args >
		StringVector::reference emplace_back( Args&&... args ) {
			return _vector.emplace_back(std::forward<Args>(args)...);
		}
		StringVector::size_type size() const noexcept {
			return _vector.size();
		}
		StringVector::reference operator[]( StringVector::size_type pos ) {
			return _vector[pos];
		}
		StringVector::const_reference operator[](StringVector::size_type pos ) const {
			return _vector[pos];
		}
		StringVector::iterator begin() noexcept {
			return _vector.begin();
		}
		StringVector::const_iterator begin() const noexcept {
			return _vector.begin();
		}
		StringVector::const_iterator cbegin() const noexcept {
			return _vector.cbegin();
		}
		StringVector::iterator end() noexcept {
			return _vector.end();
		}
		StringVector::const_iterator end() const noexcept {
			return _vector.end();
		}
		StringVector::const_iterator cend() const noexcept {
			return _vector.cend();
		}
		const StringVector& asStringVector() const {
			return _vector;
		}

		///
		std::string getParameter(std::string_view parameter) const;

		///
		void replaceParameter(const std::string_view parameter, const std::string_view value);

		///
		double getDoubleParameter(std::string_view parameter) const;

		///
		int getIntParameter(std::string_view parameter) const;

		///
		input::InputSystem getMSYSParameter() const;

		///
		std::string getURIParameter(const std::string_view parameter) const;

		// =========================================================================
		// -- Data members ---------------------------------------------------------
		// =========================================================================
	private:

		StringVector _vector;
};

#endif // TRANSPORTPARAMVECTORS_H_INCLUDE
