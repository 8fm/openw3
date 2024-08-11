#include "redmathbase.h"

namespace RedMath
{
	namespace SIMD
	{
		// RedQuaternion Constant
		const RedQuaternion RedQuaternion::IDENTITY( 0.0f, 0.0f, 0.0f, 1.0f );

		// RedQsTransformConstant
		const RedQsTransform RedQsTransform::IDENTITY( RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ), RedQuaternion( 0.0f, 0.0f, 0.0f, 1.0f ), RedVector4( 1.0f, 1.0f, 1.0f, 0.0f ) );
	}
}