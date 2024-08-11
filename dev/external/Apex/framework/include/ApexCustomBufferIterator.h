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

#ifndef __APEX_CUSTOM_BUFFER_ITERARTOR_H__
#define __APEX_CUSTOM_BUFFER_ITERARTOR_H__

#include "NxApexCustomBufferIterator.h"
#include <PsUserAllocated.h>
#include <PsArray.h>
#include <PsShare.h>

namespace physx
{
namespace apex
{

class ApexCustomBufferIterator : public NxApexCustomBufferIterator, public physx::UserAllocated
{
public:
	ApexCustomBufferIterator();

	// NxApexCustomBufferIterator methods

	virtual void		setData(void* data, physx::PxU32 elemSize, physx::PxU32 maxTriangles);

	virtual void		addCustomBuffer(const char* name, NxRenderDataFormat::Enum format, physx::PxU32 offset);

	virtual void*		getVertex(physx::PxU32 triangleIndex, physx::PxU32 vertexIndex) const;

	virtual physx::PxI32		getAttributeIndex(const char* attributeName) const;

	virtual void*		getVertexAttribute(physx::PxU32 triangleIndex, physx::PxU32 vertexIndex, const char* attributeName, NxRenderDataFormat::Enum& outFormat) const;

	virtual void*		getVertexAttribute(physx::PxU32 triangleIndex, physx::PxU32 vertexIndex, physx::PxU32 attributeIndex, NxRenderDataFormat::Enum& outFormat, const char*& outName) const;

private:
	physx::PxU8* mData;
	physx::PxU32 mElemSize;
	physx::PxU32 mMaxTriangles;
	struct CustomBuffer
	{
		const char* name;
		physx::PxU32 offset;
		NxRenderDataFormat::Enum format;
	};
	physx::Array<CustomBuffer> mCustomBuffers;
};

}
} // end namespace physx::apex


#endif // __APEX_CUSTOM_BUFFER_ITERARTOR_H__
