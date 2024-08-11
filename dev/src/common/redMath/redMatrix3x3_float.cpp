#include "redMatrix3x3_float.h"
#include "redVector3_float.h"

namespace RedMath
{
	namespace FLOAT
	{
		const RedMatrix3x3 RedMatrix3x3::ZEROS( RedVector3::ZEROS, RedVector3::ZEROS, RedVector3::ZEROS );
		const RedMatrix3x3 RedMatrix3x3::IDENTITY( RedVector3::EX, RedVector3::EY, RedVector3::EZ );
	}
}