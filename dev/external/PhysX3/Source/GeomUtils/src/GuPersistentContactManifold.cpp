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

#include "CmPhysXCommon.h"
#include "GuPersistentContactManifold.h"
#include "GuContactBuffer.h"
#include "PsAlloca.h"
#include "PxMemory.h"
#include "PsVecTransform.h"
#include "PsUtilities.h"


using namespace physx;

namespace physx
{
namespace Gu
{

static Ps::aos::FloatV distancePointSegmentSquaredLocal(const Ps::aos::Vec3VArg a, const Ps::aos::Vec3VArg b, const Ps::aos::Vec3VArg p)
{
	using namespace Ps::aos;
	const FloatV zero = FZero();
	const FloatV one = FOne();

	const Vec3V ap = V3Sub(p, a);
	const Vec3V ab = V3Sub(b, a);
	const FloatV nom = V3Dot(ap, ab);
	
	const FloatV denom = V3Dot(ab, ab);
	const FloatV tValue = FClamp(FDiv(nom, denom), zero, one);

	const FloatV t = FSel(FIsEq(denom, zero), zero, tValue);
	const Vec3V v = V3NegScaleSub(ab, t, ap);
	return V3Dot(v, v);
}

static Ps::aos::FloatV distancePointTriangleSquaredLocal(	const Ps::aos::Vec3VArg p, 
													const Ps::aos::Vec3VArg a, 
													const Ps::aos::Vec3VArg b, 
													const Ps::aos::Vec3VArg c)
{
	using namespace Ps::aos;

	const FloatV zero = FZero();
	const BoolV bTrue = BTTTT();
	//const Vec3V zero = V3Zero();
	const Vec3V ab = V3Sub(b, a);
	const Vec3V ac = V3Sub(c, a);
	const Vec3V bc = V3Sub(c, b);
	const Vec3V ap = V3Sub(p, a);
	const Vec3V bp = V3Sub(p, b);
	const Vec3V cp = V3Sub(p, c);

	const FloatV d1 = V3Dot(ab, ap); //  snom
	const FloatV d2 = V3Dot(ac, ap); //  tnom
	const FloatV d3 = V3Dot(ab, bp); // -sdenom
	const FloatV d4 = V3Dot(ac, bp); //  unom = d4 - d3
	const FloatV d5 = V3Dot(ab, cp); //  udenom = d5 - d6
	const FloatV d6 = V3Dot(ac, cp); // -tdenom
	const FloatV unom = FSub(d4, d3);
	const FloatV udenom = FSub(d5, d6);
	
	//check if p in vertex region outside a
	const BoolV con00 = FIsGrtr(zero, d1); // snom <= 0
	const BoolV con01 = FIsGrtr(zero, d2); // tnom <= 0
	const BoolV con0 = BAnd(con00, con01); // vertex region a

	if(BAllEq(con0, bTrue))
	{
		const Vec3V vv = V3Sub(p, a);
		return V3Dot(vv, vv);
	}

	//check if p in vertex region outside b
	const BoolV con10 = FIsGrtrOrEq(d3, zero);
	const BoolV con11 = FIsGrtrOrEq(d3, d4);
	const BoolV con1 = BAnd(con10, con11); // vertex region b
	if(BAllEq(con1, bTrue))
	{
		const Vec3V vv = V3Sub(p, b);
		return V3Dot(vv, vv);
	}

	//check if p in vertex region outside c
	const BoolV con20 = FIsGrtrOrEq(d6, zero);
	const BoolV con21 = FIsGrtrOrEq(d6, d5); 
	const BoolV con2 = BAnd(con20, con21); // vertex region c
	if(BAllEq(con2, bTrue))
	{
		const Vec3V vv = V3Sub(p, c);
		return V3Dot(vv, vv);
	}

	//check if p in edge region of AB
	const FloatV vc = FSub(FMul(d1, d4), FMul(d3, d2));
	
	const BoolV con30 = FIsGrtr(zero, vc);
	const BoolV con31 = FIsGrtrOrEq(d1, zero);
	const BoolV con32 = FIsGrtr(zero, d3);
	const BoolV con3 = BAnd(con30, BAnd(con31, con32));
	if(BAllEq(con3, bTrue))
	{
		const FloatV sScale = FDiv(d1, FSub(d1, d3));
		const Vec3V closest3 = V3ScaleAdd(ab, sScale, a);//V3Add(a, V3Scale(ab, sScale));
		const Vec3V vv = V3Sub(p, closest3);
		return V3Dot(vv, vv);
	}

	//check if p in edge region of BC
	const FloatV va = FSub(FMul(d3, d6),FMul(d5, d4));
	const BoolV con40 = FIsGrtr(zero, va);
	const BoolV con41 = FIsGrtrOrEq(d4, d3);
	const BoolV con42 = FIsGrtrOrEq(d5, d6);
	const BoolV con4 = BAnd(con40, BAnd(con41, con42)); 
	if(BAllEq(con4, bTrue))
	{
		const FloatV uScale = FDiv(unom, FAdd(unom, udenom));
		const Vec3V closest4 = V3ScaleAdd(bc, uScale, b);//V3Add(b, V3Scale(bc, uScale));
		const Vec3V vv = V3Sub(p, closest4);
		return V3Dot(vv, vv);
	}

	//check if p in edge region of AC
	const FloatV vb = FSub(FMul(d5, d2), FMul(d1, d6));
	const BoolV con50 = FIsGrtr(zero, vb);
	const BoolV con51 = FIsGrtrOrEq(d2, zero);
	const BoolV con52 = FIsGrtr(zero, d6);
	const BoolV con5 = BAnd(con50, BAnd(con51, con52));
	if(BAllEq(con5, bTrue))
	{
		const FloatV tScale = FDiv(d2, FSub(d2, d6));
		const Vec3V closest5 = V3ScaleAdd(ac, tScale, a);//V3Add(a, V3Scale(ac, tScale));
		const Vec3V vv = V3Sub(p, closest5);
		return V3Dot(vv, vv);
	}

	//P must project inside face region. Compute Q using Barycentric coordinates
	const FloatV denom = FRecip(FAdd(va, FAdd(vb, vc)));
	const FloatV t = FMul(vb, denom);
	const FloatV w = FMul(vc, denom);
	const Vec3V bCom = V3Scale(ab, t);
	const Vec3V cCom = V3Scale(ac, w);
	const Vec3V closest6 = V3Add(a, V3Add(bCom, cCom));

	const Vec3V vv = V3Sub(p, closest6);

	return V3Dot(vv, vv);
}

const Ps::aos::FloatV invalidateThresholds[5] = {	Ps::aos::FLoad(0.5f), 
													Ps::aos::FLoad(0.125f),
													Ps::aos::FLoad(0.25f),
													Ps::aos::FLoad(0.375f),
													Ps::aos::FLoad(0.375f)	};

const Ps::aos::FloatV invalidateThresholds2[3] = {	Ps::aos::FLoad(0.5f), 
													Ps::aos::FLoad(0.5f),
													Ps::aos::FLoad(0.75f)	};

const Ps::aos::FloatV invalidateQuatThresholds[5] = {	Ps::aos::FLoad(0.9998f), //1 degree
													Ps::aos::FLoad(0.9999f),
													Ps::aos::FLoad(0.9999f),
													Ps::aos::FLoad(0.9999f),//0.8 degree
													Ps::aos::FLoad(0.9999f)	};

const Ps::aos::FloatV invalidateQuatThresholds2[3] = {	Ps::aos::FLoad(0.9995f), //1 degree
													Ps::aos::FLoad(0.9997f),
													Ps::aos::FLoad(0.9997f)	};




}
}


#ifndef __SPU__
#if VISUALIZE_PERSISTENT_CONTACT
#include "CmRenderOutput.h"

static void drawManifoldPoint(const Gu::PersistentContact& manifold,  const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB, Cm::RenderOutput& out, PxU32 color=0xffffff)
{
	PX_UNUSED(color);

	using namespace Ps::aos;
	const Vec3V worldA = trA.transform(manifold.mLocalPointA);
	const Vec3V worldB = trB.transform(manifold.mLocalPointB);
	const Vec3V localNormal = Vec3V_From_Vec4V(manifold.mLocalNormalPen);
	const FloatV pen = V4GetW(manifold.mLocalNormalPen);
	/*const Vec3V localNormal = manifold.localNormal;
	const FloatV pen = manifold.dist;*/
	const Vec3V worldNormal = trB.rotate(localNormal);
	PxVec3 a, b, v;
	V3StoreU(worldA, a);
	V3StoreU(worldB, b);
	V3StoreU(worldNormal, v);
	const PxF32 dist = FStore(pen);
	PxVec3 e = a - v*dist;

	PxF32 size = 0.05f;
	const PxVec3 up(0.f, size, 0.f);
	const PxVec3 right(size, 0.f, 0.f);
	const PxVec3 forwards(0.f, 0.f, size);

	PxF32 size2 = 0.1f;
	const PxVec3 up2(0.f, size2, 0.f);
	const PxVec3 right2(size2, 0.f, 0.f);
	const PxVec3 forwards2(0.f, 0.f, size2);
	/*addLine(a + up, a - up, color);
	addLine(a + right, a - right, color);
	addLine(a + forwards, a - forwards, color);*/

	PxMat44 m = PxMat44::createIdentity();
	//Cm::RenderOutput& out = context->mRenderOutput;
	//out << color << m << Cm::RenderOutput::POINTS << a;
	//out << 0xffff00ff << m << Cm::RenderOutput::POINTS << b;
	out << 0xffff00ff << m << Cm::RenderOutput::LINES  << a << e;
	out << 0xff00ffff << m << Cm::RenderOutput::LINES << a + up << a - up;
	out << 0xff00ffff << m << Cm::RenderOutput::LINES << a + right << a - right;
	out << 0xff00ffff << m << Cm::RenderOutput::LINES << a + forwards << a - forwards;

	out << 0xffff0000 << m << Cm::RenderOutput::LINES << b + up2 << b - up2;
	out << 0xffff0000 << m << Cm::RenderOutput::LINES << b + right2 << b - right2;
	out << 0xffff0000 << m << Cm::RenderOutput::LINES << b + forwards2 << b - forwards2;

	out << 0xffff0000 << m << Cm::RenderOutput::LINES << a << b;

}


static PxU32 gColors[8] = { 0xff0000ff, 0xff00ff00, 0xffff0000,
							0xff00ffff, 0xffff00ff, 0xffffff00,
							0xff000080, 0xff008000};

#endif

#endif


