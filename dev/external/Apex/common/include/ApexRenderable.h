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

#ifndef APEX_RENDERABLE_H
#define APEX_RENDERABLE_H

#include "PsShare.h"
#include "PsMutex.h"

#include "foundation/PxBounds3.h"

namespace physx
{
namespace apex
{

/**
	Base class for implementations of NxApexRenderable classes
*/

class ApexRenderable
{
public:
	ApexRenderable()
	{
		mRenderBounds.setEmpty();
	}
	~ApexRenderable()
	{
		// the PS3 Mutex cannot be unlocked without first being locked, so grab the lock
		if (renderDataTryLock())
		{
			renderDataUnLock();
		}
		else
		{
			// someone is holding the lock and should not be, so assert
			PX_ALWAYS_ASSERT();
		}
	}
	void renderDataLock()
	{
		mRenderDataLock.lock();
	}
	void renderDataUnLock()
	{
		mRenderDataLock.unlock();
	}
	bool renderDataTryLock()
	{
		return mRenderDataLock.trylock();
	}
	const physx::PxBounds3&	getBounds() const
	{
		return mRenderBounds;
	}

protected:
	//physx::Mutex		mRenderDataLock;
	// Converting to be a PS3 SPU-friendly lock
	// On PC this is the same as a Mutex, on PS3 it is a 128b (!) aligned U32. Subclasses might get bigger on PS3 and they
	// are most likely distributed over more than one cache line.
	physx::AtomicLock	mRenderDataLock;

	physx::PxBounds3	mRenderBounds;
};

}
} // end namespace physx::apex

#endif // APEX_RENDERABLE_H
