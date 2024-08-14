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

#include "GuIntersectionEdgeEdge.h"
#include "GuDistanceSegmentTriangle.h"
#include "GuIntersectionRayTriangle.h"
#include "GuCapsule.h"
#include "GuTriangleVertexPointers.h"
#include "GuIntersectionTriangleBox.h"
#include "GuGeomUtilsInternal.h"
#include "GuContactMethodImpl.h"
#include "GuFeatureCode.h"
#include "GuContactBuffer.h"
#include "OPC_OBBCollider.h"
#include "CmScaling.h"
#include "GuTriangleCache.h"
#include "GuEntityReport.h"
#include "GuHeightField.h"
#include "GuHeightFieldUtil.h"
#include "GuConvexEdgeFlags.h"

using namespace physx;
using namespace Gu;

#define DEBUG_RENDER_MESHCONTACTS 0

#if DEBUG_RENDER_MESHCONTACTS
#include "PxPhysics.h"
#include "PxScene.h"
#endif

#ifdef __SPU__
extern unsigned char HeightFieldBuffer[sizeof(Gu::HeightField)+16];
#include "CmMemFetch.h"
#endif

#define USE_AABB_TRI_CULLING

//#define USE_CAPSULE_TRI_PROJ_CULLING
//#define USE_CAPSULE_TRI_SAT_CULLING
//#define PRINT_STATS
#define VISUALIZE_TOUCHED_TRIS	0
#define VISUALIZE_CULLING_BOX	0

#if VISUALIZE_TOUCHED_TRIS
#include "CmRenderOutput.h"
#include "PxsContactManager.h"
#include "PxsContext.h"
static void gVisualizeLine(const PxVec3& a, const PxVec3& b, PxcNpThreadContext& context, PxU32 color=0xffffff)
{
	PxMat44 m = PxMat44::identity();

	Cm::RenderOutput& out = context.mRenderOutput;
	out << color << m << Cm::RenderOutput::LINES << a << b;
}
static void gVisualizeTri(const PxVec3& a, const PxVec3& b, const PxVec3& c, PxcNpThreadContext& context, PxU32 color=0xffffff)
{
	PxMat44 m = PxMat44::identity();

	Cm::RenderOutput& out = context.mRenderOutput;
	out << color << m << Cm::RenderOutput::TRIANGLES << a << b << c;
}

static PxU32 gColors[8] = { 0xff0000ff, 0xff00ff00, 0xffff0000,
							0xff00ffff, 0xffff00ff, 0xffffff00,
							0xff000080, 0xff008000};
#endif

static const float fatBoxEdgeCoeff = 0.01f;

static bool PxcTestAxis(const PxVec3& axis, 
						const Gu::Segment& segment, 
						PxReal radius, 
						const PxVec3* PX_RESTRICT triVerts,
						PxReal& depth)
{
	// Project capsule
	PxReal min0 = segment.p0.dot(axis);
	PxReal max0 = segment.p1.dot(axis);
	if(min0>max0)	Ps::swap(min0, max0);
	min0 -= radius;
	max0 += radius;

	// Project triangle
	float Min1, Max1;
	{
		Min1 = Max1 = triVerts[0].dot(axis);
		const PxReal dp1 = triVerts[1].dot(axis);
//		if(dp<Min1)	Min1 = dp;
//		if(dp>Max1)	Max1 = dp;
		Min1 = physx::intrinsics::selectMin(Min1, dp1);
		Max1 = physx::intrinsics::selectMax(Max1, dp1);
		const PxReal dp2 = triVerts[2].dot(axis);
//		if(dp<Min1)	Min1 = dp;
//		if(dp>Max1)	Max1 = dp;
		Min1 = physx::intrinsics::selectMin(Min1, dp2);
		Max1 = physx::intrinsics::selectMax(Max1, dp2);
	}

	// Test projections
	if(max0<Min1 || Max1<min0)
		return false;

	const PxReal d0 = max0 - Min1;
	PX_ASSERT(d0>=0.0f);
	const PxReal d1 = Max1 - min0;
	PX_ASSERT(d1>=0.0f);
	depth = physx::intrinsics::selectMin(d0, d1);
	return true;
}

