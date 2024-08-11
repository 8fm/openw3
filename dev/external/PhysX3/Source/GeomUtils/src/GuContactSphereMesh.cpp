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

#include "GuDistancePointTriangle.h"
#include "GuIntersectionRayTriangle.h"
#include "GuTriangleVertexPointers.h"
#include "GuContactMethodImpl.h"
#include "GuContactBuffer.h"
#include "GuGeometryUnion.h"
#include "GuFeatureCode.h"
#include "OPC_VolumeCollider.h"
#include "OPC_AABBCollider.h"
#include "CmScaling.h"
#include "PxSphereGeometry.h"
#include "GuTriangleCache.h"
#include "GuEntityReport.h"
#include "GuHeightFieldUtil.h"
#include "GuConvexEdgeFlags.h"

using namespace physx;
using namespace Gu;

#ifdef __SPU__
extern unsigned char HeightFieldBuffer[sizeof(Gu::HeightField)+16];
#include "CmMemFetch.h"
#endif

//#define USE_SIMD_VERSION


#ifdef USE_SIMD_VERSION
	using namespace physx::shdfnd::aos;
#endif

#ifdef __SPU__
	namespace physx
	{
	extern bool gSphereVsMeshContactLimitExceeded;
	}
#endif

///////////////////////////////////////////////////////////////////////////////

	class TriangleIndices
	{
	public:
		PX_FORCE_INLINE TriangleIndices(const InternalTriangleMeshData& mesh, PxU32 triangleIndex)
		{
			PxU32 ref0, ref1, ref2;
			if(mesh.mFlags & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES)
			{
#ifdef __SPU__
				const IndexTriple16 indices16 = Cm::memFetch<IndexTriple16>(Cm::MemFetchPtr(mesh.mTriangles)+triangleIndex*sizeof(PxU16)*3, 5);
				ref0 = indices16.p0;
				ref1 = indices16.p1;
				ref2 = indices16.p2;
#else
				const TriangleT<PxU16>& indices = (reinterpret_cast<const TriangleT<PxU16>*>(mesh.mTriangles))[triangleIndex];
				ref0 = indices[0];
				ref1 = indices[1];
				ref2 = indices[2];
#endif
			}
			else
			{
#ifdef __SPU__
				const IndexTriple32 indices32 = Cm::memFetch<IndexTriple32>(Cm::MemFetchPtr(mesh.mTriangles)+triangleIndex*sizeof(PxU32)*3, 5);
				ref0 = indices32.p0;
				ref1 = indices32.p1;
				ref2 = indices32.p2;
#else
				const TriangleT<PxU32>& indices = (reinterpret_cast<const TriangleT<PxU32>*>(mesh.mTriangles))[triangleIndex];
				ref0 = indices[0];
				ref1 = indices[1];
				ref2 = indices[2];
#endif
			}

			mVRefs[0] = ref0;
			mVRefs[1] = ref1;
			mVRefs[2] = ref2;
		}

		PxU32	mVRefs[3];
	};

//static bool validateVertex(PxU32 vref, const PxU32 count, const Gu::ContactPoint* PX_RESTRICT contacts, const InternalTriangleMeshData& meshData)
//{
//	for(PxU32 i=0;i<count;i++)
//	{
//		const TriangleIndices T(meshData, contacts[i].internalFaceIndex1);
//		if(		T.mVRefs[0]==vref
//			||	T.mVRefs[1]==vref
//			||	T.mVRefs[2]==vref)
//			return false;
//	}
//	return true;
//}

//static bool validateEdge(PxU32 vref0, PxU32 vref1, const PxU32 count, const Gu::ContactPoint* PX_RESTRICT contacts, const InternalTriangleMeshData& meshData)
//{
//	for(PxU32 i=0;i<count;i++)
//	{
//		const TriangleIndices T(meshData, contacts[i].internalFaceIndex1);
//
//		if(T.mVRefs[0]==vref0 || T.mVRefs[0]==vref1)
//			return false;
//		if(T.mVRefs[1]==vref0 || T.mVRefs[1]==vref1)
//			return false;
//		if(T.mVRefs[2]==vref0 || T.mVRefs[2]==vref1)
//			return false;
///*		if((T.mVRefs[0]==vref0 && T.mVRefs[1]==vref1) || (T.mVRefs[0]==vref1 && T.mVRefs[1]==vref0))
//			return false;
//		if((T.mVRefs[1]==vref0 && T.mVRefs[2]==vref1) || (T.mVRefs[1]==vref1 && T.mVRefs[2]==vref0))
//			return false;
//		if((T.mVRefs[2]==vref0 && T.mVRefs[0]==vref1) || (T.mVRefs[2]==vref1 && T.mVRefs[0]==vref0))
//			return false;*/
//	}
//	return true;
//}


