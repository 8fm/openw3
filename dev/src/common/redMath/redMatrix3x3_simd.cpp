#include "redmathbase.h"

#ifdef RED_ENABLE_SIMD_MATH

namespace RedMath
{
	namespace SIMD
	{
		const RedMatrix3x3 RedMatrix3x3::ZEROS( RedVector3::ZEROS, RedVector3::ZEROS, RedVector3::ZEROS );
		const RedMatrix3x3 RedMatrix3x3::IDENTITY( RedVector3::EX, RedVector3::EY, RedVector3::EZ );
	}
}

#endif //RED_ENABLE_SIMD_MATH