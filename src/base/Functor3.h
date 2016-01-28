/* Functor3.h

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef BASE_FUNCTOR3_H_INCLUDE
#define BASE_FUNCTOR3_H_INCLUDE BASE_FUNCTOR3_H_INCLUDE

#include <base/FunctorBase.h>

namespace base {

/// The class @c Functor3 is functor with 3 argument with no return
template <class P1, class P2, class P3>
class Functor3 : protected FunctorBase {
	public:

		void operator() (P1 p1, P2 p2, P3 p3) const {
			_functionFunctor(*this, p1, p2, p3);
		}

	protected:
		typedef void (*FunctionFunctor)(const FunctorBase &, P1, P2, P3);

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		Functor3(FunctionFunctor ff, const void *callee, const void *callback, size_t sz) :
			FunctorBase(callee, callback, sz),
			_functionFunctor(ff) {}

	public:
		virtual ~Functor3() {}

	private:
		FunctionFunctor _functionFunctor;
};

template <class P1, class P2, class P3, class FUNC>
class FunctionWrapper3 : public Functor3<P1, P2, P3> {
	public:
		FunctionWrapper3(FUNC func) :
			Functor3<P1, P2, P3>(functionFunctor, nullptr, (void *)func, 0) {}

		static void functionFunctor(const FunctorBase &ftor, P1 p1, P2 p2, P3 p3) {
			(FUNC(ftor._callback))(p1, p2, p3);
		}
};

template <class P1, class P2, class P3, class CALLEE, class FUNC>
class MemberFunctionWrapper3 : public Functor3<P1, P2, P3> {
	public:
		MemberFunctionWrapper3(CALLEE &callee, const FUNC &func) :
			Functor3<P1, P2, P3>(functionFunctor, &callee, &func, sizeof(FUNC)) {}

		static void functionFunctor(const FunctorBase &ftor, P1 p1, P2 p2, P3 p3) {
			CALLEE *callee = (CALLEE *)ftor._callee;
			FUNC &callback(*(FUNC*)(void *)(ftor._callbackMember));
			(callee->*callback)(p1, p2, p3);
		}
};

template <class P1, class P2, class P3, class RETURN, class TP1, class TP2, class TP3>
inline FunctionWrapper3<P1, P2, P3, RETURN (*)(TP1, TP2, TP3)>
makeFunctor(Functor3<P1, P2, P3>*, RETURN (*f)(TP1, TP2, TP3)) {
	return FunctionWrapper3<P1, P2, P3, RETURN (*)(TP1, TP2, TP3)>(f);
}

template <class P1, class P2, class P3, class CALLEE, class RETURN, class CALL_TYPE, class TP1, class TP2, class TP3>
inline MemberFunctionWrapper3<P1, P2, P3, CALLEE, RETURN (CALL_TYPE::*)(TP1, TP2, TP3)>
makeFunctor(Functor3<P1, P2, P3>*, CALLEE &callee, RETURN (CALL_TYPE::* const &func)(TP1, TP2, TP3)) {
	typedef RETURN (CALL_TYPE::*FUNC)(TP1, TP2, TP3);
	return MemberFunctionWrapper3<P1, P2, P3, CALLEE, FUNC>(callee, func);
}

template <class P1, class P2, class P3, class CALLEE, class RETURN, class CALL_TYPE, class TP1, class TP2, class TP3>
inline MemberFunctionWrapper3<P1, P2, P3, const CALLEE, RETURN (CALL_TYPE::*)(TP1, TP2, TP3) const>
makeFunctor(Functor3<P1, P2, P3>*, const CALLEE &callee, RETURN (CALL_TYPE::* const &func)(TP1, TP2, TP3) const) {
	typedef RETURN (CALL_TYPE::*FUNC)(TP1, TP2, TP3) const;
	return MemberFunctionWrapper3<P1, P2, P3, const CALLEE, FUNC>(callee, func);
}

} // namespace base

#endif // BASE_FUNCTOR3_H_INCLUDE
