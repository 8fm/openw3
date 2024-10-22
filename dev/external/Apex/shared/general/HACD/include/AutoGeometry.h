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

#ifndef AUTO_GEOMETRY_H

#define AUTO_GEOMETRY_H

/*!
**
** Copyright (c) 20011 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/


#include "PlatformConfigHACD.h"

namespace HACD
{
	class SimpleHull
	{
	public:
		SimpleHull(void)
		{
			mBoneIndex = 0;
			mVertexCount = 0;
			mVertices  = 0;
			mTriCount = 0;
			mIndices = 0;
			mMeshVolume = 0;
			mParentIndex = -1;
			mBoneName = 0;
		}
		hacd::HaI32           mBoneIndex;
		hacd::HaI32           mParentIndex;
		const char   *mBoneName;
		hacd::HaF32         mTransform[16];
		hacd::HaU32  mVertexCount;
		hacd::HaF32        *mVertices;
		hacd::HaU32  mTriCount;
		hacd::HaU32 *mIndices;
		hacd::HaF32         mMeshVolume;
	};

	enum BoneOption
	{
		BO_INCLUDE,
		BO_EXCLUDE,
		BO_COLLAPSE
	};

	class SimpleBone
	{
	public:
		SimpleBone(void)
		{
			mOption = BO_INCLUDE;
			mParentIndex = -1;
			mBoneName = 0;
			mBoneMinimalWeight = 0.4f;
		}
		BoneOption   mOption;
		const char  *mBoneName;
		hacd::HaI32          mParentIndex;
		hacd::HaF32        mBoneMinimalWeight;
		hacd::HaF32        mTransform[16];
		hacd::HaF32        mInverseTransform[16];
	};

	class SimpleSkinnedVertex
	{
	public:
		hacd::HaF32          mPos[3];
		unsigned short mBone[4];
		hacd::HaF32          mWeight[4];
	};

	class SimpleSkinnedMesh
	{
	public:
		hacd::HaU32         mVertexCount;
		SimpleSkinnedVertex *mVertices;

	};

	class AutoGeometry
	{
	public:

		virtual SimpleHull ** createCollisionVolumes(hacd::HaF32 collapse_percentage,          // percentage volume to collapse a child into a parent
			hacd::HaU32 bone_count,
			const SimpleBone *bones,
			const SimpleSkinnedMesh *mesh,
			hacd::HaU32 &geom_count) = 0;


		virtual SimpleHull ** createCollisionVolumes(hacd::HaF32 collapse_percentage,hacd::HaU32 &geom_count) = 0;

		virtual void addSimpleSkinnedTriangle(const SimpleSkinnedVertex &v1,
			const SimpleSkinnedVertex &v2,
			const SimpleSkinnedVertex &v3) = 0;

		virtual void addSimpleBone(const SimpleBone &b) = 0;

		virtual const char * stristr(const char *str,const char *match) = 0; // case insensitive ststr

	};

	AutoGeometry * createAutoGeometry();
	void           releaseAutoGeometry(AutoGeometry *g);

}; // end of namespace


#endif
