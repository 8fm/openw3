#include "redMatrix4x4_float.h"

namespace RedMath
{
	namespace FLOAT
	{
		const RedMatrix4x4 RedMatrix4x4::IDENTITY( RedVector4( 1.0f, 0.0f, 0.0f, 0.0f ),
												   RedVector4( 0.0f, 1.0f, 0.0f, 0.0f ),
												   RedVector4( 0.0f, 0.0f, 1.0f, 0.0f ),
												   RedVector4( 0.0f, 0.0f, 0.0f, 1.0f ) );

		const RedMatrix4x4 RedMatrix4x4::ZEROS( RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ),
												RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ),
												RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ),
												RedVector4( 0.0f, 0.0f, 0.0f, 0.0f ) );
	}
}