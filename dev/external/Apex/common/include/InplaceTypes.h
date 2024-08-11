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

#ifndef __APEX_INPLACE_TYPES_H__
#define __APEX_INPLACE_TYPES_H__

#include "PsShare.h"
#include "PxMat34Legacy.h"

namespace physx
{
namespace apex
{

#ifndef __CUDACC__
template <typename T>
struct InplaceTypeTraits
{
	enum { hasReflect = true };
};

template <> struct InplaceTypeTraits<bool>
{
	enum { hasReflect = false };
};
template <> struct InplaceTypeTraits<physx::PxI32>
{
	enum { hasReflect = false };
};
template <> struct InplaceTypeTraits<physx::PxU32>
{
	enum { hasReflect = false };
};
template <> struct InplaceTypeTraits<physx::PxF32>
{
	enum { hasReflect = false };
};
template <> struct InplaceTypeTraits<physx::PxVec3>
{
	enum { hasReflect = false };
};
template <> struct InplaceTypeTraits<physx::PxVec4>
{
	enum { hasReflect = false };
};
template <> struct InplaceTypeTraits<physx::PxMat34Legacy>
{
	enum { hasReflect = false };
};
template <> struct InplaceTypeTraits<physx::PxBounds3>
{
	enum { hasReflect = false };
};

template <typename T> struct InplaceTypeTraits<T*>
{
	enum { hasReflect = false };
};

#endif


class InplaceHandleBase
{
protected:
	friend class InplaceStorage;
	static const physx::PxU32 NULL_VALUE = physx::PxU32(-1);
	physx::PxU32 _value;

public:
	PX_INLINE InplaceHandleBase()
	{
		_value = NULL_VALUE;
	}

	PX_INLINE void setNull()
	{
		_value = NULL_VALUE;
	}

	PX_CUDA_CALLABLE PX_INLINE bool isNull() const
	{
		return _value == NULL_VALUE;
	}

#ifdef __CUDACC__
	template <typename T>
	__device__ PX_INLINE const T* resolveAndCastTo(const physx::PxU8* constMem) const
	{
		return (const T*)(constMem + _value);
	}
#else
	template <typename T, typename S>
	PX_INLINE T* resolveAndCastTo(S& storage) const
	{
		return storage.resolveAndCastTo<T>(*this);
	}
#endif
};

template <typename T>
class InplaceHandle : public InplaceHandleBase
{
public:
	PX_INLINE InplaceHandle() {}

#ifdef __CUDACC__
	PX_CUDA_CALLABLE PX_INLINE const T* resolve(const physx::PxU8* constMem) const
	{
		return (const T*)(constMem + _value);
	}
#else
	template <typename S>
	PX_INLINE T* resolve(S& storage) const
	{
		return storage.resolve(*this);
	}

	template <typename S>
	PX_INLINE T* alloc(S& storage, physx::PxU32 count = 1)
	{
		return storage.alloc(*this, count);
	}

	template <typename S>
	PX_INLINE T* allocOrResolve(S& storage, physx::PxU32 count = 1)
	{
		return isNull() ? storage.alloc(*this, count) : storage.resolve(*this);
	}
#endif
};


// if AutoFreeElems is true, then InplaceHandles in deleted elements are automaticly got free on array resize!
template <typename T, bool AutoFreeElems = false>
class InplaceArray
{
	physx::PxU32 _size;
	InplaceHandle<T> _elems;

public:

	PX_CUDA_CALLABLE PX_INLINE physx::PxU32 getSize() const
	{
		return _size;
	}

#ifdef __CUDACC__
	PX_CUDA_CALLABLE PX_INLINE const T* getElems(const physx::PxU8* constMem) const
	{
		return _elems.resolve(constMem);
	}
#else
	template <typename S>
	PX_INLINE T* getElems(S& storage) const
	{
		return _elems.resolve(storage);
	}

	PX_INLINE InplaceArray()
	{
		_size = 0;
	}

	template <typename S>
	PX_INLINE bool resize(S& storage, physx::PxU32 size)
	{
		if (storage.realloc(_elems, _size, size))
		{
			_size = size;
			return true;
		}
		return false;
	}

	template <typename R>
	void reflect(R& r)
	{
		r.reflect(_size);
		r.reflect(_elems, AutoFreeElems);
	}
#endif
};


}
} // end namespace physx::apex

#endif // __APEX_INPLACE_TYPES_H__
