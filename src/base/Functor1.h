/* Functor1.h

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
#ifndef BASE_FUNCTOR1_H_INCLUDE
#define BASE_FUNCTOR1_H_INCLUDE BASE_FUNCTOR1_H_INCLUDE

#include <base/FunctorBase.h>

namespace base {

/// The class @c Functor1 is functor with 1 argument with no return
template <class P1>
class Functor1 : protected FunctorBase {
	public:

		void operator() (P1 p1) const {
			_functionFunctor(*this, p1);
		}

	protected:
		typedef void (*FunctionFunctor)(const FunctorBase &, P1);

		// =======================================================================
		// -- Constructors and destructor ----------------------------------------
		// =======================================================================
		Functor1(FunctionFunctor ff, const void *callee, const void *callback, size_t sz) :
			FunctorBase(callee, callback, sz),
			_functionFunctor(ff) {}

	public:
		virtual ~Functor1() {}

	private:
		FunctionFunctor _functionFunctor;
};

template <class P1, class FUNC>
class FunctionWrapper1 : public Functor1<P1> {
	public:
		FunctionWrapper1(FUNC func) :
			Functor1<P1>(functionFunctor, nullptr, (void *)func, 0) {}

		static void functionFunctor(const FunctorBase &ftor, P1 p1) {
			(FUNC(ftor._callback))(p1);
		}
};

template <class P1, class CALLEE, class FUNC>
class MemberFunctionWrapper1 : public Functor1<P1> {
	public:
		MemberFunctionWrapper1(CALLEE &callee, const FUNC &func) :
			Functor1<P1>(functionFunctor, &callee, &func, sizeof(FUNC)) {}

		static void functionFunctor(const FunctorBase &ftor, P1 p1) {
			CALLEE *callee = (CALLEE *)ftor._callee;
			FUNC &callback(*(FUNC*)(void *)(ftor._callbackMember));
			(callee->*callback)(p1);
		}
};

template <class P1, class RETURN, class TP1>
inline FunctionWrapper1<P1, RETURN (*)(TP1)>
makeFunctor(Functor1<P1>*, RETURN (*f)(TP1)) {
	return FunctionWrapper1<P1, RETURN (*)(TP1)>(f);
}

template <class P1, class CALLEE, class RETURN, class CALL_TYPE, class TP1>
inline MemberFunctionWrapper1<P1, CALLEE, RETURN (CALL_TYPE::*)(TP1)>
makeFunctor(Functor1<P1>*, CALLEE &callee, RETURN (CALL_TYPE::* const &func)(TP1)) {
	typedef RETURN (CALL_TYPE::*FUNC)(TP1);
	return MemberFunctionWrapper1<P1, CALLEE, FUNC>(callee, func);
}

template <class P1, class CALLEE, class RETURN, class CALL_TYPE, class TP1>
inline MemberFunctionWrapper1<P1, const CALLEE, RETURN (CALL_TYPE::*)(TP1) const>
makeFunctor(Functor1<P1>*, const CALLEE &callee, RETURN (CALL_TYPE::* const &func)(TP1) const) {
	typedef RETURN (CALL_TYPE::*FUNC)(TP1) const;
	return MemberFunctionWrapper1<P1, const CALLEE, FUNC>(callee, func);
}

} // namespace base
#endif // BASE_FUNCTOR1_H_INCLUDE
