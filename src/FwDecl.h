/* FwDecl.h

   Copyright (C) 2014 - 2020 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef FW_DECL_H_INCLUDE
#define FW_DECL_H_INCLUDE FW_DECL_H_INCLUDE

#include <memory>
#include <vector>

#define FW_DECL_NS0(CLASS) \
	class CLASS

#define FW_DECL_NS1(NS1, CLASS) \
namespace NS1 { \
	class CLASS; \
}

#define FW_DECL_NS2(NS1, NS2, CLASS) \
namespace NS1 { \
namespace NS2 { \
	class CLASS; \
} \
}

#define FW_DECL_NS3(NS1, NS2, NS3, CLASS) \
namespace NS1 { \
namespace NS2 { \
namespace NS3 { \
	class CLASS; \
} \
} \
}

///////////////////////////////////////////////////////////////////////////////

#define FW_DECL_UP_NS0(CLASS) \
	class CLASS; \
	using   Up ## CLASS = std::unique_ptr<CLASS>; \
	using  Ucp ## CLASS = std::unique_ptr<const CLASS>;

#define FW_DECL_UP_NS1(NS1, CLASS) \
namespace NS1 { \
	class CLASS; \
	using   Up ## CLASS = std::unique_ptr<CLASS>; \
	using  Ucp ## CLASS = std::unique_ptr<const CLASS>; \
}

#define FW_DECL_UP_NS2(NS1, NS2, CLASS) \
namespace NS1 { \
namespace NS2 { \
	class CLASS; \
	using   Up ## CLASS = std::unique_ptr<CLASS>; \
	using  Ucp ## CLASS = std::unique_ptr<const CLASS>; \
} \
}

#define FW_DECL_UP_NS3(NS1, NS2, NS3, CLASS) \
namespace NS1 { \
namespace NS2 { \
namespace NS3 { \
	class CLASS; \
	using   Up ## CLASS = std::unique_ptr<CLASS>; \
	using  Ucp ## CLASS = std::unique_ptr<const CLASS>; \
} \
} \
}
///////////////////////////////////////////////////////////////////////////////

#define FW_DECL_SP_NS0(CLASS) \
	class CLASS; \
	using   Sp ## CLASS = std::shared_ptr<CLASS>; \
	using  Scp ## CLASS = std::shared_ptr<const CLASS>;

#define FW_DECL_SP_NS1(NS1, CLASS) \
namespace NS1 { \
	class CLASS; \
	using   Sp ## CLASS = std::shared_ptr<CLASS>; \
	using  Scp ## CLASS = std::shared_ptr<const CLASS>; \
}

#define FW_DECL_SP_NS2(NS1, NS2, CLASS) \
namespace NS1 { \
namespace NS2 { \
	class CLASS; \
	using   Sp ## CLASS = std::shared_ptr<CLASS>; \
	using  Scp ## CLASS = std::shared_ptr<const CLASS>; \
} \
}

#define FW_DECL_SP_NS3(NS1, NS2, NS3, CLASS) \
namespace NS1 { \
namespace NS2 { \
namespace NS3 { \
	class CLASS; \
	using   Sp ## CLASS = std::shared_ptr<CLASS>; \
	using  Scp ## CLASS = std::shared_ptr<const CLASS>; \
} \
} \
}

///////////////////////////////////////////////////////////////////////////////

#define FW_DECL_VECTOR_OF_UP_NS0(CLASS) \
	FW_DECL_UP_NS0(CLASS); \
	using CLASS ## UpVector = std::vector<Up ## CLASS>;

#define FW_DECL_VECTOR_OF_UP_NS1(NS1, CLASS) \
namespace NS1 { \
	FW_DECL_UP_NS0(CLASS); \
	using CLASS ## UpVector = std::vector<Up ## CLASS>; \
}

#define FW_DECL_VECTOR_OF_UP_NS2(NS1, NS2, CLASS) \
namespace NS1 { \
namespace NS2 { \
	FW_DECL_UP_NS0(CLASS); \
	using CLASS ## UpVector = std::vector<Up ## CLASS>; \
} \
}

#define FW_DECL_VECTOR_OF_UP_NS3(NS1, NS2, NS3, CLASS) \
namespace NS1 { \
namespace NS2 { \
namespace NS3 { \
	FW_DECL_UP_NS0(CLASS); \
	using CLASS ## UpVector = std::vector<Up ## CLASS>; \
} \
} \
}

///////////////////////////////////////////////////////////////////////////////

#define FW_DECL_VECTOR_OF_SP_NS0(CLASS) \
	FW_DECL_SP_NS0(CLASS); \
	using CLASS ## SpVector = std::vector<Sp ## CLASS>;

#define FW_DECL_VECTOR_OF_SP_NS1(NS1, CLASS) \
namespace NS1 { \
	FW_DECL_SP_NS0(CLASS); \
	using CLASS ## SpVector = std::vector<Sp ## CLASS>; \
}

#define FW_DECL_VECTOR_OF_SP_NS2(NS1, NS2, CLASS) \
namespace NS1 { \
namespace NS2 { \
	FW_DECL_SP_NS0(CLASS); \
	using CLASS ## SpVector = std::vector<Sp ## CLASS>; \
} \
}

#define FW_DECL_VECTOR_OF_SP_NS3(NS1, NS2, NS3, CLASS) \
namespace NS1 { \
namespace NS2 { \
namespace NS3 { \
	FW_DECL_SP_NS0(CLASS); \
	using CLASS ## SpVector = std::vector<Sp ## CLASS>; \
} \
} \
}

#endif // FW_DECL_H_INCLUDE