struct TriangleData
{
	PxVec3	verts[3];
	float	u, v;
	float	squareDist;
	PxU32	triangleIndex;
	PxU32	inds[3];
};

struct CachedTriangleIndices
{
	PxU32 triIndex;
	PxU32 vertInds[3];

	PxU32 getHashCode() const
	{
		return triIndex;
	}

	bool operator == (const CachedTriangleIndices& other) const
	{
		return triIndex == other.triIndex;
	}
};


namespace
{

struct SphereMeshContactGeneration
{
	const PxSphereGeometry&				mShapeSphere;
	const PxTransform&					mTransform0;
	const PxTransform&					mTransform1;
	ContactBuffer&						mContactBuffer;
	const PxVec3&						mSphereCenterShape1Space;
	PxF32								mInflatedRadius;
	PxU32								mNbDelayed;
	TriangleData						mSavedData[ContactBuffer::MAX_CONTACTS];		// PT: 3328 bytes
	Gu::CacheMap<CachedTriangleIndices, ContactBuffer::MAX_CONTACTS> mCachedTriangles;
	Gu::CacheMap<Gu::CachedVertex, ContactBuffer::MAX_CONTACTS> mCachedVertices;

	SphereMeshContactGeneration(
		const PxSphereGeometry&				shapeSphere,
		const PxTransform&					transform0,
		const PxTransform&					transform1,
		ContactBuffer&						contactBuffer,
		const PxVec3&						sphereCenterShape1Space,
		PxF32								inflatedRadius
	) :
		mShapeSphere				(shapeSphere),
		mTransform0					(transform0),
		mTransform1					(transform1),
		mContactBuffer				(contactBuffer),
		mSphereCenterShape1Space	(sphereCenterShape1Space),
		mInflatedRadius				(inflatedRadius),
		mNbDelayed					(0)
	{
	}

	virtual ~SphereMeshContactGeneration() {}

	bool validateEdge(PxU32 vref0, PxU32 vref1, const PxU32 count, const Gu::ContactPoint* PX_RESTRICT contacts,
						 const PxVec3& normal, const PxReal normalThresh);


