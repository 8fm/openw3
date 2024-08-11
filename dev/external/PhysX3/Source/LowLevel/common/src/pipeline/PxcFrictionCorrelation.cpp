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


#include "PxvConfig.h"
#include "PxcCorrelationBuffer.h"
#include "PxcNpThreadContext.h"
#include "PxcNpWorkUnit.h"
#include "PxsMaterialManager.h"

using namespace physx;
using namespace Gu;

namespace physx
{

namespace
{

	PX_FORCE_INLINE void initContactPatch(PxcCorrelationBuffer::ContactPatchData& patch, PxU16 index, PxU16 materialIndex0, PxU16 materialIndex1,
		PxU16 flags)
	{
		patch.start = index;
		patch.count = 1;
		patch.next = 0;
		patch.materialIndex0 = materialIndex0;
		patch.materialIndex1 = materialIndex1;
		patch.flags = flags;
	}

	PX_FORCE_INLINE void initFrictionPatch(PxcFrictionPatch& p, const PxVec3& body0Normal, const PxVec3& body1Normal, PxU16 materialIndex0, PxU16 materialIndex1,
		PxU16 flags)
	{
		p.body0Normal = body0Normal;
		p.body1Normal = body1Normal;
		p.anchorCount = 0;
		p.broken = 0;
		p.materialIndex0 = materialIndex0;
		p.materialIndex1 = materialIndex1;
		p.materialFlags = flags;
	}

