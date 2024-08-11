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

#include "RTdef.h"
#if RT_COMPILE
#include <PsUserAllocated.h>

#include "DestructibleActor.h"
#include "PxMaterial.h"

#include "Actor.h"
#include "Compound.h"
#include "Convex.h"
#include "CompoundCreator.h"
#include "Delaunay2d.h"
#include "Delaunay3d.h"
#include "PolygonTriangulator.h"
#include "IslandDetector.h"
#include "MeshClipper.h"
#include "FracturePattern.h"

#include "SimScene.h"

namespace physx
{
namespace fracture
{


SimScene* SimScene::createSimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, PxScene *scene, bool isGrbScene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath)
{
	SimScene* s = PX_NEW(SimScene)(pxPhysics,pxCooking,scene,isGrbScene,minConvexSize,defaultMat,resourcePath);
	s->createSingletons();
	return s;
}

void SimScene::createSingletons()
{
	mCompoundCreator = PX_NEW(CompoundCreator)(this);
	mDelaunay2d = PX_NEW(Delaunay2d)(this);
	mDelaunay3d = PX_NEW(Delaunay3d)(this);
	mPolygonTriangulator = PX_NEW(PolygonTriangulator)(this);
	mIslandDetector = PX_NEW(IslandDetector)(this);
	mMeshClipper = PX_NEW(MeshClipper)(this);
	mDefaultGlass = PX_NEW(FracturePattern)(this);
	mDefaultGlass->createGlass(10.0f,0.25f,30,0.3f,0.03f,1.4f,0.3f);
	//mDefaultGlass->create3dVoronoi(PxVec3(10.0f, 10.0f, 10.0f), 50, 10.0f);
	addActor(createActor(NULL));
}

base::Actor* SimScene::createActor(::physx::apex::destructible::DestructibleActor* actor)
{
	return (base::Actor*)PX_NEW(Actor)(this,actor);
}

base::Convex* SimScene::createConvex()
{
	return (base::Convex*)PX_NEW(Convex)(this);
}

base::Compound* SimScene::createCompound(const base::FracturePattern *pattern, const base::FracturePattern *secondaryPattern, PxReal contactOffset, PxReal restOffset)
{
	return (base::Compound*)PX_NEW(Compound)(this,pattern,secondaryPattern,contactOffset,restOffset);
}

base::FracturePattern* SimScene::createFracturePattern()
{
	return (base::FracturePattern*)PX_NEW(FracturePattern)(this);
}

SimScene::SimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, PxScene *scene, bool isGrbScene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath):
		base::SimScene(pxPhysics,pxCooking,scene,isGrbScene,minConvexSize,defaultMat,resourcePath) 
{
	if( mPxDefaultMaterial == NULL )
	{
		mPxDefaultMaterial = pxPhysics->createMaterial(0.5f,0.5f,0.1f);
	}
}

// --------------------------------------------------------------------------------------------
SimScene::~SimScene()
{
	PX_DELETE(mDefaultGlass);

	mDefaultGlass = NULL;
}

}
}
#endif