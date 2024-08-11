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


#ifndef PX_PHYSICS_SCP_CONTACTSTREAM
#define PX_PHYSICS_SCP_CONTACTSTREAM

#include "Px.h"
#include "PxSimulationEventCallback.h"


namespace physx
{
	class PxShape;

namespace Sc
{
	class ActorPair;


	// Internal counterpart of PxContactPair
	struct ContactShapePair
	{
	public:
		PxShape*				shapes[2];
		const PxU8*				contactStream;
		PxU32					requiredBufferSize;
		PxU16					contactCount;
		PxU16					constraintStreamSize;
		PxU16					flags;
		PxU16					events;
		PxU32					shapeID[2];
		//26 (or 38 on 64bit)
	};
	PX_COMPILE_TIME_ASSERT(sizeof(ContactShapePair) == sizeof(PxContactPair));

	struct ContactStreamManagerFlag
	{
		enum Enum
		{
			/**
			\brief Contains deleted shapes
			*/
			eDELETED_SHAPES				= (1<<0),

			/**
			\brief Invalid stream memory not allocated
			*/
			eINVALID_STREAM				= (1<<1),

						/**
			\brief Incomplete stream will be reported
			*/
			eINCOMPLETE_STREAM			= (1<<2),
		};
	};

	class ContactStreamManager
	{
	public:
		PX_FORCE_INLINE ContactStreamManager() : maxPairCount(0), flags(0) {}
		PX_FORCE_INLINE ~ContactStreamManager() {}

		PX_FORCE_INLINE void reset();
		PX_FORCE_INLINE const PxU32& getShapePairsIndex() const;

		PxU32				bufferIndex;
		PxU16				maxPairCount;
		PxU16				currentPairCount;
		PxU16				flags;
	};

} // namespace Sc


PX_FORCE_INLINE void Sc::ContactStreamManager::reset()
{
	currentPairCount = 0;
	flags = 0;
}


PX_FORCE_INLINE const PxU32& Sc::ContactStreamManager::getShapePairsIndex() const
{	
	return bufferIndex;
}


}

#endif
