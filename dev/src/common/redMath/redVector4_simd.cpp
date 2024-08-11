#include "redmathbase.h"

#ifdef RED_ENABLE_SIMD_MATH

namespace RedMath
{
	namespace SIMD
	{
		const RedVector4 RedVector4::ZEROS( 0.0f, 0.0f, 0.0f, 0.0f );
		const RedVector4 RedVector4::ZERO_3D_POINT( 0.0f, 0.0f, 0.0f, 1.0f );
		const RedVector4 RedVector4::ONES( 1.0f, 1.0f, 1.0f, 1.0f );
		const RedVector4 RedVector4::EX( 1.0f, 0.0f, 0.0f, 0.0f );
		const RedVector4 RedVector4::EY( 0.0f, 1.0f, 0.0f, 0.0f );
		const RedVector4 RedVector4::EZ( 0.0f, 0.0f, 1.0f, 0.0f );
		const RedVector4 RedVector4::EW( 0.0f, 0.0f, 0.0f, 1.0f );
	}
}

#endif //RED_ENABLE_SIMD_MATH