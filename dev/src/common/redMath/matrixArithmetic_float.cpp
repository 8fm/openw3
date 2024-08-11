#include "matrixArithmetic_float.h"
#include "vectorArithmetic_float.h"

namespace RedMath
{
	namespace FLOAT
	{
		//////////////////////////////////////////////////////////////////////////
		void Mul( RedMatrix4x4& _a, const RedMatrix4x4& _b, const RedMatrix4x4& _c )
		{
			_a.m00 = ( _b.m00 * _c.m00 ) + ( _b.m01 * _c.m10 ) + ( _b.m02 * _c.m20 ) + ( _b.m03 * _c.m30 );
			_a.m01 = ( _b.m00 * _c.m01 ) + ( _b.m01 * _c.m11 ) + ( _b.m02 * _c.m21 ) + ( _b.m03 * _c.m31 );
			_a.m02 = ( _b.m00 * _c.m02 ) + ( _b.m01 * _c.m12 ) + ( _b.m02 * _c.m22 ) + ( _b.m03 * _c.m32 );
			_a.m03 = ( _b.m00 * _c.m03 ) + ( _b.m01 * _c.m13 ) + ( _b.m02 * _c.m23 ) + ( _b.m03 * _c.m33 );
			
			_a.m10 = ( _b.m10 * _c.m00 ) + ( _b.m11 * _c.m10 ) + ( _b.m12 * _c.m20 ) + ( _b.m13 * _c.m30 );
			_a.m11 = ( _b.m10 * _c.m01 ) + ( _b.m11 * _c.m11 ) + ( _b.m12 * _c.m21 ) + ( _b.m13 * _c.m31 );
			_a.m12 = ( _b.m10 * _c.m02 ) + ( _b.m11 * _c.m12 ) + ( _b.m12 * _c.m22 ) + ( _b.m13 * _c.m32 );
			_a.m13 = ( _b.m10 * _c.m03 ) + ( _b.m11 * _c.m13 ) + ( _b.m12 * _c.m23 ) + ( _b.m13 * _c.m33 );

			_a.m20 = ( _b.m20 * _c.m00 ) + ( _b.m21 * _c.m10 ) + ( _b.m22 * _c.m20 ) + ( _b.m23 * _c.m30 );
			_a.m21 = ( _b.m20 * _c.m01 ) + ( _b.m21 * _c.m11 ) + ( _b.m22 * _c.m21 ) + ( _b.m23 * _c.m31 );
			_a.m22 = ( _b.m20 * _c.m02 ) + ( _b.m21 * _c.m12 ) + ( _b.m22 * _c.m22 ) + ( _b.m23 * _c.m32 );
			_a.m23 = ( _b.m20 * _c.m03 ) + ( _b.m21 * _c.m13 ) + ( _b.m22 * _c.m23 ) + ( _b.m23 * _c.m33 );

			_a.m30 = ( _b.m30 * _c.m00 ) + ( _b.m31 * _c.m10 ) + ( _b.m32 * _c.m20 ) + ( _b.m33 * _c.m30 );
			_a.m31 = ( _b.m30 * _c.m01 ) + ( _b.m31 * _c.m11 ) + ( _b.m32 * _c.m21 ) + ( _b.m33 * _c.m31 );
			_a.m32 = ( _b.m30 * _c.m02 ) + ( _b.m31 * _c.m12 ) + ( _b.m32 * _c.m22 ) + ( _b.m33 * _c.m32 );
			_a.m33 = ( _b.m30 * _c.m03 ) + ( _b.m31 * _c.m13 ) + ( _b.m32 * _c.m23 ) + ( _b.m33 * _c.m33 );  
		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix4x4 Mul( const RedMatrix4x4& _a, const RedMatrix4x4& _b )
		{
			RedMatrix4x4 result;
			result.m00 = ( _a.m00 * _b.m00 ) + ( _a.m01 * _b.m10 ) + ( _a.m02 * _b.m20 ) + ( _a.m03 * _b.m30 );
			result.m01 = ( _a.m00 * _b.m01 ) + ( _a.m01 * _b.m11 ) + ( _a.m02 * _b.m21 ) + ( _a.m03 * _b.m31 );
			result.m02 = ( _a.m00 * _b.m02 ) + ( _a.m01 * _b.m12 ) + ( _a.m02 * _b.m22 ) + ( _a.m03 * _b.m32 );
			result.m03 = ( _a.m00 * _b.m03 ) + ( _a.m01 * _b.m13 ) + ( _a.m02 * _b.m23 ) + ( _a.m03 * _b.m33 );
			result.m10 = ( _a.m10 * _b.m00 ) + ( _a.m11 * _b.m10 ) + ( _a.m12 * _b.m20 ) + ( _a.m13 * _b.m30 );
			result.m11 = ( _a.m10 * _b.m01 ) + ( _a.m11 * _b.m11 ) + ( _a.m12 * _b.m21 ) + ( _a.m13 * _b.m31 );
			result.m12 = ( _a.m10 * _b.m02 ) + ( _a.m11 * _b.m12 ) + ( _a.m12 * _b.m22 ) + ( _a.m13 * _b.m32 );
			result.m13 = ( _a.m10 * _b.m03 ) + ( _a.m11 * _b.m13 ) + ( _a.m12 * _b.m23 ) + ( _a.m13 * _b.m33 );
			result.m20 = ( _a.m20 * _b.m00 ) + ( _a.m21 * _b.m10 ) + ( _a.m22 * _b.m20 ) + ( _a.m23 * _b.m30 );
			result.m21 = ( _a.m20 * _b.m01 ) + ( _a.m21 * _b.m11 ) + ( _a.m22 * _b.m21 ) + ( _a.m23 * _b.m31 );
			result.m22 = ( _a.m20 * _b.m02 ) + ( _a.m21 * _b.m12 ) + ( _a.m22 * _b.m22 ) + ( _a.m23 * _b.m32 );
			result.m23 = ( _a.m20 * _b.m03 ) + ( _a.m21 * _b.m13 ) + ( _a.m22 * _b.m23 ) + ( _a.m23 * _b.m33 );
			result.m30 = ( _a.m30 * _b.m00 ) + ( _a.m31 * _b.m10 ) + ( _a.m32 * _b.m20 ) + ( _a.m33 * _b.m30 );
			result.m31 = ( _a.m30 * _b.m01 ) + ( _a.m31 * _b.m11 ) + ( _a.m32 * _b.m21 ) + ( _a.m33 * _b.m31 );
			result.m32 = ( _a.m30 * _b.m02 ) + ( _a.m31 * _b.m12 ) + ( _a.m32 * _b.m22 ) + ( _a.m33 * _b.m32 );
			result.m33 = ( _a.m30 * _b.m03 ) + ( _a.m31 * _b.m13 ) + ( _a.m32 * _b.m23 ) + ( _a.m33 * _b.m33 );  

			return result;
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedMatrix4x4& _a, const RedMatrix4x4& _b )
		{
			_a.Set( 
				RedVector4( ( _a.m00 * _b.m00 ) + ( _a.m01 * _b.m10 ) + ( _a.m02 * _b.m20 ) + ( _a.m03 * _b.m30 ) , 
							( _a.m00 * _b.m01 ) + ( _a.m01 * _b.m11 ) + ( _a.m02 * _b.m21 ) + ( _a.m03 * _b.m31 ),
							( _a.m00 * _b.m02 ) + ( _a.m01 * _b.m12 ) + ( _a.m02 * _b.m22 ) + ( _a.m03 * _b.m32 ),
							( _a.m00 * _b.m03 ) + ( _a.m01 * _b.m13 ) + ( _a.m02 * _b.m23 ) + ( _a.m03 * _b.m33 ) ),
				RedVector4( ( _a.m10 * _b.m00 ) + ( _a.m11 * _b.m10 ) + ( _a.m12 * _b.m20 ) + ( _a.m13 * _b.m30 ),
							( _a.m10 * _b.m01 ) + ( _a.m11 * _b.m11 ) + ( _a.m12 * _b.m21 ) + ( _a.m13 * _b.m31 ),
							( _a.m10 * _b.m02 ) + ( _a.m11 * _b.m12 ) + ( _a.m12 * _b.m22 ) + ( _a.m13 * _b.m32 ),
							( _a.m10 * _b.m03 ) + ( _a.m11 * _b.m13 ) + ( _a.m12 * _b.m23 ) + ( _a.m13 * _b.m33 ) ),
				RedVector4( ( _a.m20 * _b.m00 ) + ( _a.m21 * _b.m10 ) + ( _a.m22 * _b.m20 ) + ( _a.m23 * _b.m30 ),
							( _a.m20 * _b.m01 ) + ( _a.m21 * _b.m11 ) + ( _a.m22 * _b.m21 ) + ( _a.m23 * _b.m31 ),
							( _a.m20 * _b.m02 ) + ( _a.m21 * _b.m12 ) + ( _a.m22 * _b.m22 ) + ( _a.m23 * _b.m32 ),
							( _a.m20 * _b.m03 ) + ( _a.m21 * _b.m13 ) + ( _a.m22 * _b.m23 ) + ( _a.m23 * _b.m33 ) ),
				RedVector4( ( _a.m30 * _b.m00 ) + ( _a.m31 * _b.m10 ) + ( _a.m32 * _b.m20 ) + ( _a.m33 * _b.m30 ),
							( _a.m30 * _b.m01 ) + ( _a.m31 * _b.m11 ) + ( _a.m32 * _b.m21 ) + ( _a.m33 * _b.m31 ),
							( _a.m30 * _b.m02 ) + ( _a.m31 * _b.m12 ) + ( _a.m32 * _b.m22 ) + ( _a.m33 * _b.m32 ),
							( _a.m30 * _b.m03 ) + ( _a.m31 * _b.m13 ) + ( _a.m32 * _b.m23 ) + ( _a.m33 * _b.m33 ) )
							);
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedMatrix3x3& _a, const RedMatrix3x3& _b, const RedMatrix3x3& _c )
		{
			_a.Row0.X = ( _b.Row0.X * _c.Row0.X ) + ( _b.Row0.Y * _c.Row1.X ) + ( _b.Row0.Z * _c.Row2.X );
			_a.Row0.Y = ( _b.Row0.X * _c.Row0.Y ) + ( _b.Row0.Y * _c.Row1.Y ) + ( _b.Row0.Z * _c.Row2.Y );
			_a.Row0.Z = ( _b.Row0.X * _c.Row0.Z ) + ( _b.Row0.Y * _c.Row1.Z ) + ( _b.Row0.Z * _c.Row2.Z );

			_a.Row1.X = ( _b.Row1.X * _c.Row0.X ) + ( _b.Row1.Y * _c.Row1.X ) + ( _b.Row1.Z * _c.Row2.X );
			_a.Row1.Y = ( _b.Row1.X * _c.Row0.Y ) + ( _b.Row1.Y * _c.Row1.Y ) + ( _b.Row1.Z * _c.Row2.Y );
			_a.Row1.Z = ( _b.Row1.X * _c.Row0.Z ) + ( _b.Row1.Y * _c.Row1.Z ) + ( _b.Row1.Z * _c.Row2.Z );

			_a.Row2.X = ( _b.Row2.X * _c.Row0.X ) + ( _b.Row2.Y * _c.Row1.X ) + ( _b.Row2.Z * _c.Row2.X );
			_a.Row2.Y = ( _b.Row2.X * _c.Row0.Y ) + ( _b.Row2.Y * _c.Row1.Y ) + ( _b.Row2.Z * _c.Row2.Y );
			_a.Row2.Z = ( _b.Row2.X * _c.Row0.Z ) + ( _b.Row2.Y * _c.Row1.Z ) + ( _b.Row2.Z * _c.Row2.Z );
		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix3x3 Mul( const RedMatrix3x3& _a, const RedMatrix3x3& _b )
		{
			RedMatrix3x3 result;
			Mul(result, _a, _b);
			return result;
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedMatrix3x3& _a, const RedMatrix3x3& _b )
		{
			RedMatrix3x3 result;
			Mul(result, _a, _b);
			_a = result;
		}

	}
}