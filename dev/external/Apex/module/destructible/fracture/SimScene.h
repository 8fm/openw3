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
#ifndef SIM_SCENE
#define SIM_SCENE

#include "SimSceneBase.h"

namespace physx
{
namespace apex
{
namespace destructible
{
	class DestructibleActor;
}
}
namespace fracture
{

class FracturePattern;
class Actor;

class SimScene : public base::SimScene
{
public:
	static SimScene* createSimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, PxScene *scene, bool isGrbScene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath);
protected:
	SimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, PxScene *scene, bool isGrbScene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath);
public:
	virtual ~SimScene();

	virtual void createSingletons();

	virtual base::Actor* createActor(physx::apex::destructible::DestructibleActor* actor);
	virtual base::Convex* createConvex();
	virtual base::Compound* createCompound(const base::FracturePattern *pattern, const base::FracturePattern *secondaryPattern = NULL, PxReal contactOffset = 0.005f, PxReal restOffset = -0.001f);
	virtual base::FracturePattern* createFracturePattern();

	FracturePattern* getDefaultGlass() {return mDefaultGlass;}

	bool isGrbScene() { return mIsGrbScene; }

protected:
	FracturePattern* mDefaultGlass;
};

}
}

#endif
#endif