	template <PxU32 CacheSize>
	bool processTriangleCache(Gu::TriangleCache<CacheSize>& cache)
	{
		const PxF32 r2 = mInflatedRadius * mInflatedRadius;
	
#ifdef USE_SIMD_VERSION
		const Ps::aos::FloatV r2V = FLoad(r2);
#endif

		PxVec3 v0, v1, v2;
#ifdef USE_SIMD_VERSION
		const BoolV bt = BTTTT();
		const Vec3V sphereCenterShape1SpaceV = V3LoadU(mSphereCenterShape1Space);
#endif

		PxU32 nbDelayed = mNbDelayed;

		PxU32* triIndex = cache.mTriangleIndex;
		PxVec3* verts = cache.mVertices;
		PxU32* vertIndex = cache.mIndices;
		//PxU8* edgeFlags = cache.mEdgeFlags;

		for(PxU32 a = 0; a < cache.mNumTriangles; ++a)
		{
			const PxU32 triangleIndex = *triIndex++;

			v0 = (*verts++);
			v1 = (*verts++);
			v2 = (*verts++);

			PxU32* vertInds = vertIndex;
			vertIndex +=3;


#ifdef USE_SIMD_VERSION
			const Vec3V v0V = V3LoadU(v0);
			const Vec3V v1V = V3LoadU(v1);
			const Vec3V v2V = V3LoadU(v2);
			Ps::aos::FloatV uV, vV;
			Ps::aos::Vec3V closestPV;
			const Ps::aos::FloatV squareDistV = distancePointTriangleSquared(sphereCenterShape1SpaceV, v0V, v1V, v2V, uV, vV, closestPV);
			BoolV resultV = FIsGrtrOrEq(r2V, squareDistV);
			if(BAllEq(resultV, bt))
#else
			PxReal uu, vv;
			const PxVec3 cp = Gu::closestPtPointTriangle(mSphereCenterShape1Space, v0, v1, v2, uu, vv);
			const PxReal squareDist = (cp - mSphereCenterShape1Space).magnitudeSquared();
			if(squareDist < r2)
#endif
			{
/*				// PT: default backface culling code
				// Backface culling
				const PxcPlane localPlane(v0, v1, v2);
				if(localPlane.signedDistanceHessianNormalForm(mSphereCenterShape1Space)<0.0f)
					continue;*/

				// PT: backface culling without the normalize
				const PxVec3 e0 = v1 - v0;
				const PxVec3 e1 = v2 - v0;
				const PxVec3 planeNormal = e0.cross(e1);
				const PxF32 planeD = planeNormal.dot(v0);	// PT: actually -d compared to PxcPlane

				// Backface culling
				if(planeNormal.dot(mSphereCenterShape1Space) < planeD)
					continue;

				const PxF32 enlarge = 0.0001f; // enlarge the triangle to prevent going through cracks in the mesh

				// PT: original condition:
				//		tn < mInflatedRadius		with tn = t from normalized normal
				// <=>
				//		t*m < mInflatedRadius		with m = magnitude(normal)
				//		t*t*m^2 < r^2				t can't be negative after backface culling
				const PxF32 m2 = planeNormal.magnitudeSquared();

				PxReal t,u,v;
				if(intersectLineTriangleCulling(mSphereCenterShape1Space, -planeNormal, v0, v1, v2, t, u, v, enlarge) && t*t*m2<r2)
				{
					const PxVec3 hit = mTransform1.transform(mSphereCenterShape1Space - t * planeNormal);
					const PxF32 m = PxSqrt(m2);
					const PxF32 oneOverM = 1.0f / m;
					const PxVec3 wn = mTransform1.rotate(planeNormal) * oneOverM;
//printf("Codepath0 | %f | %.02f | %.02f | %.02f\n", t - mShapeSphere.radius, wn.x, wn.y, wn.z);
					
					mContactBuffer.contact(hit, wn, t*m - mShapeSphere.radius, PXC_CONTACT_NO_FACE_INDEX, triangleIndex);

					CachedTriangleIndices inds;
					inds.triIndex = triangleIndex;
					inds.vertInds[0] = vertInds[0];
					inds.vertInds[1] = vertInds[1];
					inds.vertInds[2] = vertInds[2];
					mCachedTriangles.addData(inds);

					mCachedVertices.addData(vertInds[0]);
					mCachedVertices.addData(vertInds[1]);
					mCachedVertices.addData(vertInds[2]);

				}
				else
				{
					if(nbDelayed<ContactBuffer::MAX_CONTACTS)
					{
						TriangleData* PX_RESTRICT saved = mSavedData + nbDelayed++;
						saved->verts[0] = v0;
						saved->verts[1] = v1;
						saved->verts[2] = v2;
						saved->inds[0] = vertInds[0];
						saved->inds[1] = vertInds[1];
						saved->inds[2] = vertInds[2];
#ifdef USE_SIMD_VERSION
						FStore(uV, &saved->u);
						FStore(vV, &saved->v);
						FStore(squareDistV, &saved->squareDist);
#else
						saved->u = uu;
						saved->v = vv;
						saved->squareDist = squareDist;
#endif
						saved->triangleIndex = triangleIndex;
					}
#ifdef PX_CHECKED
					else
					{
						Ps::getFoundation().error(PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "Dropping contacts in sphere vs mesh: exceeded limit of 64 ");
					}
#elif defined(__SPU__)
					else
					{
						gSphereVsMeshContactLimitExceeded = true;
					}
#endif
				}
			}
		}

		mNbDelayed = nbDelayed;
		return true;
	}