////Transpose version
//Ps::aos::Mat33V Gu::findRotationMatrixFromZAxis(const Ps::aos::Vec3VArg to)
//{
//	using namespace Ps::aos;
//	//const FloatV zero = FZero();
//	const FloatV one = FOne();
//	const FloatV threshold = FloatV_From_F32(0.9999f);
//	//const Vec3V v = V3Merge(FNeg(V3GetY(to)), V3GetX(to), zero);
//	const FloatV vx = FNeg(V3GetY(to));
//	const FloatV vy = V3GetX(to);
//	const FloatV e = V3GetZ(to);
//	const FloatV f = FAbs(e);
//
//	if(FAllGrtr(threshold, f))
//	{
//		const FloatV h = FRecip(FAdd(one, e)); 
//		const FloatV hvx = FMul(h,vx);
//		const FloatV hvxy = FMul(hvx, vy);
//
//		/*const Vec3V col0 = V3Merge(FAdd(e, FMul(hvx, vx)), hvxy, vy);
//		const Vec3V col1 = V3Merge(hvxy, FAdd(e, FMul(h, FMul(vy, vy))), FNeg(vx));
//		const Vec3V col2 = V3Merge(FNeg(vy), vx, e);*/
//
//		const Vec3V col0 = V3Merge(FAdd(e, FMul(hvx, vx)), hvxy,							FNeg(vy));
//		const Vec3V col1 = V3Merge(hvxy,				   FAdd(e, FMul(h, FMul(vy, vy))),  vx);
//		const Vec3V col2 = V3Merge(vy,					   FNeg(vx),						e);
//
//		return Mat33V(col0, col1, col2);
//
//	}
//	else
//	{
//		const FloatV two = FloatV_From_F32(2.f);
//		const Vec3V from = V3UnitZ();
//		const Vec3V absFrom = V3UnitY();
//
//		const Vec3V u = V3Sub(absFrom, from);
//		const Vec3V v = V3Sub(absFrom, to);
//
//		const FloatV dotU = V3Dot(u, u);
//		const FloatV dotV = V3Dot(v, v);
//		const FloatV dotUV = V3Dot(u, v);
//
//		const FloatV c1 = FNeg(FMul(two, FRecip(dotU)));
//		const FloatV c2 = FNeg(FMul(two, FRecip(dotV)));
//		const FloatV c3 = FMul(c1, FMul(c2, dotUV));
//
//		const FloatV vx = V3GetX(v);
//		FloatV temp0 = FMul(c1, V3GetX(u));
//		FloatV temp1 = FMul(c2, vx);
//		FloatV temp2 = FMul(c3, vx);
//
//		Vec3V col0 = V3Add(V3Scale(u, temp0), V3Add(V3Scale(v, temp1), V3Scale(u, temp2)));
//		col0 = V3SetX(col0, FAdd(V3GetX(col0), one));
//
//		const FloatV vy = V3GetY(v);
//		temp0 = FMul(c1, V3GetY(u));
//		temp1 = FMul(c2, vy);
//		temp2 = FMul(c3, vy);
//
//		Vec3V col1 = V3Add(V3Scale(u, temp0), V3Add(V3Scale(v, temp1), V3Scale(u, temp2)));
//		col1 = V3SetY(col1, FAdd(V3GetY(col1), one));
//
//		const FloatV vz = V3GetZ(v);
//		temp0 = FMul(c1, V3GetZ(u));
//		temp1 = FMul(c2, vz);
//		temp2 = FMul(c3, vz);
//
//		Vec3V col2 = V3Add(V3Scale(u, temp0), V3Add(V3Scale(v, temp1), V3Scale(u, temp2)));
//		col2 = V3SetZ(col2, FAdd(V3GetZ(col2), one));
//
//		return M33Trnsps(Mat33V(col0, col1, col2));
//	}
//}


Ps::aos::Mat33V Gu::findRotationMatrixFromZAxis(const Ps::aos::Vec3VArg to)
{
	using namespace Ps::aos;
	//const FloatV zero = FZero();
	const FloatV one = FOne();
	const FloatV threshold = FLoad(0.9999f);
	//const Vec3V v = V3Merge(FNeg(V3GetY(to)), V3GetX(to), zero);
	
	const FloatV e = V3GetZ(to);
	const FloatV f = FAbs(e);

	if(FAllGrtr(threshold, f))
	{
		//Vec3 v = V3Cross(V3UnitZ(), to);, v(-t0.y, to.x, 0)
		const FloatV vx = FNeg(V3GetY(to));
		const FloatV vy = V3GetX(to);
		const FloatV h = FRecip(FAdd(one, e)); 
		const FloatV hvx = FMul(h,vx);
		const FloatV hvxy = FMul(hvx, vy);

		/*const Vec3V col0 = V3Merge(FAdd(e, FMul(hvx, vx)), hvxy, vy);
		const Vec3V col1 = V3Merge(hvxy, FAdd(e, FMul(h, FMul(vy, vy))), FNeg(vx));
		const Vec3V col2 = V3Merge(FNeg(vy), vx, e);*/

		const Vec3V col0 = V3Merge(FMulAdd(hvx, vx, e),		hvxy,							      vy);
		const Vec3V col1 = V3Merge(hvxy,				   FMulAdd(h, FMul(vy, vy), e),			FNeg(vx));
		const Vec3V col2 = V3Merge(FNeg(vy),					   vx,							  e);

		return Mat33V(col0, col1, col2);

	}
	else
	{
		//Mat33V tmp;
		//{
		//	const FloatV two = FloatV_From_F32(2.f);
		//	const Vec3V from = V3UnitZ();
		//	const Vec3V absFrom = V3UnitY();

		//	const Vec3V u = V3Sub(absFrom, from);
		//	const Vec3V v = V3Sub(absFrom, to);

		//	const FloatV dotU = V3Dot(u, u);
		//	const FloatV dotV = V3Dot(v, v);
		//	const FloatV dotUV = V3Dot(u, v);

		//	const FloatV c1 = FNeg(FMul(two, FRecip(dotU)));
		//	const FloatV c2 = FNeg(FMul(two, FRecip(dotV)));
		//	const FloatV c3 = FMul(c1, FMul(c2, dotUV));

		//	const FloatV vx = V3GetX(v);
		//	FloatV temp0 = FMul(c1, V3GetX(u));
		//	FloatV temp1 = FMul(c2, vx);
		//	FloatV temp2 = FMul(c3, vx);

		//	Vec3V col0 = V3Add(V3Scale(u, temp0), V3Add(V3Scale(v, temp1), V3Scale(u, temp2)));
		//	col0 = V3SetX(col0, FAdd(V3GetX(col0), one));

		//	const FloatV vy = V3GetY(v);
		//	temp0 = FMul(c1, V3GetY(u));
		//	temp1 = FMul(c2, vy);
		//	temp2 = FMul(c3, vy);

		//	Vec3V col1 = V3Add(V3Scale(u, temp0), V3Add(V3Scale(v, temp1), V3Scale(u, temp2)));
		//	col1 = V3SetY(col1, FAdd(V3GetY(col1), one));

		//	const FloatV vz = V3GetZ(v);
		//	temp0 = FMul(c1, V3GetZ(u));
		//	temp1 = FMul(c2, vz);
		//	temp2 = FMul(c3, vz);

		//	Vec3V col2 = V3Add(V3Scale(u, temp0), V3Add(V3Scale(v, temp1), V3Scale(u, temp2)));
		//	col2 = V3SetZ(col2, FAdd(V3GetZ(col2), one));

		//	//return M33Trnsps(Mat33V(col0, col1, col2));
		//	tmp = Mat33V(col0, col1, col2);
		//}

		const FloatV two = FLoad(2.f);
		const Vec3V from = V3UnitZ();
		const Vec3V absFrom = V3UnitY();

		const Vec3V u = V3Sub(absFrom, from);
		const Vec3V v = V3Sub(absFrom, to);

		const FloatV dotU = V3Dot(u, u);
		const FloatV dotV = V3Dot(v, v);
		const FloatV dotUV = V3Dot(u, v);

		const FloatV c1 = FNeg(FDiv(two, dotU));
		const FloatV c2 = FNeg(FDiv(two, dotV));
		const FloatV c3 = FMul(c1, FMul(c2, dotUV));

		const Vec3V c1u = V3Scale(u, c1);
		const Vec3V c2v = V3Scale(v, c2);
		const Vec3V c3v = V3Scale(v, c3);

		//const FloatV vx = V3GetX(v);
		/*FloatV temp0 = FMul(c1, V3GetX(u));
		FloatV temp1 = FMul(c2, vx);
		FloatV temp2 = FMul(c3, vx);*/

		FloatV temp0 = V3GetX(c1u);
		FloatV temp1 = V3GetX(c2v);
		FloatV temp2 = V3GetX(c3v);

		/*Vec3V col0 = V3Add(V3Scale(u, temp0), V3Add(V3Scale(v, temp1), V3Scale(u, temp2)));*/
		Vec3V col0 = V3ScaleAdd(u, temp0, V3ScaleAdd(v, temp1, V3Scale(u, temp2)));
		col0 = V3SetX(col0, FAdd(V3GetX(col0), one));

		temp0 = V3GetY(c1u);
		temp1 = V3GetY(c2v);
		temp2 = V3GetY(c3v);

		//Vec3V col1 = V3Add(V3Scale(u, temp0), V3Add(V3Scale(v, temp1), V3Scale(u, temp2)));
		Vec3V col1 = V3ScaleAdd(u, temp0, V3ScaleAdd(v, temp1, V3Scale(u, temp2)));
		col1 = V3SetY(col1, FAdd(V3GetY(col1), one));

		temp0 = V3GetZ(c1u);
		temp1 = V3GetZ(c2v);
		temp2 = V3GetZ(c3v);

		//Vec3V col2 = V3Add(V3Scale(u, temp0), V3Add(V3Scale(v, temp1), V3Scale(u, temp2)));
		Vec3V col2 = V3ScaleAdd(u, temp0, V3ScaleAdd(v, temp1, V3Scale(u, temp2)));
		col2 = V3SetZ(col2, FAdd(V3GetZ(col2), one));

		//return M33Trnsps(Mat33V(col0, col1, col2));
		return Mat33V(col0, col1, col2);

	}
}


void Gu::PersistentContactManifold::drawManifold( Cm::RenderOutput& out, const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB)
{
	using namespace Ps::aos;
#if VISUALIZE_PERSISTENT_CONTACT

	PxVec3 a, b;
	V3StoreU(trA.p, a);
	V3StoreU(trB.p, b);

	//PxMat44 m = PxMat44::createIdentity();
	//Cm::RenderOutput& out = context->mRenderOutput;
	/*out << color << m << Cm::RenderOutput::POINTS << a;
	out << 0xffff00ff << m << Cm::RenderOutput::POINTS << b;*/
	//out << (bHit ? 0xffff00ff : 0xff00ffff) << m << Cm::RenderOutput::LINES  << a << b;

	for(PxU32 i = 0; i< mNumContacts; ++i)
	{
		Gu::PersistentContact& m = mContactPoints[i];
		drawManifoldPoint(m, trA, trB, out, gColors[i]);
	}
#endif
}

void Gu::PersistentContactManifold::drawManifold(const Gu::PersistentContact& m, Cm::RenderOutput& out, const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB)
{
#if VISUALIZE_PERSISTENT_CONTACT
	drawManifoldPoint(m, trA, trB, out, gColors[0]);
#endif
}

void Gu::PersistentContactManifold::drawPoint(Cm::RenderOutput& out, const Ps::aos::Vec3VArg p, const PxF32 size, const PxU32 color)
{
	using namespace Ps::aos;
#if VISUALIZE_PERSISTENT_CONTACT
	const PxVec3 up(0.f, size, 0.f);
	const PxVec3 right(size, 0.f, 0.f);
	const PxVec3 forwards(0.f, 0.f, size);

	PxVec3 a;
	V3StoreU(p, a);

	PxMat44 m = PxMat44::createIdentity();
	
	out << color << m << Cm::RenderOutput::LINES << a + up << a - up;
	out << color << m << Cm::RenderOutput::LINES << a + right << a - right;
	out << color << m << Cm::RenderOutput::LINES << a + forwards << a - forwards;
#endif
}

void Gu::PersistentContactManifold::drawLine(Cm::RenderOutput& out, const Ps::aos::Vec3VArg p0, const Ps::aos::Vec3VArg p1, const PxU32 color)
{
	using namespace Ps::aos;
#if VISUALIZE_PERSISTENT_CONTACT
	PxVec3 a, b;
	V3StoreU(p0, a);
	V3StoreU(p1, b);

	PxF32 size = 0.05f;
	const PxVec3 up(0.f, size, 0.f);
	const PxVec3 right(size, 0.f, 0.f);
	const PxVec3 forwards(0.f, 0.f, size);

	PxMat44 m = PxMat44::createIdentity();
	out << color << m << Cm::RenderOutput::LINES << a << b;
#endif
}

