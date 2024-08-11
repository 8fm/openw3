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


#ifndef PX_PHYSICS_SCP_ELEMENT_INTERACTION
#define PX_PHYSICS_SCP_ELEMENT_INTERACTION

#include "ScCoreInteraction.h"
#include "ScElementInteraction.h"
#include "ScElementSim.h"

namespace physx
{
namespace Sc
{

	class NPhaseCore;
	class ElementSim;

	class ElementSimInteraction : public CoreInteraction, public ElementInteraction
	{
	public:
		PX_INLINE ElementSimInteraction(ElementSim& element0, ElementSim& element1, InteractionType type, PxU8 flags);
		virtual ~ElementSimInteraction() {}

		PX_INLINE ElementSim& getElementSim0() const;
		PX_INLINE ElementSim& getElementSim1() const;

		//--------- CoreInteraction ---------
		virtual NPhaseCore* getNPhaseCore() const;
		//-----------------------------------

		// Method to check if this interaction is the last filter relevant interaction between the two elements,
		// i.e., if this interaction gets deleted, the pair is considered lost
		virtual bool isLastFilterInteraction() const { return true; }

	private:
	};

} // namespace Sc

//////////////////////////////////////////////////////////////////////////
PX_INLINE Sc::ElementSimInteraction::ElementSimInteraction(ElementSim& element0, ElementSim& element1, InteractionType type, PxU8 flags) :
	CoreInteraction		(true),
	ElementInteraction	(element0, element1, type, flags)
{
}

PX_INLINE Sc::ElementSim& Sc::ElementSimInteraction::getElementSim0() const
{
	return static_cast<ElementSim&>(getElement0());
}

PX_INLINE Sc::ElementSim& Sc::ElementSimInteraction::getElementSim1() const
{
	return static_cast<ElementSim&>(getElement1());
}

}

#endif
