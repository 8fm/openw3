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

#ifndef APEX_VERTEX_BUFFER_H
#define APEX_VERTEX_BUFFER_H

#include "NiApexRenderMeshAsset.h"
#include "ApexVertexFormat.h"
#include "VertexBufferParameters.h"
#include <NxParameterized.h>
#include "ApexSharedUtils.h"
#include "ApexInteropableBuffer.h"

namespace physx
{
namespace apex
{

class ApexVertexBuffer : public NiApexVertexBuffer, public ApexInteropableBuffer, public NxParameterized::SerializationCallback
{
public:
	ApexVertexBuffer();
	~ApexVertexBuffer();

	// from NxVertexBuffer
	const NxVertexFormat&	getFormat() const
	{
		return mFormat;
	}
	physx::PxU32			getVertexCount() const
	{
		return mParams->vertexCount;
	}
	void*					getBuffer(physx::PxU32 bufferIndex);
	void*					getBufferAndFormatWritable(NxRenderDataFormat::Enum& format, physx::PxU32 bufferIndex)
	{
		return getBufferAndFormat(format, bufferIndex);
	}

	void*					getBufferAndFormat(NxRenderDataFormat::Enum& format, physx::PxU32 bufferIndex)
	{
		format = getFormat().getBufferFormat(bufferIndex);
		return getBuffer(bufferIndex);
	}
	bool					getBufferData(void* dstBuffer, physx::NxRenderDataFormat::Enum dstBufferFormat, physx::PxU32 dstBufferStride, physx::PxU32 bufferIndex,
	                                      physx::PxU32 startVertexIndex, physx::PxU32 elementCount) const;
	PX_INLINE const void*	getBuffer(physx::PxU32 bufferIndex) const
	{
		return (const void*)((ApexVertexBuffer*)this)->getBuffer(bufferIndex);
	}
	PX_INLINE const void*	getBufferAndFormat(NxRenderDataFormat::Enum& format, physx::PxU32 bufferIndex) const
	{
		return (const void*)((ApexVertexBuffer*)this)->getBufferAndFormat(format, bufferIndex);
	}

	// from NiApexVertexBuffer
	void					build(const NxVertexFormat& format, physx::PxU32 vertexCount);

	NxVertexFormat&			getFormatWritable()
	{
		return mFormat;
	}
	void					applyTransformation(const physx::PxMat34Legacy& transformation);
	void					applyScale(physx::PxF32 scale);
	bool					mergeBinormalsIntoTangents();

	void					copy(physx::PxU32 dstIndex, physx::PxU32 srcIndex, ApexVertexBuffer* srcBufferPtr = NULL);
	void					resize(physx::PxU32 vertexCount);

	// from NxParameterized::SerializationCallback

	void					preSerialize(void* userData);

	void					setParams(VertexBufferParameters* param);
	VertexBufferParameters* getParams()
	{
		return mParams;
	}

	physx::PxU32			getAllocationSize() const;

	void					applyPermutation(const Array<PxU32>& permutation);

protected:
	VertexBufferParameters*			mParams;

	ApexVertexFormat				mFormat;	// Wrapper class for mParams->vertexFormat
};


} // namespace apex
} // namespace physx


#endif // APEX_VERTEX_BUFFER_H
