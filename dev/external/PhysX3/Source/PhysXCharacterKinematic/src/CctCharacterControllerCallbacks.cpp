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

#include "CctInternalStructs.h"
#include "PxScene.h"
#include "PxSphereGeometry.h"
#include "PxCapsuleGeometry.h"
#include "PxBoxGeometry.h"
#include "PxConvexMesh.h"
#include "PxMeshQuery.h"
#include "PxExtensionsAPI.h"
#include "PxTriangleMeshGeometry.h"
#include "PxConvexMeshGeometry.h"
#include "PxHeightFieldGeometry.h"
#include "CmRenderOutput.h"
#include "GuIntersectionTriangleBox.h"
#include "PsMathUtils.h"
//#include <windows.h>

static const bool gVisualizeTouchedTris = false;
static const float gDebugVisOffset = 0.01f;

using namespace physx;
using namespace Cct;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// PT: HINT: disable gUsePartialUpdates for easier visualization
static void visualizeTouchedTriangles(PxU32 nbTrisToRender, PxU32 startIndex, const PxTriangle* triangles, Cm::RenderBuffer* renderBuffer, const PxVec3& offset, const PxVec3& upDirection)
{
	if(!renderBuffer)
		return;

	PxVec3 yy = -offset;
	yy += upDirection * gDebugVisOffset;	// PT: move slightly in the up direction

	for(PxU32 i=0; i<nbTrisToRender; i++)
	{
		const PxTriangle& currentTriangle = triangles[i+startIndex];

//		Cm::RenderOutput(*renderBuffer)
//			<< PxDebugColor::eARGB_GREEN << Cm::RenderOutput::TRIANGLES
//			<< currentTriangle.verts[0]+yy << currentTriangle.verts[1]+yy << currentTriangle.verts[2]+yy;
		Cm::RenderOutput(*renderBuffer)
			<< PxU32(PxDebugColor::eARGB_GREEN) << Cm::RenderOutput::LINES
			<< currentTriangle.verts[0]+yy << currentTriangle.verts[1]+yy
			<< currentTriangle.verts[1]+yy << currentTriangle.verts[2]+yy
			<< currentTriangle.verts[2]+yy << currentTriangle.verts[0]+yy;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void tessellateTriangle(PxU32& nbNewTris, const PxTriangle& tr, PxU32 index, TriArray& worldTriangles, IntArray& triIndicesArray, const PxBounds3& cullingBox, const CCTParams& params)
{
	{
		const PxVec3 boxCenter = cullingBox.getCenter();
		const PxVec3 boxExtents = cullingBox.getExtents();
		if(!Gu::intersectTriangleBox(boxCenter, boxExtents, tr.verts[0], tr.verts[1], tr.verts[2]))
			return;
	}

	PxU32 code;
	{
		const PxVec3 edge0 = tr.verts[0] - tr.verts[1];
		const PxVec3 edge1 = tr.verts[1] - tr.verts[2];
		const PxVec3 edge2 = tr.verts[2] - tr.verts[0];
		const bool split0 = edge0.magnitudeSquared()>params.mMaxEdgeLength2;
		const bool split1 = edge1.magnitudeSquared()>params.mMaxEdgeLength2;
		const bool split2 = edge2.magnitudeSquared()>params.mMaxEdgeLength2;
		code = (PxU32(split2)<<2)|(PxU32(split1)<<1)|PxU32(split0);
	}

	const PxVec3 m0 = (tr.verts[0] + tr.verts[1])*0.5f;
	const PxVec3 m1 = (tr.verts[1] + tr.verts[2])*0.5f;
	const PxVec3 m2 = (tr.verts[2] + tr.verts[0])*0.5f;

	switch(code)
	{
		case 0:     // 000: no split
		{
			worldTriangles.pushBack(tr);
			triIndicesArray.pushBack(index);
			nbNewTris += 1;
		}
		break;
		case 1:     // 001: split edge0
		{
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[0], m0, tr.verts[2]),	index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(m0, tr.verts[1], tr.verts[2]),	index, worldTriangles, triIndicesArray, cullingBox, params);
		}
		break;
		case 2:     // 010: split edge1
		{
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[0], tr.verts[1], m1),	index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[0], m1, tr.verts[2]),	index, worldTriangles, triIndicesArray, cullingBox, params);
		}
		break;
		case 3:     // 011: split edge0/edge1
		{
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[0], m0, m1),			index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[0], m1, tr.verts[2]),	index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(m0, tr.verts[1], m1),			index, worldTriangles, triIndicesArray, cullingBox, params);
		}
		break;
		case 4:     // 100: split edge2
		{
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[0], tr.verts[1], m2),	index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[1], tr.verts[2], m2),	index, worldTriangles, triIndicesArray, cullingBox, params);
		}
		break;
		case 5:     // 101: split edge0/edge2
		{
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[0], m0, m2),			index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(m0, tr.verts[1], m2),			index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(m2, tr.verts[1], tr.verts[2]),	index, worldTriangles, triIndicesArray, cullingBox, params);
		}
		break;
		case 6:     // 110: split edge1/edge2
		{
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[0], tr.verts[1], m1),	index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[0], m1, m2),			index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(m2, m1, tr.verts[2]),			index, worldTriangles, triIndicesArray, cullingBox, params);
		}
		break;
		case 7:     // 111: split edge0/edge1/edge2
		{
			tessellateTriangle(nbNewTris, PxTriangle(tr.verts[0], m0, m2),			index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(m0, tr.verts[1], m1),			index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(m2, m1, tr.verts[2]),			index, worldTriangles, triIndicesArray, cullingBox, params);
			tessellateTriangle(nbNewTris, PxTriangle(m0, m1, m2),					index, worldTriangles, triIndicesArray, cullingBox, params);
		}
		break;
	};
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void outputPlaneToStream(PxShape* planeShape, const PxRigidActor* actor, const PxTransform& globalPose, IntArray& geomStream, TriArray& worldTriangles, IntArray& triIndicesArray,
								const PxExtendedVec3& origin, const PxBounds3& tmpBounds, const CCTParams& params, Cm::RenderBuffer* renderBuffer)
{
	PX_ASSERT(planeShape->getGeometryType() == PxGeometryType::ePLANE);

	const PxF32 length = (tmpBounds.maximum - tmpBounds.minimum).magnitude();
	PxVec3 center = toVec3(origin);

	const PxPlane plane = PxPlaneEquationFromTransform(globalPose);

	PxVec3 right, up;
	Ps::computeBasis(plane.n, right, up);
	right *= length;
	up *= length;

	const PxVec3 p = plane.project(center);
	const PxVec3 p0 = p - right + up;
	const PxVec3 p1 = p - right - up;
	const PxVec3 p2 = p + right - up;
	const PxVec3 p3 = p + right + up;

	const PxU32 nbTouchedTris = 2;

	const PxVec3 offset(float(-origin.x), float(-origin.y), float(-origin.z));

	TouchedMesh* touchedMesh			= (TouchedMesh*)reserve(geomStream, sizeof(TouchedMesh)/sizeof(PxU32));
	touchedMesh->mType					= TouchedGeomType::eMESH;
	touchedMesh->mTGUserData			= planeShape;
	touchedMesh->mActor					= actor;
	touchedMesh->mOffset				= origin;
	touchedMesh->mNbTris				= nbTouchedTris;
	touchedMesh->mIndexWorldTriangles	= worldTriangles.size();

	// Reserve memory for incoming triangles
	PxTriangle* TouchedTriangles = reserve(worldTriangles, nbTouchedTris);

	triIndicesArray.pushBack(0);
	triIndicesArray.pushBack(1);

	TouchedTriangles[0].verts[0] = p0 + offset;
	TouchedTriangles[0].verts[1] = p1 + offset;
	TouchedTriangles[0].verts[2] = p2 + offset;

	TouchedTriangles[1].verts[0] = p0 + offset;
	TouchedTriangles[1].verts[1] = p2 + offset;
	TouchedTriangles[1].verts[2] = p3 + offset;

	if(gVisualizeTouchedTris)
		visualizeTouchedTriangles(touchedMesh->mNbTris, touchedMesh->mIndexWorldTriangles, &worldTriangles[0], renderBuffer, offset, params.mUpDirection);
}

