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


#include "PxcSpatial.h"
#include "PxcArticulation.h"
#include "PxcArticulationScalar.h"
#include "PxcArticulationFnsScalar.h"
#include "PxcArticulationReference.h"
#include "PxcArticulationFnsSimd.h"

namespace physx
{

#if PXC_ARTICULATION_DEBUG_VERIFY
namespace 
{
	Cm::SpatialVector SpV(Vec3V linear, Vec3V angular) 
	{
		return Cm::SpatialVector((PxVec3 &)linear, (PxVec3&)angular);
	}
}
#endif

void PxcFsApplyImpulse(PxcFsData &matrix, 
					   PxU32 linkID, 
					   Vec3V linear,
					   Vec3V angular)
{
#if PXC_ARTICULATION_DEBUG_VERIFY
	{	
		PxcSIMDSpatial imp(linear, angular);
		PxcArticulationRef::applyImpulse(matrix, reinterpret_cast<Cm::SpatialVector *>(getRefVelocity(matrix)), linkID, reinterpret_cast<Cm::SpatialVector&>(imp));
	}
#endif


	Vec3V linZ = V3Neg(linear);
	Vec3V angZ = V3Neg(angular);

	const PxcFsRow *rows = getFsRows(matrix);
	const PxcFsJointVectors* jointVectors = getJointVectors(matrix);

#if PXC_ARTICULATION_DEBUG_VERIFY
	const PxcFsRowAux *aux = getAux(matrix);
#endif
	Vec3V *deferredSZ = getDeferredSZ(matrix);

	for(PxU32 i = linkID; i!=0; i = matrix.parent[i])
	{
		const PxcFsRow &row = rows[i];
		const PxcFsJointVectors& jv = jointVectors[i];

#if PXC_ARTICULATION_DEBUG_VERIFY
		PxVec3 SZcheck;
		Cm::SpatialVector Zcheck = PxcArticulationRef::propagateImpulse(row, jv, SZcheck, SpV(linZ, angZ), aux[i]);
#endif

		Vec3V SZ = V3Add(angZ, V3Cross(linZ, jv.jointOffset));
		Vec3V lrLinear = V3Sub(linZ,  V3ScaleAdd(row.DSI[0].linear, V3GetX(SZ),
									   V3ScaleAdd(row.DSI[1].linear, V3GetY(SZ),
									       V3Scale(row.DSI[2].linear, V3GetZ(SZ)))));

		Vec3V lrAngular = V3Sub(angZ,  V3ScaleAdd(row.DSI[0].angular, V3GetX(SZ),
									    V3ScaleAdd(row.DSI[1].angular, V3GetY(SZ),
									        V3Scale(row.DSI[2].angular, V3GetZ(SZ)))));

		linZ = lrLinear;
		angZ = V3Add(lrAngular, V3Cross(jv.parentOffset, lrLinear));
		deferredSZ[i] = V3Add(deferredSZ[i], SZ);

		PX_ASSERT(Ps::aos::isFiniteVec3V(linZ));
		PX_ASSERT(Ps::aos::isFiniteVec3V(angZ));

#if PXC_ARTICULATION_DEBUG_VERIFY
		Cm::SpatialVector Z = SpV(linZ,angZ);
		PX_ASSERT((Z - Zcheck).magnitude()<1e-4*PxMax(Zcheck.magnitude(), 1.0f));
		PX_ASSERT(((PxVec3&)SZ-SZcheck).magnitude()<1e-4*PxMax(SZcheck.magnitude(), 1.0f));
#endif
	}

	matrix.deferredZ.linear = V3Add(matrix.deferredZ.linear, linZ);
	matrix.deferredZ.angular = V3Add(matrix.deferredZ.angular, angZ);

	matrix.dirty |= rows[linkID].pathToRoot;
}

PxcSIMDSpatial PxcFsGetVelocity(PxcFsData &matrix,								  
								PxU32 linkID)
{
	const PxcFsRow *rows = getFsRows(matrix);
	const PxcFsJointVectors* jointVectors = getJointVectors(matrix);

#if PXC_ARTICULATION_DEBUG_VERIFY
	const PxcFsRowAux *aux = getAux(matrix);
#endif
	PxcSIMDSpatial* PX_RESTRICT V = getVelocity(matrix);

	// find the dirty node on the path (including the root) with the lowest index
	PxcArticulationBitField toUpdate = rows[linkID].pathToRoot & matrix.dirty;


	if(toUpdate)
	{
		// store the dV elements densely and use an array map to decode - hopefully cache friendlier
		PxU32 indexToStackLoc[PXC_ARTICULATION_MAX_SIZE], count = 0;
		PxcSIMDSpatial dVStack[PXC_ARTICULATION_MAX_SIZE];

		PxcArticulationBitField ignoreNodes = (toUpdate & (0-toUpdate))-1;
		PxcArticulationBitField path = rows[linkID].pathToRoot & ~ignoreNodes, p = path;
		PxcArticulationBitField newDirty = 0;

		Vec3V ldV = V3Zero(), adV = V3Zero();
		PxcSIMDSpatial* PX_RESTRICT defV = getDeferredVel(matrix);
		Vec3V* PX_RESTRICT SZ = getDeferredSZ(matrix);

		if(p & 1)
		{
			const PxcFsInertia &m = getRootInverseInertia(matrix);
			Vec3V lZ = V3Neg(matrix.deferredZ.linear);
			Vec3V aZ = V3Neg(matrix.deferredZ.angular);

			ldV = V3Add(M33MulV3(m.ll,lZ),		 M33MulV3(m.la,aZ));
			adV = V3Add(M33TrnspsMulV3(m.la,lZ), M33MulV3(m.aa,aZ));

			V[0].linear = V3Add(V[0].linear, ldV);
			V[0].angular = V3Add(V[0].angular, adV);

			matrix.deferredZ.linear = V3Zero();
			matrix.deferredZ.angular = V3Zero();

			indexToStackLoc[0] = count;
			PxcSIMDSpatial &e = dVStack[count++];

			e.linear = ldV;
			e.angular = adV;

			newDirty = rows[0].children;
			p--;
		}

		
		while(p)	// using "for(;p;p &= (p-1))" here generates LHSs from the PxcArticulationLowestSetBit
		{
			PxU32 i = PxcArticulationLowestSetBit(p);
			const PxcFsJointVectors& jv = jointVectors[i];

			p &= (p-1);
	
			const PxcFsRow* PX_RESTRICT row = rows + i;

			ldV = V3Add(ldV, defV[i].linear);
			adV = V3Add(adV, defV[i].angular);

#if PXC_ARTICULATION_DEBUG_VERIFY
			Cm::SpatialVector dVcheck = PxcArticulationRef::propagateVelocity(*row, jv, (PxVec3&)SZ[i], SpV(ldV,adV), aux[i]);
#endif

			Vec3V DSZ = M33MulV3(row->D, SZ[i]);

			Vec3V lW = V3Add(ldV, V3Cross(adV,jv.parentOffset));
			Vec3V aW = adV;

			const PxcSIMDSpatial*PX_RESTRICT DSI = row->DSI;
			Vec3V lN = V3Merge(V3Dot(DSI[0].linear, lW),   V3Dot(DSI[1].linear, lW),  V3Dot(DSI[2].linear, lW));
			Vec3V aN = V3Merge(V3Dot(DSI[0].angular, aW),  V3Dot(DSI[1].angular, aW), V3Dot(DSI[2].angular, aW));

			Vec3V n = V3Add(V3Add(lN, aN), DSZ);

			ldV = V3Sub(lW, V3Cross(jv.jointOffset,n));
			adV = V3Sub(aW, n);

#if PXC_ARTICULATION_DEBUG_VERIFY
			Cm::SpatialVector dV = SpV(ldV,adV);
			PX_ASSERT((dV-dVcheck).magnitude()<1e-4*PxMax(dVcheck.magnitude(), 1.0f));
#endif

			V[i].linear = V3Add(V[i].linear, ldV);
			V[i].angular = V3Add(V[i].angular, adV);

			defV[i].linear = V3Zero();
			defV[i].angular = V3Zero();
			SZ[i] = V3Zero();

			indexToStackLoc[i] = count;
			PxcSIMDSpatial &e = dVStack[count++];
			newDirty |= rows[i].children;

			e.linear = ldV;
			e.angular = adV;
		}

		for(PxcArticulationBitField defer = newDirty&~path; defer; defer &= (defer-1))
		{
			PxU32 i = PxcArticulationLowestSetBit(defer);
			PxU32 parent = indexToStackLoc[matrix.parent[i]];

			defV[i].linear = V3Add(defV[i].linear, dVStack[parent].linear);
			defV[i].angular = V3Add(defV[i].angular, dVStack[parent].angular);
		}

		matrix.dirty = (matrix.dirty | newDirty)&~path;
	}
#if PXC_ARTICULATION_DEBUG_VERIFY
	Cm::SpatialVector v = reinterpret_cast<Cm::SpatialVector&>(V[linkID]);
	Cm::SpatialVector rv = reinterpret_cast<Cm::SpatialVector&>(getRefVelocity(matrix)[linkID]);
	PX_ASSERT((v-rv).magnitude()<1e-4f * PxMax(rv.magnitude(),1.0f));
#endif

	return V[linkID];
}

PX_FORCE_INLINE PxcSIMDSpatial propagateVelocitySIMD(const PxcFsRow& row,
														const PxcFsJointVectors& jv,
														const Vec3V& SZ,
														const PxcSIMDSpatial& v,
														const PxcFsRowAux& aux)
{
	PX_UNUSED(aux);

	typedef PxcArticulationFnsSimd<PxcArticulationFnsSimdBase> Fns;

	PxcSIMDSpatial w(V3Add(v.linear, V3Cross(v.angular, jv.parentOffset)), v.angular);
	Vec3V DSZ = M33MulV3(row.D, SZ);

	Vec3V n = V3Add(Fns::axisDot(row.DSI, w), DSZ);
	PxcSIMDSpatial result = w - PxcSIMDSpatial(V3Cross(jv.jointOffset,n), n);

#if PXC_ARTICULATION_DEBUG_VERIFY
	Cm::SpatialVector check = PxcArticulationRef::propagateVelocity(row, jv, reinterpret_cast<const PxVec3&>(SZ), reinterpret_cast<const Cm::SpatialVector&>(v), aux);
	PX_ASSERT((reinterpret_cast<const Cm::SpatialVector&>(result)-check).magnitude()<1e-4*PxMax(check.magnitude(), 1.0f));
#endif

	return result;
}

void PxcFsFlushVelocity(PxcFsData& matrix)
{
	typedef PxcArticulationFnsSimd<PxcArticulationFnsSimdBase> Fns;

	const PxcFsRow* PX_RESTRICT rows = getFsRows(matrix);
	const PxcFsRowAux* PX_RESTRICT aux = getAux(matrix);
	const PxcFsJointVectors*PX_RESTRICT jointVectors = getJointVectors(matrix);

	PxcSIMDSpatial V =  Fns::multiply(getRootInverseInertia(matrix), -matrix.deferredZ);
	matrix.deferredZ = PxcSIMDSpatial(PxZero);

	getVelocity(matrix)[0] += V;
	for(PxcArticulationBitField defer = rows[0].children; defer; defer &= (defer-1))
		getDeferredVel(matrix)[PxcArticulationLowestSetBit(defer)] += V;

	for(PxU32 i = 1; i<matrix.linkCount; i++)
	{
		PxcSIMDSpatial V = propagateVelocitySIMD(rows[i], jointVectors[i], getDeferredSZ(matrix)[i], getDeferredVel(matrix)[i], aux[i]);
		getDeferredVel(matrix)[i] = PxcSIMDSpatial(PxZero);
		getDeferredSZ(matrix)[i] = V3Zero();
		getVelocity(matrix)[i] += V;
		for(PxcArticulationBitField defer = rows[i].children; defer; defer &= (defer-1))
			getDeferredVel(matrix)[PxcArticulationLowestSetBit(defer)] += V;
	}

#if PXC_ARTICULATION_DEBUG_VERIFY
	for(PxU32 i=0;i<matrix.linkCount;i++)
	{
		Cm::SpatialVector v = velocityRef(matrix,i), rv = reinterpret_cast<Cm::SpatialVector&>(getRefVelocity(matrix)[i]);
		Cm::SpatialVector diff = v-rv;
		PxReal m = rv.magnitude();
		PX_UNUSED(m);
		PX_ASSERT(diff.magnitude()<1e-4*PxMax(1.0f,m));
	}
#endif

	matrix.dirty = 0;
}

}