PX_FORCE_INLINE static PxVec3 PxcComputeTriangleNormal(const PxVec3* PX_RESTRICT triVerts)
{
	return ((triVerts[0]-triVerts[1]).cross(triVerts[0]-triVerts[2])).getNormalized();
}

PX_FORCE_INLINE static PxVec3 PxcComputeTriangleCenter(const PxVec3* PX_RESTRICT triVerts)
{
	static const PxReal inv3 = 1.0f / 3.0f;
	return (triVerts[0] + triVerts[1] + triVerts[2]) * inv3;
}

static bool PxcCapsuleTriOverlap3(PxU8 edgeFlags,
								  const Gu::Segment& segment,
								  PxReal radius, 
								  const PxVec3* PX_RESTRICT triVerts, 
								  PxReal* PX_RESTRICT t=NULL,
								  PxVec3* PX_RESTRICT pp=NULL)
{
	PxReal PenDepth = PX_MAX_REAL;

	// Test normal
	PxVec3 Sep = PxcComputeTriangleNormal(triVerts);
	if(!PxcTestAxis(Sep, segment, radius, triVerts, PenDepth))
		return false;

	// Test edges
	// PT: are those flags correct? Shouldn't we use Gu::ETD_CONVEX_EDGE_01/etc ?
	//const PxU8 ignoreEdgeFlag[] = {1, 4, 2};  // for edges 0-1, 1-2, 2-0 (see Gu::InternalTriangleMeshData::mExtraTrigData)
	// ML:: use the active edge flag instead of the concave flag
	const PxU32 activeEdgeFlag[] = {Gu::ETD_CONVEX_EDGE_01, Gu::ETD_CONVEX_EDGE_12, Gu::ETD_CONVEX_EDGE_20};
	const PxVec3 CapsuleAxis = (segment.p1 - segment.p0).getNormalized();
	for(PxU32 i=0;i<3;i++)
	{
		//bool active =((edgeFlags & ignoreEdgeFlag[i]) == 0);
		
		if(edgeFlags & activeEdgeFlag[i])
		{
		
			const PxVec3 e0 = triVerts[i];
//			const PxVec3 e1 = triVerts[(i+1)%3];
			const PxVec3 e1 = triVerts[Ps::getNextIndex3(i)];
			const PxVec3 Edge = e0 - e1;

			PxVec3 Cross = CapsuleAxis.cross(Edge);
			if(!isAlmostZero(Cross))
			{
				Cross = Cross.getNormalized();
				PxReal d;
				if(!PxcTestAxis(Cross, segment, radius, triVerts, d))
					return false;
				if(d<PenDepth)
				{
					PenDepth = d;
					Sep = Cross;
				}
			}
		}
	}

	const PxVec3 CapsuleCenter = segment.computeCenter();
	const PxVec3 TriCenter = PxcComputeTriangleCenter(triVerts);
	const PxVec3 Witness = CapsuleCenter - TriCenter;

	if(!(Sep.dot(Witness) >= 0))	Sep = -Sep;

	if(t)	*t = PenDepth;
	if(pp)	*pp = Sep;

	return true;
}

