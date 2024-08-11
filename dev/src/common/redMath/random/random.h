/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_RANDOM_H_
#define _RED_RANDOM_H_

#include "../../redSystem/types.h"
#include "../../redSystem/error.h"
#include "../numericalutils.h"

namespace Red
{
	namespace Math
	{
		namespace Random
		{
			namespace Internal
			{
				// These "range" functions have been brought out of the Generator class due to
				// the fact that you can't specialise a member function template

				// Expects value to be less than or equal to max
				template< typename T >
				RED_INLINE T Range( T value, T max )
				{
					RED_ASSERT( max > 0, TXT( "Cannot ask for a maximum of 0. This will crash with Divide by 0" ) );
					return value % max;
				}

				// Expects value to be in the range 0.0 <-> 1.0
				template<>
				RED_INLINE System::Float Range( System::Float value, System::Float max )
				{
					return value * max;
				}
			}

			template< typename TEngine >
			class Generator
			{
			public:
				RED_INLINE explicit Generator()									{}
				RED_INLINE explicit Generator( System::Random::SeedValue seed )	{ Seed( seed ); }
				RED_INLINE ~Generator()											{}

				// Default seed value
				RED_INLINE void Seed()											{ m_engine.Seed(); }

				// Seed the random number generator
				RED_INLINE void Seed( System::Random::SeedValue seed )			{ m_engine.Seed( seed ); }

				// Return a value between 0 and Max() [RAND_MAX]
				template< typename TReturnType >
				RED_INLINE TReturnType Get()
				{
					return m_engine.template Get< TReturnType >();
				}

				// Return a value between 0 and max [parameter]
				template< typename TReturnType >
				RED_INLINE TReturnType Get( TReturnType max )
				{
					return Internal::Range( Get< TReturnType >(), max );
				}

				// Expects value to be in the range start <-> end (no matter the order)
				template< typename TReturnType >
				RED_INLINE TReturnType Get( TReturnType a, TReturnType b )
				{
					// This conditional is temporary to avoid divide by 0 crashes
					if( a == b )
					{
						return a;
					}

					TReturnType min = NumericalUtils::Min( a, b );
					TReturnType max = NumericalUtils::Max( a, b );

					return Internal::Range< TReturnType >( Get< TReturnType >(), ( max - min ) ) + min;
				}

				// Expects that you know what you are doing, do not use if arguments are users input
				template< typename TReturnType >
				RED_INLINE TReturnType GetUnsafe( TReturnType a, TReturnType b )
				{
					return Internal::Range< TReturnType >( Get< TReturnType >(), ( b - a ) ) + a;
				}

				// Find out the maximum value that will be returned for the template type
				template< typename TReturnType >
				RED_INLINE TReturnType Max() const
				{
					return m_engine.template Max< TReturnType >();
				}

			private:
				TEngine m_engine;
			};
		}
	}
}

#endif // _RED_RANDOM_H_
