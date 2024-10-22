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

#include "PxcContactCache.h"
#include "PxsContactManager.h"

using namespace physx;
using namespace Gu;

#if CACHE_LOCAL_CONTACTS_XP

//#define ENABLE_CONTACT_CACHE_STATS

#ifdef ENABLE_CONTACT_CACHE_STATS
	static PxU32 gNbCalls;
	static PxU32 gNbHits;
#endif

void PxcClearContactCacheStats()
{
#ifdef ENABLE_CONTACT_CACHE_STATS
	gNbCalls = 0;
	gNbHits = 0;
#endif
}

void PxcDisplayContactCacheStats()
{
#ifdef ENABLE_CONTACT_CACHE_STATS
	pxPrintf("%d|%d (%f)\n", gNbHits, gNbCalls, gNbCalls ? float(gNbHits)/float(gNbCalls) : 0.0f);
#endif
}

namespace physx
{

	const bool g_CanUseContactCache[][7] =
	{
		//PxGeometryType::eSPHERE
		{
			false,		//PxcContactSphereSphere
			false,		//PxcContactSpherePlane
			true,		//PxcContactSphereCapsule
			false,		//PxcContactSphereBox
			true,		//PxcContactSphereConvex
			true,		//PxcContactSphereMesh
			true,		//PxcContactSphereHeightField
		},

		//PxGeometryType::ePLANE
		{
			false,		//-
			false,		//PxcInvalidContactPair
			true,		//PxcContactPlaneCapsule
			true,		//PxcContactPlaneBox
			true,		//PxcContactPlaneConvex
			false,		//PxcInvalidContactPair
			false,		//PxcInvalidContactPair
		},

		//PxGeometryType::eCAPSULE
		{
			false,		//-
			false,		//-
			true,		//PxcContactCapsuleCapsule
			true,		//PxcContactCapsuleBox
			true,		//PxcContactCapsuleConvex
			true,		//PxcContactCapsuleMesh
			true,		//PxcContactCapsuleHeightField
		},

		//PxGeometryType::eBOX
		{
			false,		//-
			false,		//-
			false,		//-
			true,		//PxcContactBoxBox
			true,		//PxcContactBoxConvex
			true,		//PxcContactBoxMesh
			true,		//PxcContactBoxHeightField
		},

		//PxGeometryType::eCONVEXMESH
		{
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			true,		//PxcContactConvexConvex
			true,		//PxcContactConvexMesh2
			true,		//PxcContactConvexHeightField
		},

		//PxGeometryType::eTRIANGLEMESH
		{
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//PxcInvalidContactPair
			false,		//PxcInvalidContactPair
		},

		//PxGeometryType::eHEIGHTFIELD
		{
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//PxcInvalidContactPair
		},
	};

	const bool g_CanUsePCMContactCache[][7] =
	{
		//PxGeometryType::eSPHERE
		{
			false,		//PxcContactSphereSphere
			false,		//PxcContactSpherePlane
			false,		//PxcContactSphereCapsule
			false,		//PxcContactSphereBox
			false,		//PxcContactSphereConvex
			false,		//PxcContactSphereMesh
			false,		//PxcContactSphereHeightField
		},

		//PxGeometryType::ePLANE
		{
			false,		//-
			false,		//PxcInvalidContactPair
			true,		//PxcContactPlaneCapsule
			true,		//PxcContactPlaneBox
			true,		//PxcContactPlaneConvex
			false,		//PxcInvalidContactPair
			false,		//PxcInvalidContactPair
		},

		//PxGeometryType::eCAPSULE
		{
			false,		//-
			false,		//-
			false,		//PxcContactCapsuleCapsule
			false,		//PxcContactCapsuleBox
			false,		//PxcContactCapsuleConvex
			false,		//PxcContactCapsuleMesh
			false,		//PxcContactCapsuleHeightField
		},

		//PxGeometryType::eBOX
		{
			false,		//-
			false,		//-
			false,		//-
			false,		//PxcContactBoxBox
			false,		//PxcContactBoxConvex
			false,		//PxcContactBoxMesh
			false,		//PxcPCMContactBoxHeightField
		},

		//PxGeometryType::eCONVEXMESH
		{
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//PxcContactConvexConvex
			false,		//PxcPCMContactConvexMesh
			false,		//PxcPCMContactConvexHeightField
		},

		//PxGeometryType::eTRIANGLEMESH
		{
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//PxcInvalidContactPair
			false,		//PxcInvalidContactPair
		},

		//PxGeometryType::eHEIGHTFIELD
		{
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//-
			false,		//PxcInvalidContactPair
		},
	};

}