static void PxcGenerateVFContacts(	const Cm::Matrix34& meshAbsPose,
									ContactBuffer& contactBuffer,
									const Gu::Segment& segment, 
									const PxReal radius, 
									const PxVec3* PX_RESTRICT triVerts, 
									const PxVec3& normal, 
									PxU32 triangleIndex,
									PxReal contactDistance
									)
{
	const PxVec3* PX_RESTRICT Ptr = &segment.p0;
	for(PxU32 i=0;i<2;i++)
	{
		const PxVec3& Pos = Ptr[i];
		PxReal t,u,v;
		if(Gu::intersectLineTriangleCulling(Pos, -normal, triVerts[0], triVerts[1], triVerts[2], t, u, v) && t < radius + contactDistance)
		{
			const PxVec3 Hit = meshAbsPose.transform(Pos - t * normal);
			const PxVec3 wn = meshAbsPose.rotate(normal);
			
			contactBuffer.contact(Hit, wn, t-radius, PXC_CONTACT_NO_FACE_INDEX, triangleIndex);
			#if DEBUG_RENDER_MESHCONTACTS
			PxScene *s; PxGetPhysics().getScenes(&s, 1, 0);
			Cm::RenderOutput((Cm::RenderBuffer&)s->getRenderBuffer()) << Cm::RenderOutput::LINES << PxDebugColor::eARGB_BLUE // red
				<< Hit << (Hit + wn * 10.0f);
			#endif
		}
	}
}

//AM: This funct is almost exactly the same as PxcGenerateEEContacts2(), plus its rarely used. I think its not worth having a special case.
// PT: errr.... this "special case" is mandatory. The "normal" version uses a segment-triangle distance function, which breaks when the segment
// intersects the triangle, in which case you need to switch to a penetration-depth computation. If you don't do this thin capsules don't work.

static void PxcGenerateEEContacts(	const Cm::Matrix34& meshAbsPose,
									ContactBuffer& contactBuffer,
									const Gu::Segment& segment, 
									const PxReal radius, 
									const PxVec3* PX_RESTRICT triVerts, 
									const PxVec3& normal, 
									PxU32 triangleIndex)
{
	PxVec3 s0 = segment.p0;
	PxVec3 s1 = segment.p1;
	Ps::makeFatEdge(s0, s1, fatBoxEdgeCoeff);

	for(PxU32 i=0;i<3;i++)
	{
		PxReal dist;
		PxVec3 ip;
		if(Gu::intersectEdgeEdge(triVerts[i], triVerts[Ps::getNextIndex3(i)], -normal, s0, s1, dist, ip))
		{
			ip = meshAbsPose.transform(ip);
			const PxVec3 wn = meshAbsPose.rotate(normal);

			contactBuffer.contact(ip, wn, - (radius + dist), PXC_CONTACT_NO_FACE_INDEX, triangleIndex);
			#if DEBUG_RENDER_MESHCONTACTS
			PxScene *s; PxGetPhysics().getScenes(&s, 1, 0);
			Cm::RenderOutput((Cm::RenderBuffer&)s->getRenderBuffer()) << Cm::RenderOutput::LINES << PxDebugColor::eARGB_BLUE // red
				<< ip << (ip + wn * 10.0f);
			#endif
		}
	}
}



static void PxcGenerateEEContacts2(const Cm::Matrix34& meshAbsPose, 
									ContactBuffer& contactBuffer,
									const Gu::Segment& segment, 
									const PxReal radius, 
									const PxVec3* PX_RESTRICT triVerts, 
									const PxVec3& normal, 
									PxU32 triangleIndex,
									PxReal contactDistance)
{
	PxVec3 s0 = segment.p0;
	PxVec3 s1 = segment.p1;
	Ps::makeFatEdge(s0, s1, fatBoxEdgeCoeff);

	for(PxU32 i=0;i<3;i++)
	{
		PxReal dist;
		PxVec3 ip;
		if(Gu::intersectEdgeEdge(triVerts[i], triVerts[Ps::getNextIndex3(i)], normal, s0, s1, dist, ip) && dist < radius+contactDistance)
		{
			ip = meshAbsPose.transform(ip);
			const PxVec3 wn = meshAbsPose.rotate(normal);

			contactBuffer.contact(ip, wn, dist - radius, PXC_CONTACT_NO_FACE_INDEX, triangleIndex);
			#if DEBUG_RENDER_MESHCONTACTS
			PxScene *s; PxGetPhysics().getScenes(&s, 1, 0);
			Cm::RenderOutput((Cm::RenderBuffer&)s->getRenderBuffer()) << Cm::RenderOutput::LINES << PxDebugColor::eARGB_BLUE // red
				<< ip << (ip + wn * 10.0f);
			#endif
		}
	}
}