void Gu::PersistentContactManifold::drawPolygon( Cm::RenderOutput& out, const Ps::aos::PsTransformV& transform,  Ps::aos::Vec3V* points, const PxU32 numVerts, const PxU32 color)
{
	using namespace Ps::aos;
#if VISUALIZE_PERSISTENT_CONTACT
	for(PxU32 i=0; i<numVerts; ++i)
	{
		Vec3V tempV0 = points[i == 0 ? numVerts-1 : i-1];
		Vec3V tempV1 = points[i];
		
		drawLine(out, transform.transform(tempV0), transform.transform(tempV1), color);
	}
#endif

}

bool Gu::PersistentContactManifold::replaceManifoldPoint(const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalPen
														, const Ps::aos::FloatVArg replaceBreakingThreshold)
{
	using namespace Ps::aos;
	//const FloatV breakingThreshold = FloatV_From_F32(replaceBreakingThreshold);
	const FloatV shortestDist =  FMul(replaceBreakingThreshold, replaceBreakingThreshold); 
	
	for( PxU32 i = 0; i < mNumContacts; ++i )
	{
		const PersistentContact &mp = mContactPoints[i];

		const Vec3V diffB =  V3Sub(mp.mLocalPointB, localPointB);
		const FloatV sqDif = V3Dot(diffB, diffB);

		if(FAllGrtr(shortestDist, sqDif))
		{
			mContactPoints[i].mLocalPointA = localPointA;
			mContactPoints[i].mLocalPointB = localPointB;
			mContactPoints[i].mLocalNormalPen = localNormalPen;
			return true;
		}
	}
	return false;
}

bool Gu::PersistentContactManifold::replaceManifoldPoint2(const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalPen
														, const Ps::aos::FloatVArg replaceBreakingThreshold)
{
	using namespace Ps::aos;
	//const FloatV breakingThreshold = FloatV_From_F32(replaceBreakingThreshold);
	const FloatV shortestDist =  FMul(replaceBreakingThreshold, replaceBreakingThreshold); 
	
	for( PxU32 i = 0; i < mNumContacts; ++i )
	{
		const PersistentContact &mp = mContactPoints[i];

		const Vec3V diffB =  V3Sub(mp.mLocalPointA, localPointA);
		const FloatV sqDif = V3Dot(diffB, diffB);

		if(FAllGrtr(shortestDist, sqDif))
		{
			mContactPoints[i].mLocalPointA = localPointA;
			mContactPoints[i].mLocalPointB = localPointB;
			mContactPoints[i].mLocalNormalPen = localNormalPen;
			return true;
		}
	}
	return false;
}


PxU32 Gu::PersistentContactManifold::reduceContactSegment(const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalPen)
{
	using namespace Ps::aos;
	const Vec3V p  = localPointB;
	const Vec3V p0 = mContactPoints[0].mLocalPointB;
	const Vec3V p1 = mContactPoints[1].mLocalPointB;
	const Vec3V v0 = V3Sub(p0, p);
	const Vec3V v1 = V3Sub(p1, p);
	const FloatV dist0 = V3Dot(v0, v0);
	const FloatV dist1 = V3Dot(v1, v1);
	if(FAllGrtr(dist0, dist1))
	{
		mContactPoints[1].mLocalPointA = localPointA;
		mContactPoints[1].mLocalPointB = localPointB;
		mContactPoints[1].mLocalNormalPen = localNormalPen;
	}
	else
	{
		mContactPoints[0].mLocalPointA = localPointA;
		mContactPoints[0].mLocalPointB = localPointB;
		mContactPoints[0].mLocalNormalPen = localNormalPen;
	}

	return 0;
}



PxU32 Gu::PersistentContactManifold::reduceContactsForPCM(const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalPen)
{
	using namespace Ps::aos;


	bool chosen[5];
	physx::PxMemZero(chosen, sizeof(bool)*5);
	const FloatV zero = FZero();


	PersistentContact tempContacts[5];
	
	for(PxU32 i=0; i<4; ++i)
	{
		tempContacts[i] = mContactPoints[i];
	}
	tempContacts[4].mLocalPointA = localPointA;
	tempContacts[4].mLocalPointB = localPointB;
	tempContacts[4].mLocalNormalPen = localNormalPen;

	FloatV maxDist =V4GetW(localNormalPen);
	PxI32 index = 4;
	//Choose deepest point
	for(PxU32 i=0; i<4; ++i)
	{
		const FloatV pen = V4GetW(tempContacts[i].mLocalNormalPen);
		if(FAllGrtr(maxDist, pen))
		{
			maxDist = pen;
			index = i;
		}
	}

	chosen[index] = true;
	mContactPoints[0] = tempContacts[index];

	Vec3V dir= V3Sub(tempContacts[0].mLocalPointB, mContactPoints[0].mLocalPointB);
	maxDist = V3Dot(dir, dir);
	index = 0;

	for(PxU32 i=1; i<5; ++i)
	{
		if(!chosen[i])
		{
			dir = V3Sub(tempContacts[i].mLocalPointB, mContactPoints[0].mLocalPointB);
			const FloatV d = V3Dot(dir, dir);
			if(FAllGrtr(d, maxDist))
			{
				maxDist = d;
				index = i;
			}
		}
	}

	PX_ASSERT(chosen[index] == false);
	chosen[index] = true;
	mContactPoints[1] = tempContacts[index];

	maxDist = zero;	
	for(PxU32 i=0; i<5; ++i)
	{
		if(!chosen[i])
		{
			const FloatV sqDif = distancePointSegmentSquaredLocal(mContactPoints[0].mLocalPointB, mContactPoints[1].mLocalPointB, tempContacts[i].mLocalPointB);
			if(FAllGrtr(sqDif, maxDist))
			{
				maxDist = sqDif;
				index = i;
			}
		}
	}
	PX_ASSERT(chosen[index] == false);
	chosen[index] = true;
	mContactPoints[2]=tempContacts[index];

	//Find point farthest away from segment tempContactPoints[0] - tempContactPoints[1]
	maxDist = zero;
	for(PxU32 i=0; i<5; ++i)
	{
		if(!chosen[i])
		{
			const FloatV sqDif = distancePointTriangleSquaredLocal(	tempContacts[i].mLocalPointB, mContactPoints[0].mLocalPointB, mContactPoints[1].mLocalPointB, mContactPoints[2].mLocalPointB); 
			if(FAllGrtr(sqDif, maxDist))
			{
				maxDist= sqDif;
				index = i;
			}
		}
	}

	//PX_ASSERT(chosen[index] == false);
	if(chosen[index] == true)
	{
		//if we don't have any new contacts, which means the leftover contacts are inside the triangles
		mNumContacts = 3;
		return 0;
	}
	else
	{
		chosen[index] = true;
		mContactPoints[3]=tempContacts[index];
	}

	//Final pass, we work out the index that we didn't choose and bind it to its closest point. We then consider whether we want to swap the point if the
	//point we were about to discard is deeper...

	PxU32 notChosenIndex = 0;
	for(PxU32 a = 0; a < 5; ++a)
	{
		if(!chosen[a])
		{
			notChosenIndex = a;
			break;
		}
	}

	FloatV closest = FMax();
	index = 0;
	for(PxU32 a = 0; a < 4; ++a)
	{
		Vec3V dif = V3Sub(mContactPoints[a].mLocalPointA, tempContacts[notChosenIndex].mLocalPointA);
		const FloatV d2 = V3Dot(dif, dif);
		if(FAllGrtr(closest, d2))
		{
			closest = d2;
			index = a;
		}
	}

	if(FAllGrtr(V4GetW(mContactPoints[index].mLocalNormalPen), V4GetW(tempContacts[notChosenIndex].mLocalNormalPen)))
	{
		//Swap
		mContactPoints[index] = tempContacts[notChosenIndex];
	}


	return 0;
}



void Gu::PersistentContactManifold::addManifoldContactsToContactBuffer(Gu::ContactBuffer& contactBuffer, const Ps::aos::Vec3VArg normal, const Ps::aos::PsTransformV& transf1)
{
	using namespace Ps::aos;

	//add the manifold contacts;
	PxU32 contactCount = 0;//contactBuffer.count;
	for(PxU32 i=0; (i< mNumContacts) & (contactCount < Gu::ContactBuffer::MAX_CONTACTS); ++i)
	{
		PersistentContact& p = getContactPoint(i);
		
		const Vec3V worldP =transf1.transform(p.mLocalPointB);
		const FloatV dist = V4GetW(p.mLocalNormalPen);

		Gu::ContactPoint& contact = contactBuffer.contacts[contactCount++];
		//Fast allign store
		V4StoreA(Vec4V_From_Vec3V(normal), (PxF32*)&contact.normal.x);
		//F32Array_Aligned_From_Vec4V(Vec4V_From_Vec3V(V3Neg(transf1.rotate(Vec3V_From_Vec4V(p.localNormalPen)))), (PxF32*)&contact.normal.x);
		//F32Array_Aligned_From_Vec4V(Vec4V_From_Vec3V(worldN), (PxF32*)&contact.normal.x);
		V4StoreA(Vec4V_From_Vec3V(worldP), (PxF32*)&contact.point.x);
		FStore(dist, &contact.separation);

		contact.internalFaceIndex0 = PXC_CONTACT_NO_FACE_INDEX;
		contact.internalFaceIndex1 = PXC_CONTACT_NO_FACE_INDEX;

		
	}
	//manifold.drawManifold(&context, transf0, transf1, status == GJK_CONTACT);
	contactBuffer.count = contactCount;
}

void Gu::PersistentContactManifold::addManifoldContactsToContactBuffer(Gu::ContactBuffer& contactBuffer, const Ps::aos::Vec3VArg normal, const Ps::aos::PsTransformV& transf0, const Ps::aos::FloatVArg radius)
{
	using namespace Ps::aos;

	//add the manifold contacts;
	PxU32 contactCount = 0;//contactBuffer.count;
	for(PxU32 i=0; (i< mNumContacts) & (contactCount < Gu::ContactBuffer::MAX_CONTACTS); ++i)
	{
		PersistentContact& p = getContactPoint(i);
		
		/*const Vec3V worldP =transf1.transform(p.mLocalPointB);
		const FloatV dist = V4GetW(p.mLocalNormalPen);*/

		const Vec3V worldP =V3NegScaleSub(normal, radius, transf0.transform(p.mLocalPointA));
		const FloatV dist = FSub(V4GetW(p.mLocalNormalPen), radius);

		Gu::ContactPoint& contact = contactBuffer.contacts[contactCount++];
		//Fast allign store
		V4StoreA(Vec4V_From_Vec3V(normal), (PxF32*)&contact.normal.x);
		//F32Array_Aligned_From_Vec4V(Vec4V_From_Vec3V(V3Neg(transf1.rotate(Vec3V_From_Vec4V(p.localNormalPen)))), (PxF32*)&contact.normal.x);
		//F32Array_Aligned_From_Vec4V(Vec4V_From_Vec3V(worldN), (PxF32*)&contact.normal.x);
		V4StoreA(Vec4V_From_Vec3V(worldP), (PxF32*)&contact.point.x);
		FStore(dist, &contact.separation);
		//PX_ASSERT(PxAbs(contact.separation) < 2.f);

		contact.internalFaceIndex0 = PXC_CONTACT_NO_FACE_INDEX;
		contact.internalFaceIndex1 = PXC_CONTACT_NO_FACE_INDEX;

		
	}
	//manifold.drawManifold(&context, transf0, transf1, status == GJK_CONTACT);
	contactBuffer.count = contactCount;
}


