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

#ifndef SIMULATION_NX_COMMON_H
#define SIMULATION_NX_COMMON_H

#include "SimulationAbstract.h"

class NxActor;
class NxActorDesc;
class NxCompartment;
class NxShape;
class NxShapeDesc;

namespace physx
{
namespace apex
{

class NiApexScene;

namespace clothing
{

class ClothingCookedParam;

class SimulationNxCommon : public SimulationAbstract
{
public:
	SimulationNxCommon(ClothingScene* clothingScene);
	virtual ~SimulationNxCommon();

	virtual bool needsExpensiveCreation();
	virtual bool needsAdaptiveTargetFrequency();
	virtual bool needsManualSubstepping();
	virtual bool needsLocalSpaceGravity();
	virtual bool dependsOnScene(void* scenePointer);

	virtual bool setCookedData(NxParameterized::Interface* cookedData, PxF32 cookedActorScale, PxF32 actorScale);

	virtual void initCollision(tBoneActor* boneActors, PxU32 numBoneActors,
		tBoneSphere* boneSpheres, PxU32 numBoneSpheres,
		PxU16* spherePairIndices, PxU32 numSpherePairs,
		tBonePlane* bonePlanes, PxU32 numBonePlanes,
		PxU32* convexes, PxU32 numConvexes,
		tBoneEntry* bones, const PxMat44* boneTransforms,
		NxResourceList& actorPlanes,
		NxResourceList& actorConvexes,
		NxResourceList& actorSpheres,
		NxResourceList& actorCapsules,
		NxResourceList& actorTriangleMeshes,
		const tActorDescTemplate& actorDesc, const tShapeDescTemplate& shapeDesc, PxF32 actorScale, const PxMat44& globalPose, bool localSpaceSim );

	virtual void updateCollision(tBoneActor* boneActors, PxU32 numBoneActors,
		tBoneSphere* boneSpheres, PxU32 numBoneSpheres,
		tBonePlane* bonePlanes, PxU32 numBonePlanes,
		tBoneEntry* bones, const PxMat44* boneTransforms,
		NxResourceList& actorPlanes,
		NxResourceList& actorConvexes,
		NxResourceList& actorSpheres,
		NxResourceList& actorCapsules,
		NxResourceList& actorTriangleMeshes,
		bool teleport);

	virtual void applyCollision();
	virtual void updateCollisionDescs(const tActorDescTemplate& actorDesc, const tShapeDescTemplate& shapeDesc);
	virtual void swapCollision(SimulationAbstract* oldSimulation);

	virtual void registerPhysX(NxApexActor* actor);
	virtual void unregisterPhysX();
	virtual void disablePhysX(NxApexActor* dummy);
	virtual void reenablePhysX(NxApexActor* newMaster);

	virtual void setGlobalPose(const PxMat44& globalPose);

	virtual void visualize(NiApexRenderDebug& /*renderDebug*/, ClothingDebugRenderParams& /*clothingDebugParams*/) {}

	virtual void fetchResults(bool computePhysicsMeshNormals);
	virtual bool isSimulationMeshDirty() const;
	virtual void clearSimulationMeshDirt();

	virtual NxParameterized::Interface* getCookedData();

	virtual void setTeleportWeight(PxF32 weight, bool reset, bool localSpaceSim);

	virtual NxCompartment* getDeformableCompartment() const = 0;
	virtual void enableCCD(bool on) = 0;

	virtual bool applyWind(PxVec3* velocities, const PxVec3* normals, const tConstrainCoeffs* assetCoeffs, const PxVec3& wind, PxF32 adaption, PxF32 dt);


protected:

	static void writeToActorDesc(const tActorDescTemplate& actorTemplate, NxActorDesc& actorDesc);
	static void writeToActor(const tActorDescTemplate& actorTemplate, NxActor* actor);
	static void writeToShapeDesc(const tShapeDescTemplate& shapeTemplate, NxShapeDesc& shapeDesc);
	static void writeToShape(const tShapeDescTemplate& shapeTemplate, NxShape* shape);
	void verifyShapeTemplate(const tShapeDescTemplate& shapeTemplate);
	PxU32 sdkDirtyClothBuffer;

	NiApexScene* mApexScene;

	ClothingCookedParam* mCookedParam;

	Array<NxActor*> mActors;

	PxMat44 mGlobalPose;
	PxMat44 mGlobalPosePrevious;

	bool mDelayedCollisionEnable;
	bool mLocalSpaceSim;
};

}
} // namespace apex
} // namespace physx

#endif // SIMULATION_NX_COMMON_H