	void generateLastContacts()
	{
		const PxU32 count = mNbDelayed;
		if(!count)
			return;

		const PxU32 ccount = mContactBuffer.count;
		const Gu::ContactPoint* PX_RESTRICT contacts = mContactBuffer.contacts;

		const TriangleData* PX_RESTRICT touchedTris = mSavedData;
		for(PxU32 i=0;i<count;i++)
		{
			const PxU32 triangleIndex = touchedTris[i].triangleIndex;

			// PT: TODO: remove DMA here

			const PxU32 ref0 = touchedTris[i].inds[0];
			const PxU32 ref1 = touchedTris[i].inds[1];
			const PxU32 ref2 = touchedTris[i].inds[2];

			// PT: TODO: why bother with the feature code at all? Use edge cache directly?
			const FeatureCode FC = computeFeatureCode(mSavedData[i].u, mSavedData[i].v);

			const PxVec3 e0 = touchedTris[i].verts[1] - touchedTris[i].verts[0];
			const PxVec3 e1 = touchedTris[i].verts[2] - touchedTris[i].verts[0];
			const PxVec3 planeNormal = e0.cross(e1).getNormalized();
			const PxReal normalThresh = 0.95f;


			// PT: TODO: remove DMA here
			bool generateContact = false;
			switch(FC)
			{
				case FC_VERTEX0:
					generateContact = !mCachedVertices.contains(ref0);
					break;
				case FC_VERTEX1:
					generateContact = !mCachedVertices.contains(ref1);
					break;
				case FC_VERTEX2:
					generateContact = !mCachedVertices.contains(ref2);
					break;
				case FC_EDGE01:
					generateContact = validateEdge(ref0, ref1, ccount, contacts,
						planeNormal, normalThresh );
					break;
				case FC_EDGE12:
					generateContact = validateEdge(ref1, ref2, ccount, contacts,
						planeNormal, normalThresh );
					break;
				case FC_EDGE20:
					generateContact = validateEdge(ref0, ref2, ccount, contacts,
						planeNormal, normalThresh );
					break;
				default:
					break;
			};

			if(!generateContact)
				continue;

			const PxReal squareDist = mSavedData[i].squareDist;
			const PxReal u = mSavedData[i].u;
			const PxReal v = mSavedData[i].v;
			const PxReal depth = PxSqrt(squareDist) - mShapeSphere.radius;

			const PxVec3 localPoint = Ps::computeBarycentricPoint(mSavedData[i].verts[0], mSavedData[i].verts[1], mSavedData[i].verts[2], u, v);
			const PxVec3 worldPoint = mTransform1.transform(localPoint);

			PxVec3 normal = (mTransform0.p - worldPoint);
			const PxReal magnitude = normal.magnitude();
			if(magnitude < 0.0001f)
			{
				// Compute triangle normal
				const PxPlane localPlane(mSavedData[i].verts[0], mSavedData[i].verts[1], mSavedData[i].verts[2]);
				normal = mTransform1.rotate(localPlane.n);
//printf("Codepath1\n");
			}
			else
			{
				normal *= 1.0f/magnitude;
//printf("Codepath2 | %f | %.02f | %.02f | %.02f\n", depth, normal.x, normal.y, normal.z);
			}
			mContactBuffer.contact(worldPoint, normal, depth, PXC_CONTACT_NO_FACE_INDEX, triangleIndex);

			//CachedTriangleIndices inds;
			//inds.triIndex = triangleIndex;
			//inds.vertInds[0] = ref0;
			//inds.vertInds[1] = ref1;
			//inds.vertInds[2] = ref2;
			//mCachedTriangles.addData(inds);

			//mCachedVertices.addData(ref0);
			//mCachedVertices.addData(ref1);
			//mCachedVertices.addData(ref2);
		}
	}

private:
	SphereMeshContactGeneration& operator=(const SphereMeshContactGeneration&);

};

bool SphereMeshContactGeneration::validateEdge(PxU32 vref0, PxU32 vref1, const PxU32 count, const Gu::ContactPoint* PX_RESTRICT contacts,
						 const PxVec3& normal, const PxReal normalThresh)
{
	CachedTriangleIndices ind;
	for(PxU32 i=0;i<count;i++)
	{
		ind.triIndex = contacts[i].internalFaceIndex1;

		const CachedTriangleIndices* inds = mCachedTriangles.get(ind);

			bool match = (inds->vertInds[0]==vref0 || inds->vertInds[0]==vref1) 
			|| (inds->vertInds[1]==vref0 || inds->vertInds[1]==vref1)
			|| (inds->vertInds[2]==vref0 || inds->vertInds[2]==vref1);
		
		if(match && PxAbs(normal.dot(contacts[i].normal) > normalThresh))
			return false;
	}
	return true;
}



struct SphereMeshContactGenerationCallback : VolumeColliderTrigCallback
{
	SphereMeshContactGeneration			mGeneration;
	const InternalTriangleMeshData&		mMeshData;
	const Cm::FastVertex2ShapeScaling&	mMeshScaling;
	bool								mIdtMeshScale;

