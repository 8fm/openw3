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


#ifndef PX_FRAMEWORK_PXELEMENTINTERACTION
#define PX_FRAMEWORK_PXELEMENTINTERACTION

#include "ScInteraction.h"
#include "ScElement.h"
#include "PxvConfig.h"

namespace physx
{


namespace Sc
{

	class ElementInteraction : public Interaction
	{
		friend class Element;
	public:
		virtual ~ElementInteraction() {}
		PX_FORCE_INLINE	Element&	getElement0()	const	{ return mElement0; }
		PX_FORCE_INLINE	Element&	getElement1()	const	{ return mElement1; }

	protected:
		PX_INLINE ElementInteraction(Element& element0, Element& element1, InteractionType type, PxU8 flags);

	private:
		Element& mElement0;
		Element& mElement1;
	};

} // namespace Sc

//////////////////////////////////////////////////////////////////////////
PX_INLINE Sc::ElementInteraction::ElementInteraction(Element& element0, Element& element1, InteractionType type, PxU8 flags) :
	Interaction	(element0.getScActor(), element1.getScActor(), type, flags),
	mElement0	(element0),
	mElement1	(element1)
{
}


}

#endif