void Gu::PersistentContactManifold::addBatchManifoldContacts(const PersistentContact* manifoldContacts, const PxU32 numPoints)
{

	if(numPoints <= 4)
	{
		for(PxU32 i=0; i<numPoints; ++i)
		{
			mContactPoints[i].mLocalPointA =  manifoldContacts[i].mLocalPointA;
			mContactPoints[i].mLocalPointB =  manifoldContacts[i].mLocalPointB;
			mContactPoints[i].mLocalNormalPen =  manifoldContacts[i].mLocalNormalPen;
		}
		mNumContacts = Ps::to8(numPoints);
	}
	else
	{
		reduceBatchContacts(manifoldContacts, numPoints);
		mNumContacts = 4;
	}
}


void Gu::PersistentContactManifold::removeDuplidateManifoldContacts(physx::Gu::PersistentContact *manifoldPoints, physx::PxU32& numPoints, const physx::shdfnd::aos::FloatVArg replaceBreakingThreshold)
{
	using namespace Ps::aos;
	const FloatV mSqReplaceBreakingThreshold = FMul(replaceBreakingThreshold, replaceBreakingThreshold);
	for(PxU32 i = 0; i<numPoints; ++i)
	{
		for(PxU32 j=i+1; j<numPoints; ++j)
		{
			const Vec3V dif = V3Sub(manifoldPoints[j].mLocalPointB, manifoldPoints[i].mLocalPointB);
			const FloatV d = V3Dot(dif, dif);
			if(FAllGrtr(mSqReplaceBreakingThreshold, d))
			{
				manifoldPoints[j] = manifoldPoints[numPoints-1];
				numPoints--;
				j--;
			}
		}
	}
}


void Gu::PersistentContactManifold::reduceBatchContacts(const PersistentContact* manifoldPoints, const PxU32 numPoints)
{
	using namespace Ps::aos;
	//get the deepest points

	//PX_ALLOCA(chosen, bool, numPoints);
	//bool* chosen = (bool*)PxAlloca(sizeof(bool)*numPoints);
	bool chosen[64];// = (bool*)PxAlloca(sizeof(bool)*numPoints);
	physx::PxMemZero(chosen, sizeof(bool)*numPoints);
	FloatV max = FMax();
	//FloatV maxDis = max;
	FloatV maxDis = V4GetW(manifoldPoints[0].mLocalNormalPen);
	PxI32 index = 0;
	//keep the deepest point
	for(PxU32 i=1; i<numPoints; ++i)
	{
		const FloatV pen = V4GetW(manifoldPoints[i].mLocalNormalPen);
		if(FAllGrtr(maxDis, pen))
		{
			maxDis = pen;
			index = i;
		}
	}
	//keep the deepest points in the first position
	mContactPoints[0] =  manifoldPoints[index];
	chosen[index] = true;


	//calculate the furthest away points
	Vec3V v = V3Sub(manifoldPoints[0].mLocalPointB, mContactPoints[0].mLocalPointB);
	maxDis = V3Dot(v, v);
	index = 0;

	for(PxU32 i=1; i<numPoints; ++i)
	{
		v = V3Sub(manifoldPoints[i].mLocalPointB, mContactPoints[0].mLocalPointB);
		const FloatV d = V3Dot(v, v);
		if(FAllGrtr(d, maxDis))
		{
			maxDis = d;
			index = i;
		}
	}

	PX_ASSERT(chosen[index] == false);
	mContactPoints[1] =  manifoldPoints[index];
	chosen[index] = true;


	maxDis = FNeg(max);//FNeg(FMax());
	index = -1;
	v = V3Sub(mContactPoints[1].mLocalPointB, mContactPoints[0].mLocalPointB);
	Vec3V norm = V3Normalize(V3Cross(v, Vec3V_From_Vec4V(mContactPoints[0].mLocalNormalPen)));

	FloatV minDis = max;
	PxI32 index1 = -1;
	

	//calculate the min and max point away from the segment
	for(PxU32 i=0; i<numPoints; ++i)
	{
		if(!chosen[i])
		{
			v = V3Sub(manifoldPoints[i].mLocalPointB, mContactPoints[0].mLocalPointB);
			const FloatV d = V3Dot(v, norm);
			if(FAllGrtr(d, maxDis))
			{
				maxDis = d;
				index = i;
			}

			if(FAllGrtr(minDis, d))
			{
				minDis = d;
				index1 = i;
			}
		}
	}

	PX_ASSERT(chosen[index] == false && chosen[index1] == false);
	mContactPoints[2] =  manifoldPoints[index];
	chosen[index] = true;

	//if min and max in the same side, chose again
	const FloatV temp = FMul(minDis, maxDis);
	if(FAllGrtr(temp, FZero()))
	{
		//chose again
		maxDis = FNeg(max);
		for(PxU32 i=0; i<numPoints; ++i)
		{
			if(!chosen[i])
			{
				v = V3Sub(manifoldPoints[i].mLocalPointB, mContactPoints[0].mLocalPointB);
				const FloatV d = V3Dot(v, norm);
				if(FAllGrtr(d, maxDis))
				{
					maxDis = d;
					index1 = i;
				}
			}
		}
	}

	mContactPoints[3] = manifoldPoints[index1];
	//chosen[index1] = true;
}

//After the reduction, it will keep two contacts
void Gu::PersistentContactManifold::reduceBatchContacts2(const PersistentContact* manifoldPoints, const PxU32 numPoints)  
{
	using namespace Ps::aos;
	//get the deepest points

	//PX_ALLOCA(chosen, bool, numPoints);
	//bool* chosen = (bool*)PxAlloca(sizeof(bool)*numPoints);
	bool chosen[64];// = (bool*)PxAlloca(sizeof(bool)*numPoints);
	physx::PxMemZero(chosen, sizeof(bool)*numPoints);
	//FloatV max = FMax();
	FloatV maxDis = V4GetW(manifoldPoints[0].mLocalNormalPen);
	PxI32 index = 0;
	//keep the deepest point
	for(PxU32 i=1; i<numPoints; ++i)
	{
		const FloatV pen = V4GetW(manifoldPoints[i].mLocalNormalPen);
		if(FAllGrtr(maxDis, pen))
		{
			maxDis = pen;
			index = i;
		}
	}
	//keep the deepest points in the first position
	mContactPoints[0] =  manifoldPoints[index];
	chosen[index] = true;


	//calculate the furthest away points
	Vec3V v = V3Sub(manifoldPoints[0].mLocalPointB, mContactPoints[0].mLocalPointB);
	maxDis = V3Dot(v, v);
	index = 0;

	for(PxU32 i=1; i<numPoints; ++i)
	{
		v = V3Sub(manifoldPoints[i].mLocalPointB, mContactPoints[0].mLocalPointB);
		const FloatV d = V3Dot(v, v);
		if(FAllGrtr(d, maxDis))
		{
			maxDis = d;
			index = i;
		}
	}

	//PX_ASSERT(chosen[index] == false);
	mContactPoints[1] =  manifoldPoints[index];
	chosen[index] = true;

	PxI32 secondIndex = index;
	FloatV maxDepth = V4GetW(manifoldPoints[index].mLocalNormalPen);;
	for(PxU32 i = 0; i < numPoints; ++i)
	{
		if(!chosen[i])
		{
			Vec3V d0 = V3Sub(mContactPoints[0].mLocalPointB, manifoldPoints[i].mLocalPointB);
			Vec3V d1 = V3Sub(mContactPoints[1].mLocalPointB, manifoldPoints[i].mLocalPointB);
			const FloatV dd0 = V3Dot(d0, d0);
			const FloatV dd1 = V3Dot(d1, d1);

			if(FAllGrtr(dd0, dd1))
			{
				//This clusters to point 1
				if(FAllGrtr(maxDepth, V4GetW(manifoldPoints[i].mLocalNormalPen)))
				{
					secondIndex = i;
				}
			}
		}
	}

	if(secondIndex != index)
	{
		mContactPoints[1] = manifoldPoints[secondIndex];
	}	
}

PxU32 Gu::PersistentContactManifold::addManifoldPoint(const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalPen
													 , const Ps::aos::FloatVArg replaceBreakingThreshold)
{
	using namespace Ps::aos;
	
	if(replaceManifoldPoint(localPointA, localPointB, localNormalPen, replaceBreakingThreshold)) //replace the new point with the old one
		return 0;

	switch(mNumContacts)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		mContactPoints[mNumContacts].mLocalPointA = localPointA;
		mContactPoints[mNumContacts].mLocalPointB = localPointB;
		mContactPoints[mNumContacts++].mLocalNormalPen = localNormalPen;

		return 1;
	//case 2:
	//	return ReduceContactSegment(localPointA, localPointB, localNormalPen);
	//case 3:
	//	return ReduceContactTriangle(localPointA, localPointB, localNormalPen);
	default:
		return reduceContactsForPCM(localPointA, localPointB, localNormalPen);//should be always return zero
	};

	//if(numContacts < 64)
	//{
	//	contactPoints[numContacts].localPointA = localPointA;
	//	contactPoints[numContacts].localPointB = localPointB;
	//	contactPoints[numContacts++].localNormalPen = localNormalPen;    
	//	return 1;
	//}
	return 0;

}



PxU32 Gu::PersistentContactManifold::addManifoldPoint2(const Ps::aos::Vec3VArg localPointA, const Ps::aos::Vec3VArg localPointB, const Ps::aos::Vec4VArg localNormalPen
													  , const Ps::aos::FloatVArg replaceBreakingThreshold)
{
	using namespace Ps::aos;
	
	if(replaceManifoldPoint(localPointA, localPointB, localNormalPen, replaceBreakingThreshold)) //replace the new point with the old one
		return 0;

	switch(mNumContacts)
	{
	case 0:
	case 1:      
		mContactPoints[mNumContacts].mLocalPointA = localPointA;
		mContactPoints[mNumContacts].mLocalPointB = localPointB;
		mContactPoints[mNumContacts++].mLocalNormalPen = localNormalPen;
		return 1;
	case 2:
		return reduceContactSegment(localPointA, localPointB, localNormalPen);
	default:
		PX_ASSERT(0);
	};
	return 0;

//	PX_ASSERT(numContacts <= 2 && numContacts > 0);
}

void Gu::PersistentContactManifold::addBatchManifoldContacts2(const PersistentContact* manifoldContacts, const PxU32 numPoints)
{
	using namespace Ps::aos;
	
	if(numPoints <= 2)
	{
		for(PxU32 i=0; i<numPoints; ++i)
		{
			mContactPoints[i].mLocalPointA =  manifoldContacts[i].mLocalPointA;
			mContactPoints[i].mLocalPointB =  manifoldContacts[i].mLocalPointB;
			mContactPoints[i].mLocalNormalPen =  manifoldContacts[i].mLocalNormalPen;
		}

		mNumContacts = Ps::to8(numPoints);
	}
	else
	{
		reduceBatchContacts2(manifoldContacts, numPoints);
		mNumContacts = 2;
	}

//	PX_ASSERT(numContacts <= 2 && numContacts > 0);
}