	SphereMeshContactGenerationCallback(
		const InternalTriangleMeshData&		meshData,
		const PxSphereGeometry&				shapeSphere,
		const PxTransform&					transform0,
		const PxTransform&					transform1,
		const Cm::FastVertex2ShapeScaling&	meshScaling,
		ContactBuffer&						contactBuffer,
		const PxVec3&						sphereCenterShape1Space,
		PxF32								inflatedRadius,
		bool								idtMeshScale
	) :
		mGeneration(shapeSphere, transform0, transform1, contactBuffer, sphereCenterShape1Space, inflatedRadius),
		mMeshData					(meshData),
		mMeshScaling				(meshScaling),
		mIdtMeshScale				(idtMeshScale)
	{
	}

	virtual ~SphereMeshContactGenerationCallback() {}

	virtual bool processResults(PxU32 count, const PxVec3* trigVerts, const PxU32* indices, const PxU32* /*trigVertIndexTriples*/)
	{
		const PxU32 MaxTriangles = 16;
		Gu::TriangleCache<MaxTriangles> cache;

		PxU32 nbPasses = (count + MaxTriangles-1)/MaxTriangles;

		for(PxU32 a = 0; a < nbPasses; ++a)
		{
			cache.mNumTriangles = 0;
			PxU32 trigCount = PxMin(count, MaxTriangles);
			count -= trigCount;
			while(trigCount--)
			{
				const PxU32 triangleIndex = *indices++;
				PxVec3 v[3];

				if(mIdtMeshScale)
				{
					v[0] = (*trigVerts++);
					v[1] = (*trigVerts++);
					v[2] = (*trigVerts++);
				}
				else
				{
					v[0] = mMeshScaling * (*trigVerts++);
					v[1] = mMeshScaling * (*trigVerts++);
					v[2] = mMeshScaling * (*trigVerts++);
				}

				const TriangleIndices T(mMeshData, triangleIndex);

#ifdef __SPU__
				PxU8 triFlags = Gu::ETD_CONVEX_EDGE_01|Gu::ETD_CONVEX_EDGE_12|Gu::ETD_CONVEX_EDGE_20;
#else
				PxU8 triFlags = mMeshData.mExtraTrigData[triangleIndex];
#endif

				cache.addTriangle(v, T.mVRefs, triangleIndex, triFlags);
			}
			mGeneration.processTriangleCache(cache);
		}

		return true;
	}

	void generateLastContacts()
	{
		mGeneration.generateLastContacts();
	}
};

struct SphereHeightfieldContactGenerationCallback : Gu::EntityReport<PxU32>
{
	SphereMeshContactGeneration mGeneration;
	Gu::HeightFieldUtil& mHfUtil;

