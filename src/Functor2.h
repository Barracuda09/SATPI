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
#ifndef FUNCTOR2_H_INCLUDE
#define FUNCTOR2_H_INCLUDE FUNCTOR2_H_INCLUDE

#include "FunctorBase.h"

/// The class @c Functor2 is functor with 2 argument with no return
template <class P1, class P2>
class Functor2 : protected FunctorBase {
	public:

		void operator() (P1 p1, P2 p2) const {
			_functionFunctor(*this, p1, p2);
		}

	protected:
		typedef void (*FunctionFunctor)(const FunctorBase &, P1, P2);

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		Functor2(FunctionFunctor ff, const void *callee, const void *callback, size_t sz) :
			FunctorBase(callee, callback, sz),
			_functionFunctor(ff) {}

	public:
		virtual ~Functor2() {}

	private:
		FunctionFunctor _functionFunctor;
};

template <class P1, class P2, class FUNC>
class FunctionWrapper2 : public Functor2<P1, P2> {
	public:
		FunctionWrapper2(FUNC func) :
			Functor2<P1, P2>(functionFunctor, nullptr, (void *)func, 0) {}

		static void functionFunctor(const FunctorBase &ftor, P1 p1, P2 p2) {
			(FUNC(ftor._callback))(p1, p2);
		}
};

template <class P1, class P2, class CALLEE, class FUNC>
class MemberFunctionWrapper2 : public Functor2<P1, P2> {
	public:
		MemberFunctionWrapper2(CALLEE &callee, const FUNC &func) :
			Functor2<P1, P2>(functionFunctor, &callee, &func, sizeof(FUNC)) {}

		static void functionFunctor(const FunctorBase &ftor, P1 p1, P2 p2) {
			CALLEE *callee = (CALLEE *)ftor._callee;
			FUNC &callback(*(FUNC*)(void *)(ftor._callbackMember));
			(callee->*callback)(p1, p2);
		}
};

template <class P1, class P2, class RETURN, class TP1, class TP2>
inline FunctionWrapper2<P1, P2, RETURN (*)(TP1, TP2)>
makeFunctor(Functor2<P1, P2>*, RETURN (*f)(TP1, TP2)) {
	return FunctionWrapper2<P1, P2, RETURN (*)(TP1, TP2)>(f);
}

template <class P1, class P2, class CALLEE, class RETURN, class CALL_TYPE, class TP1, class TP2>
inline MemberFunctionWrapper2<P1, P2, CALLEE, RETURN (CALL_TYPE::*)(TP1, TP2)>
makeFunctor(Functor2<P1, P2>*, CALLEE &callee, RETURN (CALL_TYPE::* const &func)(TP1, TP2)) {
	typedef RETURN (CALL_TYPE::*FUNC)(TP1, TP2);
	return MemberFunctionWrapper2<P1, P2, CALLEE, FUNC>(callee, func);
}

template <class P1, class P2, class CALLEE, class RETURN, class CALL_TYPE, class TP1, class TP2>
inline MemberFunctionWrapper2<P1, P2, const CALLEE, RETURN (CALL_TYPE::*)(TP1, TP2) const>
makeFunctor(Functor2<P1, P2>*, const CALLEE &callee, RETURN (CALL_TYPE::* const &func)(TP1, TP2) const) {
	typedef RETURN (CALL_TYPE::*FUNC)(TP1, TP2) const;
	return MemberFunctionWrapper2<P1, P2, const CALLEE, FUNC>(callee, func);
}

#endif // FUNCTOR2_H_INCLUDE