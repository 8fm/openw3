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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "VtxWeld.h"

namespace mimp
{

template<> MeshVertex VertexLess<MeshVertex>::mFind = MeshVertex();
template<> STDNAME::vector<MeshVertex > *VertexLess<MeshVertex>::mList=0;


template<>
bool VertexLess<MeshVertex>::operator()(MiI32 v1,MiI32 v2) const
{

	const MeshVertex& a = Get(v1);
	const MeshVertex& b = Get(v2);

	if ( a.mPos[0] < b.mPos[0] ) return true;
	if ( a.mPos[0] > b.mPos[0] ) return false;

	if ( a.mPos[1] < b.mPos[1] ) return true;
	if ( a.mPos[1] > b.mPos[1] ) return false;

	if ( a.mPos[2] < b.mPos[2] ) return true;
	if ( a.mPos[2] > b.mPos[2] ) return false;


	if ( a.mNormal[0] < b.mNormal[0] ) return true;
	if ( a.mNormal[0] > b.mNormal[0] ) return false;

	if ( a.mNormal[1] < b.mNormal[1] ) return true;
	if ( a.mNormal[1] > b.mNormal[1] ) return false;

	if ( a.mNormal[2] < b.mNormal[2] ) return true;
	if ( a.mNormal[2] > b.mNormal[2] ) return false;

  if ( a.mColor < b.mColor ) return true;
  if ( a.mColor > b.mColor ) return false;


	if ( a.mTexel1[0] < b.mTexel1[0] ) return true;
	if ( a.mTexel1[0] > b.mTexel1[0] ) return false;

	if ( a.mTexel1[1] < b.mTexel1[1] ) return true;
	if ( a.mTexel1[1] > b.mTexel1[1] ) return false;

	if ( a.mTexel2[0] < b.mTexel2[0] ) return true;
	if ( a.mTexel2[0] > b.mTexel2[0] ) return false;

	if ( a.mTexel2[1] < b.mTexel2[1] ) return true;
	if ( a.mTexel2[1] > b.mTexel2[1] ) return false;

	if ( a.mTexel3[0] < b.mTexel3[0] ) return true;
	if ( a.mTexel3[0] > b.mTexel3[0] ) return false;

	if ( a.mTexel3[1] < b.mTexel3[1] ) return true;
	if ( a.mTexel3[1] > b.mTexel3[1] ) return false;


	if ( a.mTexel4[0] < b.mTexel4[0] ) return true;
	if ( a.mTexel4[0] > b.mTexel4[0] ) return false;

	if ( a.mTexel4[1] < b.mTexel4[1] ) return true;
	if ( a.mTexel4[1] > b.mTexel4[1] ) return false;

	if ( a.mTangent[0] < b.mTangent[0] ) return true;
	if ( a.mTangent[0] > b.mTangent[0] ) return false;

	if ( a.mTangent[1] < b.mTangent[1] ) return true;
	if ( a.mTangent[1] > b.mTangent[1] ) return false;

	if ( a.mTangent[2] < b.mTangent[2] ) return true;
	if ( a.mTangent[2] > b.mTangent[2] ) return false;

	if ( a.mBiNormal[0] < b.mBiNormal[0] ) return true;
	if ( a.mBiNormal[0] > b.mBiNormal[0] ) return false;

	if ( a.mBiNormal[1] < b.mBiNormal[1] ) return true;
	if ( a.mBiNormal[1] > b.mBiNormal[1] ) return false;

	if ( a.mBiNormal[2] < b.mBiNormal[2] ) return true;
	if ( a.mBiNormal[2] > b.mBiNormal[2] ) return false;

	if ( a.mWeight[0] < b.mWeight[0] ) return true;
	if ( a.mWeight[0] > b.mWeight[0] ) return false;

	if ( a.mWeight[1] < b.mWeight[1] ) return true;
	if ( a.mWeight[1] > b.mWeight[1] ) return false;

	if ( a.mWeight[2] < b.mWeight[2] ) return true;
	if ( a.mWeight[2] > b.mWeight[2] ) return false;

	if ( a.mWeight[3] < b.mWeight[3] ) return true;
	if ( a.mWeight[3] > b.mWeight[3] ) return false;

	if ( a.mBone[0] < b.mBone[0] ) return true;
	if ( a.mBone[0] > b.mBone[0] ) return false;

	if ( a.mBone[1] < b.mBone[1] ) return true;
	if ( a.mBone[1] > b.mBone[1] ) return false;

	if ( a.mBone[2] < b.mBone[2] ) return true;
	if ( a.mBone[2] > b.mBone[2] ) return false;

	if ( a.mBone[3] < b.mBone[3] ) return true;
	if ( a.mBone[3] > b.mBone[3] ) return false;

  if ( a.mRadius < b.mRadius ) return true;
  if ( a.mRadius > b.mRadius ) return false;

	return false;
};



}; // end of name space
