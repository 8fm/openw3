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

// Temp file used to compile various ICE files - don't touch!
#ifndef GU_ICESUPPORT_H
#define GU_ICESUPPORT_H

#include "PxVec3.h"
#include "Ice/IceContainer.h"

namespace physx
{

namespace Gu
{
	class Box;
}

	// PT: emulate Gu::Container::Reserve()
	PX_FORCE_INLINE PxU32* reserve(Ps::Array<PxU32>& container, PxU32 sizeToReserve)
	{
		const PxU32 currentSize = container.size();
		const PxU32 currentCap = container.capacity();
		const PxU32 requiredSize = currentSize + sizeToReserve;
		if(requiredSize > currentCap)
		{
			container.reserve(currentCap*2);
		}
		container.resizeUninitialized(currentSize + sizeToReserve);
		return container.begin() + currentSize;
	}

	class FIFOStack : public Gu::Container
	{
		public:
		//! Constructor
									FIFOStack() : mCurIndex(0)	{}
		//! Destructor
									~FIFOStack()				{}
		// Management
		PX_INLINE	FIFOStack&		Push(PxU32 entry)			{	Add(entry);	return *this;	}
					bool			Pop(PxU32 &entry);
		private:
					PxU32			mCurIndex;			//!< Current index within the container
	};

	PX_PHYSX_COMMON_API void CreateSweptOBB(Gu::Box& dest, const Gu::Box& box, const PxVec3& dir, float d);
}

#endif
	
