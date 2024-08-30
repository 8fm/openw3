/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef _NUMERICLIMITS_H_
#define _NUMERICLIMITS_H_

#include <climits> // for SCHAR_*, UCHAR_*, SHRT_*, INT_*, UINT_*, ...
#include <limits>  // for std::numeric_limits
#include <float.h> // for FLT_*, DBL_*

template<class _Type>
class NumericLimits
{
public:
	static _Type	Min() 
	{
		return _Type();
	}

	static _Type	Max() 
	{
		return _Type();
	}

	static _Type	Infinity() 
	{
		return _Type();
	}

	static _Type	Epsilon() 
	{
		return _Type();
	}
};

#define DEFINE_NUMERIC_LIMITS(type, min, max, inf, epsilon)			template<>							\
																	class NumericLimits<type>			\
																	{									\
																	public:								\
																		static type	Min()				\
																		{								\
																			return type(min);			\
																		}								\
																										\
																		static type	Max()				\
																		{								\
																			return type(max);			\
																		}								\
																										\
																		static type	Infinity()			\
																		{								\
																			return type(inf);			\
																		}								\
																										\
																		static type	Epsilon()			\
																		{								\
																			return type(epsilon);		\
																		}								\
																	};

DEFINE_NUMERIC_LIMITS( Bool, false, true, 0, 0 );

DEFINE_NUMERIC_LIMITS( Int8, SCHAR_MIN, SCHAR_MAX, 0, 0 );
DEFINE_NUMERIC_LIMITS( Uint8, 0, UCHAR_MAX, 0, 0 );	

DEFINE_NUMERIC_LIMITS( Int16, SHRT_MIN,  SHRT_MAX, 0, 0 );
DEFINE_NUMERIC_LIMITS( Uint16, 0, USHRT_MAX, 0, 0 );

DEFINE_NUMERIC_LIMITS( Int32, INT_MIN, INT_MAX, 0, 0 );
DEFINE_NUMERIC_LIMITS( Uint32, 0, UINT_MAX, 0, 0 );

//DEFINE_NUMERIC_LIMITS( Int64, _I64_MIN,  _I64_MAX, 0, 0 );	// TSK, TSK, TSK, use standards please!
//DEFINE_NUMERIC_LIMITS( Uint64, 0, _UI64_MAX, 0, 0 );			// not MS crap

DEFINE_NUMERIC_LIMITS( Int64, LLONG_MIN,  LLONG_MAX, 0, 0 );
DEFINE_NUMERIC_LIMITS( Uint64, 0, ULLONG_MAX, 0, 0 );

DEFINE_NUMERIC_LIMITS( Float, FLT_MIN, FLT_MAX, std::numeric_limits< Float >::infinity(), FLT_EPSILON );
DEFINE_NUMERIC_LIMITS( Double, DBL_MIN, DBL_MAX, std::numeric_limits< Double >::infinity(), DBL_EPSILON );

// DEFINE_NUMERIC_LIMITS( UniChar, WCHAR_MIN, WCHAR_MAX, 0, 0);

#endif