Ps::aos::FloatV Gu::SinglePersistentContactManifold::addBatchManifoldContactsConvex(const PersistentContact* manifoldContact, const PxU32 numContactExt, PCMContactPatch& patch, const Ps::aos::FloatVArg replaceBreakingThreshold)
{
	PX_UNUSED(replaceBreakingThreshold);

	using namespace Ps::aos;
	
	if(patch.mTotalSize <= GU_SINGLE_MANIFOLD_CACHE_SIZE)
	{
		PCMContactPatch* currentPatch = &patch;

		//this is because we already add the manifold contacts into manifoldContact array
		PxU32 tempNumContacts = 0;
		while(currentPatch)
		{
			for(PxU32 j=currentPatch->mStartIndex; j<currentPatch->mEndIndex; ++j)
			{
				mContactPoints[tempNumContacts].mLocalPointA =  manifoldContact[j].mLocalPointA;
				mContactPoints[tempNumContacts].mLocalPointB =  manifoldContact[j].mLocalPointB;
				mContactPoints[tempNumContacts++].mLocalNormalPen =  manifoldContact[j].mLocalNormalPen;
			}
			currentPatch = currentPatch->mNextPatch;
		} 
		mNumContacts = tempNumContacts;
		return patch.mPatchMaxPen;
	}
	else
	{
		
		const FloatV maxPen = reduceBatchContactsConvex(manifoldContact, numContactExt, patch);
		mNumContacts = GU_SINGLE_MANIFOLD_CACHE_SIZE;
		return maxPen;
	}
}

Ps::aos::FloatV Gu::SinglePersistentContactManifold::addBatchManifoldContactsSphere(const PersistentContact* manifoldContact, const PxU32 numContactExt, PCMContactPatch& patch, const Ps::aos::FloatVArg replaceBreakingThreshold)
{
	PX_UNUSED(replaceBreakingThreshold);

	using namespace Ps::aos;

	const FloatV maxPen = reduceBatchContactsSphere(manifoldContact, numContactExt, patch);
	mNumContacts = 1;
	return maxPen;
}

Ps::aos::FloatV Gu::SinglePersistentContactManifold::addBatchManifoldContactsCapsule(const PersistentContact* manifoldContact, const PxU32 numContactExt, PCMContactPatch& patch, const Ps::aos::FloatVArg replaceBreakingThreshold)
{
	PX_UNUSED(replaceBreakingThreshold);

	using namespace Ps::aos;

	if(patch.mTotalSize <=GU_CAPSULE_MANIFOLD_CACHE_SIZE)
	{
		PCMContactPatch* currentPatch = &patch;

		//this is because we already add the manifold contacts into manifoldContact array
		PxU32 tempNumContacts = 0;
		while(currentPatch)
		{
			for(PxU32 j=currentPatch->mStartIndex; j<currentPatch->mEndIndex; ++j)
			{
				mContactPoints[tempNumContacts].mLocalPointA =  manifoldContact[j].mLocalPointA;
				mContactPoints[tempNumContacts].mLocalPointB =  manifoldContact[j].mLocalPointB;
				mContactPoints[tempNumContacts++].mLocalNormalPen =  manifoldContact[j].mLocalNormalPen;
			}
			currentPatch = currentPatch->mNextPatch;
		} 
		mNumContacts = tempNumContacts;
		return patch.mPatchMaxPen;
	}
	else
	{
		
		const FloatV maxPen = reduceBatchContactsCapsule(manifoldContact, numContactExt, patch);
		mNumContacts = GU_CAPSULE_MANIFOLD_CACHE_SIZE;
		return maxPen;
	}

}

//void Gu::SinglePersistentContactManifold::reduceBatchContactsKeepAllDeepestContacts(const PersistentContact* manifoldContactExt, const PxU32 numContacts, PCMContactPatch& patch)
//{
//	using namespace Ps::aos;
//	bool* chosen = (bool*)PxAlloca(sizeof(bool)*numContacts);
//	physx::PxMemZero(chosen, sizeof(bool)*numContacts);
//	FloatV max = FMax();
//	FloatV maxDist = max;
//	PxU32 index = 0;
//
//	for(PxU32 i= 0; i< GU_SINGLE_MANIFOLD_CACHE_SIZE; ++i)
//	{
//		PCMContactPatch* currentPatch = &patch;
//		while(currentPatch)
//		{
//			for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
//			{
//				if(!chosen[i])
//				{
//					const FloatV pen = V4GetW(manifoldContactExt[i].mLocalNormalPen);
//					if(FAllGrtr(maxDist, pen))
//					{
//						maxDist = pen;
//						index = i;
//					}
//				}
//			}
//			currentPatch = currentPatch->mNextPatch;
//		}
//
//		chosen[index] = true;
//		maxDist = max;
//		mContactPoints[i] = manifoldContactExt[index];
//	}
//
//}

Ps::aos::FloatV Gu::SinglePersistentContactManifold::reduceBatchContactsSphere(const PersistentContact* manifoldContactExt, const PxU32 numContacts, PCMContactPatch& patch)
{
	PX_UNUSED(numContacts);

	using namespace Ps::aos;
	FloatV max = FMax();
	FloatV maxDist = max;
	PxI32 index = -1;

	
	PCMContactPatch* currentPatch = &patch;
	while(currentPatch)
	{
		for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
		{
			const FloatV pen = V4GetW(manifoldContactExt[i].mLocalNormalPen);
			if(FAllGrtr(maxDist, pen))
			{
				maxDist = pen;
				index = i;
			}
		}
		currentPatch = currentPatch->mNextPatch;
	}

	PX_ASSERT(index!=-1);
	mContactPoints[0] = manifoldContactExt[index];

	return maxDist;

}

Ps::aos::FloatV Gu::SinglePersistentContactManifold::reduceBatchContactsCapsule(const PersistentContact* manifoldContactExt, const PxU32 numContacts, PCMContactPatch& patch)
{

		using namespace Ps::aos;

	bool* chosen = (bool*)PxAlloca(sizeof(bool)*numContacts);
	physx::PxMemZero(chosen, sizeof(bool)*numContacts);
	const FloatV max = FMax();
	//const FloatV nmax = FNeg(max);
	FloatV maxDis = max;
	PxI32 index = -1;

	FloatV maxPen = max;


	PCMContactPatch* currentPatch = &patch;
	while(currentPatch)
	{
		for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
		{
			const FloatV pen = V4GetW(manifoldContactExt[i].mLocalNormalPen);
			//const FloatV v = V3Dot(manifoldContactExt[i].mLocalPointB, manifoldContactExt[i].mLocalPointB);
			if(FAllGrtr(maxDis, pen))
			{
				maxDis = pen;
				index = i;
			}
		}
		currentPatch = currentPatch->mNextPatch;
	}

	chosen[index] = true;
	mContactPoints[0] = manifoldContactExt[index];
	maxPen = FMin(maxPen, V4GetW(manifoldContactExt[index].mLocalNormalPen));


	//calculate the furthest away points
	Vec3V v = V3Sub(manifoldContactExt[patch.mStartIndex].mLocalPointB, mContactPoints[0].mLocalPointB);
	maxDis = V3Dot(v, v);
	index = patch.mStartIndex;

	currentPatch = &patch;
	while(currentPatch)
	{
		for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
		{
			v = V3Sub(manifoldContactExt[i].mLocalPointB, mContactPoints[0].mLocalPointB);
			const FloatV d = V3Dot(v, v);
			if(FAllGrtr(d, maxDis))
			{
				maxDis = d;
				index = i;
			}
		}
		currentPatch = currentPatch->mNextPatch;
	}

	PX_ASSERT(chosen[index] == false);
	chosen[index] = true;
	mContactPoints[1] = manifoldContactExt[index];
	maxPen = FMin(maxPen, V4GetW(manifoldContactExt[index].mLocalNormalPen));

	//keep the second deepest point
	maxDis = max;
	currentPatch = &patch;
	while(currentPatch)
	{
		for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
		{
			if(!chosen[i])
			{
				const FloatV pen = V4GetW(manifoldContactExt[i].mLocalNormalPen);
				//const FloatV v = V3Dot(manifoldContactExt[i].mLocalPointB, manifoldContactExt[i].mLocalPointB);
				if(FAllGrtr(maxDis, pen))
				{
					maxDis = pen;
					index = i;
				}
			}
		}
		currentPatch = currentPatch->mNextPatch;
	}
	PX_ASSERT(chosen[index] == false);
	chosen[index] = true;
	mContactPoints[2] = manifoldContactExt[index];
	maxPen = FMin(maxPen, V4GetW(manifoldContactExt[index].mLocalNormalPen));

	return maxPen;

	//using namespace Ps::aos;

	//bool* chosen = (bool*)PxAlloca(sizeof(bool)*numContacts);
	//physx::PxMemZero(chosen, sizeof(bool)*numContacts);
	//const FloatV max = FMax();
	//const FloatV nmax = FNeg(max);
	////FloatV maxDis = nmax;
	//FloatV maxDis = max;
	//PxI32 index = -1;

	//struct Cluster
	//{
	//	PxU32 index;
	//	PxU32 origIndex;
	//	Ps::aos::FloatV penetration;
	//};

	//Cluster clusters[2]; 

	////keep the deepest point
	//PCMContactPatch* currentPatch = &patch;
	//while(currentPatch)
	//{
	//	for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
	//	{
	//		const FloatV pen = V4GetW(manifoldContactExt[i].mLocalNormalPen);
	//		//const FloatV v = V3Dot(manifoldContactExt[i].mLocalPointB, manifoldContactExt[i].mLocalPointB);
	//		if(FAllGrtr(maxDis, pen))
	//		//if(FAllGrtr(v, maxDis))
	//		{
	//			//maxDis = v;
	//			maxDis = pen;
	//			index = i;
	//		}
	//	}
	//	currentPatch = currentPatch->mNextPatch;
	//}

	//chosen[index] = true;
	//clusters[0].index=index;
	//clusters[0].origIndex = index;
	//clusters[0].penetration = V4GetW(manifoldContactExt[index].mLocalNormalPen);


	////calculate the furthest away points
	//Vec3V v = V3Sub(manifoldContactExt[patch.mStartIndex].mLocalPointB, manifoldContactExt[index].mLocalPointB);
	//maxDis = V3Dot(v, v);
	//index = patch.mStartIndex;

	//currentPatch = &patch;
	//while(currentPatch)
	//{
	//	for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
	//	{
	//		v = V3Sub(manifoldContactExt[i].mLocalPointB, manifoldContactExt[clusters[0].index].mLocalPointB);
	//		const FloatV d = V3Dot(v, v);
	//		if(FAllGrtr(d, maxDis))
	//		{
	//			maxDis = d;
	//			index = i;
	//		}
	//	}
	//	currentPatch = currentPatch->mNextPatch;
	//}

	//PX_ASSERT(chosen[index] == false);
	//chosen[index] = true;
	//clusters[1].index=index;
	//clusters[1].origIndex = index;
	//clusters[1].penetration = V4GetW(manifoldContactExt[index].mLocalNormalPen);

	//currentPatch = &patch;
	//	
	//while(currentPatch)
	//{
	//	for(PxU32 a = currentPatch->mStartIndex; a < currentPatch->mEndIndex; ++a)
	//	{
	//		PxU32 closest = 0;
	//		maxDis = max;
	//		for(PxU32 b = 0; b < 2; ++b)
	//		{
	//			Vec3V dif = V3Sub(manifoldContactExt[clusters[b].origIndex].mLocalPointB, manifoldContactExt[a].mLocalPointB);
	//			FloatV d = V3Dot(dif, dif);
	//			if(FAllGrtr(maxDis, d))
	//			{
	//				closest = b;
	//				maxDis = d;
	//			}
	//		}

	//		if(FAllGrtr(clusters[closest].penetration, V4GetW(manifoldContactExt[a].mLocalNormalPen)))
	//		{
	//			clusters[closest].penetration = V4GetW(manifoldContactExt[a].mLocalNormalPen);
	//			clusters[closest].index = a;
	//		}

	//	}
	//	currentPatch = currentPatch->mNextPatch;
	//}  

	////We now have a list of indices in clusters that are the final points we want to assign to the manifold
	//FloatV maxPen = max;
	//physx::PxMemZero(chosen, sizeof(bool)*numContacts);
	//for(PxU32 a = 0; a < 2; ++a)
	//{
	//	chosen[clusters[a].index] = true;
	//	mContactPoints[a] = manifoldContactExt[clusters[a].index];
	//	maxPen = FMin(maxPen, V4GetW(manifoldContactExt[clusters[a].index].mLocalNormalPen));
	//}

	////for(PxU32 a = 2; a < 3; ++a)
	//{
	//	maxDis = max;
	//	currentPatch = &patch;
	//	
	//	while(currentPatch)
	//	{
	//		for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
	//		{
	//			if(!chosen[i])
	//			{
	//				const FloatV pen = V4GetW(manifoldContactExt[i].mLocalNormalPen);
	//				if(FAllGrtr(maxDis, pen))
	//				{
	//					maxDis = pen;
	//					index = i;
	//				}
	//			}
	//		}
	//		currentPatch = currentPatch->mNextPatch;
	//	}
	//	mContactPoints[2] = manifoldContactExt[index];
	//	//maxPen = FMin(maxPen, V4GetW(manifoldContactExt[index].localNormalPen));
	//	maxPen = FMin(maxPen, maxDis);
	//	chosen[index] = true;

	//}

	//return maxPen;
}

