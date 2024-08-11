/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_NUMERICAL_UTILS_H_
#define _REDMATH_LIB_NUMERICAL_UTILS_H_

#include "../redSystem/types.h"

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
# include <float.h> // For _isnan
#elif defined( RED_PLATFORM_ORBIS )
# include <math.h> // For isnan
#endif

namespace Red
{
	namespace Math
	{
		namespace NumericalUtils
		{
			RED_INLINE Red::System::Bool IsNan( Red::System::Float r )
			{
#ifdef RED_PLATFORM_ORBIS
				return ::isnan( r ) != 0;
#elif defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
				return ::_isnan( r ) != 0;
#else
# error Unsupported platform
				return false;
#endif
			}

			RED_INLINE Red::System::Bool IsNan( Red::System::Double r )
			{
#ifdef RED_PLATFORM_ORBIS
				return ::isnan( r ) != 0;
#elif defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
 				return ::_isnan( r ) != 0;
#else
# error Unsupported platform
 				return false;
#endif
			}

			RED_INLINE bool IsFinite( float r )
			{
				// Check the 8 exponent bits.
				// Usually NAN == (exponent = all 1, mantissa = non-zero)
				//         INF == (exponent = all 1, mantissa = zero)
				// This simply checks the exponent

				// Removed whilst porting.
				//COMPILE_ASSERT( sizeof( Float ) == sizeof( Uint32 ) );

				union 
				{
					float f;
					unsigned int i;
				} val;

				val.f = r;
				return ( ( val.i & 0x7f800000 ) != 0x7f800000 );
			};


			template < class T >
			RED_INLINE T Max( const T& a, const T& b )   
			{ 
				return ( a >= b ) ? a : b; 
			}

			template < class T > 
			RED_INLINE T Min( const T& a, const T& b )   
			{ 
				return ( a <= b ) ? a : b; 
			}

			template < class T > 
			RED_INLINE T Max( const T& a, const T& b, const T& c )   
			{ 
				return Max( Max( a, b ), c );
			}

			template < class T > 
			RED_INLINE T Min( const T& a, const T& b, const T& c )   
			{ 
				return Min( Min( a, b ), c );
			}

			template < class T > 
			RED_INLINE T Clamp( const T& x, const T& min, const T& max )		
			{ 
				return Min( Max( x, min ), max );
			}

			template < class T > 
			RED_INLINE T Abs( const T& a )				
			{ 
				return ( a>=T( 0 ) ) ? a : -a; 
			}

			template < class T > 
			RED_INLINE int Sgn( const T& a )				
			{ 
				static const T zero = T( 0 );
				return ( a > zero ) ? 1 : ( a<zero ? -1 : 0 );
			}

			template < class T > 
			RED_INLINE void Swap( T& a, T& b )								
			{ 
				const T tmp = a; a = b;	b = tmp; 
			}

			template < class Val >
			RED_INLINE Val ArithmeticAverage( const Val& v1, const Val& v2 )
			{
				return ( v1 + v2 ) / 2.f;
			}

			template < >
			RED_INLINE int ArithmeticAverage< int >( const int& v1, const int& v2 ) 
			{ 
				return ( v1 + v2 ) / 2; 
			}
			
			template < >
			RED_INLINE unsigned int ArithmeticAverage< unsigned int >( const unsigned int& v1, const unsigned int& v2 ) 
			{ 
				return (v1 + v2) / 2; 
			}

			template <class T> 
			RED_INLINE T Lerp( float frac, const T& a, const T& b )
			{ 
				return a + ( ( b - a ) * frac ) ;
			}

		};
	};
};


#endif //_REDMATH_LIB_NUMERICAL_UTILS_H_