static void PxcContactCapsuleTriangle(	PxU8 data,
										const Cm::Matrix34& meshAbsPose,
										const PxVec3* PX_RESTRICT triVerts, 
										const Gu::Segment& segment, 
										PxReal capsuleRadius, 
										ContactBuffer& contactBuffer,
										PxU32 triangleIndex,
										PxReal contactDistance)
{

	const PxVec3& p0 = triVerts[0];
	const PxVec3& p1 = triVerts[1];
	const PxVec3& p2 = triVerts[2];

	PxReal t,u,v;
	const PxReal squareDist = Gu::distanceSegmentTriangleSquared(segment, p0, p1-p0, p2-p0, &t, &u, &v);

	const PxReal inflatedRadius = capsuleRadius + contactDistance;

	// PT: do cheaper test first!
	if(squareDist >= inflatedRadius*inflatedRadius) 
		return;

	// Cull away back-facing triangles   
	const PxPlane localPlane(p0, p1, p2);
	if(localPlane.distance(segment.computeCenter()) < 0.0f)
		return;

	if (squareDist > 0.001f)
	{
		// Contact information
		PxVec3 normal;
		if(selectNormal(data, u, v))
		{
			normal = localPlane.n;
		}
		else
		{
			PxVec3 pointOnTriangle = Ps::computeBarycentricPoint(p0, p1, p2, u, v);

			const PxVec3 pointOnSegment = segment.getPointAt(t);
			normal = pointOnSegment - pointOnTriangle;
			PxReal l = normal.magnitude();
			if(l == 0) 
				return;
			normal = normal / l;
		}

		PxcGenerateEEContacts2(meshAbsPose, contactBuffer, segment, capsuleRadius, triVerts, normal, triangleIndex, contactDistance);
		PxcGenerateVFContacts(meshAbsPose, contactBuffer, segment, capsuleRadius, triVerts, normal, triangleIndex, contactDistance);
	}
	else
	{
		PxVec3 SepAxis;
		if(!PxcCapsuleTriOverlap3(data, segment, inflatedRadius, triVerts, NULL, &SepAxis)) 
			return;

		PxcGenerateEEContacts(meshAbsPose, contactBuffer, segment, capsuleRadius, triVerts, SepAxis, triangleIndex);
		PxcGenerateVFContacts(meshAbsPose, contactBuffer, segment, capsuleRadius, triVerts, SepAxis, triangleIndex, contactDistance);
	}
}

struct ContactCapsuleMesh
{
	ContactBuffer&						contactBuffer;
	const PxTransform&					transform1;
	const Gu::Segment&					meshCapsule;
	PxReal								inflatedRadius;
	PxReal								contactDistance;
	PxReal								shapeCapsuleRadius;
	//Gu::Capsule							cacheCapsule;

	ContactCapsuleMesh(
		ContactBuffer& contactBuffer,
		const PxTransform& transform1, const Gu::Segment& meshCapsule,
		PxReal inflatedRadius, PxReal contactDistance,
		PxReal shapeCapsuleRadius
	) :
		contactBuffer(contactBuffer),
		transform1(transform1),
		meshCapsule(meshCapsule), inflatedRadius(inflatedRadius),
		contactDistance(contactDistance),
		shapeCapsuleRadius(shapeCapsuleRadius)
	{
		PX_ASSERT(contactBuffer.count==0);
	}