Ps::aos::FloatV Gu::SinglePersistentContactManifold::reduceBatchContactsConvex(const PersistentContact* manifoldContactExt, const PxU32 numContacts, PCMContactPatch& patch)
{
	using namespace Ps::aos;

	bool* chosen = (bool*)PxAlloca(sizeof(bool)*numContacts);
	physx::PxMemZero(chosen, sizeof(bool)*numContacts);
	const FloatV max = FMax();
	const FloatV nmax = FNeg(max);
	FloatV maxDis = nmax;
	PxI32 index = -1;

	struct Cluster
	{
		PxU32 index;
		PxU32 origIndex;
		Ps::aos::FloatV penetration;
	};

	Cluster clusters[4]; 

	PCMContactPatch* currentPatch = &patch;
	while(currentPatch)
	{
		for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
		{
			//const FloatV pen = V4GetW(manifoldContactExt[i].localNormalPen);
			const FloatV v = V3Dot(manifoldContactExt[i].mLocalPointB, manifoldContactExt[i].mLocalPointB);
			if(FAllGrtr(v, maxDis))
			{
				maxDis = v;
				index = i;
			}
		}
		currentPatch = currentPatch->mNextPatch;
	}

	chosen[index] = true;
	clusters[0].index=index;
	clusters[0].origIndex = index;
	clusters[0].penetration = V4GetW(manifoldContactExt[index].mLocalNormalPen);


	//calculate the furthest away points
	Vec3V v = V3Sub(manifoldContactExt[patch.mStartIndex].mLocalPointB, manifoldContactExt[index].mLocalPointB);
	maxDis = V3Dot(v, v);
	index = patch.mStartIndex;

	currentPatch = &patch;
	while(currentPatch)
	{
		for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
		{
			v = V3Sub(manifoldContactExt[i].mLocalPointB, manifoldContactExt[clusters[0].index].mLocalPointB);
			const FloatV d = V3Dot(v, v);
			if(FAllGrtr(d, maxDis))
			{
				maxDis = d;
				index = i;
			}
		}
		currentPatch = currentPatch->mNextPatch;
	}

	PX_ASSERT(chosen[index] == false);
	chosen[index] = true;
	clusters[1].index=index;
	clusters[1].origIndex = index;
	clusters[1].penetration = V4GetW(manifoldContactExt[index].mLocalNormalPen);


	maxDis = nmax;
	index = -1;
	v = V3Sub(manifoldContactExt[clusters[1].index].mLocalPointB, manifoldContactExt[clusters[0].index].mLocalPointB);
	Vec3V norm = V3Normalize(V3Cross(v, Vec3V_From_Vec4V(manifoldContactExt[clusters[0].index].mLocalNormalPen)));
	FloatV minDis = max;
	PxI32 index1 = -1;
	

	//calculate the point furthest way to the segment
	currentPatch = &patch;
	
	while(currentPatch)
	{
		for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
		{
			if(!chosen[i])
			{
				v = V3Sub(manifoldContactExt[i].mLocalPointB, manifoldContactExt[clusters[0].index].mLocalPointB);
				const FloatV d = V3Dot(v, norm);
				if(FAllGrtr(d, maxDis))
				{
					maxDis = d;
					index = i;
				}

				if(FAllGrtr(minDis, d))
				{
					minDis = d;
					index1 = i;
				}
			}
		}
		currentPatch = currentPatch->mNextPatch;
	}

	PX_ASSERT(chosen[index] == false);

	chosen[index] = true;
	clusters[2].index=index;
	clusters[2].origIndex = index;
	clusters[2].penetration = V4GetW(manifoldContactExt[index].mLocalNormalPen);

	//if min and max in the same side, choose again
	const FloatV temp = FMul(minDis, maxDis);
	if(FAllGrtr(temp, FZero()))
	{
		//choose again
		maxDis = nmax;
		//calculate the point furthest way to the segment
		currentPatch = &patch;
		
		while(currentPatch)
		{
			for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
			{
				if(!chosen[i])
				{
					v = V3Sub(manifoldContactExt[i].mLocalPointB, manifoldContactExt[clusters[0].index].mLocalPointB);
					const FloatV d = V3Dot(v, norm);
					if(FAllGrtr(d, maxDis))
					{
						maxDis = d;
						index1 = i;
					}
				}
			}
			currentPatch = currentPatch->mNextPatch;
		}
	}

	PX_ASSERT(chosen[index1] == false);

	chosen[index1] = true;
	clusters[3].index = index1;
	clusters[3].origIndex = index1;
	clusters[3].penetration = V4GetW(manifoldContactExt[index1].mLocalNormalPen);


	currentPatch = &patch;
		
	while(currentPatch)
	{
		for(PxU32 a = currentPatch->mStartIndex; a < currentPatch->mEndIndex; ++a)
		{
			PxU32 closest = 0;
			maxDis = max;
			for(PxU32 b = 0; b < 4; ++b)
			{
				Vec3V dif = V3Sub(manifoldContactExt[clusters[b].origIndex].mLocalPointB, manifoldContactExt[a].mLocalPointB);
				FloatV d = V3Dot(dif, dif);
				if(FAllGrtr(maxDis, d))
				{
					closest = b;
					maxDis = d;
				}
			}

			if(FAllGrtr(clusters[closest].penetration, V4GetW(manifoldContactExt[a].mLocalNormalPen)))
			{
				clusters[closest].penetration = V4GetW(manifoldContactExt[a].mLocalNormalPen);
				clusters[closest].index = a;
			}

		}
		currentPatch = currentPatch->mNextPatch;
	}  

	//We now have a list of indices in clusters that are the final points we want to assign to the manifold
	FloatV maxPen = max;
	physx::PxMemZero(chosen, sizeof(bool)*numContacts);
	for(PxU32 a = 0; a < 4; ++a)
	{
		chosen[clusters[a].index] = true;
		mContactPoints[a] = manifoldContactExt[clusters[a].index];
		maxPen = FMin(maxPen, V4GetW(manifoldContactExt[clusters[a].index].mLocalNormalPen));
	}

	for(PxU32 a = 4; a < GU_SINGLE_MANIFOLD_CACHE_SIZE; ++a)
	{
		maxDis = max;
		currentPatch = &patch;
		
		while(currentPatch)
		{
			for(PxU32 i=currentPatch->mStartIndex; i<currentPatch->mEndIndex; ++i)
			{
				if(!chosen[i])
				{
					const FloatV pen = V4GetW(manifoldContactExt[i].mLocalNormalPen);
					if(FAllGrtr(maxDis, pen))
					{
						maxDis = pen;
						index = i;
					}
				}
			}
			currentPatch = currentPatch->mNextPatch;
		}
		mContactPoints[a] = manifoldContactExt[index];
		//maxPen = FMin(maxPen, V4GetW(manifoldContactExt[index].localNormalPen));
		maxPen = FMin(maxPen, maxDis);
		chosen[index] = true;

	}

	return maxPen;
}

PxU32 Gu::SinglePersistentContactManifold::reduceContacts(PersistentContact* manifoldPoints, PxU32 numPoints)
{
	using namespace Ps::aos;  
	//get the deepest points

	//PX_ALLOCA(chosen, bool, numPoints);
	bool* chosen = (bool*)PxAlloca(sizeof(bool)*numPoints);
	physx::PxMemZero(chosen, sizeof(bool)*numPoints);
	FloatV max = FMax();
	FloatV maxDis = max;
	PxI32 index = -1;
	PersistentContact newManifold[4];

	//keep the deepest point
	for(PxU32 i=0; i<numPoints; ++i)
	{
		const FloatV pen = V4GetW(manifoldPoints[i].mLocalNormalPen);
		if(FAllGrtr(maxDis, pen))
		{
			maxDis = pen;
			index = i;
		}
	}
	//keep the deepest points in the first position
	//contactPoints[0] = manifoldPoints[index];
	newManifold[0] = manifoldPoints[index];
	chosen[index] = true;


	//calculate the furthest away points
	Vec3V v = V3Sub(manifoldPoints[0].mLocalPointB, newManifold[0].mLocalPointB);
	maxDis = V3Dot(v, v);
	index = 0;

	for(PxU32 i=1; i<numPoints; ++i)
	{
		v = V3Sub(manifoldPoints[i].mLocalPointB, newManifold[0].mLocalPointB);
		const FloatV d = V3Dot(v, v);
		if(FAllGrtr(d, maxDis))
		{
			maxDis = d;
			index = i;
		}  
	}

	PX_ASSERT(chosen[index] == false);
	newManifold[1] = manifoldPoints[index];
	chosen[index] = true;


	maxDis = FNeg(max);
	index = -1;
	v = V3Sub(newManifold[1].mLocalPointB, newManifold[0].mLocalPointB);
	Vec3V norm = V3Normalize(V3Cross(v, Vec3V_From_Vec4V(newManifold[0].mLocalNormalPen)));
	FloatV minDis = max;
	PxI32 index1 = -1;
	

	//calculate the point furthest way to the segment
	for(PxU32 i=0; i<numPoints; ++i)
	{
		if(!chosen[i])
		{
			v = V3Sub(manifoldPoints[i].mLocalPointB, newManifold[0].mLocalPointB);
			const FloatV d = V3Dot(v, norm);
			if(FAllGrtr(d, maxDis))
			{
				maxDis = d;
				index = i;
			}

			if(FAllGrtr(minDis, d))
			{
				minDis = d;
				index1 = i;
			}
		}
	}

	PX_ASSERT(chosen[index] == false && chosen[index1]== false);

	chosen[index] = true;
	newManifold[2] = manifoldPoints[index];
	
	const FloatV temp = FMul(minDis, maxDis);
	if(FAllGrtr(temp, FZero()))
	{
		//chose the something further away from newManifold[2] 
		maxDis = FNeg(max);
		for(PxU32 i=0; i<numPoints; ++i)
		{
			if(!chosen[i])
			{
				v = V3Sub(manifoldPoints[i].mLocalPointB, newManifold[0].mLocalPointB);
				const FloatV d = V3Dot(v, norm);
				if(FAllGrtr(d, maxDis))
				{
					maxDis = d;
					index1 = i;
				}
			}
		}

	}

	newManifold[3]= manifoldPoints[index1];
	chosen[index1] = true;

	//copy the new manifold back
	for(PxU32 i=0; i<4; ++i)
	{
		manifoldPoints[i] = newManifold[i];
	}

	return 4;
}

