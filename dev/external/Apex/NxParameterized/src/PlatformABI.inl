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

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

PX_INLINE physx::PxU32 PlatformABI::align(physx::PxU32 len, physx::PxU32 border)
{
	physx::PxU32 n = (len + border - 1) / border;
	return n * border;
}

PX_INLINE bool PlatformABI::isNormal() const
{
	return 1 == sizes.Char && 1 == sizes.Bool				//Wide (> 1) bytes not supported
		&& 4 == sizes.real									//Some code relies on short physx::PxReal (physx::PxF32)
		&& ( 4 == sizes.pointer || 8 == sizes.pointer );	//Some code relies on pointers being either 32- or 64-bit
}

PX_INLINE physx::PxU32 PlatformABI::getMetaEntryAlignment() const
{
	return physx::PxMax(aligns.i32, aligns.pointer);
}

PX_INLINE physx::PxU32 PlatformABI::getMetaInfoAlignment() const
{
	return PxMax3(getHintAlignment(), aligns.i32, aligns.pointer);
}

PX_INLINE physx::PxU32 PlatformABI::getHintAlignment() const
{
	return physx::PxMax(aligns.i32, getHintValueAlignment());
}

PX_INLINE physx::PxU32 PlatformABI::getHintValueAlignment() const
{
	return PxMax3(aligns.pointer, aligns.i64, aligns.f64);
}

PX_INLINE physx::PxU32 PlatformABI::getHintValueSize() const
{
	return align(8, getHintValueAlignment()); //Size of union = aligned size of maximum element
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<bool>() const
{
	return aligns.Bool;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxI8>() const
{
	return aligns.i8;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxI16>() const
{
	return aligns.i16;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxI32>() const
{
	return aligns.i32;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxI64>() const
{
	return aligns.i64;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxU8>() const
{
	return getAlignment<physx::PxI8>();
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxU16>() const
{
	return getAlignment<physx::PxI16>();
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxU32>() const
{
	return getAlignment<physx::PxI32>();
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxU64>() const
{
	return getAlignment<physx::PxI64>();
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxF32>() const
{
	return aligns.f32;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxF64>() const
{
	return aligns.f64;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxVec2>() const
{
	return aligns.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxVec3>() const
{
	return aligns.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxVec4>() const
{
	return aligns.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxQuat>() const
{
	return aligns.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxMat33>() const
{
	return aligns.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxMat34Legacy>() const
{
	return aligns.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxMat44>() const
{
	return aligns.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxBounds3>() const
{
	return aligns.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<void *>() const
{
	return aligns.pointer;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getAlignment<physx::PxTransform>() const
{
	return aligns.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<bool>() const
{
	return sizes.Bool;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxI8>() const
{
	return 1;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxI16>() const
{
	return 2;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxI32>() const
{
	return 4;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxI64>() const
{
	return 8;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxU8>() const
{
	return getSize<physx::PxI8>();
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxU16>() const
{
	return getSize<physx::PxI16>();
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxU32>() const
{
	return getSize<physx::PxI32>();
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxU64>() const
{
	return getSize<physx::PxI64>();
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxF32>() const
{
	return 4;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxF64>() const
{
	return 8;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxVec2>() const
{
	return 2 * sizes.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxVec3>() const
{
	return 3 * sizes.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxVec4>() const
{
	return 4 * sizes.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxQuat>() const
{
	return 4 * sizes.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxMat33>() const
{
	return 9 * sizes.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxMat34Legacy>() const
{
	return 12 * sizes.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxMat44>() const
{
	return 16 * sizes.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxBounds3>() const
{
	return 6 * sizes.real;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<void *>() const
{
	return sizes.pointer;
}

template <> PX_INLINE physx::PxU32 PlatformABI::getSize<physx::PxTransform>() const
{
	return 7 * sizes.real;
}
