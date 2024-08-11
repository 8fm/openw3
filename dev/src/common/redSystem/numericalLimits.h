/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef RED_SYSTEM_NUMERICLIMITS_H
#define RED_SYSTEM_NUMERICLIMITS_H

#include "types.h"

#include <cfloat>
#include <limits>

namespace Red
{
	namespace System
	{
		// This class defines the limits for the core red system types
		// Use this where possible rather than the built-in macros as some types may have different sizes than expected!
		template< class _Type >
		class NumericLimits
		{
		public:
			static _Type Min()		{ return _Type(); }
			static _Type Max()		{ return _Type(); }
			static _Type Infinity()	{ return _Type(); }
			static _Type Epsilon()	{ return _Type(); }
		};

#		define RED_SYSTEM_DEFINE_NUMERIC_LIMITS( type, min, max, inf, epsilon )	\
		template<>																\
		class NumericLimits< type >												\
		{																		\
		public:																	\
			static type	Min() { return type( min ); }							\
			static type	Max() { return type( max ); }							\
			static type	Infinity() { return type( inf ); }						\
			static type	Epsilon() { return type( epsilon ); }					\
		};

		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Bool, false, true, 0, 0 );

		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Int8, SCHAR_MIN, SCHAR_MAX, 0, 0 );
		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Uint8, 0, UCHAR_MAX, 0, 0 );	

		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Int16, SHRT_MIN,  SHRT_MAX, 0, 0 );
		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Uint16, 0, USHRT_MAX, 0, 0 );

		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Int32, INT_MIN, INT_MAX, 0, 0 );
		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Uint32, 0, UINT_MAX, 0, 0 );

		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Int64, LLONG_MIN,  LLONG_MAX, 0, 0 );
		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Uint64, 0, ULLONG_MAX, 0, 0 );

		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Float, FLT_MIN, FLT_MAX, std::numeric_limits< Float >::infinity(), FLT_EPSILON );
		RED_SYSTEM_DEFINE_NUMERIC_LIMITS( Double, DBL_MIN, DBL_MAX, std::numeric_limits< Double >::infinity(), DBL_EPSILON );
	}
}

#endif // RED_SYSTEM_NUMERICLIMITS_H
