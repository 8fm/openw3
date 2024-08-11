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

template <typename T> PX_INLINE void PlatformOutputStream::align()
{
	align(mTargetParams.getAlignment<T>());
}

template <typename T> PX_INLINE physx::PxU32 PlatformOutputStream::storeSimple(T x)
{
	align(mTargetParams.getAlignment<T>());

	physx::PxU32 off = size();

	data.append(x);

	char *p = getData() + off;

	//It is unsafe to call SwapBytes directly on floats (doubles, vectors, etc.)
	//because values may become NaNs which will lead to all sorts of errors
	if( mCurParams.endian != mTargetParams.endian )
		SwapBytes(p, sizeof(T), GetDataType<T>::value);

	return off;
}

template<> PX_INLINE physx::PxI32 PlatformOutputStream::storeSimpleArraySlow<physx::PxMat34Legacy>(Handle &)
{
	PX_ASSERT(0);
	return -1;
}

template <typename T> PX_INLINE physx::PxI32 PlatformOutputStream::storeSimpleArraySlow(Handle &handle)
{
	physx::PxI32 n;
	handle.getArraySize(n);

	align<T>();
	physx::PxU32 off = size();

	data.reserve(size() + n * physx::PxMax(mTargetParams.getAlignment<T>(), mTargetParams.getSize<T>()));

	for(physx::PxI32 i = 0; i < n; ++i)
	{
		handle.set(i);

		T val;
		handle.getParam<T>(val);
		storeSimple<T>(val);

		handle.popIndex();
	}

	return off;
}

template<> PX_INLINE physx::PxU32 PlatformOutputStream::storeSimpleArray<physx::PxMat34Legacy>(Handle &)
{
	PX_ASSERT(0);
	return PX_MAX_U32;
}

template <typename T> PX_INLINE physx::PxU32 PlatformOutputStream::storeSimpleArray(Handle &handle)
{
	if( TYPE_BOOL != handle.parameterDefinition()->child(0)->type() //Bools are special (see custom version of storeSimple)
			&& mTargetParams.getSize<T>() == mCurParams.getSize<T>()
			&& mTargetParams.getSize<T>() >= mTargetParams.getAlignment<T>()
			&& mCurParams.getSize<T>() >= mCurParams.getAlignment<T>() ) //No gaps between array elements on both platforms
	{
		//Fast path

		physx::PxI32 n;
		handle.getArraySize(n);

		align<T>();
		physx::PxU32 off = size();

		const physx::PxI32 elemSize = (physx::PxI32)sizeof(T);

		skipBytes((physx::PxU32)(elemSize * n));

		char *p = getData() + off;
		handle.getParamArray<T>((T *)p, n);

		if( mCurParams.endian != mTargetParams.endian )
		{
			for(physx::PxI32 i = 0; i < n; ++i)
			{
				SwapBytes(p, elemSize, GetDataType<T>::value);
					p += elemSize;
			}
		}

		return off;
	}
	else
	{
		return storeSimpleArraySlow<T>(handle);
	}
}