	SphereHeightfieldContactGenerationCallback(
		Gu::HeightFieldUtil& hfUtil,
		const PxSphereGeometry&				shapeSphere,
		const PxTransform&					transform0,
		const PxTransform&					transform1,
		ContactBuffer&						contactBuffer,
		const PxVec3&						sphereCenterShape1Space,
		PxF32								inflatedRadius
	) :
		mGeneration(shapeSphere, transform0, transform1, contactBuffer, sphereCenterShape1Space, inflatedRadius),
		mHfUtil					(hfUtil)
	{
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
				PxVec3 normal;
				currentTriangle.normal(normal);

				PxU32 adjInds[3];
				mHfUtil.getTriangle(mGeneration.mTransform1, currentTriangle, vertIndices, adjInds, triangleIndex, false, false);

				PxU8 triFlags = 0; //KS - temporary until we can calculate triFlags for HF

				for(PxU32 a = 0; a < 3; ++a)
				{
					if(adjInds[a] != 0xFFFFFFFF)
					{
						PxTriangle adjTri;
						mHfUtil.getTriangle(mGeneration.mTransform1, adjTri, NULL, NULL, adjInds[a], false, false);
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
};


}

namespace physx
{
bool Gu::contactSphereMesh(GU_CONTACT_METHOD_ARGS)
{
	PX_UNUSED(cache);

	const PxSphereGeometry& shapeSphere = shape0.get<const PxSphereGeometry>();
	const PxTriangleMeshGeometryLL& shapeMesh = shape1.get<const PxTriangleMeshGeometryLL>();

#ifdef __SPU__
	// PT: TODO: cache this one
	// fetch meshData to temp buffer
	PX_ALIGN_PREFIX(16) char meshDataBuf[sizeof(InternalTriangleMeshData)] PX_ALIGN_SUFFIX(16);
	Cm::memFetchAlignedAsync(PxU64(meshDataBuf), PxU64(shapeMesh.meshData), sizeof(InternalTriangleMeshData), 5);
#endif

	Cm::FastVertex2ShapeScaling meshScaling;	// PT: TODO: get rid of default ctor :(
	const bool idtMeshScale = shapeMesh.scale.isIdentity();
	if(!idtMeshScale)
		meshScaling.init(shapeMesh.scale);

	// We must be in local space to use the cache
	const PxVec3 sphereCenterShape1Space = transform1.transformInv(transform0.p);
	PxReal inflatedRadius = shapeSphere.radius + contactDistance;

#ifdef __SPU__
	Cm::memFetchWait(5);
	const InternalTriangleMeshData* meshData = reinterpret_cast<const InternalTriangleMeshData*>(meshDataBuf);
#else
	const InternalTriangleMeshData* meshData = shapeMesh.meshData;
#endif

	// mesh scale is not baked into cached verts
	SphereMeshContactGenerationCallback callback(
		*meshData,
		shapeSphere,
		transform0, transform1,
		meshScaling,
		contactBuffer,
		sphereCenterShape1Space,
		inflatedRadius, idtMeshScale);

	//switched from sphereCollider to boxCollider so we can support nonuniformly scaled meshes by scaling the query region:
	Gu::CollisionAABB queryBox;
	queryBox.mCenter = sphereCenterShape1Space;
	queryBox.mExtents = PxVec3(inflatedRadius);
	if(!idtMeshScale)
		meshScaling.transformQueryBounds(queryBox.mCenter, queryBox.mExtents);

	Gu::RTreeMidphaseData hmd;
	meshData->mOpcodeModel.getRTreeMidphaseData(hmd);

	Gu::HybridAABBCollider aabbCollider;
	aabbCollider.SetPrimitiveTests(false);
	aabbCollider.Collide(queryBox, hmd, false, &callback);

	callback.generateLastContacts();

	return (contactBuffer.count > 0);
}

bool Gu::contactSphereHeightField(GU_CONTACT_METHOD_ARGS)
{
	PX_UNUSED(cache);

	const PxSphereGeometry& shapeSphere = shape0.get<const PxSphereGeometry>();
	const PxHeightFieldGeometryLL& shapeMesh = shape1.get<const PxHeightFieldGeometryLL>();

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

	//Gu::HeightFieldUtil hfUtil((const PxHeightFieldGeometry&)shapeMesh);

	const PxVec3 sphereCenterShape1Space = transform1.transformInv(transform0.p);
	PxReal inflatedRadius = shapeSphere.radius + contactDistance;
	PxVec3 inflatedRV3(inflatedRadius);

	PxBounds3 bounds(sphereCenterShape1Space - inflatedRV3, sphereCenterShape1Space + inflatedRV3);

	SphereHeightfieldContactGenerationCallback blockCallback(hfUtil, shapeSphere, transform0, transform1, contactBuffer, sphereCenterShape1Space, inflatedRadius);

	hfUtil.overlapAABBTriangles(transform1, bounds, 0, &blockCallback);

	blockCallback.mGeneration.generateLastContacts();

	return (contactBuffer.count > 0);
}

}