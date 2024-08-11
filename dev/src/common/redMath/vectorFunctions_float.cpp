#include "vectorFunctions_float.h"


namespace RedMath
{
	namespace FLOAT
	{
		//////////////////////////////////////////////////////////////////////////
		// 'SQUARE ROOT' Functions
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void SqrRoot( RedScalar& _a )
		{
			_a.Set( Red::Math::MSqrt( _a.X ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar SqrRoot( const RedScalar& _a )
		{
			return Red::Math::MSqrt( _a.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SqrRoot( RedVector2& _a )
		{
			_a.Set( Red::Math::MSqrt( _a.X ), Red::Math::MSqrt( _a.Y ) ); 
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 SqrRoot( const RedVector2& _a )
		{
			return RedVector2( Red::Math::MSqrt( _a.X ), Red::Math::MSqrt( _a.Y ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SqrRoot( RedVector3& _a )
		{
			_a.Set( Red::Math::MSqrt( _a.X ), Red::Math::MSqrt( _a.Y ), Red::Math::MSqrt( _a.Z ) ); 
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 SqrRoot( const RedVector3& _a )
		{
			return RedVector3( Red::Math::MSqrt( _a.X ), Red::Math::MSqrt( _a.Y ), Red::Math::MSqrt( _a.Z ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SqrRoot( RedVector4& _a )
		{
			_a.Set( Red::Math::MSqrt( _a.X ), Red::Math::MSqrt( _a.Y ), Red::Math::MSqrt( _a.Z ), Red::Math::MSqrt( _a.W ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 SqrRoot( const RedVector4& _a )
		{
			return RedVector4( Red::Math::MSqrt( _a.X ), Red::Math::MSqrt( _a.Y ), Red::Math::MSqrt( _a.Z ), Red::Math::MSqrt( _a.W ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void SqrRoot3( RedVector4& _a )
		{
			_a.Set( Red::Math::MSqrt( _a.X ), Red::Math::MSqrt( _a.Y ), Red::Math::MSqrt( _a.Z ), _a.W ); 
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 SqrRoot3( const RedVector4& _a )
		{
			return RedVector4( Red::Math::MSqrt( _a.X ), Red::Math::MSqrt( _a.Y ), Red::Math::MSqrt( _a.Z ), _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		// 'RECIPROCAL SQUARE ROOT' Functions
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void RSqrRoot( RedScalar& _a )
		{
			_a.Set( Red::Math::MRsqrt( _a.X ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar RSqrRoot( const RedScalar& _a )
		{
			return Red::Math::MRsqrt( _a.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void RSqrRoot( RedVector2& _a )
		{
			_a.Set( Red::Math::MRsqrt( _a.X ), Red::Math::MRsqrt( _a.Y ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 RSqrRoot( const RedVector2& _a )
		{
			return RedVector2( Red::Math::MRsqrt( _a.X ), Red::Math::MRsqrt( _a.Y ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void RSqrRoot( RedVector3& _a )
		{
			_a.Set( Red::Math::MRsqrt( _a.X ), Red::Math::MRsqrt( _a.Y ), Red::Math::MRsqrt( _a.Z ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 RSqrRoot( const RedVector3& _a )
		{
			return RedVector3( Red::Math::MRsqrt( _a.X ), Red::Math::MRsqrt( _a.Y ), Red::Math::MRsqrt( _a.Z ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void RSqrRoot( RedVector4& _a )
		{
			_a.Set( Red::Math::MRsqrt( _a.X ), Red::Math::MRsqrt( _a.Y ), Red::Math::MRsqrt( _a.Z ), Red::Math::MRsqrt( _a.W ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RSqrRoot( const RedVector4& _a )
		{
			return RedVector4( Red::Math::MRsqrt( _a.X ), Red::Math::MRsqrt( _a.Y ), Red::Math::MRsqrt( _a.Z ), Red::Math::MRsqrt( _a.W ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void RSqrRoot3( RedVector4& _a )
		{
			_a.Set( Red::Math::MRsqrt( _a.X ), Red::Math::MRsqrt( _a.Y ), Red::Math::MRsqrt( _a.Z ), _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 RSqrRoot3( const RedVector4& _a )
		{
			return RedVector4( Red::Math::MRsqrt( _a.X ), Red::Math::MRsqrt( _a.Y ), Red::Math::MRsqrt( _a.Z ), _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		// 'DOT PRODUCT' Functions
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void Dot( RedScalar& _a, const RedVector2& _b, const RedVector2& _c )
		{
			_a.Set( ( ( _b.X * _c.X ) + ( _b.Y * _c.Y ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Dot( const RedVector2& _a, const RedVector2& _b )
		{
			return ( ( _a.X * _b.X ) + ( _a.Y * _b.Y ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Dot( RedScalar& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.Set( ( ( _b.X * _c.X ) + ( _b.Y * _c.Y ) + ( _b.Z * _c.Z ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Dot( const RedVector3& _a, const RedVector3& _b )
		{
			return ( ( _a.X * _b.X ) + ( _a.Y * _b.Y ) + ( _a.Z * _b.Z ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Dot( RedScalar& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.Set( ( ( _b.X * _c.X ) + ( _b.Y * _c.Y ) + ( _b.Z * _c.Z ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Dot( const RedVector4& _a, const RedVector3& _b )
		{
			return ( ( _a.X * _b.X ) + ( _a.Y * _b.Y ) + ( _a.Z * _b.Z ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Dot( RedScalar& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( ( ( _b.X * _c.X ) + ( _b.Y * _c.Y ) + ( _b.Z * _c.Z ) + ( _b.W * _c.W ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Dot( const RedVector4& _a, const RedVector4& _b )
		{
			return ( ( _a.X * _b.X ) + ( _a.Y * _b.Y ) + ( _a.Z * _b.Z ) + ( _a.W * _b.W ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Dot3( RedScalar& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( ( ( _b.X * _c.X ) + ( _b.Y * _c.Y ) + ( _b.Z * _c.Z ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Dot3( const RedVector4& _a, const RedVector4& _b )
		{
			return ( ( _a.X * _b.X ) + ( _a.Y * _b.Y ) + ( _a.Z * _b.Z ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// 'NORMALIZED DOT PRODUCT' Functions
		// Ensure the input vectors are of unit length.
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void UnitDot( RedScalar& _a, const RedVector2& _b, const RedVector2& _c )
		{
			const RedScalar dotProduct( Dot( _b, _c ) );
			RedScalar lengthSqr( Mul( _b.SquareLength(), _c.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				_a.SetZeros();
				return;
			}
			_a.Set( Div( dotProduct, lengthSqr ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar UnitDot( const RedVector2& _a, const RedVector2& _b )
		{
			const RedScalar dotProduct( Dot( _a, _b ) );
			RedScalar lengthSqr( Mul( _a.SquareLength(), _b.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				return 0.0f;
			}
			return Div( dotProduct, lengthSqr );
		}

		//////////////////////////////////////////////////////////////////////////
		void UnitDot( RedScalar& _a, const RedVector3& _b, const RedVector3& _c )
		{
			const RedScalar dotProduct( Dot( _b, _c ) );
			RedScalar lengthSqr( Mul( _b.SquareLength(), _c.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				_a.SetZeros();
				return;
			}
			_a.Set( Div( dotProduct, lengthSqr ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar UnitDot( const RedVector3& _a, const RedVector3& _b )
		{
			const RedScalar dotProduct( Dot( _a, _b ) );
			RedScalar lengthSqr( Mul( _a.SquareLength(), _b.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				return 0.0f;
			}
			return Div( dotProduct, lengthSqr );
		}

		//////////////////////////////////////////////////////////////////////////
		void UnitDot( RedScalar& _a, const RedVector4& _b, const RedVector3& _c )
		{
			const RedScalar dotProduct( Dot( _b, _c ) );
			RedScalar lengthSqr( Mul( _b.SquareLength3(), _c.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				_a.SetZeros();
				return;
			}
			_a.Set( Div( dotProduct, lengthSqr ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar UnitDot( const RedVector4& _a, const RedVector3& _b )
		{
			const RedScalar dotProduct( Dot( _a, _b ) );
			RedScalar lengthSqr( Mul( _a.SquareLength3(), _b.SquareLength() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				return 0.0f;
			}
			return Div( dotProduct, lengthSqr );
		}

		//////////////////////////////////////////////////////////////////////////
		void UnitDot( RedScalar& _a, const RedVector4& _b, const RedVector4& _c )
		{
			const RedScalar dotProduct( Dot( _b, _c ) );
			RedScalar lengthSqr( Mul( _b.SquareLength4(), _c.SquareLength4() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				_a.SetZeros();
				return;
			}
			_a.Set( Div( dotProduct, lengthSqr ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar UnitDot( const RedVector4& _a, const RedVector4& _b )
		{
			const RedScalar dotProduct( Dot( _a, _b ) );
			RedScalar lengthSqr( Mul( _a.SquareLength4(), _b.SquareLength4() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				return 0.0f;
			}
			return Div( dotProduct, lengthSqr );
		}

		//////////////////////////////////////////////////////////////////////////
		void UnitDot3( RedScalar& _a, const RedVector4& _b, const RedVector4& _c )
		{
			const RedScalar dotProduct( Dot3( _b, _c ) );
			RedScalar lengthSqr( Mul( _b.SquareLength3(), _c.SquareLength3() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				_a.SetZeros();
				return;
			}
			_a.Set( Div( dotProduct, lengthSqr ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar UnitDot3( const RedVector4& _a, const RedVector4& _b )
		{
			const RedScalar dotProduct( Dot3( _a, _b ) );
			RedScalar lengthSqr( Mul( _a.SquareLength3(), _b.SquareLength3() ) );
			SqrRoot( lengthSqr );
			if( lengthSqr.IsZero() )
			{
				return 0.0f;
			}
			return Div( dotProduct, lengthSqr );
		}

		//////////////////////////////////////////////////////////////////////////
		// 'CROSS PRODUCT' Functions
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Technically it doesn't exist, but we return the scalar 'Z' value if it
		// was a 3-D vector.
		//////////////////////////////////////////////////////////////////////////
		void Cross( RedScalar& _a, const RedVector2& _b, const RedVector2& _c )
		{
			_a.Set( ( ( _b.X * _c.Y ) - ( _b.Y * _c.X ) ) );
		}

		//////////////////////////////////////////////////////////////////////////
		// Technically it doesn't exist, but we return the scalar 'Z' value if it
		// was a 3-D vector.
		//////////////////////////////////////////////////////////////////////////
		RedScalar Cross( const RedVector2& _a, const RedVector2& _b )
		{
			return ( ( _a.X * _b.Y ) - ( _a.Y * _b.X ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Cross( RedVector3& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.Set( ( _b.Y * _c.Z ) + ( _b.Z * _c.Y ), ( _b.Z * _c.X ) + ( _b.X * _c.Z ), ( _b.X * _c.Y ) + ( _b.Y * _c.X ) );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Cross( const RedVector3& _a, const RedVector3& _b )
		{
			return RedVector3( ( _a.Y * _b.Z ) + ( _a.Z * _b.Y ), ( _a.Z * _b.X ) + ( _a.X * _b.Z ), ( _a.X * _b.Y ) + ( _a.Y * _b.X ) );
		}

		//////////////////////////////////////////////////////////////////////////
		void Cross( RedVector4& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.Set( ( _b.Y * _c.Z ) + ( _b.Z * _c.Y ), ( _b.Z * _c.X ) + ( _b.X * _c.Z ), ( _b.X * _c.Y ) + ( _b.Y * _c.X ), 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Cross( const RedVector4& _a, const RedVector3& _b )
		{
			return RedVector4( ( _a.Y * _b.Z ) + ( _a.Z * _b.Y ), ( _a.Z * _b.X ) + ( _a.X * _b.Z ), ( _a.X * _b.Y ) + ( _a.Y * _b.X ), 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		void Cross( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( ( _b.Y * _c.Z ) + ( _b.Z * _c.Y ), ( _b.Z * _c.X ) + ( _b.X * _c.Z ), ( _b.X * _c.Y ) + ( _b.Y * _c.X ), 1.0f );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Cross( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( ( _a.Y * _b.Z ) + ( _a.Z * _b.Y ), ( _a.Z * _b.X ) + ( _a.X * _b.Z ), ( _a.X * _b.Y ) + ( _a.Y * _b.X ), 1.0f );
		}
	}
}