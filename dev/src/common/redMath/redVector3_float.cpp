#include "redVector3_float.h"

namespace RedMath
{
	namespace FLOAT
	{
		const RedVector3 RedVector3::ZEROS = RedVector3( 0.0f, 0.0f, 0.0f );
		const RedVector3 RedVector3::ONES = RedVector3( 1.0f, 1.0f, 1.0f );
		const RedVector3 RedVector3::EX = RedVector3( 1.0f, 0.0f, 0.0f );
		const RedVector3 RedVector3::EY = RedVector3( 0.0f, 1.0f, 0.0f );
		const RedVector3 RedVector3::EZ = RedVector3( 0.0f, 0.0f, 1.0f );
	}
}