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

/*
General notes:

	rtree depth-first traversal looks like this:
	push top level page onto stack

	pop page from stack
	for each node in page
	  if node overlaps with testrect
	    push node's subpage

	we want to efficiently keep track of current stack level to know if the current page is a leaf or not
	(since we don't store a flag with the page due to no space, we can't determine it just by looking at current page)
	since we traverse depth first, the levels for nodes on the stack look like this:
	l0 l0 l1 l2 l2 l3 l3 l3 l4

	we can encode this as an array of 4 bits per level count into a 32-bit integer
	to simplify the code->level computation we also keep track of current level by incrementing the level whenever any subpages
	from current test page are pushed onto the stack
	when we pop a page off the stack we use this encoding to determine if we should decrement the stack level
*/

#include "GuRTree.h"
#include "PsIntrinsics.h"
#include "PxBounds3.h"
#include "GuBox.h"
#include "PsVecMath.h"
#include "CmMemFetch.h"
#include "PxQueryReport.h" // for PxAgain

//#define VERIFY_RTREE
#ifdef VERIFY_RTREE
#include "GuIntersectionRayBox.h"
#include "GuIntersectionBoxBox.h"
#include "GuIntersectionCapsuleBox.h"
#include "stdio.h"
#endif

using namespace physx;
using namespace physx::shdfnd;
using namespace Ps::aos;

#if 0 // these datatypes are for debugging convenience only
struct U168 { PxU16 a0, a1, a2, a3, a4, a5, a6, a7, a8; } tmpu168;
struct U324 { PxU32 x,y,z,w; } tmpu324;
struct F324
{
	F324(Ps::aos::Vec4V&x) { *this = *(F324*)&x; }
	bool operator==(const F324& v) const { return v.x == x && v.y == y && v.z == z && v.w == w; }
	F324() {}
	
	PxF32 x,y,z,w;
} tmpf324;
#endif

