// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
#ifndef __APEX_STREAM_H__
#define __APEX_STREAM_H__

#include "NxApexDefs.h"
#include "foundation/PxPlane.h"


namespace physx
{
namespace apex
{

// Public, useful operators for serializing nonversioned data follow.
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, bool& b)
{
	b = (0 != stream.readByte());
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxI8& b)
{
	b = stream.readByte();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxI16& w)
{
	w = stream.readWord();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxI32& d)
{
	d = stream.readDword();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxU8& b)
{
	b = stream.readByte();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxU16& w)
{
	w = stream.readWord();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxU32& d)
{
	d = stream.readDword();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxF32& f)
{
	f = stream.readFloat();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxF64& f)
{
	f = stream.readDouble();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxVec2& v)
{
	stream >> v.x >> v.y;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxVec3& v)
{
	stream >> v.x >> v.y >> v.z;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxVec4& v)
{
	stream >> v.x >> v.y >> v.z >> v.w;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxBounds3& b)
{
	stream >> b.minimum >> b.maximum;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxQuat& q)
{
	stream >> q.x >> q.y >> q.z >> q.w;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxPlane& p)
{
	stream >> p.n.x >> p.n.y >> p.n.z >> p.d;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxMat44& m)
{
	stream >> m.column0 >> m.column1 >> m.column2 >> m.column3;
	return stream;
}

// The opposite of the above operators--takes data and writes it to a stream.
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const bool b)
{
	stream.storeByte(b ? (physx::PxU8)1 : (physx::PxU8)0);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxI8 b)
{
	stream.storeByte(b);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxI16 w)
{
	stream.storeWord(w);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxI32 d)
{
	stream.storeDword(d);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxU8 b)
{
	stream.storeByte(b);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxU16 w)
{
	stream.storeWord(w);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxU32 d)
{
	stream.storeDword(d);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxF32 f)
{
	stream.storeFloat(f);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxF64 f)
{
	stream.storeDouble(f);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxVec2& v)
{
	stream << v.x << v.y;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxVec3& v)
{
	stream << v.x << v.y << v.z;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxVec4& v)
{
	stream << v.x << v.y << v.z << v.w;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxBounds3& b)
{
	stream << b.minimum << b.maximum;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxQuat& q)
{
	stream << q.x << q.y << q.z << q.w;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxPlane& p)
{
	stream << p.n.x << p.n.y << p.n.z << p.d;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxMat44& m)
{
	stream << m.column0 << m.column1 << m.column2 << m.column3;
	return stream;
}


}
} // end namespace apex

#endif // __APEX_STREAM_H__