/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_PROXY_H_
#define _RED_MEMORY_PROXY_H_

// This is a clever trick used to resolve the type of variable or type, 
// I can also call either static function or normal function on result. 
// See operators.h and operator.hpp for more info, and see where magic happen. 
#define RED_MEMORY_DECLARE_PROXY( name, alignment ) \
	RED_MEMORY_INLINE name * operator()() { return this; } \
	typedef std::integral_constant< int, alignment > DefaultAlignmentType

#endif