	template <PxU32 CacheSize>
	bool processTriangleCache(Gu::TriangleCache<CacheSize>& cache)
	{
		const Cm::Matrix34 meshAbsPose = Cm::Matrix34(transform1);

		#ifdef USE_AABB_TRI_CULLING
		PxVec3 bc = (meshCapsule.p0 + meshCapsule.p1)*0.5f;
		PxVec3 be = (meshCapsule.p0 - meshCapsule.p1)*0.5f;
		be.x = fabsf(be.x) + inflatedRadius;
		be.y = fabsf(be.y) + inflatedRadius;
		be.z = fabsf(be.z) + inflatedRadius;

		#if VISUALIZE_CULLING_BOX
		{
			Cm::RenderOutput& out = context.mRenderOutput;
			PxTransform idt = PxTransform(PxIdentity);
			out << idt;
			out << 0xffffffff;
			out << Cm::DebugBox(bc, be, true);
		}
		#endif
		#endif
		#ifdef USE_CAPSULE_TRI_PROJ_CULLING
		PxVec3 capsuleCenter = (meshCapsule.p0 + meshCapsule.p1)*0.5f;
		#endif

		#ifdef PRINT_STATS
		static int nb0=0;
		static int nb1=0;
		#endif

		PxVec3* verts = cache.mVertices;
		PxU32* triInds = cache.mTriangleIndex;
		//PxU32* triVertInds = cache.mIndices;
		PxU8* edgeFlags = cache.mEdgeFlags;

		PxU32 numTrigs = cache.mNumTriangles;

		while(numTrigs--)
		{
			const PxU32 triangleIndex = *triInds++;
			PxVec3 triVerts[3];
			
			triVerts[0] = verts[0];
			triVerts[1] = verts[1];
			triVerts[2] = verts[2];
			
			verts += 3;

			PxU8 extraData = *edgeFlags++;

			#ifdef PRINT_STATS
			nb0++;
			#endif


			#ifdef USE_AABB_TRI_CULLING
			if(!Gu::intersectTriangleBox(bc, be, triVerts[0], triVerts[1], triVerts[2]))
				continue;
			#endif


			#ifdef USE_CAPSULE_TRI_PROJ_CULLING
			PxVec3 triCenter = (triVerts[0] + triVerts[1] + triVerts[2])*0.33333333f;
			PxVec3 delta = capsuleCenter - triCenter;

			PxReal depth;
			if(!PxcTestAxis(delta, meshCapsule, inflatedRadius, triVerts, depth))
				continue;
			#endif

			#if VISUALIZE_TOUCHED_TRIS
			gVisualizeTri(triVerts[0], triVerts[1], triVerts[2], context, PxDebugColor::eARGB_RED);
			#endif

			#ifdef USE_CAPSULE_TRI_SAT_CULLING
			PxVec3 SepAxis;
			if(!PxcCapsuleTriOverlap3(extraData, meshCapsule, inflatedRadius, triVerts, NULL, &SepAxis)) 
				continue;
			#endif

			//PxU32 oldCount = contactBuffer.count;
			PxcContactCapsuleTriangle(
				extraData, meshAbsPose, triVerts, meshCapsule, shapeCapsuleRadius,
				contactBuffer, triangleIndex, contactDistance);

			#ifdef PRINT_STATS
			nb1++;
			printf("%f\n", float(nb1)*100.0f / float(nb0));
			#endif
		}

		return true;
	}

private:
	ContactCapsuleMesh& operator=(const ContactCapsuleMesh&);
};

struct CapsuleHeightfieldContactGenerationCallback : Gu::EntityReport<PxU32>
{
	ContactCapsuleMesh mGeneration;
	Gu::HeightFieldUtil& mHfUtil;

	CapsuleHeightfieldContactGenerationCallback(
		ContactBuffer& contactBuffer,
		const PxTransform& transform1, Gu::HeightFieldUtil& hfUtil, const Gu::Segment& meshCapsule,
		PxReal inflatedRadius, PxReal contactDistance, PxReal shapeCapsuleRadius
	) : 
		mGeneration(contactBuffer, transform1, meshCapsule, inflatedRadius, contactDistance, shapeCapsuleRadius),
		mHfUtil(hfUtil)
	{
		PX_ASSERT(contactBuffer.count==0);
	}



