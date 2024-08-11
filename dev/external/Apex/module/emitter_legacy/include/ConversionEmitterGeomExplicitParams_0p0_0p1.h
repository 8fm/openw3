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

#ifndef __CONVERSIONEMITTERGEOMEXPLICITPARAMS_0P0_0P1H__
#define __CONVERSIONEMITTERGEOMEXPLICITPARAMS_0P0_0P1H__

#include "ParamConversionTemplate.h"
#include "EmitterGeomExplicitParams_0p0.h"
#include "EmitterGeomExplicitParams_0p1.h"

namespace physx
{
namespace apex
{
namespace legacy
{

typedef ParamConversionTemplate<EmitterGeomExplicitParams_0p0, EmitterGeomExplicitParams_0p1, 0, 1> ConversionEmitterGeomExplicitParams_0p0_0p1Parent;

class ConversionEmitterGeomExplicitParams_0p0_0p1: ConversionEmitterGeomExplicitParams_0p0_0p1Parent
{
public:
	static NxParameterized::Conversion* Create(NxParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionEmitterGeomExplicitParams_0p0_0p1));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionEmitterGeomExplicitParams_0p0_0p1)(t) : 0;
	}

protected:
	ConversionEmitterGeomExplicitParams_0p0_0p1(NxParameterized::Traits* t) : ConversionEmitterGeomExplicitParams_0p0_0p1Parent(t) {}

	const NxParameterized::PrefVer* getPreferredVersions() const
	{
		static NxParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char *)longName, (PxU32)preferredVersion }

			{ 0, 0 } // Terminator (do not remove!)
		};

		return prefVers;
	}

#	define NX_ERR_CHECK_RETURN(x) { if( NxParameterized::ERROR_NONE != (x) ) return false; }

	bool convert()
	{
		NxParameterized::Handle h(*mNewData);
		physx::PxI32 size;

		size = mLegacyData->positions.arraySizes[0];
		NX_ERR_CHECK_RETURN(mNewData->getParameterHandle("points.positions", h));
		NX_ERR_CHECK_RETURN(h.resizeArray(size));
		for (physx::PxI32 i = 0; i < size; ++i)
		{
			mNewData->points.positions.buf[i].position = mLegacyData->positions.buf[i];
			mNewData->points.positions.buf[i].doDetectOverlaps = false;
		}

		size = mLegacyData->velocities.arraySizes[0];
		NX_ERR_CHECK_RETURN(mNewData->getParameterHandle("points.velocities", h));
		NX_ERR_CHECK_RETURN(h.resizeArray(size));
		NX_ERR_CHECK_RETURN(h.setParamVec3Array(&mLegacyData->velocities.buf[0], size));

		return true;
	}
};

}
}
} //end of physx::apex:: namespace

#endif
