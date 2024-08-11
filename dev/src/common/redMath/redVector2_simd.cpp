#include "redmathbase.h"

#ifdef RED_ENABLE_SIMD_MATH

namespace RedMath
{
	namespace SIMD
	{
		const RedVector2 RedVector2::ZEROS( _mm_setzero_ps() );
		const RedVector2 RedVector2::ONES( _mm_set_ps1( 1.0f ) );
		const RedVector2 RedVector2::EX( 1.0f, 0.0f );
		const RedVector2 RedVector2::EY( 0.0f, 1.0f );
	}
}

#endif //RED_ENABLE_SIMD_MATH