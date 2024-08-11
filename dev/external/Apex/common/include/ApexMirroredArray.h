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

#ifndef APEX_MIRRORED_ARRAY_H
#define APEX_MIRRORED_ARRAY_H

#include "NxApexDefs.h"

#include "ApexMirrored.h"
#include <new>

#if defined(__CUDACC__) || defined(PX_PS3) || defined(PX_ANDROID) || defined(PX_PS4)
#define DEFAULT_NAME "unassigned"
#else
#include <typeinfo>
#define DEFAULT_NAME typeid(T).name()
#endif

#pragma warning(push)
#pragma warning(disable:4348)


namespace physx
{
namespace apex
{

template <class T>
class ApexMirroredArray
{
	PX_NOCOPY(ApexMirroredArray);

public:
	/*!
	Default array constructor.
	Initialize an empty array
	*/
	explicit PX_INLINE ApexMirroredArray(NiApexScene& scene, NV_ALLOC_INFO_PARAMS_DECL("", 0, DEFAULT_NAME, UNASSIGNED)) :
		mData(scene, NV_ALLOC_INFO_PARAMS_INPUT()), mCapacity(0), mSize(0) {};

	/*!
	Default destructor
	*/
	PX_INLINE ~ApexMirroredArray()
	{
		mData.free();
	}

	/*!
	Return an element from this array. Operation is O(1).
	\param i
	The index of the element that will be returned.
	\return
	Element i in the array.
	*/
	PX_INLINE const T& get(physx::PxU32 i) const
	{
		return mData.getCpuPtr()[i];
	}

	/*!
	Return an element from this array. Operation is O(1).
	\param i
	The index of the element that will be returned.
	\return
	Element i in the array.
	*/
	PX_INLINE T& get(physx::PxU32 i)
	{
		return mData.getCpuPtr()[i];
	}

	/*!
	Array indexing operator.
	\param i
	The index of the element that will be returned.
	\return
	The element i in the array.
	*/
	PX_INLINE const T& operator[](physx::PxU32 i) const
	{
		return get(i);
	}

	/*!
	Array indexing operator.
	\param i
	The index of the element that will be returned.
	\return
	The element i in the array.
	*/
	PX_INLINE T& operator[](physx::PxU32 i)
	{
		return get(i);
	}

	/*!
	\return
	returns whether GPU buffer has been allocated for this array
	*/
	PX_INLINE bool cpuPtrIsValid() const
	{
		return mData.cpuPtrIsValid();
	}

	/*!
	Returns the plain array representation.
	\return
	The sets representation.
	*/
	PX_INLINE T* getPtr() const
	{
		return mData.getCpuPtr();
	}

#if defined(APEX_CUDA_SUPPORT)
	/*!
	\return
	returns whether GPU buffer has been allocated for this array
	*/
	PX_INLINE bool gpuPtrIsValid() const
	{
		return mData.gpuPtrIsValid();
	}

	PX_INLINE T* getGpuPtr() const
	{
		return mData.getGpuPtr();
	}

	PX_INLINE void copyDeviceToHostDesc(PxGpuCopyDesc& desc, physx::PxU32 size, physx::PxU32 offset) const
	{
		PX_ASSERT(gpuPtrIsValid() && cpuPtrIsValid());
		if (size == 0)
		{
			size = mSize;
		}
		mData.copyDeviceToHostDesc(desc, sizeof(T) * size, sizeof(T) * offset);
	}
	PX_INLINE void copyDeviceToHostQ(PxGpuCopyDescQueue& queue, physx::PxU32 size = 0, physx::PxU32 offset = 0) const
	{
		PxGpuCopyDesc desc;
		copyDeviceToHostDesc(desc, size, offset);
		queue.enqueue(desc);
	}

	PX_INLINE void copyHostToDeviceDesc(PxGpuCopyDesc& desc, physx::PxU32 size, physx::PxU32 offset) const
	{
		PX_ASSERT(gpuPtrIsValid() && cpuPtrIsValid());
		if (size == 0)
		{
			size = mSize;
		}
		mData.copyHostToDeviceDesc(desc, sizeof(T) * size, sizeof(T) * offset);
	}
	PX_INLINE void copyHostToDeviceQ(PxGpuCopyDescQueue& queue, physx::PxU32 size = 0, physx::PxU32 offset = 0) const
	{
		PxGpuCopyDesc desc;
		copyHostToDeviceDesc(desc, size, offset);
		queue.enqueue(desc);
	}
	PX_INLINE void swapGpuPtr(ApexMirroredArray<T>& other)
	{
		PX_ASSERT(mCapacity == other.mCapacity);

		mData.swapGpuPtr(other.mData);
	}
#endif /* APEX_CUDA_SUPPORT */

	/*!
	Returns the number of entries in the array. This can, and probably will,
	differ from the array size.
	\return
	The number of of entries in the array.
	*/
	PX_INLINE physx::PxU32 getSize() const
	{
		return mSize;
	}

	PX_INLINE size_t getByteSize() const
	{
		return mData.getByteSize();
	}

	PX_INLINE char* getName() const
	{
		return mData.getName();
	}

	/*!
	Clears the array.
	*/
	PX_INLINE void clear()
	{
		mSize = 0;
		mData.free();
		mCapacity = 0;
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Resize array
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_INLINE void setSize(const physx::PxU32 size, ApexMirroredPlace::Enum place = ApexMirroredPlace::DEFAULT)
	{
		if (size > mCapacity)
		{
			mCapacity = size;
		}
		mData.realloc(sizeof(T) * mCapacity, place);
		mSize = size;
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Ensure that the array has at least size capacity.
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_INLINE void reserve(const physx::PxU32 capacity, ApexMirroredPlace::Enum place = ApexMirroredPlace::DEFAULT)
	{
		if (capacity > mCapacity)
		{
			mCapacity = capacity;
		}
		mData.realloc(sizeof(T) * mCapacity, place);
	}

	//////////////////////////////////////////////////////////////////////////
	/*!
	Query the capacity(allocated mem) for the array.
	*/
	//////////////////////////////////////////////////////////////////////////
	PX_INLINE physx::PxU32 getCapacity()
	{
		return mCapacity;
	}

private:
	ApexMirrored<T> mData;
	physx::PxU32    mCapacity;
	physx::PxU32    mSize;
};

}
} // end namespace physx::apex

#pragma warning(pop)

#endif