//PxU32 Gu::SinglePersistentContactManifold::reduceContacts(PersistentContact* manifoldPoints, PxU32 numPoints)
//{
//	using namespace Ps::aos;  
//	//get the deepest points
//
//	//PX_ALLOCA(chosen, bool, numPoints);
//	//bool* chosen = (bool*)PxAlloca(sizeof(bool)*numPoints);
//	//physx::PxMemZero(chosen, sizeof(bool)*numPoints);
//	FloatV max = FMax();
//	FloatV maxDis = max;
//	PxI32 index = -1;
//	//PersistentContact newManifold[4];
//
//	//keep the deepest point
//	for(PxU32 i=0; i<numPoints; ++i)
//	{
//		const FloatV pen = V4GetW(manifoldPoints[i].localNormalPen);
//		if(FAllGrtr(maxDis, pen))
//		{
//			maxDis = pen;
//			index = i;
//		}
//	}
//	//keep the deepest points in the first position
//	PersistentContact tmp;
//	if(index != 0)
//	{
//		tmp = manifoldPoints[0];
//		manifoldPoints[0] = manifoldPoints[index];
//		manifoldPoints[index] = tmp;
//	}
//
//	//calculate the furthest away points
//	Vec3V v = V3Sub(manifoldPoints[1].localPointB, manifoldPoints[0].localPointB);
//	max = V3Dot(v, v);
//	index = 0;
//
//	for(PxU32 i=2; i<numPoints; ++i)
//	{
//		v = V3Sub(manifoldPoints[i].localPointB, manifoldPoints[0].localPointB);
//		const FloatV d = V3Dot(v, v);
//		if(FAllGrtr(d, maxDis))
//		{
//			maxDis = d;
//			index = i;
//		}
//	}
//
//	if(index != 1)
//	{
//		tmp = manifoldPoints[1];
//		manifoldPoints[1] = manifoldPoints[index];
//		manifoldPoints[index] = tmp;
//	}
//
//	maxDis = FNeg(max);
//	index = -1;
//	v = V3Sub(manifoldPoints[1].localPointB, manifoldPoints[0].localPointB);
//	Vec3V norm = V3Normalize(V3Cross(v, Vec3V_From_Vec4V(manifoldPoints[0].localNormalPen)));
//	FloatV minDis = max;
//	PxI32 index1 = -1;
//	
//
//	//calculate the point furthest way to the segment
//	for(PxU32 i=2; i<numPoints; ++i)
//	{
//		v = V3Sub(manifoldPoints[i].localPointB, manifoldPoints[0].localPointB);
//		const FloatV d = V3Dot(v, norm);
//		if(FAllGrtr(d, maxDis))
//		{
//			maxDis = d;
//			index = i;
//		}
//
//		if(FAllGrtr(minDis, d))
//		{
//			minDis = d;
//			index1 = i;
//		}
//	}
//
//	if(index != 2)
//	{
//		tmp = manifoldPoints[2];
//		manifoldPoints[2] = manifoldPoints[index];
//		manifoldPoints[index] = tmp;
//	}
//	
//	const FloatV temp = FMul(minDis, maxDis);
//	if(FAllGrtr(temp, FZero()))
//	{
//		//chose the something further away from newManifold[2] 
//		maxDis = FNeg(max);
//		for(PxU32 i=3; i<numPoints; ++i)
//		{
//			//if(!chosen[i])
//			{
//				v = V3Sub(manifoldPoints[i].localPointB, manifoldPoints[0].localPointB);
//				const FloatV d = V3Dot(v, norm);
//				if(FAllGrtr(d, maxDis))
//				{
//					maxDis = d;
//					index1 = i;
//				}
//			}
//		}
//
//	}
//
//	if(index != 3)
//	{
//		tmp = manifoldPoints[3];
//		manifoldPoints[3] = manifoldPoints[index1];
//		manifoldPoints[index1] = tmp;
//	}
//
//	return 4;
//}


Ps::aos::FloatV Gu::SinglePersistentContactManifold::refreshContactPoints(const Ps::aos::PsMatTransformV& aToB, const Ps::aos::FloatVArg projectBreakingThreshold, const Ps::aos::FloatVArg contactOffset)
{
	using namespace Ps::aos;
	const FloatV sqProjectBreakingThreshold =  FMul(projectBreakingThreshold, projectBreakingThreshold); 
	const BoolV bTrue = BTTTT();

	//PX_ASSERT(numContacts <= PXC_MANIFOLD_CACHE_SIZE);

	FloatV maxPen = FZero();
	// first refresh worldspace positions and distance
	for (PxU32 i=mNumContacts; i > 0; --i)
	{
		PersistentContact& manifoldPoint = mContactPoints[i-1];
		const Vec3V localAInB = aToB.transform( manifoldPoint.mLocalPointA ); // from a to b
		const Vec3V localBInB = manifoldPoint.mLocalPointB;
		const Vec3V v = V3Sub(localAInB, localBInB); 

		const Vec3V localNormal = Vec3V_From_Vec4V(manifoldPoint.mLocalNormalPen); // normal in b space
		const FloatV dist= V3Dot(v, localNormal);

		const Vec3V projectedPoint = V3NegScaleSub(localNormal,  dist, localAInB);//manifoldPoint.worldPointA - manifoldPoint.worldPointB * manifoldPoint.m_distance1;
		const Vec3V projectedDifference = V3Sub(localBInB, projectedPoint);

		const FloatV distance2d = V3Dot(projectedDifference, projectedDifference);
		const BoolV con = BOr(FIsGrtr(dist, contactOffset), FIsGrtr(distance2d, sqProjectBreakingThreshold));
		if(BAllEq(con, bTrue))
		{
			removeContactPoint(i-1);
		} 
		else
		{
			manifoldPoint.mLocalNormalPen = V4SetW(Vec4V_From_Vec3V(localNormal), dist);
			maxPen = FMin(maxPen, dist);
		}
	}

	return maxPen;
}


void Gu::SinglePersistentContactManifold::drawManifold( Cm::RenderOutput& out, const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB)
{
	using namespace Ps::aos;
#if VISUALIZE_PERSISTENT_CONTACT

	PxVec3 a, b;
	V3StoreU(trA.p, a);
	V3StoreU(trB.p, b);

	//PxMat44 m = PxMat44::createIdentity();
	//Cm::RenderOutput& out = context->mRenderOutput;
	/*out << color << m << Cm::RenderOutput::POINTS << a;
	out << 0xffff00ff << m << Cm::RenderOutput::POINTS << b;*/
	//out << (bHit ? 0xffff00ff : 0xff00ffff) << m << Cm::RenderOutput::LINES  << a << b;

	for(PxU32 i = 0; i< mNumContacts; ++i)
	{
		Gu::PersistentContact& m = mContactPoints[i];
		drawManifoldPoint(m, trA, trB, out, gColors[i]);
	}
#endif
}

void Gu::MultiplePersistentContactManifold::drawManifold( Cm::RenderOutput& out, const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB)
{
	for(PxU32 i=0; i<mNumManifolds; ++i)
	{
		SinglePersistentContactManifold* manifold = getManifold(i);
		manifold->drawManifold(out, trA, trB);
	}
}

void Gu::MultiplePersistentContactManifold::drawLine(Cm::RenderOutput& out, const Ps::aos::Vec3VArg p0, const Ps::aos::Vec3VArg p1, const PxU32 color)
{
	using namespace Ps::aos;
#if VISUALIZE_PERSISTENT_CONTACT
	PxVec3 a, b;
	V3StoreU(p0, a);
	V3StoreU(p1, b);

	PxMat44 m = PxMat44::createIdentity();
	out << color << m << Cm::RenderOutput::LINES << a << b;
#endif
}

void Gu::MultiplePersistentContactManifold::drawLine(Cm::RenderOutput& out, const PxVec3 p0, const PxVec3 p1, const PxU32 color)
{
	using namespace Ps::aos;
#if VISUALIZE_PERSISTENT_CONTACT

	PxMat44 m = PxMat44::createIdentity();
	out << color << m << Cm::RenderOutput::LINES << p0 << p1;
#endif
}

void Gu::MultiplePersistentContactManifold::drawPoint(Cm::RenderOutput& out, const Ps::aos::Vec3VArg p, const PxF32 size, const PxU32 color)
{
	using namespace Ps::aos;
#if VISUALIZE_PERSISTENT_CONTACT
	const PxVec3 up(0.f, size, 0.f);
	const PxVec3 right(size, 0.f, 0.f);
	const PxVec3 forwards(0.f, 0.f, size);

	PxVec3 a;
	V3StoreU(p, a);

	PxMat44 m = PxMat44::createIdentity();
	
	out << color << m << Cm::RenderOutput::LINES << a + up << a - up;
	out << color << m << Cm::RenderOutput::LINES << a + right << a - right;
	out << color << m << Cm::RenderOutput::LINES << a + forwards << a - forwards;
#endif
}


void Gu::MultiplePersistentContactManifold::drawPolygon( Cm::RenderOutput& out, const Ps::aos::PsTransformV& transform,  Ps::aos::Vec3V* points, const PxU32 numVerts, const PxU32 color)
{
	using namespace Ps::aos;
#if VISUALIZE_PERSISTENT_CONTACT
	for(PxU32 i=0; i<numVerts; ++i)
	{
		Vec3V tempV0 = points[i == 0 ? numVerts-1 : i-1];
		Vec3V tempV1 = points[i];
		
		drawLine(out, transform.transform(tempV0), transform.transform(tempV1), color);
	}
#endif

}