static void outputSphereToStream(PxShape* sphereShape, const PxRigidActor* actor, const PxTransform& globalPose, IntArray& geomStream, const PxExtendedVec3& origin)
{
	PX_ASSERT(sphereShape->getGeometryType() == PxGeometryType::eSPHERE);
	PxExtendedSphere WorldSphere;
	{
		PxSphereGeometry sg;
		sphereShape->getSphereGeometry(sg);

		WorldSphere.radius = sg.radius;
		WorldSphere.center.x = globalPose.p.x;
		WorldSphere.center.y = globalPose.p.y;
		WorldSphere.center.z = globalPose.p.z;
	}

	TouchedSphere* PX_RESTRICT touchedSphere = (TouchedSphere* PX_RESTRICT)reserve(geomStream, sizeof(TouchedSphere)/sizeof(PxU32));
	touchedSphere->mType		= TouchedGeomType::eSPHERE;
	touchedSphere->mTGUserData	= sphereShape;
	touchedSphere->mActor		= actor;
	touchedSphere->mOffset		= origin;
	touchedSphere->mRadius		= WorldSphere.radius;
	touchedSphere->mCenter.x	= float(WorldSphere.center.x - origin.x);
	touchedSphere->mCenter.y	= float(WorldSphere.center.y - origin.y);
	touchedSphere->mCenter.z	= float(WorldSphere.center.z - origin.z);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void outputCapsuleToStream(PxShape* capsuleShape, const PxRigidActor* actor, const PxTransform& globalPose, IntArray& geomStream, const PxExtendedVec3& origin)
{
	PX_ASSERT(capsuleShape->getGeometryType() == PxGeometryType::eCAPSULE);
	PxExtendedCapsule WorldCapsule;
	{
		PxCapsuleGeometry cg;
		capsuleShape->getCapsuleGeometry(cg);

		PxVec3 p0 = cg.halfHeight * globalPose.q.getBasisVector0();
		PxVec3 p1 = -p0;
		p0 += globalPose.p;
		p1 += globalPose.p;

		WorldCapsule.radius	= cg.radius;
		WorldCapsule.p0.x	= p0.x;
		WorldCapsule.p0.y	= p0.y;
		WorldCapsule.p0.z	= p0.z;
		WorldCapsule.p1.x	= p1.x;
		WorldCapsule.p1.y	= p1.y;
		WorldCapsule.p1.z	= p1.z;
	}

	TouchedCapsule* PX_RESTRICT touchedCapsule = (TouchedCapsule* PX_RESTRICT)reserve(geomStream, sizeof(TouchedCapsule)/sizeof(PxU32));
	touchedCapsule->mType		= TouchedGeomType::eCAPSULE;
	touchedCapsule->mTGUserData	= capsuleShape;
	touchedCapsule->mActor		= actor;
	touchedCapsule->mOffset		= origin;
	touchedCapsule->mRadius		= WorldCapsule.radius;
	touchedCapsule->mP0.x		= float(WorldCapsule.p0.x - origin.x);
	touchedCapsule->mP0.y		= float(WorldCapsule.p0.y - origin.y);
	touchedCapsule->mP0.z		= float(WorldCapsule.p0.z - origin.z);
	touchedCapsule->mP1.x		= float(WorldCapsule.p1.x - origin.x);
	touchedCapsule->mP1.y		= float(WorldCapsule.p1.y - origin.y);
	touchedCapsule->mP1.z		= float(WorldCapsule.p1.z - origin.z);

	//char str[1000] = {0};
	//sprintf( str, "[Warning] outputCapsuleToStream: p0 %0.3f (%0.3f) p1 %0.3f (%0.3f) radius %0.3f origin %0.3f\n", WorldCapsule.p0.z, touchedCapsule->mP0.z, WorldCapsule.p1.z, touchedCapsule->mP1.z, WorldCapsule.radius, origin.z );
	//OutputDebugStringA( str );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void outputBoxToStream(	PxShape* boxShape, const PxRigidActor* actor, const PxTransform& globalPose, IntArray& geomStream, TriArray& worldTriangles, IntArray& triIndicesArray,
								const PxExtendedVec3& origin, const PxBounds3& tmpBounds, const CCTParams& params)
{
	PX_ASSERT(boxShape->getGeometryType() == PxGeometryType::eBOX);
	PxBoxGeometry bg;
	boxShape->getBoxGeometry(bg);

	//8 verts in local space.
	const PxF32 dx = bg.halfExtents.x;
	const PxF32 dy = bg.halfExtents.y;
	const PxF32 dz = bg.halfExtents.z;
	PxVec3 boxVerts[8]=
	{
		PxVec3(-dx,-dy,-dz),
		PxVec3(+dx,-dy,-dz),
		PxVec3(+dx,+dy,-dz),
		PxVec3(-dx,+dy,-dz),
		PxVec3(-dx,-dy,+dz),
		PxVec3(+dx,-dy,+dz),
		PxVec3(+dx,+dy,+dz),
		PxVec3(-dx,+dy,+dz)
	};
	//Transform verts into world space.
	const PxVec3 pxOrigin = toVec3(origin);
	for(PxU32 i = 0; i < 8; i++)
	{
		boxVerts[i] = globalPose.transform(boxVerts[i]) - pxOrigin;
	}

	//Index of triangles.
	const PxU32 boxTris[12][3]=
	{
		{0,2,1},
		{2,0,3},	//0,1,2,3

		{3,6,2},
		{6,3,7},	//3,2,6,7
		
		{7,5,6},
		{5,7,4},	//7,6,5,4

		{4,1,5},
		{1,4,0},	//4,5,1,0

		{0,7,3},
		{7,0,4},	//0,3,7,4

		{2,5,1},
		{5,2,6}		//2,1,5,6
	};

	TouchedMesh* touchedMesh			= (TouchedMesh*)reserve(geomStream, sizeof(TouchedMesh)/sizeof(PxU32));
	touchedMesh->mType					= TouchedGeomType::eMESH;
	touchedMesh->mTGUserData			= boxShape;
	touchedMesh->mActor					= actor;
	touchedMesh->mOffset				= origin;
	touchedMesh->mIndexWorldTriangles	= worldTriangles.size();

	if(params.mTessellation)
	{
		const PxBoxGeometry boxGeom(tmpBounds.getExtents());
		const PxVec3 offset(float(-origin.x), float(-origin.y), float(-origin.z));
		const PxBounds3 cullingBox = PxBounds3::centerExtents(tmpBounds.getCenter() + offset, boxGeom.halfExtents);

		PxU32 nbCreatedTris = 0;
		for(PxU32 i=0; i<12; i++)
		{
			// Compute triangle in world space, add to array
			const PxTriangle currentTriangle(boxVerts[boxTris[i][0]], boxVerts[boxTris[i][1]], boxVerts[boxTris[i][2]]);

			PxU32 nbNewTris = 0;
			tessellateTriangle(nbNewTris, currentTriangle, PX_INVALID_U32, worldTriangles, triIndicesArray, cullingBox, params);
			nbCreatedTris += nbNewTris;
		}
		touchedMesh->mNbTris = nbCreatedTris;
	}
	else
	{
		touchedMesh->mNbTris = 12;

		// Reserve memory for incoming triangles
		PxTriangle* TouchedTriangles = reserve(worldTriangles, 12);

		for(PxU32 i=0; i<12; i++)
		{
			PxTriangle& currentTriangle = TouchedTriangles[i];
			currentTriangle.verts[0] = boxVerts[boxTris[i][0]];
			currentTriangle.verts[1] = boxVerts[boxTris[i][1]];
			currentTriangle.verts[2] = boxVerts[boxTris[i][2]];

			triIndicesArray.pushBack(PX_INVALID_U32);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PxU32 createInvisibleWalls(const CCTParams& params, const PxTriangle& currentTriangle, TriArray& worldTriangles, IntArray& triIndicesArray)
{
	const PxF32 wallHeight = params.mInvisibleWallHeight;
	if(wallHeight==0.0f)
		return 0;

	PxU32 nbNewTris = 0;	// Number of newly created tris

	const PxVec3& upDirection = params.mUpDirection;

	PxVec3 normal;
	currentTriangle.normal(normal);
	if(testSlope(normal, upDirection, params.mSlopeLimit))
	{
		const PxVec3 upWall = upDirection*wallHeight;
		PxVec3 v0p = currentTriangle.verts[0] +	upWall;
		PxVec3 v1p = currentTriangle.verts[1] +	upWall;
		PxVec3 v2p = currentTriangle.verts[2] +	upWall;

		// Extrude edge 0-1
		PxVec3 faceNormal01;
		{
			// 0-1-0p
			const PxTriangle tri0_1_0p(currentTriangle.verts[0], currentTriangle.verts[1], v0p);
			worldTriangles.pushBack(tri0_1_0p);

			// 0p-1-1p
			const PxTriangle tri0p_1_1p(v0p, currentTriangle.verts[1], v1p);
			worldTriangles.pushBack(tri0p_1_1p);

			tri0p_1_1p.normal(faceNormal01);
		}

		// Extrude edge 1-2
		PxVec3 faceNormal12;
		{
			// 1p-1-2p
			const PxTriangle tri1p_1_2p(v1p, currentTriangle.verts[1], v2p);
			worldTriangles.pushBack(tri1p_1_2p);

			// 2p-1-2
			const PxTriangle tri2p_1_2(v2p, currentTriangle.verts[1], currentTriangle.verts[2]);
			worldTriangles.pushBack(tri2p_1_2);

			tri2p_1_2.normal(faceNormal12);
		}

		// Extrude edge 2-0
		PxVec3 faceNormal20;
		{
			// 0p-2-0
			const PxTriangle tri0p_2_0(v0p, currentTriangle.verts[2], currentTriangle.verts[0]);
			worldTriangles.pushBack(tri0p_2_0);

			// 0p-2p-2
			const PxTriangle tri0p_2p_2(v0p, v2p, currentTriangle.verts[2]);
			worldTriangles.pushBack(tri0p_2p_2);

			tri0p_2p_2.normal(faceNormal20);
		}

		const PxU32 triIndex = PX_INVALID_U32;
		for(PxU32 i=0;i<6;i++)
			triIndicesArray.pushBack(triIndex);

		nbNewTris += 6;
	}
	return nbNewTris;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void outputMeshToStream(	PxShape* meshShape, const PxRigidActor* actor, const PxTransform& meshPose, IntArray& geomStream, TriArray& worldTriangles, IntArray& triIndicesArray,
								const PxExtendedVec3& origin, const PxBounds3& tmpBounds, const CCTParams& params, Cm::RenderBuffer* renderBuffer)
{
	PX_ASSERT(meshShape->getGeometryType() == PxGeometryType::eTRIANGLEMESH);
	// Do AABB-mesh query

	PxTriangleMeshGeometry triGeom;
	meshShape->getTriangleMeshGeometry(triGeom);

	const PxBoxGeometry boxGeom(tmpBounds.getExtents());
	const PxTransform boxPose(tmpBounds.getCenter(), PxQuat(PxIdentity));

	// Collide AABB against current mesh
	PxMeshOverlapUtil overlapUtil;
	const PxU32 nbTouchedTris = overlapUtil.findOverlap(boxGeom, boxPose, triGeom, meshPose);

	const PxVec3 offset(float(-origin.x), float(-origin.y), float(-origin.z));

	TouchedMesh* touchedMesh			= (TouchedMesh*)reserve(geomStream, sizeof(TouchedMesh)/sizeof(PxU32));
	touchedMesh->mType					= TouchedGeomType::eMESH;
	touchedMesh->mTGUserData			= meshShape;
	touchedMesh->mActor					= actor;
	touchedMesh->mOffset				= origin;
	touchedMesh->mNbTris				= nbTouchedTris;
	touchedMesh->mIndexWorldTriangles	= worldTriangles.size();

	const PxU32* PX_RESTRICT indices = overlapUtil.getResults();

	if(params.mSlopeLimit!=0.0f)
	{
		if(!params.mTessellation)
		{
			// Loop through touched triangles
			PxU32 nbCreatedTris = 0;
			for(PxU32 i=0; i < nbTouchedTris; i++)
			{
				const PxU32 triangleIndex = indices[i];

				// Compute triangle in world space, add to array
				PxTriangle currentTriangle;
				PxMeshQuery::getTriangle(triGeom, meshPose, triangleIndex, currentTriangle);
				currentTriangle.verts[0] += offset;
				currentTriangle.verts[1] += offset;
				currentTriangle.verts[2] += offset;

				const PxU32 nbNewTris = createInvisibleWalls(params, currentTriangle, worldTriangles, triIndicesArray);
				nbCreatedTris += nbNewTris;
				if(!nbNewTris)
				{
					worldTriangles.pushBack(currentTriangle);

					triIndicesArray.pushBack(triangleIndex);
					nbCreatedTris++;
				}
			}
			touchedMesh->mNbTris = nbCreatedTris;
		}
		else
		{
			const PxBounds3 cullingBox = PxBounds3::centerExtents(boxPose.p + offset, boxGeom.halfExtents);

			// Loop through touched triangles
			PxU32 nbCreatedTris = 0;
			for(PxU32 i=0; i < nbTouchedTris; i++)
			{
				const PxU32 triangleIndex = indices[i];

				// Compute triangle in world space, add to array
				PxTriangle currentTriangle;
				PxMeshQuery::getTriangle(triGeom, meshPose, triangleIndex, currentTriangle);
				currentTriangle.verts[0] += offset;
				currentTriangle.verts[1] += offset;
				currentTriangle.verts[2] += offset;

				PxU32 nbNewTris = createInvisibleWalls(params, currentTriangle, worldTriangles, triIndicesArray);
				nbCreatedTris += nbNewTris;
				if(!nbNewTris)
				{
/*					worldTriangles.pushBack(currentTriangle);
					triIndicesArray.pushBack(triangleIndex);
					nbCreatedTris++;*/
					tessellateTriangle(nbNewTris, currentTriangle, triangleIndex, worldTriangles, triIndicesArray, cullingBox, params);
					nbCreatedTris += nbNewTris;
//					printf("Tesselate: %d new tris\n", nbNewTris);
				}
			}
			touchedMesh->mNbTris = nbCreatedTris;
		}
	}
	else
	{
		if(!params.mTessellation)
		{
			// Reserve memory for incoming triangles
			PxTriangle* TouchedTriangles = reserve(worldTriangles, nbTouchedTris);

			// Loop through touched triangles
			for(PxU32 i=0; i < nbTouchedTris; i++)
			{
				const PxU32 triangleIndex = indices[i];

				// Compute triangle in world space, add to array
				PxTriangle& currentTriangle = *TouchedTriangles++;
				PxMeshQuery::getTriangle(triGeom, meshPose, triangleIndex, currentTriangle);
				currentTriangle.verts[0] += offset;
				currentTriangle.verts[1] += offset;
				currentTriangle.verts[2] += offset;

				triIndicesArray.pushBack(triangleIndex);
			}
		}
		else
		{
			const PxBounds3 cullingBox = PxBounds3::centerExtents(boxPose.p + offset, boxGeom.halfExtents);

			PxU32 nbCreatedTris = 0;
			for(PxU32 i=0; i < nbTouchedTris; i++)
			{
				const PxU32 triangleIndex = indices[i];

				// Compute triangle in world space, add to array
				PxTriangle currentTriangle;
				PxMeshQuery::getTriangle(triGeom, meshPose, triangleIndex, currentTriangle);

				currentTriangle.verts[0] += offset;
				currentTriangle.verts[1] += offset;
				currentTriangle.verts[2] += offset;

				PxU32 nbNewTris = 0;
				tessellateTriangle(nbNewTris, currentTriangle, triangleIndex, worldTriangles, triIndicesArray, cullingBox, params);
//				printf("Tesselate: %d new tris\n", nbNewTris);
				nbCreatedTris += nbNewTris;
			}
			touchedMesh->mNbTris = nbCreatedTris;
		}
	}

	if(gVisualizeTouchedTris)
		visualizeTouchedTriangles(touchedMesh->mNbTris, touchedMesh->mIndexWorldTriangles, &worldTriangles[0], renderBuffer, offset, params.mUpDirection);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void outputHeightFieldToStream(	PxShape* hfShape, const PxRigidActor* actor, const PxTransform& heightfieldPose, IntArray& geomStream, TriArray& worldTriangles, IntArray& triIndicesArray,
										const PxExtendedVec3& origin, const PxBounds3& tmpBounds, const CCTParams& params, Cm::RenderBuffer* renderBuffer)
{
	PX_ASSERT(hfShape->getGeometryType() == PxGeometryType::eHEIGHTFIELD);
	// Do AABB-mesh query

	PxHeightFieldGeometry hfGeom;
	hfShape->getHeightFieldGeometry(hfGeom);

	const PxBoxGeometry boxGeom(tmpBounds.getExtents());
	const PxTransform boxPose(tmpBounds.getCenter(), PxQuat(PxIdentity));

	// Collide AABB against current heightfield
	PxMeshOverlapUtil overlapUtil;
	const PxU32 nbTouchedTris = overlapUtil.findOverlap(boxGeom, boxPose, hfGeom, heightfieldPose);

	const PxVec3 offset(float(-origin.x), float(-origin.y), float(-origin.z));

	TouchedMesh* touchedMesh			= (TouchedMesh*)reserve(geomStream, sizeof(TouchedMesh)/sizeof(PxU32));
	touchedMesh->mType					= TouchedGeomType::eMESH; // ptchernev: seems to work
	touchedMesh->mTGUserData			= hfShape;
	touchedMesh->mActor					= actor;
	touchedMesh->mOffset				= origin;
	touchedMesh->mNbTris				= nbTouchedTris;
	touchedMesh->mIndexWorldTriangles	= worldTriangles.size();

	const PxU32* PX_RESTRICT indices = overlapUtil.getResults();

	if(params.mSlopeLimit!=0.0f)
	{
		if(!params.mTessellation)
		{
			// Loop through touched triangles
			PxU32 nbCreatedTris = 0;
			for(PxU32 i=0; i < nbTouchedTris; i++)
			{
				const PxU32 triangleIndex = indices[i];

				// Compute triangle in world space, add to array
				PxTriangle currentTriangle;
				PxMeshQuery::getTriangle(hfGeom, heightfieldPose, triangleIndex, currentTriangle);
				currentTriangle.verts[0] += offset;
				currentTriangle.verts[1] += offset;
				currentTriangle.verts[2] += offset;

				const PxU32 nbNewTris = createInvisibleWalls(params, currentTriangle, worldTriangles, triIndicesArray);
				nbCreatedTris += nbNewTris;
				if(!nbNewTris)
				{
					worldTriangles.pushBack(currentTriangle);

					triIndicesArray.pushBack(triangleIndex);
					nbCreatedTris++;
				}
			}
			touchedMesh->mNbTris = nbCreatedTris;
		}
		else
		{
			const PxBounds3 cullingBox = PxBounds3::centerExtents(boxPose.p + offset, boxGeom.halfExtents);

			// Loop through touched triangles
			PxU32 nbCreatedTris = 0;
			for(PxU32 i=0; i < nbTouchedTris; i++)
			{
				const PxU32 triangleIndex = indices[i];

				// Compute triangle in world space, add to array
				PxTriangle currentTriangle;
				PxMeshQuery::getTriangle(hfGeom, heightfieldPose, triangleIndex, currentTriangle);
				currentTriangle.verts[0] += offset;
				currentTriangle.verts[1] += offset;
				currentTriangle.verts[2] += offset;

				PxU32 nbNewTris = createInvisibleWalls(params, currentTriangle, worldTriangles, triIndicesArray);
				nbCreatedTris += nbNewTris;
				if(!nbNewTris)
				{
					tessellateTriangle(nbNewTris, currentTriangle, triangleIndex, worldTriangles, triIndicesArray, cullingBox, params);
					nbCreatedTris += nbNewTris;
//					printf("Tesselate: %d new tris\n", nbNewTris);
				}
			}
			touchedMesh->mNbTris = nbCreatedTris;
		}
	}
	else
	{
		if(!params.mTessellation)
		{
			// Reserve memory for incoming triangles
			PxTriangle* TouchedTriangles = reserve(worldTriangles, nbTouchedTris);

			// Loop through touched triangles
			for(PxU32 i=0; i < nbTouchedTris; i++)
			{
				const PxU32 triangleIndex = indices[i];

				// Compute triangle in world space, add to array
				PxTriangle& currentTriangle = *TouchedTriangles++;
				PxMeshQuery::getTriangle(hfGeom, heightfieldPose, triangleIndex, currentTriangle);
				currentTriangle.verts[0] += offset;
				currentTriangle.verts[1] += offset;
				currentTriangle.verts[2] += offset;

				triIndicesArray.pushBack(triangleIndex);
			}
		}
		else
		{
			const PxBounds3 cullingBox = PxBounds3::centerExtents(boxPose.p + offset, boxGeom.halfExtents);

			PxU32 nbCreatedTris = 0;
			for(PxU32 i=0; i < nbTouchedTris; i++)
			{
				const PxU32 triangleIndex = indices[i];

				// Compute triangle in world space, add to array
				PxTriangle currentTriangle;
				PxMeshQuery::getTriangle(hfGeom, heightfieldPose, triangleIndex, currentTriangle);

				currentTriangle.verts[0] += offset;
				currentTriangle.verts[1] += offset;
				currentTriangle.verts[2] += offset;

				PxU32 nbNewTris = 0;
				tessellateTriangle(nbNewTris, currentTriangle, triangleIndex, worldTriangles, triIndicesArray, cullingBox, params);
//				printf("Tesselate: %d new tris\n", nbNewTris);
				nbCreatedTris += nbNewTris;
			}
			touchedMesh->mNbTris = nbCreatedTris;
		}
	}

	if(gVisualizeTouchedTris)
		visualizeTouchedTriangles(touchedMesh->mNbTris, touchedMesh->mIndexWorldTriangles, &worldTriangles[0], renderBuffer, offset, params.mUpDirection);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void outputConvexToStream(PxShape* convexShape, const PxRigidActor* actor, const PxTransform& absPose_, IntArray& geomStream, TriArray& worldTriangles, IntArray& triIndicesArray,
								 const PxExtendedVec3& origin, const PxBounds3& tmpBounds, const CCTParams& params, Cm::RenderBuffer* renderBuffer)
{
	PX_ASSERT(convexShape->getGeometryType() == PxGeometryType::eCONVEXMESH);
	PxConvexMeshGeometry cg;
	convexShape->getConvexMeshGeometry(cg);
	PX_ASSERT(cg.convexMesh);

	// Do AABB-mesh query

	PxU32* TF;

	// Collide AABB against current mesh
	// The overlap function doesn't exist for convexes so let's just dump all tris
	PxConvexMesh& cm = *cg.convexMesh;

	// PT: convex triangles are not exposed anymore so we need to access convex polygons & triangulate them

	// PT: TODO: this is copied from "DrawObjects", move this to a shared place. Actually a helper directly in PxConvexMesh would be useful.
	PxU32 Nb = 0;
	{
		const PxU32 nbPolys = cm.getNbPolygons();
		const PxU8* polygons = cm.getIndexBuffer();

		for(PxU32 i=0;i<nbPolys;i++)
		{
			PxHullPolygon data;
			cm.getPolygonData(i, data);
			Nb += data.mNbVerts - 2;
		}

		// PT: revisit this code. We don't use the polygon offset?
		TF = (PxU32*)PxAlloca(sizeof(PxU32)*Nb*3);
		PxU32* t = TF;
		for(PxU32 i=0;i<nbPolys;i++)
		{
			PxHullPolygon data;
			cm.getPolygonData(i, data);

			const PxU32 nbV = data.mNbVerts;

			const PxU32 nbTris = nbV - 2;
			const PxU8 vref0 = *polygons;
			for(PxU32 j=0;j<nbTris;j++)
			{
				const PxU32 vref1 = polygons[(j+1)%nbV];
				const PxU32 vref2 = polygons[(j+2)%nbV];
				*t++ = vref0;
				*t++ = vref1;
				*t++ = vref2;
			}
			polygons += nbV;
		}
	}

	// PT: you can't use PxTransform with a non-uniform scaling
	const PxMat33 rot = PxMat33(absPose_.q) * cg.scale.toMat33();
	const PxMat44 absPose(rot, absPose_.p);

	const PxVec3 p = absPose.getPosition();
	const PxVec3 MeshOffset(float(p.x - origin.x), float(p.y - origin.y), float(p.z - origin.z));	// LOSS OF ACCURACY

	const PxVec3 offset(float(-origin.x), float(-origin.y), float(-origin.z));

	TouchedMesh* touchedMesh			= (TouchedMesh*)reserve(geomStream, sizeof(TouchedMesh)/sizeof(PxU32));
	touchedMesh->mType					= TouchedGeomType::eMESH;
	touchedMesh->mTGUserData			= convexShape;
	touchedMesh->mActor					= actor;
	touchedMesh->mOffset				= origin;
	touchedMesh->mIndexWorldTriangles	= worldTriangles.size();

	const PxVec3* verts = cm.getVertices();
	// Loop through touched triangles
	if(params.mTessellation)
	{
		const PxBoxGeometry boxGeom(tmpBounds.getExtents());
		const PxBounds3 cullingBox = PxBounds3::centerExtents(tmpBounds.getCenter() + offset, boxGeom.halfExtents);

		PxU32 nbCreatedTris = 0;
		while(Nb--)
		{
			// Compute triangle in world space, add to array
			PxTriangle currentTriangle;

			const PxU32 vref0 = *TF++;
			const PxU32 vref1 = *TF++;
			const PxU32 vref2 = *TF++;

			currentTriangle.verts[0] = MeshOffset + absPose.rotate(verts[vref0]);
			currentTriangle.verts[1] = MeshOffset + absPose.rotate(verts[vref1]);
			currentTriangle.verts[2] = MeshOffset + absPose.rotate(verts[vref2]);

			PxU32 nbNewTris = 0;
			tessellateTriangle(nbNewTris, currentTriangle, PX_INVALID_U32, worldTriangles, triIndicesArray, cullingBox, params);
			nbCreatedTris += nbNewTris;
		}
		touchedMesh->mNbTris = nbCreatedTris;
	}
	else
	{
		// Reserve memory for incoming triangles
		PxTriangle* TouchedTriangles = reserve(worldTriangles, Nb);

		touchedMesh->mNbTris = Nb;
		while(Nb--)
		{
			// Compute triangle in world space, add to array
			PxTriangle& currentTriangle = *TouchedTriangles++;

			const PxU32 vref0 = *TF++;
			const PxU32 vref1 = *TF++;
			const PxU32 vref2 = *TF++;

			currentTriangle.verts[0] = MeshOffset + absPose.rotate(verts[vref0]);
			currentTriangle.verts[1] = MeshOffset + absPose.rotate(verts[vref1]);
			currentTriangle.verts[2] = MeshOffset + absPose.rotate(verts[vref2]);

			triIndicesArray.pushBack(PX_INVALID_U32);
		}
	}
	if(gVisualizeTouchedTris)
		visualizeTouchedTriangles(touchedMesh->mNbTris, touchedMesh->mIndexWorldTriangles, &worldTriangles[0], renderBuffer, offset, params.mUpDirection);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PxU32 Cct::getSceneTimestamp(const InternalCBData_FindTouchedGeom* userData)
{
	PX_ASSERT(userData);
	const PxInternalCBData_FindTouchedGeom* internalData = static_cast<const PxInternalCBData_FindTouchedGeom*>(userData);
	PxScene* scene = internalData->scene;
	return scene->getSceneQueryStaticTimestamp();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Cct::findTouchedGeometry(
	const InternalCBData_FindTouchedGeom* userData,
	const PxExtendedBounds3& worldBounds,		// ### we should also accept other volumes

	TriArray& worldTriangles,
	IntArray& triIndicesArray,
	IntArray& geomStream,

	const CCTFilter& filter,
	const CCTParams& params)
{
	PX_ASSERT(userData);
	
	const PxInternalCBData_FindTouchedGeom* internalData = static_cast<const PxInternalCBData_FindTouchedGeom*>(userData);
	PxScene* scene = internalData->scene;
	Cm::RenderBuffer* renderBuffer = internalData->renderBuffer;

	PxExtendedVec3 Origin;	// Will be TouchedGeom::mOffset
	getCenter(worldBounds, Origin);

	// Find touched *boxes* i.e. touched objects' AABBs in the world
	// We collide against dynamic shapes too, to get back dynamic boxes/etc
	// TODO: add active groups in interface!
	PxQueryFlags sqFilterFlags;
	if(filter.mStaticShapes)	sqFilterFlags |= PxQueryFlag::eSTATIC;
	if(filter.mDynamicShapes)	sqFilterFlags |= PxQueryFlag::eDYNAMIC;
	if(filter.mFilterCallback)
	{
		if(filter.mPreFilter)
			sqFilterFlags |= PxQueryFlag::ePREFILTER;
		if(filter.mPostFilter)
			sqFilterFlags |= PxQueryFlag::ePOSTFILTER;
	}

	// ### this one is dangerous
	const PxBounds3 tmpBounds(toVec3(worldBounds.minimum), toVec3(worldBounds.maximum));	// LOSS OF ACCURACY

	// PT: unfortunate conversion forced by the PxGeometry API
	PxVec3 center = tmpBounds.getCenter(), extents = tmpBounds.getExtents();

	const PxU32 size = 100;
	PxOverlapHit hits[size];

	PxQueryFilterData sceneQueryFilterData = filter.mFilterData ? PxQueryFilterData(*filter.mFilterData, sqFilterFlags) : PxQueryFilterData(sqFilterFlags);

	PxOverlapBuffer hitBuffer(hits, size);
	sceneQueryFilterData.flags |= PxQueryFlag::eNO_BLOCK; // fix for DE8255
	scene->overlap(PxBoxGeometry(extents), PxTransform(center), hitBuffer, sceneQueryFilterData, filter.mFilterCallback);
	PxU32 numberHits = hitBuffer.getNbAnyHits();

	//if(filter.mStaticShapes)
	//{
		//char str[1000] = {0};
		//sprintf( str,"[Warning] findTouchedGeometry: hit %i min %0.3f max %0.3f\n", numberHits, worldBounds.minimum.z, worldBounds.maximum.z);
		//OutputDebugStringA( str );
	//}

	for(PxU32 i = 0; i < numberHits; i++)
	{
		const PxOverlapHit& hit = hitBuffer.getAnyHit(i);
		PxShape* shape = hit.shape;
		PxRigidActor* actor = hit.actor;
		if(!shape || !actor)
			continue;

		// Filtering

		// Discard all CCT shapes, i.e. kinematic actors we created ourselves. We don't need to collide with them since they're surrounded
		// by the real CCT volume - and collisions with those are handled elsewhere.
		if(internalData->cctShapeHashSet->contains(shape))
			continue;

		// Ubi (EA) : Discarding Triggers :
		if(shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
			continue;

		// PT: here you might want to disable kinematic objects.

		// Output shape to stream
		const PxTransform globalPose = getShapeGlobalPose(*shape, *actor);

		const PxGeometryType::Enum type = shape->getGeometryType();	// ### VIRTUAL!
		if(type==PxGeometryType::eSPHERE)				outputSphereToStream		(shape, actor, globalPose, geomStream, Origin);
		else	if(type==PxGeometryType::eCAPSULE)		outputCapsuleToStream		(shape, actor, globalPose, geomStream, Origin);
		else	if(type==PxGeometryType::eBOX)			outputBoxToStream			(shape, actor, globalPose, geomStream, worldTriangles, triIndicesArray, Origin, tmpBounds, params);
		else	if(type==PxGeometryType::eTRIANGLEMESH)	outputMeshToStream			(shape, actor, globalPose, geomStream, worldTriangles, triIndicesArray, Origin, tmpBounds, params, renderBuffer);
		else	if(type==PxGeometryType::eHEIGHTFIELD)	outputHeightFieldToStream	(shape, actor, globalPose, geomStream, worldTriangles, triIndicesArray, Origin, tmpBounds, params, renderBuffer);
		else	if(type==PxGeometryType::eCONVEXMESH)	outputConvexToStream		(shape, actor, globalPose, geomStream, worldTriangles, triIndicesArray, Origin, tmpBounds, params, renderBuffer);
		else	if(type==PxGeometryType::ePLANE)		outputPlaneToStream			(shape, actor, globalPose, geomStream, worldTriangles, triIndicesArray, Origin, tmpBounds, params, renderBuffer);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "CctCharacterControllerManager.h"
#include "CctObstacleContext.h"
#include "PxControllerBehavior.h"

// #### hmmm, in the down case, isn't reported length too big ? It contains our artificial up component,
// that might confuse the user

static void fillCCTHit(PxControllerHit& hit, const SweptContact& contact, const PxVec3& dir, float length, Controller* controller)
{
	hit.controller		= controller->getPxController();
	hit.worldPos		= contact.mWorldPos;
	hit.worldNormal		= contact.mWorldNormal;
	hit.dir				= dir;
	hit.length			= length;
}

static const PxU32 defaultBehaviorFlags = 0;

PxU32 Cct::shapeHitCallback(const InternalCBData_OnHit* userData, const SweptContact& contact, const PxVec3& dir, float length)
{
	Controller* controller = static_cast<const PxInternalCBData_OnHit*>(userData)->controller;

	PxControllerShapeHit hit;
	fillCCTHit(hit, contact, dir, length, controller);

	hit.shape			= (PxShape*)contact.mGeom->mTGUserData;
	hit.actor			= const_cast<PxRigidActor*>(contact.mGeom->mActor);
	hit.triangleIndex	= contact.mTriangleIndex;

	if(controller->mReportCallback)
		controller->mReportCallback->onShapeHit(hit);

	PxControllerBehaviorCallback* behaviorCB = controller->mBehaviorCallback;
	return behaviorCB ? behaviorCB->getBehaviorFlags(*hit.shape, *hit.actor) : defaultBehaviorFlags;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PX_FORCE_INLINE PxU32 handleObstacleHit(const PxObstacle& touchedObstacle, const ObstacleHandle& obstacleHandle, PxControllerObstacleHit& hit, const PxInternalCBData_OnHit* internalData, Controller* controller)
{
	hit.userData = touchedObstacle.mUserData;
	const_cast<PxInternalCBData_OnHit*>(internalData)->touchedObstacle = &touchedObstacle;	// (*) PT: TODO: revisit
	const_cast<PxInternalCBData_OnHit*>(internalData)->touchedObstacleHandle = obstacleHandle;

	if(controller->mReportCallback)
		controller->mReportCallback->onObstacleHit(hit);

	PxControllerBehaviorCallback* behaviorCB = controller->mBehaviorCallback;
	return behaviorCB ? behaviorCB->getBehaviorFlags(touchedObstacle) : defaultBehaviorFlags;
}

PxU32 Cct::userHitCallback(const InternalCBData_OnHit* userData, const SweptContact& contact, const PxVec3& dir, float length)
{
	const PxInternalCBData_OnHit* internalData = static_cast<const PxInternalCBData_OnHit*>(userData);
	Controller* controller = internalData->controller;

	const PxU32 objectCode = (PxU32)(size_t)contact.mGeom->mTGUserData;
	const UserObjectType type = decodeType(objectCode);
	const PxU32 index = decodeIndex(objectCode);

	if(type==USER_OBJECT_CCT)
	{
		PX_ASSERT(index<controller->mManager->getNbControllers());
		Controller** controllers = controller->mManager->getControllers();
		Controller* other = controllers[index];

		PxControllersHit hit;
		fillCCTHit(hit, contact, dir, length, controller);

		hit.other = other->getPxController();

		if(controller->mReportCallback)
			controller->mReportCallback->onControllerHit(hit);

		PxControllerBehaviorCallback* behaviorCB = controller->mBehaviorCallback;
		return behaviorCB ? behaviorCB->getBehaviorFlags(*hit.other) : defaultBehaviorFlags;
	}
	else if(type==USER_OBJECT_BOX_OBSTACLE)
	{
		PX_ASSERT(internalData->obstacles);
		PX_ASSERT(index<internalData->obstacles->mBoxObstacles.size());

		PxControllerObstacleHit hit;
		fillCCTHit(hit, contact, dir, length, controller);

		const ObstacleContext::InternalBoxObstacle& obstacle = internalData->obstacles->mBoxObstacles[index];
		const PxBoxObstacle& touchedObstacle = obstacle.mData;
		return handleObstacleHit(touchedObstacle, obstacle.mHandle , hit, internalData, controller);
	}
	else if(type==USER_OBJECT_CAPSULE_OBSTACLE)
	{
		PX_ASSERT(internalData->obstacles);
		PX_ASSERT(index<internalData->obstacles->mCapsuleObstacles.size());

		PxControllerObstacleHit hit;
		fillCCTHit(hit, contact, dir, length, controller);

		const ObstacleContext::InternalCapsuleObstacle& obstacle = internalData->obstacles->mCapsuleObstacles[index];
		const PxCapsuleObstacle& touchedObstacle = obstacle.mData;
		return handleObstacleHit(touchedObstacle, obstacle.mHandle, hit, internalData, controller);
	}
	else PX_ASSERT(0);

	return defaultBehaviorFlags;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