	virtual bool onEvent(PxU32 nb, PxU32* indices)
	{
		const PxU32 CacheSize = 16;
		Gu::TriangleCache<CacheSize> cache;

		PxU32 nbPasses = (nb+(CacheSize-1))/CacheSize;
		PxU32 nbTrigs = nb;
		PxU32* inds = indices;

		PxU8 nextInd[] = {2,0,1};

		for(PxU32 a = 0; a < nbPasses; ++a)
		{
			cache.mNumTriangles = 0;
			PxU32 trigCount = PxMin(nbTrigs, CacheSize);
			nbTrigs -= trigCount;
			while(trigCount--)
			{
				PxU32 triangleIndex = *(inds++);
				PxU32 vertIndices[3];

				PxTriangle currentTriangle;	// in world space

				PxU32 adjInds[3];
				mHfUtil.getTriangle(mGeneration.transform1, currentTriangle, vertIndices, adjInds, triangleIndex, false, false);

				PxVec3 normal;
				currentTriangle.normal(normal);

				PxU8 triFlags = 0; //KS - temporary until we can calculate triFlags for HF

				for(PxU32 a = 0; a < 3; ++a)
				{
					if(adjInds[a] != 0xFFFFFFFF)
					{
						PxTriangle adjTri;
						mHfUtil.getTriangle(mGeneration.transform1, adjTri, NULL, NULL, adjInds[a], false, false);
						//We now compare the triangles to see if this edge is active

						PxVec3 adjNormal;
						adjTri.denormalizedNormal(adjNormal);
						PxU32 otherIndex = nextInd[a];
						PxF32 projD = adjNormal.dot(currentTriangle.verts[otherIndex] - adjTri.verts[0]);
						if(projD < 0.f)
						{
							adjNormal.normalize();

							PxF32 proj = adjNormal.dot(normal);

							if(proj < 0.999f)
							{
								triFlags |= 1 << (a+3);
							}
						}
					}
					else
					{
						triFlags |= 1 << (a+3);
					}
				}

				cache.addTriangle(currentTriangle.verts, vertIndices, triangleIndex, triFlags);
			}
			mGeneration.processTriangleCache<CacheSize>(cache);
		}
		return true;
	}

private:
	CapsuleHeightfieldContactGenerationCallback& operator=(const CapsuleHeightfieldContactGenerationCallback&);
};





struct ContactCapsuleMeshCallback : Gu::VolumeColliderTrigCallback
{
	ContactCapsuleMesh					mGeneration;
	const PxTriangleMeshGeometryLL&		shapeMesh;
	const Cm::FastVertex2ShapeScaling&	scaling;
	bool								idtMeshScale;
	const Gu::InternalTriangleMeshData* meshData;

	ContactCapsuleMeshCallback(
		ContactBuffer& contactBuffer,
		const PxTransform& transform1, const PxTriangleMeshGeometryLL& shapeMesh, const Gu::Segment& meshCapsule,
		PxReal inflatedRadius, const Cm::FastVertex2ShapeScaling& scaling, PxReal contactDistance,
		PxReal shapeCapsuleRadius, const Gu::InternalTriangleMeshData* meshData, bool idtMeshScale
	) : 
		mGeneration(contactBuffer, transform1, meshCapsule, inflatedRadius, contactDistance, shapeCapsuleRadius),
		shapeMesh(shapeMesh),
		scaling(scaling), idtMeshScale(idtMeshScale),
		meshData(meshData)
	{
		PX_ASSERT(contactBuffer.count==0);
	}

	virtual ~ContactCapsuleMeshCallback() {}