namespace physx
{
namespace Gu {

using namespace Ps::aos;

#define v_absm(a) V4Andc(a, signMask)
#define V4FromF32A(x) V4LoadA(x)
#define PxF32FV(x) FStore(x)
#define CAST_U8(a) reinterpret_cast<PxU8*>(a)

PX_INLINE bool isPowerOfTwo(PxU32 n) { return ((n&(n-1))==0); }

/////////////////////////////////////////////////////////////////////////
void RTree::traverseAABB(
	const PxVec3& boxMin, const PxVec3& boxMax,
	const PxU32 maxResults, PxU32* resultsPtr,
	Callback* callback
) const
{
	PX_UNUSED(resultsPtr);

	//pxPrintf("traverseAABB\n");
	PX_ASSERT(callback);
	PX_ASSERT(maxResults >= mPageSize);
	PX_UNUSED(maxResults);

	const PxU32 maxStack = 128;
	PxU32 stack1[maxStack];
	PxU32* stack = stack1+1;

	PX_ASSERT(mPages);
	PX_ASSERT((Cm::MemFetchPtr(mPages) & 127) == 0);
	PX_ASSERT((Cm::MemFetchPtr(this) & 15) == 0);

	// conservatively quantize the input box
	Vec4V nqMin = Vec4V_From_PxVec3_WUndefined(boxMin);
	Vec4V nqMax = Vec4V_From_PxVec3_WUndefined(boxMax);

	Vec4V nqMinx4 = V4SplatElement<0>(nqMin);
	Vec4V nqMiny4 = V4SplatElement<1>(nqMin);
	Vec4V nqMinz4 = V4SplatElement<2>(nqMin);
	Vec4V nqMaxx4 = V4SplatElement<0>(nqMax);
	Vec4V nqMaxy4 = V4SplatElement<1>(nqMax);
	Vec4V nqMaxz4 = V4SplatElement<2>(nqMax);

	// on 64-bit platforms the dynamic rtree pointer is also relative to mPages
	PxU8* treeNodes8 = PX_IS_X64 ? CAST_U8(get64BitBasePage()) : CAST_U8((mFlags & IS_DYNAMIC) ? NULL : mPages);
	PxU32* stackPtr = stack;

	// AP potential perf optimization - fetch the top level right away
	PX_ASSERT(RTREE_PAGE_SIZE == 4 || RTREE_PAGE_SIZE == 8);
	PX_ASSERT(isPowerOfTwo(mPageSize));
	PX_ASSERT(((mFlags & IS_DYNAMIC) && mNumRootPages == 1) || (mNumRootPages > 0)); // always only 1 top page for dynamic trees

	for (PxI32 j = PxI32(mNumRootPages-1); j >= 0; j --)
		*stackPtr++ = (mFlags & IS_DYNAMIC) ? pagePtrTo32Bits(mPages+j) : j*sizeof(RTreePage);

	PxU32 cacheTopValid = true;
	PxU32 cacheTop = (mFlags & IS_DYNAMIC) ? pagePtrTo32Bits(mPages) : 0;

	do {
		stackPtr--;
		PxU32 top;
		if (cacheTopValid) // branch is faster than lhs
			top = cacheTop;
		else
			top = stackPtr[0];
		PX_ASSERT(!cacheTopValid || stackPtr[0] == cacheTop);
		RTreePage* __restrict tn = (RTreePage*)(treeNodes8 + top);
		#ifdef __SPU__
			RTreePage tmpPage;
			Cm::memFetchAlignedAsync(PxU64(&tmpPage), PxU64(treeNodes8+top), sizeof(RTreePage), 5);
			Cm::memFetchWait(5);
			tn = &tmpPage;
		#endif
		const PxU32* ptrs = ((RTreePage *)tn)->ptrs;

		Vec4V minx4 = V4LoadA(tn->minx);
		Vec4V miny4 = V4LoadA(tn->miny);
		Vec4V minz4 = V4LoadA(tn->minz);
		Vec4V maxx4 = V4LoadA(tn->maxx);
		Vec4V maxy4 = V4LoadA(tn->maxy);
		Vec4V maxz4 = V4LoadA(tn->maxz);

		#if RTREE_PAGE_SIZE == 8
		Vec4V minx4b = V4LoadA(tn->minx+4);
		Vec4V miny4b = V4LoadA(tn->miny+4);
		Vec4V minz4b = V4LoadA(tn->minz+4);
		Vec4V maxx4b = V4LoadA(tn->maxx+4);
		Vec4V maxy4b = V4LoadA(tn->maxy+4);
		Vec4V maxz4b = V4LoadA(tn->maxz+4);
		#endif

		BoolV res0 = V4IsGrtr(nqMinx4, maxx4); BoolV res1 = V4IsGrtr(nqMiny4, maxy4); BoolV res2 = V4IsGrtr(nqMinz4, maxz4);
		BoolV res3 = V4IsGrtr(minx4, nqMaxx4); BoolV res4 = V4IsGrtr(miny4, nqMaxy4); BoolV res5 = V4IsGrtr(minz4, nqMaxz4);
		BoolV resx = BOr(BOr(BOr(res0, res1), BOr(res2, res3)), BOr(res4, res5));
		PX_ALIGN_PREFIX(16) PxU32 resa[RTREE_PAGE_SIZE] PX_ALIGN_SUFFIX(16);

		#if RTREE_PAGE_SIZE == 8
		BoolV res0b = V4IsGrtr(nqMinx4, maxx4b); BoolV res1b = V4IsGrtr(nqMiny4, maxy4b); BoolV res2b = V4IsGrtr(nqMinz4, maxz4b);
		BoolV res3b = V4IsGrtr(minx4b, nqMaxx4); BoolV res4b = V4IsGrtr(miny4b, nqMaxy4); BoolV res5b = V4IsGrtr(minz4b, nqMaxz4);
		BoolV resxb = BOr(BOr(BOr(res0b, res1b), BOr(res2b, res3b)), BOr(res4b, res5b));
		#endif

		VecU32V res4x = VecU32V_From_BoolV(resx); 
		U4StoreA(res4x, resa);
		#if RTREE_PAGE_SIZE == 8
		VecU32V res4xb = VecU32V_From_BoolV(resxb); 
		U4StoreA(res4xb, resa+4);
		#endif

		// search for VERIFY_RTREE, insert here if needed for debugging
		cacheTopValid = false;
		for (PxU32 i = 0; i < RTreePage::SIZE; i++)
		{
			PxU32 ptr = ptrs[i] & ~1; // clear the isLeaf bit
			if (resa[i])
				continue;
			if (tn->isLeaf(i))
			{
				if (!callback->processResults(1, &ptr))
					return;
			}
			else
			{
				*(stackPtr++) = ptr;
				cacheTop = ptr;
				cacheTopValid = true;
			}
		}
	} while (stackPtr > stack);
}

/////////////////////////////////////////////////////////////////////////
bool RTree::findObjectBackTrack(PxU32, const PxVec3&, const PxVec3&, NodeHandle*, PxU32) const
{
	// AP: was removed during Mesh version 10 optimizations, only needed for dynamic rtrees and will probably not be needed eventually
	PX_ASSERT(0 && "Unimplemented.");
	return false;
}

/////////////////////////////////////////////////////////////////////////
template <int inflate>
//PX_PHYSX_COMMON_API
void RTree::traverseRay(
	const PxVec3& rayOrigin, const PxVec3& rayDir,
	const PxU32 maxResults, PxU32* resultsPtr, Gu::RTree::CallbackRaycast* callback,
	const PxVec3* fattenAABBs, PxF32 maxT) const
{
	PX_UNUSED(resultsPtr);
	PX_UNUSED(maxResults);

	//pxPrintf("traverseRay\n");
	const PxU32 maxStack = 128;
	PxU32 stack1[maxStack];
	const PxU32* stack = stack1+1;

	PX_ASSERT(mPages);
	PX_ASSERT((Cm::MemFetchPtr(mPages) & 127) == 0);
	PX_ASSERT((Cm::MemFetchPtr(this) & 15) == 0);

	PxU8* treeNodes8 = PX_IS_X64 ? CAST_U8(get64BitBasePage()) : CAST_U8((mFlags & IS_DYNAMIC) ? NULL : mPages);
	PxU32* stackPtr = (PxU32*)stack;

	Vec4V fattenAABBsX, fattenAABBsY, fattenAABBsZ;
	PX_UNUSED(fattenAABBsX); PX_UNUSED(fattenAABBsY); PX_UNUSED(fattenAABBsZ);
	if (inflate)
	{
		Vec4V fattenAABBs4 = Vec4V_From_PxVec3_WUndefined(*fattenAABBs);
		static const Vec4V epsInflateFloat4 = Vec4VLoadXYZW(1e-7f, 1e-7f, 1e-7f, 1e-7f);
		fattenAABBs4 = V4Add(fattenAABBs4, epsInflateFloat4); // US2385 - shapes are "closed" meaning exactly touching shapes should report overlap
		fattenAABBsX = V4SplatElement<0>(fattenAABBs4);
		fattenAABBsY = V4SplatElement<1>(fattenAABBs4);
		fattenAABBsZ = V4SplatElement<2>(fattenAABBs4);
	}

	static const VecU32V signMask = VecU32VLoadXYZW((PxU32(1)<<31), (PxU32(1)<<31), (PxU32(1)<<31), (PxU32(1)<<31));
	static const Vec4V epsFloat4 = V4Load(1e-9f);
	static const Vec4V zeroes = V4Zero();
	static const Vec4V twos = V4Load(2.0f);

	Vec4V maxT4;
	PxF32 maxTAdj = maxT * 0.9999999f;
	maxT4 = V4Load(maxTAdj);
	Vec4V rayP = Vec4V_From_PxVec3_WUndefined(rayOrigin);
	Vec4V rayD = Vec4V_From_PxVec3_WUndefined(rayDir);
	VecU32V raySign = V4U32and(VecU32V_ReinterpretFrom_Vec4V(rayD), signMask);
	Vec4V rayDAbs = V4Abs(rayD); // abs value of rayD
	Vec4V rayInvD = Vec4V_ReinterpretFrom_VecU32V(V4U32or(raySign, VecU32V_ReinterpretFrom_Vec4V(V4Max(rayDAbs, epsFloat4)))); // clamp near-zero components up to epsilon
	rayD = rayInvD;

	//rayInvD = V4Recip(rayInvD);
	// Newton-Raphson iteration for reciprocal (see wikipedia):
	// X[n+1] = X[n]*(2-original*X[n]), X[0] = V4RecipFast estimate
	//rayInvD = rayInvD*(twos-rayD*rayInvD);
	rayInvD = V4RecipFast(rayInvD); // initial estimate, not accurate enough
	rayInvD = V4Mul(rayInvD, V4NegMulSub(rayD, rayInvD, twos));

	// P+tD=a; t=(a-P)/D
	// t=(a - p.x)*1/d.x = a/d.x +(- p.x/d.x)
	Vec4V rayPinvD = V4NegMulSub(rayInvD, rayP, zeroes);
	Vec4V rayInvDsplatX = V4SplatElement<0>(rayInvD);
	Vec4V rayInvDsplatY = V4SplatElement<1>(rayInvD);
	Vec4V rayInvDsplatZ = V4SplatElement<2>(rayInvD);
	Vec4V rayPinvDsplatX = V4SplatElement<0>(rayPinvD);
	Vec4V rayPinvDsplatY = V4SplatElement<1>(rayPinvD);
	Vec4V rayPinvDsplatZ = V4SplatElement<2>(rayPinvD);

	PX_ASSERT(RTREE_PAGE_SIZE == 4 || RTREE_PAGE_SIZE == 8);
	PX_ASSERT(mNumRootPages > 0);
	PX_ASSERT(((mFlags & IS_DYNAMIC) && mNumRootPages == 1) || (mNumRootPages > 0)); // always only 1 top page for dynamic trees

	for (PxI32 j = PxI32(mNumRootPages-1); j >= 0; j --)
		*stackPtr++ = (mFlags & IS_DYNAMIC) ? pagePtrTo32Bits(mPages) : j*sizeof(RTreePage);
	bool cacheTopValid = true;
	PxU32 cacheTop = (mFlags & IS_DYNAMIC) ? pagePtrTo32Bits(mPages) : 0;

	do {
		stackPtr--;

		PxU32 top;
		// Disabled cacheTop because of reordering complications
		if (cacheTopValid) // branch is faster than lhs
			top = cacheTop;
		else
			top = stackPtr[0];
		PX_ASSERT(!cacheTopValid || top == cacheTop);
		RTreePage* __restrict tn = (RTreePage*)(treeNodes8 + top);
		#ifdef __SPU__
			RTreePage tmpPage;
			Cm::memFetchAlignedAsync(PxU64(&tmpPage), PxU64(treeNodes8+top), sizeof(RTreePage), 5);
			Cm::memFetchWait(5);
			tn = &tmpPage;
		#endif
		//prefetchLine(tn+64);
		PxU32* ptrs = ((RTreePage *)tn)->ptrs;

		// 6i load
		Vec4V minx4a = V4FromF32A(tn->minx), miny4a = V4FromF32A(tn->miny), minz4a = V4FromF32A(tn->minz);
		Vec4V maxx4a = V4FromF32A(tn->maxx), maxy4a = V4FromF32A(tn->maxy), maxz4a = V4FromF32A(tn->maxz);
		#if RTREE_PAGE_SIZE == 8
		Vec4V minx4b = V4FromF32A(tn->minx+4), miny4b = V4FromF32A(tn->miny+4), minz4b = V4FromF32A(tn->minz+4);
		Vec4V maxx4b = V4FromF32A(tn->maxx+4), maxy4b = V4FromF32A(tn->maxy+4), maxz4b = V4FromF32A(tn->maxz+4);
		#endif

		// 1i disabled test
		VecU32V ignore4a = (VecU32V)V4IsGrtrV32u(minx4a, maxx4a); // 1 if degenerate box (empty slot in the page)
		#if RTREE_PAGE_SIZE == 8
		VecU32V ignore4b = (VecU32V)V4IsGrtrV32u(minx4b, maxx4b); // 1 if degenerate box (empty slot in the page)
		#endif

		if (inflate)
		{
			// 6i
			maxx4a = V4Add(maxx4a, fattenAABBsX); maxy4a = V4Add(maxy4a, fattenAABBsY); maxz4a = V4Add(maxz4a, fattenAABBsZ);
			minx4a = V4Sub(minx4a, fattenAABBsX); miny4a = V4Sub(miny4a, fattenAABBsY); minz4a = V4Sub(minz4a, fattenAABBsZ);
			#if RTREE_PAGE_SIZE == 8
			maxx4b = V4Add(maxx4b, fattenAABBsX); maxy4b = V4Add(maxy4b, fattenAABBsY); maxz4b = V4Add(maxz4b, fattenAABBsZ);
			minx4b = V4Sub(minx4b, fattenAABBsX); miny4b = V4Sub(miny4b, fattenAABBsY); minz4b = V4Sub(minz4b, fattenAABBsZ);
			#endif
		}

		// P+tD=a; t=(a-P)/D
		// t=(a - p.x)*1/d.x = a/d.x +(- p.x/d.x)
		// 6i
		Vec4V tminxa0 = V4MulAdd(minx4a, rayInvDsplatX, rayPinvDsplatX);
		Vec4V tminya0 = V4MulAdd(miny4a, rayInvDsplatY, rayPinvDsplatY);
		Vec4V tminza0 = V4MulAdd(minz4a, rayInvDsplatZ, rayPinvDsplatZ);
		Vec4V tmaxxa0 = V4MulAdd(maxx4a, rayInvDsplatX, rayPinvDsplatX);
		Vec4V tmaxya0 = V4MulAdd(maxy4a, rayInvDsplatY, rayPinvDsplatY);
		Vec4V tmaxza0 = V4MulAdd(maxz4a, rayInvDsplatZ, rayPinvDsplatZ);
		#if RTREE_PAGE_SIZE == 8
		Vec4V tminxb0 = V4MulAdd(minx4b, rayInvDsplatX, rayPinvDsplatX);
		Vec4V tminyb0 = V4MulAdd(miny4b, rayInvDsplatY, rayPinvDsplatY);
		Vec4V tminzb0 = V4MulAdd(minz4b, rayInvDsplatZ, rayPinvDsplatZ);
		Vec4V tmaxxb0 = V4MulAdd(maxx4b, rayInvDsplatX, rayPinvDsplatX);
		Vec4V tmaxyb0 = V4MulAdd(maxy4b, rayInvDsplatY, rayPinvDsplatY);
		Vec4V tmaxzb0 = V4MulAdd(maxz4b, rayInvDsplatZ, rayPinvDsplatZ);
		#endif

		// test half-spaces
		// P+tD=dN
		// t = (d(N,D)-(P,D))/(D,D) , (D,D)=1

		// compute 4x dot products (N,D) and (P,N) for each AABB in the page

		// 6i
		// now compute tnear and tfar for each pair of planes for each box
		Vec4V tminxa = V4Min(tminxa0, tmaxxa0); Vec4V tmaxxa = V4Max(tminxa0, tmaxxa0);
		Vec4V tminya = V4Min(tminya0, tmaxya0); Vec4V tmaxya = V4Max(tminya0, tmaxya0);
		Vec4V tminza = V4Min(tminza0, tmaxza0); Vec4V tmaxza = V4Max(tminza0, tmaxza0);

		#if RTREE_PAGE_SIZE == 8
		Vec4V tminxb = V4Min(tminxb0, tmaxxb0); Vec4V tmaxxb = V4Max(tminxb0, tmaxxb0);
		Vec4V tminyb = V4Min(tminyb0, tmaxyb0); Vec4V tmaxyb = V4Max(tminyb0, tmaxyb0);
		Vec4V tminzb = V4Min(tminzb0, tmaxzb0); Vec4V tmaxzb = V4Max(tminzb0, tmaxzb0);
		#endif

		// 8i
		Vec4V maxOfNeasa = V4Max(V4Max(tminxa, tminya), tminza);
		Vec4V minOfFarsa = V4Min(V4Min(tmaxxa, tmaxya), tmaxza);
		ignore4a = V4U32or(ignore4a, V4IsGrtrV32u(epsFloat4, minOfFarsa));  // if tfar is negative, ignore since its a ray, not a line
		ignore4a = V4U32or(ignore4a, V4IsGrtrV32u(maxOfNeasa, maxT4));  // if tnear is over maxT, ignore this result
			#if RTREE_PAGE_SIZE == 8
			Vec4V maxOfNeasb = V4Max(V4Max(tminxb, tminyb), tminzb);
			Vec4V minOfFarsb = V4Min(V4Min(tmaxxb, tmaxyb), tmaxzb);
			ignore4b = V4U32or(ignore4b, V4IsGrtrV32u(epsFloat4, minOfFarsb));  // if tfar is negative, ignore since its a ray, not a line
			ignore4b = V4U32or(ignore4b, V4IsGrtrV32u(maxOfNeasb, maxT4));  // if tnear is over maxT, ignore this result
			#endif

		// 2i
		VecU32V resa4 = (VecU32V)V4IsGrtrV32u(maxOfNeasa, minOfFarsa); // if 1 => fail
		resa4 = V4U32or(resa4, ignore4a);
		#if RTREE_PAGE_SIZE == 8
		VecU32V resb4 = (VecU32V)V4IsGrtrV32u(maxOfNeasb, minOfFarsb); // if 1 => fail
		resb4 = V4U32or(resb4, ignore4b);
		#endif

		// 1i
		PX_ALIGN_PREFIX(16) PxU32 resa[8] PX_ALIGN_SUFFIX(16);
		V4U32StoreAligned(resa4, (VecU32V*)resa);
		#if RTREE_PAGE_SIZE == 8
		V4U32StoreAligned(resb4, (VecU32V*)(resa+4));
		#endif

		cacheTopValid = false;

		struct Unroll
		{
			static PX_FORCE_INLINE PxAgain codeBlock(
				PxU32 i, const PxU32* ptrs, const PxU32* resa, const RTreePage* __restrict tn, CallbackRaycast* callback,
				PxU32*& stackPtr, float& maxT, Vec4V& maxT4, PxU32& cacheTop, bool& cacheTopValid)
			{
				PxU32 ptr = ptrs[i] & ~1; /* clear the isLeaf bit */
				if (!resa[i])
				{
					if (tn->isLeaf(i))
					{
						cacheTop = stackPtr[-1];
						cacheTopValid = true;
						PxF32 newMaxT = maxT;
						if (!callback->processResults(1, &ptr, newMaxT))
							return false;
						/* shrink the ray if newMaxT is reduced compared to the original maxT */
						if (maxT != newMaxT)
						{
							PX_ASSERT(newMaxT < maxT);
							maxT = newMaxT;
							PxF32 maxTAdj = newMaxT * 0.9999999f;
							maxT4 = Vec4VLoadXYZW(maxTAdj, maxTAdj, maxTAdj, maxTAdj);
						}
					}
					else
					{
						*(stackPtr++) = ptr;
						cacheTop = ptr;
						cacheTopValid = true;
					}
				}
				return true;
			};
		};

		if (!Unroll::codeBlock(0, ptrs, resa, tn, callback, stackPtr, maxT, maxT4, cacheTop, cacheTopValid))
			return;
		if (!Unroll::codeBlock(1, ptrs, resa, tn, callback, stackPtr, maxT, maxT4, cacheTop, cacheTopValid))
			return;
		if (!Unroll::codeBlock(2, ptrs, resa, tn, callback, stackPtr, maxT, maxT4, cacheTop, cacheTopValid))
			return;
		if (!Unroll::codeBlock(3, ptrs, resa, tn, callback, stackPtr, maxT, maxT4, cacheTop, cacheTopValid))
			return;
		#if RTREE_PAGE_SIZE == 8
		if (!Unroll::codeBlock(4, ptrs, resa, tn, callback, stackPtr, maxT, maxT4, cacheTop, cacheTopValid))
			return;
		if (!Unroll::codeBlock(5, ptrs, resa, tn, callback, stackPtr, maxT, maxT4, cacheTop, cacheTopValid))
			return;
		if (!Unroll::codeBlock(6, ptrs, resa, tn, callback, stackPtr, maxT, maxT4, cacheTop, cacheTopValid))
			return;
		if (!Unroll::codeBlock(7, ptrs, resa, tn, callback, stackPtr, maxT, maxT4, cacheTop, cacheTopValid))
			return;
		#endif
	} while (stackPtr > stack);
}

template void RTree::traverseRay<0>(
	const PxVec3&, const PxVec3&, const PxU32, PxU32*, Gu::RTree::CallbackRaycast*, const PxVec3*, PxF32 maxT) const;
template void RTree::traverseRay<1>(
	const PxVec3&, const PxVec3&, const PxU32, PxU32*, Gu::RTree::CallbackRaycast*, const PxVec3*, PxF32 maxT) const;


/////////////////////////////////////////////////////////////////////////
void RTree::traverseOBB(
	const Gu::Box& obb, const PxU32 maxResults, PxU32* resultsPtr, Gu::RTree::Callback* callback) const
{
	PX_UNUSED(resultsPtr);
	PX_UNUSED(maxResults);

	//pxPrintf("traverseOBB\n");
	const PxU32 maxStack = 128;
	PxU32 stack[maxStack];

	PX_ASSERT(mPages);
	PX_ASSERT((Cm::MemFetchPtr(mPages) & 127) == 0);
	PX_ASSERT((Cm::MemFetchPtr(this) & 15) == 0);

	PxU8* treeNodes8 = PX_IS_X64 ? CAST_U8(get64BitBasePage()) : CAST_U8((mFlags & IS_DYNAMIC) ? NULL : mPages);
	PxU32* stackPtr = stack;

	VecU32V signMask = VecU32VLoadXYZW(PxU32(1)<<31, PxU32(1)<<31, PxU32(1)<<31, PxU32(1)<<31);
	Vec4V ones, halves, eps;
	ones = Vec4VLoadXYZW(1.0f, 1.0f, 1.0f, 1.0f);
	halves = Vec4VLoadXYZW(0.5f, 0.5f, 0.5f, 0.5f);
	eps = Vec4VLoadXYZW(1e-6f, 1e-6f, 1e-6f, 1e-6f);
	
	PX_UNUSED(ones);

	Vec4V zeroes = V4Zero();
	// project 
	Vec4V obbO = Vec4V_From_PxVec3_WUndefined(obb.center);
	Vec4V obbE = Vec4V_From_PxVec3_WUndefined(obb.extents);
	// Gu::Box::rot matrix columns are the OBB axes
	Vec4V obbX = Vec4V_From_PxVec3_WUndefined(obb.rot.column0);
	Vec4V obbY = Vec4V_From_PxVec3_WUndefined(obb.rot.column1);
	Vec4V obbZ = Vec4V_From_PxVec3_WUndefined(obb.rot.column2);

#if defined(PX_WINDOWS) || defined(PX_XBOXONE)
	// Visual Studio compiler hangs with #defines
	// On VMX platforms we use #defines in the other branch of this #ifdef to avoid register spills (LHS)
	Vec4V obbESplatX = V4SplatElement<0>(obbE);
	Vec4V obbESplatY = V4SplatElement<1>(obbE);
	Vec4V obbESplatZ = V4SplatElement<2>(obbE);
	Vec4V obbESplatNegX = V4Sub(zeroes, obbESplatX);
	Vec4V obbESplatNegY = V4Sub(zeroes, obbESplatY);
	Vec4V obbESplatNegZ = V4Sub(zeroes, obbESplatZ);
	Vec4V obbXE = V4MulAdd(obbX, obbESplatX, zeroes); // scale axii by E
	Vec4V obbYE = V4MulAdd(obbY, obbESplatY, zeroes); // scale axii by E
	Vec4V obbZE = V4MulAdd(obbZ, obbESplatZ, zeroes); // scale axii by E
	Vec4V obbOSplatX = V4SplatElement<0>(obbO);
	Vec4V obbOSplatY = V4SplatElement<1>(obbO);
	Vec4V obbOSplatZ = V4SplatElement<2>(obbO);
	Vec4V obbXSplatX = V4SplatElement<0>(obbX);
	Vec4V obbXSplatY = V4SplatElement<1>(obbX);
	Vec4V obbXSplatZ = V4SplatElement<2>(obbX);
	Vec4V obbYSplatX = V4SplatElement<0>(obbY);
	Vec4V obbYSplatY = V4SplatElement<1>(obbY);
	Vec4V obbYSplatZ = V4SplatElement<2>(obbY);
	Vec4V obbZSplatX = V4SplatElement<0>(obbZ);
	Vec4V obbZSplatY = V4SplatElement<1>(obbZ);
	Vec4V obbZSplatZ = V4SplatElement<2>(obbZ);
	Vec4V obbXESplatX = V4SplatElement<0>(obbXE);
	Vec4V obbXESplatY = V4SplatElement<1>(obbXE);
	Vec4V obbXESplatZ = V4SplatElement<2>(obbXE);
	Vec4V obbYESplatX = V4SplatElement<0>(obbYE);
	Vec4V obbYESplatY = V4SplatElement<1>(obbYE);
	Vec4V obbYESplatZ = V4SplatElement<2>(obbYE);
	Vec4V obbZESplatX = V4SplatElement<0>(obbZE);
	Vec4V obbZESplatY = V4SplatElement<1>(obbZE);
	Vec4V obbZESplatZ = V4SplatElement<2>(obbZE);
#else
	#define obbESplatX V4SplatElement<0>(obbE)
	#define obbESplatY V4SplatElement<1>(obbE)
	#define obbESplatZ V4SplatElement<2>(obbE)
	#define obbESplatNegX V4Sub(zeroes, obbESplatX)
	#define obbESplatNegY V4Sub(zeroes, obbESplatY)
	#define obbESplatNegZ V4Sub(zeroes, obbESplatZ)
	#define obbXE V4MulAdd(obbX, obbESplatX, zeroes)
	#define obbYE V4MulAdd(obbY, obbESplatY, zeroes)
	#define obbZE V4MulAdd(obbZ, obbESplatZ, zeroes)
	#define obbOSplatX V4SplatElement<0>(obbO)
	#define obbOSplatY V4SplatElement<1>(obbO)
	#define obbOSplatZ V4SplatElement<2>(obbO)
	#define obbXSplatX V4SplatElement<0>(obbX)
	#define obbXSplatY V4SplatElement<1>(obbX)
	#define obbXSplatZ V4SplatElement<2>(obbX)
	#define obbYSplatX V4SplatElement<0>(obbY)
	#define obbYSplatY V4SplatElement<1>(obbY)
	#define obbYSplatZ V4SplatElement<2>(obbY)
	#define obbZSplatX V4SplatElement<0>(obbZ)
	#define obbZSplatY V4SplatElement<1>(obbZ)
	#define obbZSplatZ V4SplatElement<2>(obbZ)
	#define obbXESplatX V4SplatElement<0>(obbXE)
	#define obbXESplatY V4SplatElement<1>(obbXE)
	#define obbXESplatZ V4SplatElement<2>(obbXE)
	#define obbYESplatX V4SplatElement<0>(obbYE)
	#define obbYESplatY V4SplatElement<1>(obbYE)
	#define obbYESplatZ V4SplatElement<2>(obbYE)
	#define obbZESplatX V4SplatElement<0>(obbZE)
	#define obbZESplatY V4SplatElement<1>(obbZE)
	#define obbZESplatZ V4SplatElement<2>(obbZE)
#endif

	PX_ASSERT(mPageSize == 4 || mPageSize == 8);
	PX_ASSERT(mNumRootPages > 0);
	PX_ASSERT(((mFlags & IS_DYNAMIC) && mNumRootPages == 1) || (mNumRootPages > 0)); // always only 1 top page for dynamic trees

	for (PxI32 j = PxI32(mNumRootPages-1); j >= 0; j --)
		*stackPtr++ = (mFlags & IS_DYNAMIC) ? pagePtrTo32Bits(mPages) : j*sizeof(RTreePage);
	PxU32 cacheTopValid = true;
	PxU32 cacheTop = (mFlags & IS_DYNAMIC) ? pagePtrTo32Bits(mPages) : 0;

	PX_ALIGN_PREFIX(16) PxU32 resa_[4] PX_ALIGN_SUFFIX(16);

	do {
		stackPtr--;

		PxU32 top;
		if (cacheTopValid) // branch is faster than lhs
			top = cacheTop;
		else
			top = stackPtr[0];
		PX_ASSERT(!cacheTopValid || top == cacheTop);
		RTreePage* __restrict tn = (RTreePage*)(treeNodes8 + top);
		#ifdef __SPU__
			RTreePage tmpPage;
			Cm::memFetchAlignedAsync(PxU64(&tmpPage), PxU64(treeNodes8+top), sizeof(RTreePage), 5);
			Cm::memFetchWait(5);
			tn = &tmpPage;
		#endif
#if RTREE_PAGE_SIZE == 8
		for (PxU32 offs = 0; offs != 8; offs += 4)
#else
		const PxU32 offs = 0;
#endif
		{
		PxU32* ptrs = ((RTreePage *)tn)->ptrs;

		if (0)
		{
			#define PF prefetchLine // __dcbt
			PxU32 ptr0 = ptrs[0+offs], ptr1 = ptrs[1+offs], ptr2 = ptrs[2+offs], ptr3 = ptrs[3+offs];
			PF(treeNodes8+ptr0); PF(treeNodes8+ptr1); PF(treeNodes8+ptr2); PF(treeNodes8+ptr3);
			#undef PF
		}

		// 6i
		Vec4V minx4a = V4LoadA(tn->minx+offs);
		Vec4V miny4a = V4LoadA(tn->miny+offs);
		Vec4V minz4a = V4LoadA(tn->minz+offs);
		Vec4V maxx4a = V4LoadA(tn->maxx+offs);
		Vec4V maxy4a = V4LoadA(tn->maxy+offs);
		Vec4V maxz4a = V4LoadA(tn->maxz+offs);

		VecU32V noOverlapa;
		VecU32V resa4u;
		{
			// PRECOMPUTE FOR A BLOCK
			// 109 instr per 4 OBB/AABB
			// ABB iteration 1, start with OBB origin as other point -- 6
			Vec4V p1ABBxa = V4Max(minx4a, V4Min(maxx4a, obbOSplatX));
			Vec4V p1ABBya = V4Max(miny4a, V4Min(maxy4a, obbOSplatY));
			Vec4V p1ABBza = V4Max(minz4a, V4Min(maxz4a, obbOSplatZ));

			// OBB iteration 1, move to OBB space first -- 12
			Vec4V p1ABBOxa = V4Sub(p1ABBxa, obbOSplatX);
			Vec4V p1ABBOya = V4Sub(p1ABBya, obbOSplatY);
			Vec4V p1ABBOza = V4Sub(p1ABBza, obbOSplatZ);
			Vec4V obbPrjXa = V4MulAdd(p1ABBOxa, obbXSplatX, V4MulAdd(p1ABBOya, obbXSplatY, V4MulAdd(p1ABBOza, obbXSplatZ, zeroes)));
			Vec4V obbPrjYa = V4MulAdd(p1ABBOxa, obbYSplatX, V4MulAdd(p1ABBOya, obbYSplatY, V4MulAdd(p1ABBOza, obbYSplatZ, zeroes)));
			Vec4V obbPrjZa = V4MulAdd(p1ABBOxa, obbZSplatX, V4MulAdd(p1ABBOya, obbZSplatY, V4MulAdd(p1ABBOza, obbZSplatZ, zeroes)));
			// clamp AABB point in OBB space to OBB extents. Since we scaled the axii, the extents are [-1,1] -- 6
			Vec4V pOBBxa = V4Max(obbESplatNegX, V4Min(obbPrjXa, obbESplatX));
			Vec4V pOBBya = V4Max(obbESplatNegY, V4Min(obbPrjYa, obbESplatY));
			Vec4V pOBBza = V4Max(obbESplatNegZ, V4Min(obbPrjZa, obbESplatZ));
			// go back to AABB space. we have x,y,z in obb space, need to multiply by axii -- 9
			Vec4V p1OBBxa = V4MulAdd(pOBBxa, obbXSplatX, V4MulAdd(pOBBya, obbYSplatX, V4MulAdd(pOBBza, obbZSplatX, obbOSplatX)));
			Vec4V p1OBBya = V4MulAdd(pOBBxa, obbXSplatY, V4MulAdd(pOBBya, obbYSplatY, V4MulAdd(pOBBza, obbZSplatY, obbOSplatY)));
			Vec4V p1OBBza = V4MulAdd(pOBBxa, obbXSplatZ, V4MulAdd(pOBBya, obbYSplatZ, V4MulAdd(pOBBza, obbZSplatZ, obbOSplatZ)));

			// ABB iteration 2 -- 6 instructions
			Vec4V p2ABBxa = V4Max(minx4a, V4Min(maxx4a, p1OBBxa));
			Vec4V p2ABBya = V4Max(miny4a, V4Min(maxy4a, p1OBBya));
			Vec4V p2ABBza = V4Max(minz4a, V4Min(maxz4a, p1OBBza));
			// above blocks add up to 12+12+15=39 instr
			// END PRECOMPUTE FOR A BLOCK

			// for AABBs precompute extents and center -- 9i
			Vec4V abbCxa = V4MulAdd(V4Add(maxx4a, minx4a), halves, zeroes);
			Vec4V abbCya = V4MulAdd(V4Add(maxy4a, miny4a), halves, zeroes);
			Vec4V abbCza = V4MulAdd(V4Add(maxz4a, minz4a), halves, zeroes);
			Vec4V abbExa = V4Sub(maxx4a, abbCxa);
			Vec4V abbEya = V4Sub(maxy4a, abbCya);
			Vec4V abbEza = V4Sub(maxz4a, abbCza);

			// now test separating axes D1 = p1OBB-p1ABB and D2 = p1OBB-p2ABB -- 37 instructions per axis
			// D1 first -- 3 instructions
			Vec4V d1xa = V4Sub(p1OBBxa, p1ABBxa), d1ya = V4Sub(p1OBBya, p1ABBya), d1za = V4Sub(p1OBBza, p1ABBza);

			// for AABB compute projections of extents and center -- 6
			Vec4V abbExd1Prja = V4MulAdd(d1xa, abbExa, zeroes);
			Vec4V abbEyd1Prja = V4MulAdd(d1ya, abbEya, zeroes);
			Vec4V abbEzd1Prja = V4MulAdd(d1za, abbEza, zeroes);
			Vec4V abbCd1Prja = V4MulAdd(d1xa, abbCxa, V4MulAdd(d1ya, abbCya, V4MulAdd(d1za, abbCza, zeroes)));

			// for obb project each halfaxis and origin and add abs values of half-axis projections -- 12 instructions
			Vec4V obbXEd1Prja = V4MulAdd(d1xa, obbXESplatX, V4MulAdd(d1ya, obbXESplatY, V4MulAdd(d1za, obbXESplatZ, zeroes)));
			Vec4V obbYEd1Prja = V4MulAdd(d1xa, obbYESplatX, V4MulAdd(d1ya, obbYESplatY, V4MulAdd(d1za, obbYESplatZ, zeroes)));
			Vec4V obbZEd1Prja = V4MulAdd(d1xa, obbZESplatX, V4MulAdd(d1ya, obbZESplatY, V4MulAdd(d1za, obbZESplatZ, zeroes)));
			Vec4V obbOd1Prja = V4MulAdd(d1xa, obbOSplatX, V4MulAdd(d1ya, obbOSplatY, V4MulAdd(d1za, obbOSplatZ, zeroes)));

			// compare lengths between projected centers with sum of projected radii -- 16i
			Vec4V originDiffd1a = v_absm(V4Sub(abbCd1Prja, obbOd1Prja));
			Vec4V absABBRd1a = V4Add(V4Add(v_absm(abbExd1Prja), v_absm(abbEyd1Prja)), v_absm(abbEzd1Prja));
			Vec4V absOBBRd1a = V4Add(V4Add(v_absm(obbXEd1Prja), v_absm(obbYEd1Prja)), v_absm(obbZEd1Prja));
			VecU32V noOverlapd1a = (VecU32V)V4IsGrtrV32u(V4Sub(originDiffd1a, eps), V4Add(absABBRd1a, absOBBRd1a));
			VecU32V epsNoOverlapd1a = (VecU32V)V4IsGrtrV32u(originDiffd1a, eps);

			// D2 next (35 instr)
			// 3i
			Vec4V d2xa = V4Sub(p1OBBxa, p2ABBxa), d2ya = V4Sub(p1OBBya, p2ABBya), d2za = V4Sub(p1OBBza, p2ABBza);
			// for AABB compute projections of extents and center -- 6
			Vec4V abbExd2Prja = V4MulAdd(d2xa, abbExa, zeroes);
			Vec4V abbEyd2Prja = V4MulAdd(d2ya, abbEya, zeroes);
			Vec4V abbEzd2Prja = V4MulAdd(d2za, abbEza, zeroes);
			Vec4V abbCd2Prja = V4MulAdd(d2xa, abbCxa, V4MulAdd(d2ya, abbCya, V4MulAdd(d2za, abbCza, zeroes)));
			// for obb project each halfaxis and origin and add abs values of half-axis projections -- 12i
			Vec4V obbXEd2Prja = V4MulAdd(d2xa, obbXESplatX, V4MulAdd(d2ya, obbXESplatY, V4MulAdd(d2za, obbXESplatZ, zeroes)));
			Vec4V obbYEd2Prja = V4MulAdd(d2xa, obbYESplatX, V4MulAdd(d2ya, obbYESplatY, V4MulAdd(d2za, obbYESplatZ, zeroes)));
			Vec4V obbZEd2Prja = V4MulAdd(d2xa, obbZESplatX, V4MulAdd(d2ya, obbZESplatY, V4MulAdd(d2za, obbZESplatZ, zeroes)));
			Vec4V obbOd2Prja = V4MulAdd(d2xa, obbOSplatX, V4MulAdd(d2ya, obbOSplatY, V4MulAdd(d2za, obbOSplatZ, zeroes)));
			// compare lengths between projected centers with sum of projected radii -- 16i
			Vec4V originDiffd2a = v_absm(V4Sub(abbCd2Prja, obbOd2Prja));
			Vec4V absABBRd2a = V4Add(V4Add(v_absm(abbExd2Prja), v_absm(abbEyd2Prja)), v_absm(abbEzd2Prja));
			Vec4V absOBBRd2a = V4Add(V4Add(v_absm(obbXEd2Prja), v_absm(obbYEd2Prja)), v_absm(obbZEd2Prja));
			VecU32V noOverlapd2a = (VecU32V)V4IsGrtrV32u(V4Sub(originDiffd2a, eps), V4Add(absABBRd2a, absOBBRd2a));
			VecU32V epsNoOverlapd2a = (VecU32V)V4IsGrtrV32u(originDiffd2a, eps);

			// 8i
			noOverlapa = V4U32or(V4U32and(noOverlapd1a, epsNoOverlapd1a), V4U32and(noOverlapd2a, epsNoOverlapd2a));
			VecU32V ignore4a = (VecU32V)V4IsGrtrV32u(minx4a, maxx4a); // 1 if degenerate box (empty slot)
			noOverlapa = V4U32or(noOverlapa, ignore4a);
			resa4u = V4U32Andc(U4Load(1), noOverlapa); // 1 & ~noOverlap
			V4U32StoreAligned(resa4u, (VecU32V*)resa_);
			///// 8+16+12+6+3+16+12+6+3+9+6+9+6+12+6+6=136i from load to result
		}

		// search for VERIFY_RTREE, insert here if needed for debugging
		cacheTopValid = false;
		for (PxU32 i = 0; i < 4; i++)
		{
			PxU32 ptr = ptrs[i+offs] & ~1; // clear the isLeaf bit
			if (resa_[i])
			{
				if (tn->isLeaf(i))
				{
					//pxPrintf("obb collider: report value %d\n", ptr);
					if (!callback->processResults(1, &ptr))
						return;
				}
				else
				{
					*(stackPtr++) = ptr;
					cacheTop = ptr;
					cacheTopValid = true;
				}
			}
		}
		} // end of scope for 2-iteration loop for 8-way or dummy scope for 4-way rtree
	} while (stackPtr > stack);
}

} // namespace Gu

}
