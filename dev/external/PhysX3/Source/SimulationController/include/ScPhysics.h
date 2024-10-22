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


#ifndef PX_PHYSICS_SC_PHYSICS
#define PX_PHYSICS_SC_PHYSICS

#include "PxPhysics.h"
#include "PxScene.h"
#include "PsUserAllocated.h"
#include "CmPhysXCommon.h"

namespace physx
{

namespace cloth
{
class Factory;
}


class PxMaterial;
class PxTolerancesScale;

#if PX_SUPPORT_GPU_PHYSX
class PxPhysXGpu;
#endif

namespace Sc
{

	class Scene;

	class Physics : public Ps::UserAllocated
	{
	public:
		PX_INLINE static Physics&						getInstance()	{ return *mInstance; }

														Physics(const PxTolerancesScale&);
														~Physics(); // use release() instead
	public:
						void							release();

		PX_FORCE_INLINE	const PxTolerancesScale&		getTolerancesScale()								const	{ return mScale;	}

#if PX_USE_CLOTH_API
		void						registerCloth();
		PX_INLINE bool				hasLowLevelClothFactory() const { return mLowLevelClothFactory != 0; }
		PX_INLINE cloth::Factory&	getLowLevelClothFactory() { PX_ASSERT(mLowLevelClothFactory); return *mLowLevelClothFactory; }
#endif

	private:
						PxTolerancesScale			mScale;
		static			Physics*					mInstance;
						cloth::Factory*				mLowLevelClothFactory;

	public:
		static			const PxReal				sWakeCounterOnCreation;
	};

} // namespace Sc

}

#endif
