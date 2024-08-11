#include "redmathbase.h"

#ifdef RED_ENABLE_SIMD_MATH

namespace RedMath
{
	namespace SIMD
	{
		const RedVector3 RedVector3::ZEROS( 0.0f, 0.0f, 0.0f );
		const RedVector3 RedVector3::ONES( 1.0f, 1.0f, 1.0f );
		const RedVector3 RedVector3::EX( 1.0f, 0.0f, 0.0f );
		const RedVector3 RedVector3::EY( 0.0f, 1.0f, 0.0f );
		const RedVector3 RedVector3::EZ( 0.0f, 0.0f, 1.0f );
	}
}

#endif //RED_ENABLE_SIMD_MATH