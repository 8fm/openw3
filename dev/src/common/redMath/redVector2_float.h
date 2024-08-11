/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _REDMATH_LIB_REDVECTOR2_FLOAT_H_
#define _REDMATH_LIB_REDVECTOR2_FLOAT_H_
#include "../redSystem/types.h"
#include "redScalar_float.h"
#include "mathTypes.h"

namespace RedMath
{
	namespace FLOAT
	{
		RED_ALIGNED_CLASS( RedVector2, 16 )
		{
		public:
			// Constructors
			RED_INLINE RedVector2();
			RED_INLINE RedVector2( const RedScalar& _v );
			RED_INLINE RedVector2( const RedVector2& _v );
			RED_INLINE RedVector2( Red::System::Float _x, Red::System::Float _y );
			RED_INLINE RedVector2( const Red::System::Float* _f );

			// Destructor
			RED_INLINE ~RedVector2();

			// Operators
			RED_INLINE RedVector2& operator = ( const RedScalar& _v );
			RED_INLINE RedVector2& operator = ( const RedVector2& _v );

			RED_INLINE const Red::System::Float* AsFloat() const;

			//////////////////////////////////////////////////////////////////////////
			// Methods
			//////////////////////////////////////////////////////////////////////////

			// Settings
			RED_INLINE void Set( Red::System::Float _x, Red::System::Float _y );
			RED_INLINE void Set( const RedScalar& _v );
			RED_INLINE void Set( const RedVector2& _v );
			RED_INLINE void Set( const Red::System::Float* _f );

			// Special Sets.
			RED_INLINE void SetZeros();
			RED_INLINE void SetOnes();

			// Manipulation Methods.
			RED_INLINE RedVector2& Negate();
			RED_INLINE RedVector2 Negated() const;

			RED_INLINE RedScalar Length() const;
			RED_INLINE RedScalar SquareLength() const;

			RED_INLINE RedVector2& Normalize();
			RED_INLINE RedVector2 Normalized() const;

			RED_INLINE Red::System::Bool IsAlmostEqual( const RedVector2& _v, Red::System::Float _epsilon = RED_EPSILON ) const;
			RED_INLINE Red::System::Bool IsZero() const;
			RED_INLINE Red::System::Bool IsAlmostZero( Red::System::Float _epsilon = RED_EPSILON ) const;

			static const RedVector2 ZEROS;
			static const RedVector2 ONES;
			static const RedVector2 EX;
			static const RedVector2 EY;

			union
			{
				struct
				{
					Red::System::Float X;
					Red::System::Float Y;
					Red::System::Float Z;
					Red::System::Float W;
				};

				struct 
				{
					Red::System::Uint32 Xi;
					Red::System::Uint32 Yi;
					Red::System::Uint32 Zi;
					Red::System::Uint32 Wi;
				};
				Red::System::Float V[4];
			};
		};
	}
}

#include "redVector2_float.inl"

#endif