static PX_FORCE_INLINE void updateContact(	Gu::ContactPoint& dst, const PxcLocalContactsCache& contactsData, 
											const Cm::Matrix34& world0, const Cm::Matrix34& world1,
											const PxVec3& point, const PxVec3& normal, float separation)
{
	const PxVec3 tmp0 = contactsData.mTransform0.transformInv(point);
	const PxVec3 worldpt0 = world0.transform(tmp0);

	const PxVec3 tmp1 = contactsData.mTransform1.transformInv(point);
	const PxVec3 worldpt1 = world1.transform(tmp1);

	const PxVec3 motion = worldpt0 - worldpt1;
	dst.normal		= normal;
	dst.point		= (worldpt0 + worldpt1)*0.5f;
	//dst.point		= point;
	dst.separation	= separation + motion.dot(normal);
}

namespace physx
{
	void PxcCacheLocalContacts(const PxGeometryType::Enum type0, const PxGeometryType::Enum type1, PxcNpThreadContext& context, PxcNpCache& pairContactCache, const PxTransform& tm0, const PxTransform& tm1,
		const PxReal contactDistance, const PxcContactMethod conMethod, const Gu::GeometryUnion& shape0, const Gu::GeometryUnion& shape1);
}

static PX_FORCE_INLINE void prefetchData128(PxU8* PX_RESTRICT ptr, PxU32 size)
{
	// PT: always prefetch the cache line containing our address (which unfortunately won't be aligned to 128 most of the time)
	Ps::prefetchLine(ptr, 0);
	// PT: compute start offset of our data within its cache line
	const PxU32 startOffset = PxU32(size_t(ptr)&127);
	// PT: prefetch next cache line if needed
	if(startOffset+size>128)
		Ps::prefetchLine(ptr+128, 0);
}

static PX_FORCE_INLINE PxU8* outputToCache(PxU8* PX_RESTRICT bytes, const PxVec3& v)
{
	*(PxVec3*)bytes = v;
	return bytes + sizeof(PxVec3);
}

static PX_FORCE_INLINE PxU8* outputToCache(PxU8* PX_RESTRICT bytes, PxReal v)
{
	*(PxReal*)bytes = v;
	return bytes + sizeof(PxReal);
}

static PX_FORCE_INLINE PxU8* outputToCache(PxU8* PX_RESTRICT bytes, PxU32 v)
{
	*(PxU32*)bytes = v;
	return bytes + sizeof(PxU32);
}

//PxU32 gContactCache_NbCalls = 0;
//PxU32 gContactCache_NbHits = 0;

static PX_FORCE_INLINE PxReal maxComponentDeltaPos(const PxTransform& t0, const PxTransform& t1)
{
	PxReal delta =       PxAbs(t0.p.x - t1.p.x);
	delta = PxMax(delta, PxAbs(t0.p.y - t1.p.y));
	delta = PxMax(delta, PxAbs(t0.p.z - t1.p.z));
	return delta;
}

static PX_FORCE_INLINE PxReal maxComponentDeltaRot(const PxTransform& t0, const PxTransform& t1)
{
	PxReal delta =       PxAbs(t0.q.x - t1.q.x);
	delta = PxMax(delta, PxAbs(t0.q.y - t1.q.y));
	delta = PxMax(delta, PxAbs(t0.q.z - t1.q.z));
	delta = PxMax(delta, PxAbs(t0.q.w - t1.q.w));
	return delta;
}

