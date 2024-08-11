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

#include "ApexCustomBufferIterator.h"

namespace physx
{
namespace apex
{

ApexCustomBufferIterator::ApexCustomBufferIterator() :
	mData(NULL),
	mElemSize(0),
	mMaxTriangles(0)
{
}

void ApexCustomBufferIterator::setData(void* data, physx::PxU32 elemSize, physx::PxU32 maxTriangles)
{
	mData = (physx::PxU8*)data;
	mElemSize = elemSize;
	mMaxTriangles = maxTriangles;
}

void ApexCustomBufferIterator::addCustomBuffer(const char* name, NxRenderDataFormat::Enum format, physx::PxU32 offset)
{
	CustomBuffer buffer;
	buffer.name = name;
	buffer.offset = offset;
	buffer.format = format;

	mCustomBuffers.pushBack(buffer);
}
void* ApexCustomBufferIterator::getVertex(physx::PxU32 triangleIndex, physx::PxU32 vertexIndex) const
{
	if (mData == NULL || triangleIndex >= mMaxTriangles)
	{
		return NULL;
	}

	return mData + mElemSize * (triangleIndex * 3 + vertexIndex);
}
physx::PxI32 ApexCustomBufferIterator::getAttributeIndex(const char* attributeName) const
{
	if (attributeName == NULL || attributeName[0] == 0)
	{
		return -1;
	}

	for (physx::PxU32 i = 0; i < mCustomBuffers.size(); i++)
	{
		if (strcmp(mCustomBuffers[i].name, attributeName) == 0)
		{
			return i;
		}
	}
	return -1;
}
void* ApexCustomBufferIterator::getVertexAttribute(physx::PxU32 triangleIndex, physx::PxU32 vertexIndex, const char* attributeName, NxRenderDataFormat::Enum& outFormat) const
{
	outFormat = NxRenderDataFormat::UNSPECIFIED;

	physx::PxU8* elementData = (physx::PxU8*)getVertex(triangleIndex, vertexIndex);
	if (elementData == NULL)
	{
		return NULL;
	}


	for (physx::PxU32 i = 0; i < mCustomBuffers.size(); i++)
	{
		if (strcmp(mCustomBuffers[i].name, attributeName) == 0)
		{
			outFormat = mCustomBuffers[i].format;
			return elementData + mCustomBuffers[i].offset;
		}
	}
	return NULL;
}

void* ApexCustomBufferIterator::getVertexAttribute(physx::PxU32 triangleIndex, physx::PxU32 vertexIndex, physx::PxU32 attributeIndex, NxRenderDataFormat::Enum& outFormat, const char*& outName) const
{
	outFormat = NxRenderDataFormat::UNSPECIFIED;
	outName = NULL;

	physx::PxU8* elementData = (physx::PxU8*)getVertex(triangleIndex, vertexIndex);
	if (elementData == NULL || attributeIndex >= mCustomBuffers.size())
	{
		return NULL;
	}

	outName = mCustomBuffers[attributeIndex].name;
	outFormat = mCustomBuffers[attributeIndex].format;
	return elementData + mCustomBuffers[attributeIndex].offset;
}

}
} // end namespace physx::apex