	virtual bool processResults(PxU32 numTrigs, const PxVec3* trigVertTriples, const PxU32* trigIndices, const PxU32* trigVertexIndexTriple)
	{
		#if 0
		printf("nt = %d\n", numTrigs);
		for (PxU32 i = 0; i < numTrigs*3; i ++)
			printf(" %.2f %.2f %2f--", trigVertTriples[i].x, trigVertTriples[i].y, trigVertTriples[i].z);
		printf("inds = ", numTrigs);
		for (PxU32 i = 0; i < numTrigs; i ++)
			printf(" %d; ", trigIndices[i]);
		printf("\n");
		#endif

		const PxU32 CacheSize = 16;
		Gu::TriangleCache<CacheSize> cache;

		PxU32 nbPasses = (numTrigs + CacheSize-1)/CacheSize;


		for(PxU32 a = 0; a < nbPasses; ++a)
		{
			cache.mNumTriangles = 0;
			PxU32 trigCount = PxMin(numTrigs, CacheSize);
			numTrigs -= trigCount;
			while(trigCount--)
			{
				const PxU32 triangleIndex = *trigIndices++;
				PxVec3 triVerts[3];
				if(idtMeshScale)
				{
					triVerts[0] = trigVertTriples[0];
					triVerts[1] = trigVertTriples[1];
					triVerts[2] = trigVertTriples[2];
				}
				else
				{
					triVerts[0] = scaling * trigVertTriples[0];
					triVerts[1] = scaling * trigVertTriples[1];
					triVerts[2] = scaling * trigVertTriples[2];
				}
				trigVertTriples += 3;

				//ML::set all the edges to be active, if the mExtraTrigData exist, we overwrite this flag
				PxU8 extraData = Gu::ETD_CONVEX_EDGE_01|Gu::ETD_CONVEX_EDGE_12|Gu::ETD_CONVEX_EDGE_20;
				//PxU8 extraData = 0;
				if (meshData->mExtraTrigData)
					extraData = Cm::memFetch<PxU8>(
						Cm::MemFetchPtr(meshData->mExtraTrigData)+triangleIndex*sizeof(meshData->mExtraTrigData[0]), 5);
				Cm::memFetchWait(5);

				cache.addTriangle(triVerts, trigVertexIndexTriple, triangleIndex, extraData);
				trigVertexIndexTriple += 3;
			}
			mGeneration.processTriangleCache(cache);
		}

		return true;
	}

private:
	ContactCapsuleMeshCallback& operator=(const ContactCapsuleMeshCallback&);
};

