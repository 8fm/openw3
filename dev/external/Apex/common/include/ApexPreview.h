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

#ifndef __APEX_PREVIEW_H__
#define __APEX_PREVIEW_H__

#include "ApexRenderable.h"
#include "ApexInterface.h"
#include "PxMat34Legacy.h"


#if NX_SDK_VERSION_MAJOR == 2
class NxActorDescBase;
#endif

namespace physx
{
namespace apex
{

class ApexContext;

/**
	Class that implements preview interface
*/
class ApexPreview : public ApexRenderable
{
public:
	ApexPreview();
	virtual  						~ApexPreview();

	// Each class that derives from ApexPreview may optionally implement this function
	virtual NxApexRenderable*		getRenderable()
	{
		return NULL;
	}

	virtual void					setPose(const physx::PxMat44& pose);
	virtual const physx::PxMat44	getPose() const;

	virtual void					release() = 0;
	void							destroy();

protected:
	bool					mInRelease;

	physx::PxMat34Legacy	mPose;
};

}
} // end namespace physx::apex

#endif // __APEX_PREVIEW_H__
