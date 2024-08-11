#include "redmathbase.h"

#ifdef RED_ENABLE_SIMD_MATH

namespace RedMath
{
	namespace SIMD
	{
		const RedMatrix4x4 RedMatrix4x4::IDENTITY( RedVector4( 1.0f, 0.0f, 0.0f, 0.0f ),
			RedVector4( 0.0f, 1.0f, 0.0f, 0.0f ),
			RedVector4( 0.0f, 0.0f, 1.0f, 0.0f ),
			RedVector4( 0.0f, 0.0f, 0.0f, 1.0f ) );

		const RedMatrix4x4 RedMatrix4x4::ZEROS( RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ),
			RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ),
			RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ),
			RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ) );

		//////////////////////////////////////////////////////////////////////////
		RedScalar RedMatrix4x4::Det() const
		{
			SIMDVector rot11 = _mm_shuffle_ps(Row1.V, Row1.V, _MM_SHUFFLE(0, 3, 2, 1));
			SIMDVector rot12 = _mm_shuffle_ps(Row2.V, Row2.V, _MM_SHUFFLE(0, 3, 2, 1));
			SIMDVector rot13 = _mm_shuffle_ps(Row3.V, Row3.V, _MM_SHUFFLE(0, 3, 2, 1));
			SIMDVector rot21 = _mm_shuffle_ps(Row1.V, Row1.V, _MM_SHUFFLE(1, 0, 3, 2));
			SIMDVector rot22 = _mm_shuffle_ps(Row2.V, Row2.V, _MM_SHUFFLE(1, 0, 3, 2));
			SIMDVector rot23 = _mm_shuffle_ps(Row3.V, Row3.V, _MM_SHUFFLE(1, 0, 3, 2));
			SIMDVector rot31 = _mm_shuffle_ps(Row1.V, Row1.V, _MM_SHUFFLE(2, 1, 0, 3));
			SIMDVector rot32 = _mm_shuffle_ps(Row2.V, Row2.V, _MM_SHUFFLE(2, 1, 0, 3));
			SIMDVector rot33 = _mm_shuffle_ps(Row3.V, Row3.V, _MM_SHUFFLE(2, 1, 0, 3));

			SIMDVector tmp0 = _mm_sub_ps(_mm_mul_ps(rot22, rot33), _mm_mul_ps(rot32, rot23));
			SIMDVector tmp1 = _mm_sub_ps(_mm_mul_ps(rot32, rot13), _mm_mul_ps(rot12, rot33));
			SIMDVector tmp2 = _mm_sub_ps(_mm_mul_ps(rot12, rot23), _mm_mul_ps(rot22, rot13));

			SIMDVector prod = _mm_mul_ps(rot11, tmp0);
			prod = _mm_add_ps(prod, _mm_mul_ps(rot21, tmp1));
			prod = _mm_add_ps(prod, _mm_mul_ps(rot31, tmp2));

			SIMDVector sign = _mm_set_ps(-1.f, 1.f, -1.f, 1.f);
			prod = _mm_mul_ps(prod, sign);

			// Calculate determinant
			prod = _mm_mul_ps(prod, Row0.V);

#if RED_MATH_SSE_VERSION >= 3
			SIMDVector sum = _mm_hadd_ps(prod, prod);
			sum = _mm_hadd_ps(sum, sum);
#else
			SIMDVector sum = _mm_add_ps(prod, _mm_movehl_ps(prod, prod));
			sum = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(0, 0, 0, 1)));
			sum = _mm_shuffle_ps(sum, sum, 0); // Result expected in all components
