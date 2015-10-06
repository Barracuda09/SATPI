/* Functor2.h

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef FUNCTORBASE_H_INCLUDE
#define FUNCTORBASE_H_INCLUDE FUNCTORBASE_H_INCLUDE

#include <string.h>

/// The class @c FunctorBase
class FunctorBase {
	public:
		typedef void (FunctorBase::*MemberFunction) ();

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		FunctorBase() : _callback(NULL), _callee(NULL) {}

		FunctorBase(const void *callee, const void *callback, size_t Size)  {
			if (callee) {
				// callback must be a member function
				_callee = (void*)callee;
				memcpy(_callbackMember, callback, Size);
			} else {
				// Ok must be a regular pointer to a function
				_callback = callback;
			}
		}

		virtual ~FunctorBase() {}

		union {
			const void *_callback;
			char _callbackMember[sizeof(MemberFunction)];
		};
		void *_callee;
};

#endif // FUNCTORBASE_H_INCLUDE