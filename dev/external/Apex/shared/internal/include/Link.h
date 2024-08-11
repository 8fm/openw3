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
#ifndef LINK_H
#define LINK_H

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxAssert.h"

namespace physx
{
namespace apex
{

class Link
{
public:
	Link()
	{
		adj[1] = adj[0] = this;
	}

	virtual	~Link()
	{
		remove();
	}

	/*
		which = 0:	(-A-...-B-link-)  +  (-this-X-...-Y-)  =  (-A-...-B-link-this-X-...-Y-)
		which = 1:	(-X-...-Y-this-)  +  (-link-A-...-B-)  =  (-X-...-Y-this-link-A-...-B-)
	 */
	void	setAdj(physx::PxU32 which, Link* link)
	{
		physx::PxU32 other = (which &= 1) ^ 1;
		Link* linkAdjOther = link->adj[other];
		adj[which]->adj[other] = linkAdjOther;
		linkAdjOther->adj[which] = adj[which];
		adj[which] = link;
		link->adj[other] = this;
	}

	Link*	getAdj(physx::PxU32 which) const
	{
		return adj[which & 1];
	}

	void	remove()
	{
		adj[1]->adj[0] = adj[0];
		adj[0]->adj[1] = adj[1];
		adj[1] = adj[0] = this;
	}

	bool	isSolitary() const
	{
		PX_ASSERT((adj[0] == this) == (adj[1] == this));
		return adj[0] == this;
	}

protected:
	Link*	adj[2];
};

}
} // end namespace physx::apex


#endif // #ifndef LINK_H
