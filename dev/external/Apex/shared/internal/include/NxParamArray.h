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
#ifndef NX_PARAM_ARRAY_H
#define NX_PARAM_ARRAY_H

#include "NxParameterized.h"
#include "PsUserAllocated.h"
#include "foundation/PxAssert.h"

namespace physx
{
namespace apex
{

struct NxParamDynamicArrayStruct
{
	void* buf;
	bool isAllocated;
	int elementSize;
	int arraySizes[1];
};


template <class ElemType>
class NxParamArray : public physx::UserAllocated
{
public:

	NxParamArray() : mParams(NULL), mArrayHandle(0), mArrayStruct(NULL) {}

	NxParamArray(NxParameterized::Interface* params, const char* arrayName, NxParamDynamicArrayStruct* arrayStruct) :
		mParams(params),
		mArrayHandle(*params),
		mArrayStruct(arrayStruct)
	{
		PX_ASSERT(mParams);

		mParams->getParameterHandle(arrayName, mArrayHandle);

		PX_ASSERT(mArrayStruct->elementSize == sizeof(ElemType));
	}

	NxParamArray(NxParameterized::Interface* params, const NxParameterized::Handle& handle, NxParamDynamicArrayStruct* arrayStruct) :
		mParams(params),
		mArrayHandle(handle),
		mArrayStruct(arrayStruct)
	{
		PX_ASSERT(mArrayStruct->elementSize == sizeof(ElemType));
	}

	PX_INLINE bool init(NxParameterized::Interface* params, const char* arrayName, NxParamDynamicArrayStruct* arrayStruct)
	{
		if (mParams == NULL && mArrayStruct == NULL)
		{
			mParams = params;
			mArrayStruct = arrayStruct;
			mArrayHandle.setInterface(mParams);
			mParams->getParameterHandle(arrayName, mArrayHandle);

			PX_ASSERT(mArrayStruct->elementSize == sizeof(ElemType));

			return true;
		}
		return false;
	}

	PX_INLINE physx::PxU32 size() const
	{
		// this only works for fixed structs
		//return (physx::PxU32)mArrayHandle.parameterDefinition()->arraySize(0);
		int outSize = 0;
		if (mParams != NULL)
		{
			PX_ASSERT(mArrayHandle.getConstInterface() == mParams);
			mArrayHandle.getArraySize(outSize);
		}
		return (physx::PxU32)outSize;
	}

	/**
	Returns a constant reference to an element in the sequence.
	*/
	PX_INLINE const ElemType& operator[](unsigned int n) const
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);
#if _DEBUG
		physx::PxU32 NxParamArraySize = 0;
		mArrayHandle.getArraySize((int&)NxParamArraySize);
		PX_ASSERT(NxParamArraySize > n);
#endif
		return ((ElemType*)mArrayStruct->buf)[n];
	}

	/**
	Returns a reference to an element in the sequence.
	*/
	PX_INLINE ElemType& operator[](unsigned int n)
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);
		//NxParameterized::Handle indexHandle;
		//arrayHandle.getChildHandle( n, indexHandle );
#if _DEBUG
		physx::PxU32 NxParamArraySize = 0;
		mArrayHandle.getArraySize((int&)NxParamArraySize);
		PX_ASSERT(NxParamArraySize > n);
#endif
		return ((ElemType*)mArrayStruct->buf)[n];
	}

	// resize is marginally useful because the ElemType doesn't have proper
	// copy constructors, and if strings are withing ElemType that doesn't work well
	PX_INLINE void resize(unsigned int n)
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);
		PX_ASSERT(mParams == mArrayHandle.getConstInterface());
		mArrayHandle.resizeArray(n);
	}

	// pushBack is marginally useful because the ElemType doesn't have proper
	// copy constructors, and if strings are withing ElemType that doesn't work well
	PX_INLINE void pushBack(const ElemType& x)
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);

		physx::PxU32 NxParamArraySize = 0;

		mArrayHandle.getArraySize((int&)NxParamArraySize);
		mArrayHandle.resizeArray(NxParamArraySize + 1);

		((ElemType*)mArrayStruct->buf)[NxParamArraySize] = x;
	}

	PX_INLINE ElemType& pushBack()
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);

		physx::PxU32 NxParamArraySize = 0;

		mArrayHandle.getArraySize((int&)NxParamArraySize);
		mArrayHandle.resizeArray(NxParamArraySize + 1);

		return ((ElemType*)mArrayStruct->buf)[NxParamArraySize];
	}

	PX_INLINE void replaceWithLast(unsigned position)
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);

		physx::PxU32 arraySize = size();
		PX_ASSERT(position < arraySize);
		if (position != arraySize - 1)
		{
			// TODO should we call the destructor here or not?
			//(*this)[position].~ElemType();

			ElemType elem = back();

			// put the replaced one in the back (possibly to be deleted)
			(*this)[arraySize - 1] = (*this)[position];

			(*this)[position] = elem;
		}
		popBack();
	}

	PX_INLINE bool isEmpty() const
	{
		return size() == 0;
	}

	PX_INLINE ElemType* begin()
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);
		return &((ElemType*)mArrayStruct->buf)[0];
	}

	PX_INLINE ElemType* end()
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);
		return &((ElemType*)mArrayStruct->buf)[size()];
	}

	PX_INLINE ElemType& front()
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);
		return ((ElemType*)mArrayStruct->buf)[0];
	}

	PX_INLINE ElemType& back()
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);
		return ((ElemType*)mArrayStruct->buf)[size() - 1];
	}

	PX_INLINE void clear()
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);
		resize(0);
	}

	PX_INLINE void popBack()
	{
		PX_ASSERT(mParams != NULL && mArrayStruct != NULL);
		resize(size() - 1);
	}

private:
	NxParameterized::Interface*	mParams;
	NxParameterized::Handle		mArrayHandle;
	NxParamDynamicArrayStruct*	mArrayStruct;
};

}
} // end namespace physx::apex

#endif

