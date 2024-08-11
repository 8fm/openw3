/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef __MATRIX4UNALIGNED_H__
#define __MATRIX4UNALIGNED_H__

#include "vector4unaligned.h"
#include <vectormath.h>

// Matrix4Unaligned is meant to store a Matrix4, except that padding and alignment are
// 4-byte granular (GPU), rather than 16-byte (SSE). This is to bridge
// the SSE math library with GPU structures that assume 4-byte granularity.
// While a Matrix4 has many operations, Matrix4Unaligned can only be converted to and from Matrix4.
struct Matrix4Unaligned
{
	Vector4Unaligned x;
	Vector4Unaligned y;
	Vector4Unaligned z;
	Vector4Unaligned w;
};

inline Matrix4Unaligned ToMatrix4Unaligned( const Matrix4& r )
{
	const Matrix4Unaligned result = { ToVector4Unaligned(r.getCol0()), ToVector4Unaligned(r.getCol1()), ToVector4Unaligned(r.getCol2()), ToVector4Unaligned(r.getCol3()) };
	return result;
}

inline Matrix4 ToMatrix4( const Matrix4Unaligned& r )
{
	return Matrix4( ToVector4(r.x), ToVector4(r.y), ToVector4(r.z), ToVector4(r.w) );
}

#endif
