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
#ifndef NX_SUBSTANCE_SOURCE_ACTOR_H
#define NX_SUBSTANCE_SOURCE_ACTOR_H

#include "NxApex.h"
#include "NxApexShape.h"

namespace physx
{
namespace apex
{

class NxSubstanceSourceAsset;


/**
 \brief Turbulence ScalarSource Actor class
 */
class NxSubstanceSourceActor : public NxApexActor//, public NxApexRenderable
{
protected:
	virtual ~NxSubstanceSourceActor() {}

public:
	 /**
	 \brief Returns the asset the instance has been created from.
	 */
	virtual NxSubstanceSourceAsset*			getSubstanceSourceAsset() const = 0;

	virtual void setEnabled(bool enable) = 0;

	virtual bool isEnabled() const = 0;

	/*enable whether or not to use heat in the simulation (enabling heat reduces performance).<BR>
	If you are enabling heat then you also need to add temperature sources (without temperature sources you will see no effect of heat on the simulation, except a drop in performance)
	 */

	///intersect the collision shape against a given AABB
	virtual bool						intersectAgainstAABB(physx::PxBounds3) = 0;

	virtual  NxApexShape* 				getShape() const = 0;

	///If it is a box, cast to box class, return NULL otherwise
	virtual NxApexBoxShape* 			getBoxShape() = 0;

	///If it is a sphere, cast to sphere class, return NULL otherwise
	virtual NxApexSphereShape* 			getSphereShape() = 0;

	///Return average value of density
	virtual PxF32						getAverageDensity() const = 0;

	///Retrun STD value of density
	virtual PxF32						getStdDensity() const = 0;

	///Set average and STD values for density
	virtual void						setDensity(PxF32 averageDensity, PxF32 stdDensity) = 0;

	virtual void						release() = 0;
};


}
} // end namespace physx::apex

#endif 
