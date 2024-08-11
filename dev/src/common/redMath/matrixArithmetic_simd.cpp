#include "redmathbase.h"

namespace RedMath
{
	namespace SIMD
	{
		//////////////////////////////////////////////////////////////////////////
		void Mul( RedMatrix4x4& _a, const RedMatrix4x4& _b, const RedMatrix4x4& _c )
		{
			_a = Mul( _b, _c );
		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix4x4 Mul( const RedMatrix4x4& _a, const RedMatrix4x4& _b )
		{
			RED_MATH_MEM_ALIGNMENT_CHECK(&_a, 16);
			RED_MATH_MEM_ALIGNMENT_CHECK(&_b, 16);

			RedMatrix4x4 result;
			SIMDVector line0, line1, res;

			line0 = _mm_load_ps(&_b.Data[0]);
			line1 = _mm_set1_ps(_a.Data[0]);
			res = _mm_mul_ps(line0, line1);

			line0 = _mm_load_ps(&_b.Data[4]);
			line1 = _mm_set1_ps(_a.Data[1]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			line0 = _mm_load_ps(&_b.Data[8]);
			line1 = _mm_set1_ps(_a.Data[2]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			line0 = _mm_load_ps(&_b.Data[12]);
			line1 = _mm_set1_ps(_a.Data[3]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			_mm_store_ps(&result.Data[0], res);

			line0 = _mm_load_ps(&_b.Data[0]);
			line1 = _mm_set1_ps(_a.Data[4]);
			res = _mm_mul_ps(line0, line1);

			line0 = _mm_load_ps(&_b.Data[4]);
			line1 = _mm_set1_ps(_a.Data[5]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			line0 = _mm_load_ps(&_b.Data[8]);
			line1 = _mm_set1_ps(_a.Data[6]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			line0 = _mm_load_ps(&_b.Data[12]);
			line1 = _mm_set1_ps(_a.Data[7]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			_mm_store_ps(&result.Data[4], res);

			line0 = _mm_load_ps(&_b.Data[0]);
			line1 = _mm_set1_ps(_a.Data[8]);
			res = _mm_mul_ps(line0, line1);

			line0 = _mm_load_ps(&_b.Data[4]);
			line1 = _mm_set1_ps(_a.Data[9]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			line0 = _mm_load_ps(&_b.Data[8]);
			line1 = _mm_set1_ps(_a.Data[10]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			line0 = _mm_load_ps(&_b.Data[12]);
			line1 = _mm_set1_ps(_a.Data[11]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			_mm_store_ps(&result.Data[8], res);

			line0 = _mm_load_ps(&_b.Data[0]);
			line1 = _mm_set1_ps(_a.Data[12]);
			res = _mm_mul_ps(line0, line1);

			line0 = _mm_load_ps(&_b.Data[4]);
			line1 = _mm_set1_ps(_a.Data[13]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			line0 = _mm_load_ps(&_b.Data[8]);
			line1 = _mm_set1_ps(_a.Data[14]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			line0 = _mm_load_ps(&_b.Data[12]);
			line1 = _mm_set1_ps(_a.Data[15]);
			res = _mm_add_ps(_mm_mul_ps(line0, line1), res);

			_mm_store_ps(&result.Data[12], res);

			return result;
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedMatrix4x4& _a, const RedMatrix4x4& _b )
		{
			_a = Mul(_a, _b);
		}

		//////////////////////////////////////////////////////////////////////////
		void Mul( RedMatrix3x3& _a, const RedMatrix3x3& _b, const RedMatrix3x3& _c )
		{
			_a = Mul( _b, _c );
		}

		//////////////////////////////////////////////////////////////////////////
		RedMatrix3x3 Mul( const RedMatrix3x3& _a, const RedMatrix3x3& _b )
		{
			RedMatrix3x3 result;

			result.Row0 = TransformVector( _b, _a.Row0 );
			result.Row1 = TransformVector( _b, _a.Row1 );
			result.Row2 = TransformVector( _b, _a.Row2 );

			return result;
		}

		//////////////////////////////////////////////////////////////////////////
		void SetMul( RedMatrix3x3& _a, const RedMatrix3x3& _b )
		{
			_a = Mul(_a, _b);
		}
	}
}