#endif
			return sum;
		}

		//////////////////////////////////////////////////////////////////////////
		// This function is using Cramers's rule and is inspired from the following
		// document published by Intel: http://download.intel.com/design/PentiumIII/sml/24504301.pdf
		void RedMatrix4x4::Invert()
		{
			RedMatrix4x4 minors;
			RedMatrix4x4 transpose;
			SIMDVector tmp, det;

			// ---------------------------------------------------
			// Transpose matrix

			tmp = _mm_set1_ps(0.f);	// this is actually unnecessary but avoid an 
									// "uninitialized local variable" Ms compiler warning/error

			tmp = _mm_loadh_pi(_mm_loadl_pi(tmp, (__m64*)(Data)), (__m64*)(Data + 4));
			transpose.Row1.V = _mm_loadh_pi(_mm_loadl_pi(transpose.Row1.V, (__m64*)(Data + 8)), (__m64*)(Data + 12));

			transpose.Row0.V = _mm_shuffle_ps(tmp, transpose.Row1.V, _MM_SHUFFLE(2, 0, 2, 0));
			transpose.Row1.V = _mm_shuffle_ps(transpose.Row1.V, tmp, _MM_SHUFFLE(3, 1, 3, 1));

			tmp = _mm_loadh_pi(_mm_loadl_pi(tmp, (__m64*)(Data + 2)), (__m64*)(Data + 6));
			transpose.Row3.V = _mm_loadh_pi(_mm_loadl_pi(transpose.Row3.V, (__m64*)(Data + 10)), (__m64*)(Data + 14));

			transpose.Row2.V = _mm_shuffle_ps(tmp, transpose.Row3.V, _MM_SHUFFLE(2, 0, 2, 0));
			transpose.Row3.V = _mm_shuffle_ps(transpose.Row3.V, tmp, _MM_SHUFFLE(3, 1, 3, 1));

			// ---------------------------------------------------
			// Calculate cofactors row by row reusing some of the products 

			tmp = _mm_mul_ps(transpose.Row2.V, transpose.Row3.V);
			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 3, 0, 1));

			minors.Row0.V = _mm_mul_ps(transpose.Row1.V, tmp);
			minors.Row1.V = _mm_mul_ps(transpose.Row0.V, tmp);

			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 0, 3, 2));

			minors.Row0.V = _mm_sub_ps(_mm_mul_ps(transpose.Row1.V, tmp), minors.Row0.V);
			minors.Row1.V = _mm_sub_ps(_mm_mul_ps(transpose.Row0.V, tmp), minors.Row1.V);
			minors.Row1.V = _mm_shuffle_ps(minors.Row1.V, minors.Row1.V, _MM_SHUFFLE(1, 0, 3, 2));

			// ---------------------------------------------------

			tmp = _mm_mul_ps(transpose.Row1.V, transpose.Row2.V);
			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 3, 0, 1));

			minors.Row0.V = _mm_add_ps(_mm_mul_ps(transpose.Row3.V, tmp), minors.Row0.V);
			minors.Row3.V = _mm_mul_ps(transpose.Row0.V, tmp);

			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 0, 3, 2));

			minors.Row0.V = _mm_sub_ps(minors.Row0.V, _mm_mul_ps(transpose.Row3.V, tmp));
			minors.Row3.V = _mm_sub_ps(_mm_mul_ps(transpose.Row0.V, tmp), minors.Row3.V);
			minors.Row3.V = _mm_shuffle_ps(minors.Row3.V, minors.Row3.V, _MM_SHUFFLE(1, 0, 3, 2));

			// ---------------------------------------------------

			tmp = _mm_mul_ps(_mm_shuffle_ps(transpose.Row1.V, transpose.Row1.V, _MM_SHUFFLE(1, 0, 3, 2)), transpose.Row3.V);
			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 3, 0, 1));
			transpose.Row2.V = _mm_shuffle_ps(transpose.Row2.V, transpose.Row2.V, _MM_SHUFFLE(1, 0, 3, 2));

			minors.Row0.V = _mm_add_ps(_mm_mul_ps(transpose.Row2.V, tmp), minors.Row0.V);
			minors.Row2.V = _mm_mul_ps(transpose.Row0.V, tmp);

			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 0, 3, 2));

			minors.Row0.V = _mm_sub_ps(minors.Row0.V, _mm_mul_ps(transpose.Row2.V, tmp));
			minors.Row2.V = _mm_sub_ps(_mm_mul_ps(transpose.Row0.V, tmp), minors.Row2.V);
			minors.Row2.V = _mm_shuffle_ps(minors.Row2.V, minors.Row2.V, _MM_SHUFFLE(1, 0, 3, 2));

			// ---------------------------------------------------

			tmp = _mm_mul_ps(transpose.Row0.V, transpose.Row1.V);
			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 3, 0, 1));

			minors.Row2.V = _mm_add_ps(_mm_mul_ps(transpose.Row3.V, tmp), minors.Row2.V);
			minors.Row3.V = _mm_sub_ps(_mm_mul_ps(transpose.Row2.V, tmp), minors.Row3.V);

			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 0, 3, 2));

			minors.Row2.V = _mm_sub_ps(_mm_mul_ps(transpose.Row3.V, tmp), minors.Row2.V);
			minors.Row3.V = _mm_sub_ps(minors.Row3.V, _mm_mul_ps(transpose.Row2.V, tmp));

			// ---------------------------------------------------

			tmp = _mm_mul_ps(transpose.Row0.V, transpose.Row3.V);
			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 3, 0, 1));

			minors.Row1.V = _mm_sub_ps(minors.Row1.V, _mm_mul_ps(transpose.Row2.V, tmp));
			minors.Row2.V = _mm_add_ps(_mm_mul_ps(transpose.Row1.V, tmp), minors.Row2.V);

			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 0, 3, 2));

			minors.Row1.V = _mm_add_ps(_mm_mul_ps(transpose.Row2.V, tmp), minors.Row1.V);
			minors.Row2.V = _mm_sub_ps(minors.Row2.V, _mm_mul_ps(transpose.Row1.V, tmp));

			// ---------------------------------------------------

			tmp = _mm_mul_ps(transpose.Row0.V, transpose.Row2.V);
			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 3, 0, 1));

			minors.Row1.V = _mm_add_ps(_mm_mul_ps(transpose.Row3.V, tmp), minors.Row1.V);
			minors.Row3.V = _mm_sub_ps(minors.Row3.V, _mm_mul_ps(transpose.Row1.V, tmp));

			tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 0, 3, 2));

			minors.Row1.V = _mm_sub_ps(minors.Row1.V, _mm_mul_ps(transpose.Row3.V, tmp));
			minors.Row3.V = _mm_add_ps(_mm_mul_ps(transpose.Row1.V, tmp), minors.Row3.V);

			// ---------------------------------------------------
			// Calculate determinant and reciprocal

			det = _mm_mul_ps(transpose.Row0.V, minors.Row0.V);
			det = _mm_add_ps(_mm_shuffle_ps(det, det, _MM_SHUFFLE(1, 0, 3, 2)), det);
			det = _mm_add_ss(_mm_shuffle_ps(det, det, _MM_SHUFFLE(2, 3, 0, 1)), det);
#if 1
			// Using fast rcpps and Newton-Raphson algorithm
			tmp = _mm_rcp_ss(det);
			det = _mm_sub_ss(_mm_add_ss(tmp, tmp), _mm_mul_ss(det, _mm_mul_ss(tmp, tmp)));
#else
			// Using higher latency division (more precise)
			det = _mm_div_ss(RedVector4::ONES.V, det);
#endif
			det = _mm_shuffle_ps(det, det, _MM_SHUFFLE(0, 0, 0, 0));

			Row0.V = _mm_mul_ps(det, minors.Row0.V);
			Row1.V = _mm_mul_ps(det, minors.Row1.V);
			Row2.V = _mm_mul_ps(det, minors.Row2.V);
			Row3.V = _mm_mul_ps(det, minors.Row3.V);
		}
	}
}

#endif //RED_ENABLE_SIMD_MATH