void Gu::MultiplePersistentContactManifold::addManifoldContactPoints(PersistentContact* manifoldContact, PxU32 numManifoldContacts, PCMContactPatch** contactPatch, const PxU32 numPatch, const Ps::aos::FloatVArg sqReplaceBreakingThreshold, const Ps::aos::FloatVArg acceptanceEpsilon, PxU8 maxContactsPerManifold)
{
	using namespace Ps::aos;
	
	if(mNumManifolds == 0)
	{
		//PxcPersistentContactManifold& manifold = manifolds[numManifolds++];
		for(PxU32 i=0; i<numPatch; ++i)
		{
			PCMContactPatch* patch = contactPatch[i];
			//this mean the patch hasn't been add to the manifold
			if(patch->mRoot == patch)
			{
				
				SinglePersistentContactManifold* manifold = getEmptyManifold();
				if(manifold)
				{
					FloatV _maxPen;
					//manifold->numContacts = 0;
					switch(maxContactsPerManifold)
					{
					
					case GU_SPHERE_MANIFOLD_CACHE_SIZE://sphere
						_maxPen = manifold->addBatchManifoldContactsSphere(manifoldContact, numManifoldContacts, *patch,  sqReplaceBreakingThreshold);
						break;
					case GU_CAPSULE_MANIFOLD_CACHE_SIZE://capsule, need to implement keep two deepest
						_maxPen = manifold->addBatchManifoldContactsCapsule(manifoldContact, numManifoldContacts, *patch, sqReplaceBreakingThreshold);
						break;
					default://cache size GU_SINGLE_MANIFOLD_CACHE_SIZE
						_maxPen = manifold->addBatchManifoldContactsConvex(manifoldContact, numManifoldContacts, *patch, sqReplaceBreakingThreshold);
						break;
					}
					FStore(_maxPen, &mMaxPen[mManifoldIndices[mNumManifolds]]);
					mNumManifolds++;
				}
				else
				{
					//We already pre-sorted the patches so we know we can return here
					return;
				}
				
			}
		}
	
		
	}
	else
	{
		PCMContactPatch tempPatch;
		for(PxU32 i=0; i<numPatch; ++i)
		{
			bool found = false;
			PCMContactPatch* patch = contactPatch[i];
			//this mean the patch has't been add to the manifold
			if(patch->mRoot == patch)
			{
				PX_ASSERT(mNumManifolds <= GU_MAX_MANIFOLD_SIZE);
				for(PxU32 j=0; j<mNumManifolds; ++j)
				{
					PX_ASSERT(mManifoldIndices[j] < GU_MAX_MANIFOLD_SIZE);
					SinglePersistentContactManifold& manifold = *getManifold(j);

					const Vec3V pNor = manifold.getLocalNormal();
					const FloatV d = V3Dot(patch->mPatchNormal, pNor);

					if(FAllGrtrOrEq(d, acceptanceEpsilon))
					{
						//appending the exiting contacts to the manifold list
						for(PxU32 k=0; k< manifold.mNumContacts; ++k)
						{
							PxU32 index = k + numManifoldContacts;
							//not duplicate point
							PX_ASSERT(index < 64);
							manifoldContact[index].mLocalPointA = manifold.mContactPoints[k].mLocalPointA;
							manifoldContact[index].mLocalPointB = manifold.mContactPoints[k].mLocalPointB;
							manifoldContact[index].mLocalNormalPen = manifold.mContactPoints[k].mLocalNormalPen;
						}

						//create a new patch for the exiting manifold
						tempPatch.mStartIndex = numManifoldContacts;
						tempPatch.mEndIndex = numManifoldContacts + manifold.mNumContacts;
						tempPatch.mPatchNormal = pNor;//manifold.getLocalNormal();
						tempPatch.mRoot = patch;
						tempPatch.mNextPatch = NULL;
						
						patch->mEndPatch->mNextPatch = &tempPatch;
				
						//numManifoldContacts += manifold.numContacts;
						patch->mTotalSize += manifold.mNumContacts;
						patch->mPatchMaxPen = FMin(patch->mPatchMaxPen, FLoad(mMaxPen[mManifoldIndices[j]]));
					
						//manifold.numContacts = 0;

						PX_ASSERT((numManifoldContacts+manifold.mNumContacts) <= 64);
						FloatV _maxPen;
						switch(maxContactsPerManifold)
						{
						case 1://sphere
							_maxPen = manifold.addBatchManifoldContactsSphere(manifoldContact, numManifoldContacts+manifold.mNumContacts, *patch, sqReplaceBreakingThreshold);
							break;
						case 2://capsule, need to implement keep two deepest
							_maxPen = manifold.addBatchManifoldContactsCapsule(manifoldContact, numManifoldContacts+manifold.mNumContacts, *patch, sqReplaceBreakingThreshold);
							break;
						default://cache size GU_SINGLE_MANIFOLD_CACHE_SIZE
							_maxPen = manifold.addBatchManifoldContactsConvex(manifoldContact, numManifoldContacts+manifold.mNumContacts, *patch, sqReplaceBreakingThreshold);
							break;
						}

					//	manifold.addBatchManifoldContacts(manifoldContact, numManifoldContacts+manifold.mNumContacts, *patch, mMaxPen[mManifoldIndices[j]], sqReplaceBreakingThreshold);
						FStore(_maxPen, &mMaxPen[mManifoldIndices[j]]);
						found = true;
						break;
					}
				}

				if(!found)// && numManifolds < 4)
				{
					SinglePersistentContactManifold* manifold = getEmptyManifold();
					if(manifold)
					{
						FloatV _maxPen;
						//manifold->numContacts = 0;
						switch(maxContactsPerManifold)
						{
						case 1://sphere
							_maxPen = manifold->addBatchManifoldContactsSphere(manifoldContact, numManifoldContacts, *patch, sqReplaceBreakingThreshold);
							break;
						case 2://capsule, need to implement keep two deepest
							_maxPen = manifold->addBatchManifoldContactsCapsule(manifoldContact, numManifoldContacts, *patch, sqReplaceBreakingThreshold);
							break;
						default://cache size GU_SINGLE_MANIFOLD_CACHE_SIZE
							_maxPen = manifold->addBatchManifoldContactsConvex(manifoldContact, numManifoldContacts, *patch,  sqReplaceBreakingThreshold);
							break;
						}
						FStore(_maxPen, &mMaxPen[mManifoldIndices[mNumManifolds]]);
						//manifold->addBatchManifoldContacts(manifoldContact, numManifoldContacts, *patch, mMaxPen[mManifoldIndices[mNumManifolds]], sqReplaceBreakingThreshold);
						mNumManifolds++;
					}	
					else
					{
						//ML: no existing manifold has the same normal as this patch, we need to find the shallowest penetration manifold. If this manifold is shallower than
						//the current patch, replace the manifold with the current patch
						PxU32 index = 0;
						for(PxU32 i=1; i<mNumManifolds; ++i)
						{
							//if(FAllGrtr(mMaxPen[mManifoldIndices[i]], mMaxPen[mManifoldIndices[index]]))
							if(mMaxPen[mManifoldIndices[i]] > mMaxPen[mManifoldIndices[index]])
							{
								index = i;
							}
						}

						if(FAllGrtr(FLoad(mMaxPen[mManifoldIndices[index]]), patch->mPatchMaxPen))
						{
							SinglePersistentContactManifold* manifold = getManifold(index);
							manifold->mNumContacts = 0;
							//maxPen[manifoldIndices[index]] = patch->maxPen;
							FloatV _maxPen;
							switch(maxContactsPerManifold)
							{
							case 1://sphere
								_maxPen = manifold->addBatchManifoldContactsSphere(manifoldContact, numManifoldContacts, *patch, sqReplaceBreakingThreshold);
								break;
							case 2://capsule, need to implement keep two deepest
								_maxPen = manifold->addBatchManifoldContactsCapsule(manifoldContact, numManifoldContacts, *patch, sqReplaceBreakingThreshold);
								break;
							default://cache size GU_SINGLE_MANIFOLD_CACHE_SIZE
								_maxPen = manifold->addBatchManifoldContactsConvex(manifoldContact, numManifoldContacts, *patch, sqReplaceBreakingThreshold);
								break;
							}

							FStore(_maxPen, &mMaxPen[mManifoldIndices[index]]);
							//manifold->addBatchManifoldContacts(manifoldContact, numManifoldContacts, *patch, mMaxPen[mManifoldIndices[index]], sqReplaceBreakingThreshold);
						}
						return;
					}
				}
			}
		}
	}
}


bool Gu::MultiplePersistentContactManifold::addManifoldContactsToContactBuffer(Gu::ContactBuffer& contactBuffer, const Ps::aos::PsTransformV& meshTransform)
{
	using namespace Ps::aos;
	PxU32 contactCount = 0;//contactBuffer.count;
	PxU32 numContacts = 0;
	mNumTotalContacts = 0;
	//drawManifold(*gRenderOutPut, convexTransform, meshTransform);
	for(PxU32 i=0; i < mNumManifolds; ++i)
	{
		Gu::SinglePersistentContactManifold& manifold = *getManifold(i);
		numContacts = manifold.getNumContacts();
		PX_ASSERT(mNumTotalContacts + numContacts <= 0xFF);
		mNumTotalContacts += Ps::to8(numContacts);
		const Vec3V normal = manifold.getWorldNormal(meshTransform);
		//const Vec3V normal = V3Normalize(meshTransform.rotate(Vec3V_From_Vec4V(manifold.getContactPoint(0).localNormalPen)));
		
		for(PxU32 j=0; (j< numContacts) & (contactCount < ContactBuffer::MAX_CONTACTS); ++j)
		{
			Gu::PersistentContact& p = manifold.getContactPoint(j);
			
			//const Vec3V normal = meshTransform.rotate(Vec3V_From_Vec4V(p.localNormalPen));
			const Vec3V worldP =meshTransform.transform(p.mLocalPointB);
			const FloatV dist = V4GetW(p.mLocalNormalPen);
			
			Gu::ContactPoint& contact = contactBuffer.contacts[contactCount++];
			//Fast allign store
			V4StoreA(Vec4V_From_Vec3V(normal), (PxF32*)&contact.normal.x);
			V4StoreA(Vec4V_From_Vec3V(worldP), (PxF32*)&contact.point.x);
			FStore(dist, &contact.separation);

			contact.internalFaceIndex0 = PXC_CONTACT_NO_FACE_INDEX;
			contact.internalFaceIndex1 = PXC_CONTACT_NO_FACE_INDEX;

		}
	}
//#if defined(__SPU__)
//	pxPrintf("contactCount %i\n", contactCount);
//#endif
	PX_ASSERT(contactCount <= 64);
	contactBuffer.count = contactCount;
	return contactCount > 0;
}

bool Gu::MultiplePersistentContactManifold::addManifoldContactsToContactBuffer(Gu::ContactBuffer& contactBuffer, const Ps::aos::PsTransformV& trA, const Ps::aos::PsTransformV& trB, const Ps::aos::FloatVArg radius)
{
	using namespace Ps::aos;
	PxU32 contactCount = 0;//contactBuffer.count;
	PxU32 numContacts = 0;
	mNumTotalContacts = 0;
	//drawManifold(*gRenderOutPut, convexTransform, meshTransform);
	for(PxU32 i=0; i < mNumManifolds; ++i)
	{
		Gu::SinglePersistentContactManifold& manifold = *getManifold(i);
		numContacts = manifold.getNumContacts();
		PX_ASSERT(mNumTotalContacts + numContacts <= 0xFF);
		mNumTotalContacts += Ps::to8(numContacts);
		const Vec3V normal = manifold.getWorldNormal(trB);
		//const Vec3V normal = V3Normalize(meshTransform.rotate(Vec3V_From_Vec4V(manifold.getContactPoint(0).localNormalPen)));
		
		for(PxU32 j=0; (j< numContacts) & (contactCount < ContactBuffer::MAX_CONTACTS); ++j)
		{
			Gu::PersistentContact& p = manifold.getContactPoint(j);
			
			//const Vec3V normal = meshTransform.rotate(Vec3V_From_Vec4V(p.localNormalPen));
			//const Vec3V worldP =meshTransform.transform(p.mLocalPointB);
			const Vec3V worldP =V3NegScaleSub(normal, radius, trA.transform(p.mLocalPointA));
			const FloatV dist = FSub(V4GetW(p.mLocalNormalPen), radius);
			
			Gu::ContactPoint& contact = contactBuffer.contacts[contactCount++];
			//Fast allign store
			V4StoreA(Vec4V_From_Vec3V(normal), (PxF32*)&contact.normal.x);
			V4StoreA(Vec4V_From_Vec3V(worldP), (PxF32*)&contact.point.x);
			FStore(dist, &contact.separation);

			contact.internalFaceIndex0 = PXC_CONTACT_NO_FACE_INDEX;
			contact.internalFaceIndex1 = PXC_CONTACT_NO_FACE_INDEX;

		}
	}
//#if defined(__SPU__)
//	pxPrintf("contactCount %i\n", contactCount);
//#endif
	PX_ASSERT(contactCount <= 64);
	contactBuffer.count = contactCount;
	return contactCount > 0;
}
