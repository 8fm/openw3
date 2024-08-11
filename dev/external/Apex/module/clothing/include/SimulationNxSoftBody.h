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

#ifndef SIMULATION_NX_SOFTBODY_H
#define SIMULATION_NX_SOFTBODY_H


#include "SimulationNxCommon.h"

#include "NxApexDefs.h"
#if NX_SDK_VERSION_MAJOR != 2
#error "SimulationNxSoftBody.h must only be included for builds with 2.8.x"
#endif

class NxSoftBody;

namespace physx
{
namespace apex
{

class NiApexScene;

namespace clothing
{
class ClothingCookedParam;

class SimulationNxSoftBody : public SimulationNxCommon
{
public:
	SimulationNxSoftBody(ClothingScene* clothingScene, bool useHW);
	virtual ~SimulationNxSoftBody();

	virtual SimulationType::Enum getType() const { return SimulationType::SOFTBODY2x; }
	virtual bool dependsOnScene(void* scenePointer);
	virtual PxU32 getNumSolverIterations() const;
	virtual bool initPhysics(NxScene* scene, PxU32 physicalMeshId, PxU32 submeshId, PxU32* indices, PxVec3* restPositions, tMaterial* material, const PxMat44& globalPose, const PxVec3& scaledGravity, bool localSpaceSim);

	virtual void registerPhysX(NxApexActor* actor);
	virtual void unregisterPhysX();
	virtual void disablePhysX(NxApexActor* dummy);
	virtual void reenablePhysX(NxApexActor* newMaster, const PxMat44& globalPose);

	virtual void setStatic(bool on);
	virtual bool applyPressure(PxF32 pressure);

	virtual bool raycast(const PxVec3& rayOrigin, const PxVec3& rayDirection, PxF32& hitTime, PxVec3& hitNormal, PxU32& vertexIndex);
	virtual void attachVertexToGlobalPosition(PxU32 vertexIndex, const PxVec3& globalPosition);
	virtual void freeVertex(PxU32 vertexIndex);

	virtual void verifyTimeStep(PxF32 substepSize);

	virtual void setPositions(PxVec3* positions);
	virtual void setConstrainCoefficients(const tConstrainCoeffs* assetCoeffs, PxF32 maxDistanceBias, PxF32 maxDistanceScale, PxF32 maxDistanceDeform, PxF32 actorScale);
	virtual void getVelocities(PxVec3* velocities) const;
	virtual void setVelocities(PxVec3* velocities);


	virtual void setSolverIterations(PxU32 iterations);
	virtual void updateConstrainPositions(bool isDirty);
	virtual bool applyClothingMaterial(tMaterial* material, PxVec3 scaledGravity);
	virtual void applyClothingDesc(tClothingDescTemplate& clothingTemplate);

	virtual NxCompartment* getDeformableCompartment() const;
	virtual void enableCCD(bool on);

protected:
	PxU32 applyMaterialFlags(PxU32 flags, tMaterial& material) const;

	NxSoftBody* mNxSoftBody;
	NxCompartment* mCompartment;

};

}
} // namespace apex
} // namespace physx


#endif // SIMULATION_NX_SOFTBODY_H
