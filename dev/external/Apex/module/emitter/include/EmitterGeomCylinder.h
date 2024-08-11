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

#ifndef __EMITTER_GEOM_CYLINDER_H__
#define __EMITTER_GEOM_CYLINDER_H__

#include "EmitterGeom.h"
#include "PsUserAllocated.h"
#include "EmitterGeomCylinderParams.h"

namespace NxParameterized
{
class Interface;
};

namespace physx
{
namespace apex
{
namespace emitter
{

class EmitterGeomCylinder : public NxEmitterCylinderGeom, public EmitterGeom
{
public:
	EmitterGeomCylinder(NxParameterized::Interface* params);

	/* Asset callable methods */
	NxEmitterGeom*				getNxEmitterGeom();
	const NxEmitterCylinderGeom* 	isCylinderGeom() const
	{
		return this;
	}
	NxApexEmitterType::Enum		getEmitterType() const
	{
		return mType;
	}
	void						setEmitterType(NxApexEmitterType::Enum t);
	void	                    setRadius(physx::PxF32 radius)
	{
		*mRadius = radius;
	}
	physx::PxF32	            getRadius() const
	{
		return *mRadius;
	}
	void	                    setHeight(physx::PxF32 height)
	{
		*mHeight = height;
	}
	physx::PxF32	            getHeight() const
	{
		return *mHeight;
	}
	void						destroy()
	{
		delete this;
	}

	/* AssetPreview methods */
	void                        drawPreview(physx::PxF32 scale, NxApexRenderDebug* renderDebug) const;

	/* Actor callable methods */
	void						visualize(const physx::PxMat34Legacy& pose, NiApexRenderDebug& renderDebug);

	void						computeFillPositions(physx::Array<physx::PxVec3>& positions,
	        physx::Array<physx::PxVec3>& velocities,
	        const physx::PxMat34Legacy&,
	        physx::PxF32,
	        physx::PxBounds3& outBounds,
	        QDSRand& rand) const;

	physx::PxF32				computeEmitterVolume() const;
	physx::PxVec3				randomPosInFullVolume(const physx::PxMat34Legacy& pose, QDSRand& rand) const;
	bool						isInEmitter(const physx::PxVec3& pos, const physx::PxMat34Legacy& pose) const;

protected:
	NxApexEmitterType::Enum		mType;
	physx::PxF32*				mRadius;
	physx::PxF32*				mHeight;
	EmitterGeomCylinderParams*	mGeomParams;
};

}
}
} // end namespace physx::apex

#endif