namespace physx
{
bool Gu::contactCapsuleMesh(GU_CONTACT_METHOD_ARGS)
{
	PX_UNUSED(cache);

	// Get actual shape data
	const PxCapsuleGeometry& shapeCapsule = shape0.get<const PxCapsuleGeometry>();
	const PxTriangleMeshGeometryLL& shapeMesh = shape1.get<const PxTriangleMeshGeometryLL>();

	const bool idtMeshScale = shapeMesh.scale.isIdentity();

	Cm::FastVertex2ShapeScaling meshScaling;	// PT: TODO: remove default ctor
	if(!idtMeshScale)
		meshScaling.init(shapeMesh.scale);

	// Capsule data
	Gu::Segment worldCapsule;
	getCapsuleSegment(transform0, shapeCapsule, worldCapsule);
	const PxReal inflatedRadius = shapeCapsule.radius + contactDistance;		//AM: inflate!

	//to avoid transforming all the trig vertices to world space, transform the capsule to the mesh's space instead
	//we use Opcode's always single precision math below to avoid having to do a conversion on all vertices.
	const Gu::Segment meshCapsule(	// Capsule in mesh space
		transform1.transformInv(worldCapsule.p0),
		transform1.transformInv(worldCapsule.p1));

	// We must be in local space to use the cache
	Gu::Capsule queryCapsule(meshCapsule, inflatedRadius);

	const Gu::InternalTriangleMeshData* meshData = shapeMesh.meshData;
#ifdef __SPU__
	// fetch meshData to temp storage
	PX_ALIGN_PREFIX(16) char meshDataBuf[sizeof(Gu::InternalTriangleMeshData)] PX_ALIGN_SUFFIX(16);
	Cm::memFetchAlignedAsync(Cm::MemFetchPtr(meshDataBuf), Cm::MemFetchPtr(shapeMesh.meshData), sizeof(Gu::InternalTriangleMeshData), 5);
	Cm::memFetchWait(5);
	meshData = reinterpret_cast<const Gu::InternalTriangleMeshData*>(meshDataBuf);
#endif
	ContactCapsuleMeshCallback callback(
		contactBuffer, transform1, shapeMesh, meshCapsule,
		inflatedRadius, meshScaling, contactDistance, shapeCapsule.radius, meshData, idtMeshScale);

	//switched from capsuleCollider to boxCollider so we can support nonuniformly scaled meshes by scaling the query region:
	//callback.mGeneration.cacheCapsule = queryCapsule;

	//bound the capsule in shape space by an OBB:
	Gu::Box queryBox;
	queryBox.create(queryCapsule);

	//apply the skew transform to the box:
	if(!idtMeshScale)
		meshScaling.transformQueryBounds(queryBox.center, queryBox.extents, queryBox.rot);

	Gu::RTreeMidphaseData hmd;	// PT: I suppose doing the "conversion" at runtime is fine
	meshData->mOpcodeModel.getRTreeMidphaseData(hmd);

	Gu::HybridOBBCollider collider;
	collider.SetPrimitiveTests(false);
	// TODO: loose primitive tests are probably a gain on PC. assess this and enable for PC.
#ifdef PX_VMX
	collider.SetLoosePrimitiveTests(true);
#endif

	collider.Collide<0,1,0>(queryBox, hmd, &callback);

	return (contactBuffer.count > 0);
}


bool Gu::contactCapsuleHeightfield(GU_CONTACT_METHOD_ARGS)
{
	PX_UNUSED(cache);

	// Get actual shape data
	const PxCapsuleGeometry& shapeCapsule = shape0.get<const PxCapsuleGeometry>();
	const PxHeightFieldGeometryLL& shapeMesh = shape1.get<const PxHeightFieldGeometryLL>();

	// Capsule data
	Gu::Segment worldCapsule;
	getCapsuleSegment(transform0, shapeCapsule, worldCapsule);
	const PxReal inflatedRadius = shapeCapsule.radius + contactDistance;		//AM: inflate!

	//to avoid transforming all the trig vertices to world space, transform the capsule to the mesh's space instead
	//we use Opcode's always single precision math below to avoid having to do a conversion on all vertices.
	const Gu::Segment meshCapsule(	// Capsule in mesh space
		transform1.transformInv(worldCapsule.p0),
		transform1.transformInv(worldCapsule.p1));

	// We must be in local space to use the cache
	//Gu::Capsule queryCapsule(meshCapsule, inflatedRadius);

#ifdef __SPU__
		const Gu::HeightField& hf = *Cm::memFetchAsync<const Gu::HeightField>(HeightFieldBuffer, Cm::MemFetchPtr(static_cast<Gu::HeightField*>(shapeMesh.heightField)), sizeof(Gu::HeightField), 1);
		Cm::memFetchWait(1);
#if HF_TILED_MEMORY_LAYOUT
		g_sampleCache.init((uintptr_t)(hf.getData().samples), hf.getData().tilesU);
#endif
#else
	const Gu::HeightField& hf = *static_cast<Gu::HeightField*>(shapeMesh.heightField);
#endif
	Gu::HeightFieldUtil hfUtil(shapeMesh, hf);

	//Gu::HeightFieldUtil hfUtil((PxHeightFieldGeometry&)shapeMesh);

	CapsuleHeightfieldContactGenerationCallback callback(
		contactBuffer, transform1, hfUtil, meshCapsule, inflatedRadius, contactDistance, shapeCapsule.radius);

	//switched from capsuleCollider to boxCollider so we can support nonuniformly scaled meshes by scaling the query region:
	//callback.mGeneration.cacheCapsule = queryCapsule;

	//bound the capsule in shape space by an OBB:

	PxBounds3 bounds;
	bounds.maximum = PxVec3(shapeCapsule.halfHeight + inflatedRadius, inflatedRadius, inflatedRadius);
	bounds.minimum = -bounds.maximum;

	bounds = PxBounds3::transformFast(transform1.transformInv(transform0), bounds);

	hfUtil.overlapAABBTriangles(transform1, bounds, 0, &callback);

	return (contactBuffer.count > 0);
}


}