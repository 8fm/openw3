#include "vectorArithmetic_float.h"

namespace RedMath
{
	namespace FLOAT
	{
		//////////////////////////////////////////////////////////////////////////
		// '+' Addition Functions.
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void Add( RedScalar& _a, const RedScalar& _b, const RedScalar& _c )
		{
			_a.Set( _b.X + _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector2& _a, const RedVector2& _b, const RedScalar& _c )
		{
			_a.Set( _b.X + _c.X, _b.Y + _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector3& _a, const RedVector3& _b, const RedScalar& _c )
		{
			_a.Set( _b.X + _c.X, _b.Y + _c.X, _b.Z + _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector4& _a, const RedVector4& _b, const RedScalar& _c )
		{
			_a.Set( _b.X + _c.X, _b.Y + _c.X, _b.Z + _c.X, _b.W + _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector2& _a, const RedVector2& _b, const RedVector2& _c )
		{
			_a.Set( _b.X + _c.X, _b.Y + _c.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector3& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.Set( _b.X + _c.X, _b.Y + _c.Y, _b.Z + _c.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector4& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.Set( _b.X + _c.X, _b.Y + _c.Y, _b.Z + _c.Z, _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( _b.X + _c.X, _b.Y + _c.Y, _b.Z + _c.Z, _b.W + _c.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void Add3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( _b.X + _c.X, _b.Y + _c.Y, _b.Z + _c.Z, _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Add( const RedScalar& _a, const RedScalar& _b )
		{
			return ( _a.X + _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Add( const RedVector2& _a, const RedScalar& _b )
		{
			return RedVector2( _a.X + _b.X, _a.Y + _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Add( const RedVector3& _a, const RedScalar& _b )
		{
			return RedVector3( _a.X + _b.X, _a.Y + _b.X, _a.Z + _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Add( const RedVector4& _a, const RedScalar& _b )
		{
			return RedVector4( _a.X + _b.X, _a.Y + _b.X, _a.Z + _b.X, _a.W + _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Add( const RedVector2& _a, const RedVector2& _b )
		{
			return RedVector2( _a.X + _b.X, _a.Y + _b.Y);
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Add( const RedVector3& _a, const RedVector3& _b )
		{
			return RedVector3( _a.X + _b.X, _a.Y + _b.Y, _a.Z + _b.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Add( const RedVector4& _a, const RedVector3& _b )
		{
			return RedVector4( _a.X + _b.X, _a.Y + _b.Y, _a.Z + _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Add( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _a.X + _b.X, _a.Y + _b.Y, _a.Z + _b.Z, _a.W + _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Add3( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _a.X + _b.X, _a.Y + _b.Y, _a.Z + _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedScalar& _a, const RedScalar&_b )
		{
			_a.Set( _a.X + _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector2& _a, const RedScalar&_b )
		{
			_a.Set( _a.X + _b.X, _a.Y + _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector3& _a, const RedScalar&_b )
		{
			_a.Set( _a.X + _b.X, _a.Y + _b.X, _a.Z + _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector4& _a, const RedScalar&_b )
		{
			_a.Set( _a.X + _b.X, _a.Y + _b.X, _a.Z + _b.X, _a.W + _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector2& _a, const RedVector2&_b )
		{
			_a.Set( _a.X + _b.X, _a.Y + _b.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector3& _a, const RedVector3& _b )
		{
			_a.Set( _a.X + _b.X, _a.Y + _b.Y, _a.Z + _b.Z );
		}
		
		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector4& _a, const RedVector3& _b )
		{
			_a.Set( _a.X + _b.X, _a.Y + _b.Y, _a.Z + _b.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd( RedVector4& _a, const RedVector4& _b )
		{
			_a.Set( _a.X + _b.X, _a.Y + _b.Y, _a.Z + _b.Z, _a.W + _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetAdd3( RedVector4& _a, const RedVector4& _b )
		{
			_a.Set( _a.X + _b.X, _a.Y + _b.Y, _a.Z + _b.Z );
		}
		
		//////////////////////////////////////////////////////////////////////////
		// '-' Subtraction Functions
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedScalar& _a, const RedScalar& _b, const RedScalar& _c )
		{
			_a.Set( _b.X - _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector2& _a, const RedVector2& _b, const RedScalar& _c )
		{
			_a.Set( _b.X - _c.X, _b.Y - _c.X );
		}
		
		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector3& _a, const RedVector3& _b, const RedScalar& _c )
		{
			_a.Set( _b.X - _c.X, _b.Y - _c.X, _b.Z - _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector4& _a, const RedVector4& _b, const RedScalar& _c )
		{
			_a.Set( _b.X - _c.X, _b.Y - _c.X, _b.Z - _c.X, _b.W - _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector2& _a, const RedVector2& _b, const RedVector2& _c )
		{
			_a.Set( _b.X - _c.X, _b.Y - _c.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector3& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.Set( _b.X - _c.X, _b.Y - _c.Y, _b.Z - _c.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector4& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.Set( _b.X - _c.X, _b.Y - _c.Y, _b.Z - _c.Z, _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( _b.X - _c.X, _b.Y - _c.Y, _b.Z - _c.Z, _b.W - _c.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void Sub3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( _b.X - _c.X, _b.Y - _c.Y, _b.Z - _c.Z, _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Sub( const RedScalar& _a, const RedScalar& _b )
		{
			return ( _a.X - _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Sub( const RedVector2& _a, const RedScalar& _b )
		{
			return RedVector2( _a.X - _b.X, _a.Y - _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Sub( const RedVector3& _a, const RedScalar& _b )
		{
			return RedVector3( _a.X - _b.X, _a.Y - _b.X, _a.Z - _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Sub( const RedVector4& _a, const RedScalar& _b )
		{
			return RedVector4( _a.X - _b.X, _a.Y - _b.X, _a.Z - _b.X, _a.W - _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Sub( const RedVector2& _a, const RedVector2& _b )
		{
			return RedVector2( _a.X - _b.X, _a.Y - _b.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Sub( const RedVector3& _a, const RedVector3& _b )
		{
			return RedVector3( _a.X - _b.X, _a.Y - _b.Y, _a.Z - _b.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Sub( const RedVector4& _a, const RedVector3& _b )
		{
			return RedVector4( _a.X - _b.X, _a.Y - _b.Y, _a.Z - _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Sub( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _a.X - _b.X, _a.Y - _b.Y, _a.Z - _b.Z, _a.W - _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Sub3( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _a.X - _b.X, _a.Y - _b.Y, _a.Z - _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedScalar& _a, const RedScalar& _b )
		{
			_a.Set( _a.X - _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector2& _a, const RedScalar& _b )
		{
			_a.Set( _a.X - _b.X, _a.Y - _b.X );
		}
		
		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector3& _a, const RedScalar& _b )
		{
			_a.Set( _a.X - _b.X, _a.Y - _b.X, _a.Z - _b.X );
		}

		void SetSub( RedVector4& _a, const RedScalar& _b )
		{
			_a.Set( _a.X - _b.X, _a.Y - _b.X, _a.Z - _b.X, _a.W - _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector2& _a, const RedVector2& _b )
		{
			_a.Set( _a.X - _b.X, _a.Y - _b.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector3& _a, const RedVector3& _b )
		{
			_a.Set( _a.X - _b.X, _a.Y - _b.Y, _a.Z - _b.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector4& _a, const RedVector3& _b )
		{
			_a.Set( _a.X - _b.X, _a.Y - _b.Y, _a.Z - _b.Z , _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub( RedVector4& _a, const RedVector4& _b )
		{
			_a.Set( _a.X - _b.X, _a.Y - _b.Y, _a.Z - _b.Z, _a.W - _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetSub3( RedVector4& _a, const RedVector4& _b )
		{
			_a.Set( _a.X - _b.X, _a.Y - _b.Y, _a.Z - _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		// '*' Multiplication Functions.
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedScalar& _a, const RedScalar& _b, const RedScalar& _c )
		{
			_a.Set( _b.X * _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector2& _a, const RedVector2& _b, const RedScalar& _c )
		{
			_a.Set( _b.X * _c.X, _b.Y * _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector3& _a, const RedVector3& _b, const RedScalar& _c )
		{
			_a.Set( _b.X * _c.X, _b.Y * _c.X, _b.Z * _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector4& _a, const RedVector4& _b, const RedScalar& _c )
		{
			_a.Set( _b.X * _c.X, _b.Y * _c.X, _b.Z * _c.X, _b.W * _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector2& _a, const RedVector2& _b, const RedVector2& _c )
		{
			_a.Set( _b.X * _c.X, _b.Y * _c.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector3& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.Set( _b.X * _c.X, _b.Y * _c.Y, _b.Z * _c.Z );
		}
		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector4& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.Set( _b.X * _c.X, _b.Y * _c.Y, _b.Z * _c.Z, _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( _b.X * _c.X, _b.Y * _c.Y, _b.Z * _c.Z, _b.W * _c.W );
		}

		void Mul3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( _b.X * _c.X, _b.Y * _c.Y, _b.Z * _c.Z, _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Mul( const RedScalar& _a, const RedScalar& _b )
		{
			return ( _a.X * _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Mul( const RedVector2& _a, const RedScalar& _b )
		{
			return RedVector2( _a.X * _b.X, _a.Y * _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Mul( const RedVector3& _a, const RedScalar& _b )
		{
			return RedVector3( _a.X * _b.X, _a.Y * _b.X, _a.Z * _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Mul( const RedVector4& _a, const RedScalar& _b )
		{
			return RedVector4( _a.X * _b.X, _a.Y * _b.X, _a.Z * _b.X, _a.W * _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Mul( const RedVector2& _a, const RedVector2& _b )
		{
			return RedVector2( _a.X * _b.X, _a.Y * _b.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Mul( const RedVector3& _a, const RedVector3& _b )
		{
			return RedVector3( _a.X * _b.X, _a.Y * _b.Y, _a.Z * _b.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Mul( const RedVector4& _a, const RedVector3& _b )
		{
			return RedVector4( _a.X * _b.X, _a.Y * _b.Y, _a.Z * _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Mul( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _a.X * _b.X, _a.Y * _b.Y, _a.Z * _b.Z, _a.W * _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Mul3( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _a.X * _b.X, _a.Y * _b.Y, _a.Z * _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedScalar& _a, const RedScalar& _b )
		{
			_a.Set( _a.X * _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector2& _a, const RedScalar& _b )
		{
			_a.Set( _a.X * _b.X, _a.Y * _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector3& _a, const RedScalar& _b )
		{
			_a.Set( _a.X * _b.X, _a.Y * _b.X, _a.Z * _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector4& _a, const RedScalar& _b )
		{
			_a.Set( _a.X * _b.X, _a.Y * _b.X, _a.Z * _b.X, _a.W * _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector2& _a, const RedVector2& _b )
		{
			_a.Set( _a.X * _b.X, _a.Y * _b.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector3& _a, const RedVector3& _b )
		{
			_a.Set( _a.X * _b.X, _a.Y * _b.Y, _a.Z * _b.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector4& _a, const RedVector3& _b )
		{
			_a.Set( _a.X * _b.X, _a.Y * _b.Y, _a.Z * _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedVector4& _a, const RedVector4& _b )
		{
			_a.Set( _a.X * _b.X, _a.Y * _b.Y, _a.Z * _b.Z, _a.W * _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul3( RedVector4& _a, const RedVector4& _b )
		{
			_a.Set( _a.X * _b.X, _a.Y * _b.Y, _a.Z * _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		// '/' Divisor Function
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void Div( RedScalar& _a, const RedScalar& _b, const RedScalar& _c )
		{
			_a.Set( _b.X / _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector2& _a, const RedVector2& _b, const RedScalar& _c )
		{
			_a.Set( _b.X / _c.X, _b.Y / _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector3& _a, const RedVector3& _b, const RedScalar& _c )
		{
			_a.Set( _b.X / _c.X, _b.Y / _c.X, _b.Z / _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector4& _a, const RedVector4& _b, const RedScalar& _c )
		{
			_a.Set( _b.X / _c.X, _b.Y / _c.X, _b.Z / _c.X, _b.W / _c.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector2& _a, const RedVector2& _b, const RedVector2& _c )
		{
			_a.Set( _b.X / _c.X, _b.Y / _c.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector3& _a, const RedVector3& _b, const RedVector3& _c )
		{
			_a.Set( _b.X / _c.X, _b.Y / _c.Y, _b.Z / _c.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector4& _a, const RedVector4& _b, const RedVector3& _c )
		{
			_a.Set( _b.X / _c.X, _b.Y / _c.Y, _b.Z / _c.Z, _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( _b.X / _c.X, _b.Y / _c.Y, _b.Z / _c.Z, _b.W / _c.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void Div3( RedVector4& _a, const RedVector4& _b, const RedVector4& _c )
		{
			_a.Set( _b.X / _c.X, _b.Y / _c.Y, _b.Z / _c.Z, _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedScalar Div( const RedScalar& _a, const RedScalar& _b )
		{
			return ( _a.X / _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Div( const RedVector2& _a, const RedScalar& _b )
		{
			return RedVector2( _a.X / _b.X, _a.Y / _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Div( const RedVector3& _a, const RedScalar& _b )
		{
			return RedVector3( _a.X / _b.X, _a.Y / _b.X, _a.Z / _b.X  );
		}
		
		//////////////////////////////////////////////////////////////////////////
		RedVector4 Div( const RedVector4& _a, const RedScalar& _b )
		{
			return RedVector4( _a.X / _b.X, _a.Y / _b.X, _a.Z / _b.X, _a.W / _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector2 Div( const RedVector2& _a, const RedVector2& _b )
		{
			return RedVector2( _a.X / _b.X, _a.Y / _b.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector3 Div( const RedVector3& _a, const RedVector3& _b )
		{
			return RedVector3( _a.X / _b.X, _a.Y / _b.Y, _a.Z / _b.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Div( const RedVector4& _a, const RedVector3& _b )
		{
			return RedVector4( _a.X / _b.X, _a.Y / _b.Y, _a.Z / _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		RedVector4 Div( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _a.X / _b.X, _a.Y / _b.Y, _a.Z / _b.Z, _a.W / _b.W );
		}

		RedVector4 Div3( const RedVector4& _a, const RedVector4& _b )
		{
			return RedVector4( _a.X / _b.X, _a.Y / _b.Y, _a.Z / _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedScalar& _a, const RedScalar& _b )
		{
			_a.Set( _a.X / _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector2& _a, const RedScalar& _b )
		{
			_a.Set( _a.X / _b.X, _a.Y / _b.X );
		}


		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector3& _a, const RedScalar& _b )
		{
			_a.Set( _a.X / _b.X, _a.Y / _b.X , _a.Z / _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector4& _a, const RedScalar& _b )
		{
			_a.Set( _a.X / _b.X, _a.Y / _b.X , _a.Z / _b.X, _a.W / _b.X );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector2& _a, const RedVector2& _b )
		{
			_a.Set( _a.X / _b.X, _a.Y / _b.Y );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector3& _a, const RedVector3& _b )
		{
			_a.Set( _a.X / _b.X, _a.Y / _b.Y, _a.Z / _b.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector4& _a, const RedVector3& _b )
		{
			_a.Set( _a.X / _b.X, _a.Y / _b.Y, _a.Z / _b.Z, _a.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv( RedVector4& _a, const RedVector4& _b )
		{
			_a.Set( _a.X / _b.X, _a.Y / _b.Y, _a.Z / _b.Z, _a.W / _b.W );
		}

		//////////////////////////////////////////////////////////////////////////
		void SetDiv3( RedVector4& _a, const RedVector4& _b )
		{
			_a.Set( _a.X / _b.X, _a.Y / _b.Y, _a.Z / _b.Z, _a.W );
		}
	}
}