void physx::PxcCacheLocalContacts(	const PxGeometryType::Enum type0, const PxGeometryType::Enum type1,
									PxcNpThreadContext& context, PxcNpCache& pairContactCache,
									const PxTransform& tm0, const PxTransform& tm1,
									const PxReal contactDistance, const PxcContactMethod conMethod,
									const Gu::GeometryUnion& shape0, const Gu::GeometryUnion& shape1)
{
	PX_UNUSED(type0);
	PX_UNUSED(type1);

	/*const bool useContactCache = g_CanUseContactCache[type0][type1];
	if(!useContactCache)
	{
		conMethod(shape0, shape1, tm0, tm1, contactDistance, pairContactCache, context);
		return;
	}*/

//	gContactCache_NbCalls++;

	if(pairContactCache.ptr)
		prefetchData128(pairContactCache.ptr, pairContactCache.size);

	ContactBuffer& contactBuffer = context.mContactBuffer;
	contactBuffer.reset();

	PxcLocalContactsCache contactsData;
	PxU32 nbCachedBytes;
	const PxU8* cachedBytes = PxcNpCacheRead2(pairContactCache, contactsData, nbCachedBytes);

	pairContactCache.ptr = 0;
	pairContactCache.size = 0;

#ifdef ENABLE_CONTACT_CACHE_STATS
	gNbCalls++;
#endif

	const PxU32 payloadSize = (sizeof(PxcLocalContactsCache)+3)&~3;

	if(cachedBytes)
	{
		// PT: we used to store the relative TM but it's better to save memory and recompute it
		const PxTransform t0to1 = tm1.transformInv(tm0);
		const PxTransform relTM = contactsData.mTransform1.transformInv(contactsData.mTransform0);

		const PxReal epsilon = 0.01f;
		if(		maxComponentDeltaPos(t0to1, relTM)<epsilon*context.mToleranceLength
			&&	maxComponentDeltaRot(t0to1, relTM)<epsilon)
		{
//			gContactCache_NbHits++;
			const PxU32 nbContacts = contactsData.mNbCachedContacts;
#if __SPU__
//			pxPrintf("Codepath1: %d\n", nbContacts);
#endif

			PxU8* ls = PxcNpCacheWriteInitiate(context.mNpCacheStreamPair, pairContactCache, contactsData, nbCachedBytes);
			prefetchData128(ls, (payloadSize + 4 + nbCachedBytes + 0xF)&~0xF);

			contactBuffer.count = nbContacts;
			if(nbContacts)
			{
				Gu::ContactPoint* PX_RESTRICT dst = contactBuffer.contacts;
				Ps::invalidateCache(dst, nbContacts*sizeof(Gu::ContactPoint));

				const Cm::Matrix34 world1(tm1);
				const Cm::Matrix34 world0(tm0);

				const bool sameNormal = contactsData.mSameNormal;

				const PxU8* contacts = (const PxU8*)cachedBytes;
				const PxVec3* normal0 = NULL;
				for(PxU32 i=0;i<nbContacts;i++)
				{
					if(i!=nbContacts-1)
						Ps::prefetchLine(contacts, 128);

					const PxVec3* cachedNormal;
					if(!i || !sameNormal)
					{
						cachedNormal	= (const PxVec3*)contacts;	contacts += sizeof(PxVec3);
						normal0 = cachedNormal;
					}
					else
					{
						cachedNormal = normal0;
					}

					const PxVec3* cachedPoint	= (const PxVec3*)contacts;	contacts += sizeof(PxVec3);
					const PxReal* cachedPD		= (const PxReal*)contacts;	contacts += sizeof(PxReal);

					updateContact(*dst, contactsData, world0, world1, *cachedPoint, *cachedNormal, *cachedPD);

					if(contactsData.mUseFaceIndices)
					{
						const PxU32* cachedIndex0	= (const PxU32*)contacts;	contacts += sizeof(PxU32);
						const PxU32* cachedIndex1	= (const PxU32*)contacts;	contacts += sizeof(PxU32);

						dst->internalFaceIndex0	= *cachedIndex0;
						dst->internalFaceIndex1	= *cachedIndex1;
					}
					else
					{
						dst->internalFaceIndex0	= PXC_CONTACT_NO_FACE_INDEX;
						dst->internalFaceIndex1	= PXC_CONTACT_NO_FACE_INDEX;
					}
					dst++;
				}
			}
			if(ls)
				PxcNpCacheWriteFinalize(ls, contactsData, nbCachedBytes, cachedBytes);
#ifdef ENABLE_CONTACT_CACHE_STATS
			gNbHits++;
#endif
			return;
		}
		else
		{
			// PT: if we reach this point we cached the contacts but we couldn't use them next frame
			// => waste of time and memory
		}
	}

	conMethod(shape0, shape1, tm0, tm1, contactDistance, pairContactCache, context.mContactBuffer);

	//if(contactBuffer.count)
	{
		contactsData.mTransform0 = tm0;
		contactsData.mTransform1 = tm1;

#if __SPU__
//		pxPrintf("Codepath0: %d\n", contactBuffer.count);
#endif

		PxU32 nbBytes = 0;
		const PxU8* bytes = NULL;
		const PxU32 count = contactBuffer.count;
		if(count)
		{
			const bool useFaceIndices =		contactBuffer.contacts[0].internalFaceIndex0!=PXC_CONTACT_NO_FACE_INDEX
										||	contactBuffer.contacts[0].internalFaceIndex1!=PXC_CONTACT_NO_FACE_INDEX;
			contactsData.mNbCachedContacts	= Ps::to16(count);
			contactsData.mUseFaceIndices	= useFaceIndices;

			const Gu::ContactPoint* PX_RESTRICT srcContacts = contactBuffer.contacts;
			// PT: this loop should not be here. We should output the contacts directly compressed, as we used to.
			bool sameNormal = true;
			{
				const PxVec3 normal0 = srcContacts->normal;
				for(PxU32 i=1;i<count;i++)
				{
					if(srcContacts[i].normal!=normal0)
					{
						sameNormal = false;
						break;
					}
				}			
			}
			contactsData.mSameNormal = sameNormal;

			if(!sameNormal)
			{
				const PxU32 sizeof_CachedContactPoint = sizeof(PxVec3) + sizeof(PxVec3) + sizeof(PxReal);
				const PxU32 sizeof_CachedContactPointAndFaceIndices = sizeof_CachedContactPoint + sizeof(PxU32)*2;
				const PxU32 sizeOfItem = useFaceIndices ? sizeof_CachedContactPointAndFaceIndices : sizeof_CachedContactPoint;
				nbBytes = count * sizeOfItem;
			}
			else
			{
				const PxU32 sizeof_CachedContactPoint = sizeof(PxVec3) + sizeof(PxReal);
				const PxU32 sizeof_CachedContactPointAndFaceIndices = sizeof_CachedContactPoint + sizeof(PxU32)*2;
				const PxU32 sizeOfItem = useFaceIndices ? sizeof_CachedContactPointAndFaceIndices : sizeof_CachedContactPoint;
				nbBytes = sizeof(PxVec3) + count * sizeOfItem;
			}
			PxU8* ls = PxcNpCacheWriteInitiate(context.mNpCacheStreamPair, pairContactCache, contactsData, nbBytes);
			if(ls)
			{
				*reinterpret_cast<PxcLocalContactsCache*>(ls) = contactsData;
				*reinterpret_cast<PxU32*>(ls+payloadSize) = nbBytes;
				bytes = ls+payloadSize+sizeof(PxU32);
				PxU8* dest = const_cast<PxU8*>(bytes);
				for(PxU32 i=0;i<count;i++)
				{
					if(!i || !sameNormal)
						dest = outputToCache(dest, srcContacts[i].normal);
					dest = outputToCache(dest, srcContacts[i].point);
					dest = outputToCache(dest, srcContacts[i].separation);
					if(useFaceIndices)
					{
						dest = outputToCache(dest, srcContacts[i].internalFaceIndex0);
						dest = outputToCache(dest, srcContacts[i].internalFaceIndex1);
					}
				}
				PX_ASSERT(size_t(dest) - size_t(bytes)==nbBytes);
			}
			else
			{
				contactsData.mNbCachedContacts = 0;
				PxcNpCacheWrite(context.mNpCacheStreamPair, pairContactCache, contactsData, 0, bytes);
			}
		}
		else
		{
			contactsData.mNbCachedContacts	= 0;
			contactsData.mUseFaceIndices	= false;
			PxcNpCacheWrite(context.mNpCacheStreamPair, pairContactCache, contactsData, nbBytes, bytes);
		}
	}
}


#endif