	PX_FORCE_INLINE void initContactPatchCoulomb(PxcCorrelationBufferCoulomb::ContactPatchData& patch, PxU16 index, PxU16 faceIndex0, PxU16 faceIndex1,
		PxU16 flags)
	{
		patch.start = index;
		patch.count = 1;
		patch.next = 0;
		patch.materialIndex0 = faceIndex0;
		patch.materialIndex1 = faceIndex1;
		patch.flags = flags;
	}

}

bool createContactPatches(PxcNpThreadContext& threadContext, PxU32 startIndex, PxReal normalTolerance)
{

	PxcCorrelationBuffer& fb = threadContext.mCorrelationBuffer;
	ContactBuffer& cb = threadContext.mContactBuffer;

	// PT: this rewritten version below doesn't have LHS

	PxU32 contactPatchCount = fb.contactPatchCount;
	const PxU32 countactCount = cb.count - startIndex;
	if(contactPatchCount == PxcCorrelationBuffer::MAX_FRICTION_PATCHES)
		return false;
	if(countactCount>0)
	{
		PxcCorrelationBuffer::ContactPatchData* currentPatchData = fb.contactPatches + contactPatchCount;
		const Gu::ContactPoint* PX_RESTRICT contacts = &cb.contacts[startIndex];

		PxU16 count=1;

		initContactPatch(fb.contactPatches[contactPatchCount++], Ps::to16(startIndex), (PxU16)(contacts[0].internalFaceIndex0 &0xffff), (PxU16)(contacts[0].internalFaceIndex0 >> 16),
			(PxU16)contacts[0].internalFaceIndex1);
		//initContactPatch(fb.contactPatches[contactPatchCount++], 0, materialInfo[0].mMaterialIndex0, materialInfo[0].mMaterialIndex1);

		PxU32 patchIndex = 0;

		for(PxU32 i=1;i<countactCount;i++)
		{
			const Gu::ContactPoint& curContact = contacts[i];
			const Gu::ContactPoint& preContact = contacts[patchIndex];
			//const PxsMaterialInfo& curMat = materialInfo[i];
			//const PxsMaterialInfo& preMat = materialInfo[patchIndex];

			if(curContact.internalFaceIndex0 == preContact.internalFaceIndex0
				&& curContact.internalFaceIndex1 == preContact.internalFaceIndex1
				&& curContact.normal.dot(preContact.normal)>=normalTolerance)
			{
				count++;
			}
			else
			{
				//KS -	this will only be triggered inside the block solver prep. At this point, it will fall back to the one-at-a-time solver prep. As 
				//		the maximum number of contacts in the contact buffer == maximum number of contact patches, this will not fail for one-at-a-time code
				if(contactPatchCount == Gu::ContactBuffer::MAX_CONTACTS)
					return false;
				patchIndex = i;
				currentPatchData->count = count;
				count = 1;
				currentPatchData = fb.contactPatches + contactPatchCount;
				//initContactPatch(fb.contactPatches[contactPatchCount++], i, curContact.internalFaceIndex0, curContact.internalFaceIndex1);
				initContactPatch(fb.contactPatches[contactPatchCount++], Ps::to16(i+startIndex), (PxU16)(curContact.internalFaceIndex0 &0xffff), (PxU16)(curContact.internalFaceIndex0 >> 16),
					(PxU16)curContact.internalFaceIndex1);
			}
		}
		if(count!=1)
			currentPatchData->count = count;
	}
	fb.contactPatchCount = contactPatchCount;
	return true;
}

bool createContactPatchesCoulomb(PxcCorrelationBufferCoulomb& fb, ContactBuffer& cb, PxReal normalTolerance, PxU32 startContactIndex)
{
	PxU32 contactPatchCount = fb.contactPatchCount;
	const PxU32 countactCount = cb.count - startContactIndex;

	if(contactPatchCount == PxcCorrelationBufferCoulomb::MAX_FRICTION_PATCHES)
		return false;

	if(countactCount>0)
	{
		PxcCorrelationBufferCoulomb::ContactPatchData* currentPatchData = fb.contactPatches + contactPatchCount;
		const Gu::ContactPoint* PX_RESTRICT contacts = &cb.contacts[startContactIndex];

		PxU16 count=1;

		initContactPatchCoulomb(fb.contactPatches[contactPatchCount++], Ps::to16(startContactIndex), (PxU16)(contacts[0].internalFaceIndex0 &0xffff), (PxU16)(contacts[0].internalFaceIndex0 >> 16),
			(PxU16)contacts[0].internalFaceIndex1);
		
		for(PxU32 i=1;i<countactCount;i++)
		{
			const Gu::ContactPoint& curContact = contacts[i];
			const Gu::ContactPoint& preContact = contacts[i-1];
			//const PxsMaterialInfo& curMat = materialInfo[i];
			//const PxsMaterialInfo& preMat = materialInfo[i-1];

			if(curContact.normal.dot(preContact.normal)>=normalTolerance 
				&& curContact.internalFaceIndex0 == preContact.internalFaceIndex0
				&& curContact.internalFaceIndex1 == preContact.internalFaceIndex1)
			{
				count++;
			}
			else
			{
				if(contactPatchCount == Gu::ContactBuffer::MAX_CONTACTS)
					return false;
				currentPatchData->count = count;
				count = 1;
				currentPatchData = fb.contactPatches + contactPatchCount;
				initContactPatchCoulomb(fb.contactPatches[contactPatchCount++], Ps::to16(i+startContactIndex), (PxU16)(curContact.internalFaceIndex0 &0xffff), (PxU16)(curContact.internalFaceIndex0 >> 16),
					(PxU16)curContact.internalFaceIndex1);

			}
		}
		if(count!=1)
			currentPatchData->count = count;
	}
	fb.contactPatchCount = contactPatchCount;
	return true;
}

// run over the patches, finding a correlated friction patch for each, or creating
// a new one if we can't find a match

bool correlatePatches(PxcCorrelationBuffer& fb, 
					  const ContactBuffer& cb,
					  const PxTransform& bodyFrame0,
					  const PxTransform& bodyFrame1,
					  PxReal normalTolerance,
					  PxU32 startContactPatchIndex,
					  PxU32 startFrictionPatchIndex)
{
	bool overflow = false;

	for(PxU32 i=startFrictionPatchIndex;i<fb.frictionPatchCount+1;i++)
	{
		fb.correlationListHeads[i] = PxcCorrelationBuffer::LIST_END;
		fb.frictionPatchContactCounts[i] = 0;
	}

	//PxU32 maxCorrelationHeadIndex = 0;
	for(PxU32 i=startContactPatchIndex;i<fb.contactPatchCount;i++)
	{
		PxcCorrelationBuffer::ContactPatchData &c = fb.contactPatches[i];
		PxVec3 patchNormal = cb.contacts[c.start].normal;
		PxVec3 body0Normal = bodyFrame0.rotateInv(patchNormal);
		PxU32 j=startFrictionPatchIndex;
		for(;j<fb.frictionPatchCount && ((body0Normal.dot(fb.frictionPatches[j].body0Normal) < normalTolerance) 
			|| fb.frictionPatches[j].materialIndex0 != c.materialIndex0|| fb.frictionPatches[j].materialIndex1 != c.materialIndex1) ;j++)
			;

		if(j==fb.frictionPatchCount)
		{
			overflow |= j==PxcCorrelationBuffer::MAX_FRICTION_PATCHES;
			if(overflow)
				continue;	
			fb.contactID[fb.frictionPatchCount][0] = 0xffff;
			fb.contactID[fb.frictionPatchCount][1] = 0xffff;
			initFrictionPatch(fb.frictionPatches[fb.frictionPatchCount++],
				body0Normal,bodyFrame1.rotateInv(patchNormal), c.materialIndex0, c.materialIndex1, c.flags);
			if(j+1 < PxcCorrelationBuffer::MAX_FRICTION_PATCHES)
			{
				fb.frictionPatchContactCounts[j+1] = 0;
				fb.correlationListHeads[j+1] = PxcCorrelationBuffer::LIST_END;
			}

		}
		c.next = Ps::to16(fb.correlationListHeads[j]);
		fb.correlationListHeads[j] = i;
		fb.frictionPatchContactCounts[j] += c.count;
	}

	//fb.correlationListHeads[maxCorrelationHeadIndex+1] = PxcCorrelationBuffer::LIST_END;
	return overflow;
}


bool correlatePatchesCoulomb(PxcCorrelationBufferCoulomb& fb, 
					  const ContactBuffer& cb,
					  const PxTransform& bodyFrame0,
					  const PxTransform& bodyFrame1,
					  PxReal normalTolerance,
					  PxU32 numFrictionPerPoint,
					  PxU32 startContactPatchIndex,
					  PxU32 startFrictionPatchIndex)
{
	PX_UNUSED(bodyFrame0);
	PX_UNUSED(bodyFrame1);

	bool overflow = false;
	PxU32 frictionPatchCount = fb.frictionPatchCount;

	for(PxU32 i=startContactPatchIndex;i<fb.contactPatchCount;i++)
	{
		PxcCorrelationBufferCoulomb::ContactPatchData &c = fb.contactPatches[i];
		const PxVec3 patchNormal = cb.contacts[c.start].normal;

		const PxU8 frictionConstrtaintCount = Ps::to8(c.count * numFrictionPerPoint);

		PxU32 j=startFrictionPatchIndex;
		for(;j<frictionPatchCount && ((patchNormal.dot(fb.frictionPatches[j].normal) < normalTolerance) 
			|| fb.frictionPatches[j].materialIndex0 != c.materialIndex0|| fb.frictionPatches[j].materialIndex1 != c.materialIndex1);j++)
			;


		if(j==frictionPatchCount)
		{
			overflow |= j==PxcCorrelationBuffer::MAX_FRICTION_PATCHES;
			if(overflow)
				continue;

			//fb.correlationListHeads[frictionPatchCount] = PxcCorrelationBuffer::LIST_END;
			fb.frictionPatches[frictionPatchCount].numConstraints = frictionConstrtaintCount;
			fb.frictionPatches[frictionPatchCount].normal = patchNormal;
			fb.frictionPatchContactCounts[frictionPatchCount] = c.count;
			fb.frictionPatches[frictionPatchCount].materialIndex0 = c.materialIndex0;
			fb.frictionPatches[frictionPatchCount].materialIndex1 = c.materialIndex1;
			fb.frictionPatches[frictionPatchCount].materialFlags = c.flags;
			fb.contactID[frictionPatchCount][0] = 0xffff;
			fb.contactID[frictionPatchCount++][1] = 0xffff;
			c.next = PxcCorrelationBuffer::LIST_END;
		}
		else
		{
			fb.frictionPatches[j].numConstraints += frictionConstrtaintCount;
			fb.frictionPatchContactCounts[j] += c.count;
			c.next = Ps::to16(fb.correlationListHeads[j]);
		}

		//c.next = fb.correlationListHeads[j];
		fb.correlationListHeads[j] = i;
	}

	fb.frictionPatchCount = frictionPatchCount;

	return overflow;
}


// run over the friction patches, trying to find two anchors per patch. If we already have
// anchors that are close, we keep them, which gives us persistent spring behavior

void growPatches(PxcCorrelationBuffer& fb,
				 const ContactBuffer& cb,
				 const PxTransform& bodyFrame0,
				 const PxTransform& bodyFrame1,
				 PxReal	,	//unused correlationDistance
				 PxU32 frictionPatchStartIndex,
				 PxReal frictionOffsetThreshold)
{
	for(PxU32 i=frictionPatchStartIndex;i<fb.frictionPatchCount;i++)
	{
		PxcFrictionPatch& fp = fb.frictionPatches[i];

		if(fp.anchorCount==2 || fb.correlationListHeads[i]==PxcCorrelationBuffer::LIST_END)
			continue;

		PxVec3 worldAnchors[2];
		PxU16 anchorCount = 0;
		PxReal pointDistSq = 0.0f, dist0, dist1;

		// if we have an anchor already, keep it
		if(fp.anchorCount == 1)
		{
			worldAnchors[anchorCount++] = bodyFrame0.transform(fp.body0Anchors[0]);
		}

		for(PxU32 patch = fb.correlationListHeads[i]; 
			patch!=PxcCorrelationBuffer::LIST_END; 
			patch = fb.contactPatches[patch].next)
		{
			PxcCorrelationBuffer::ContactPatchData& cp = fb.contactPatches[patch];
			for(PxU16 j=0;j<cp.count;j++)
			{
				const PxVec3& worldPoint = cb.contacts[cp.start+j].point;

				if(cb.contacts[cp.start+j].separation < frictionOffsetThreshold)
				{

					switch(anchorCount)
					{
					case 0:
						fb.contactID[i][0] = cp.start+j;
						worldAnchors[0] = worldPoint;
						anchorCount++;
						break;
					case 1:
						pointDistSq = (worldPoint-worldAnchors[0]).magnitudeSquared(); 
						if (pointDistSq > (0.025f * 0.025f))
						{
							fb.contactID[i][1] = cp.start+j;
							worldAnchors[1] = worldPoint;
							anchorCount++;
						}
						break;
					default: //case 2
						dist0 = (worldPoint-worldAnchors[0]).magnitudeSquared();
						dist1 = (worldPoint-worldAnchors[1]).magnitudeSquared();
						if (dist0 > dist1)
						{
							if(dist0 > pointDistSq)
							{
								fb.contactID[i][1] = cp.start+j;
								worldAnchors[1] = worldPoint;
								pointDistSq = dist0;
							}
						}
						else if (dist1 > pointDistSq)
						{
							fb.contactID[i][0] = cp.start+j;
							worldAnchors[0] = worldPoint;
							pointDistSq = dist1;
						}
					}
				}
			}
		}

		//PX_ASSERT(anchorCount > 0);

		// add the new anchor(s) to the patch
		for(PxU32 i = fp.anchorCount; i < anchorCount; i++)
		{
			fp.body0Anchors[i] = bodyFrame0.transformInv(worldAnchors[i]);
			fp.body1Anchors[i] = bodyFrame1.transformInv(worldAnchors[i]);
		}
		fp.anchorCount = anchorCount;
	}
}

}
