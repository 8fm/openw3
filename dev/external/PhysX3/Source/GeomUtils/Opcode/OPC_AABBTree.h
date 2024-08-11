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

#ifndef OPC_AABBTREE_H
#define OPC_AABBTREE_H

#include "PxMemory.h"
#include "PsUserAllocated.h"
#include "Opcode.h"
#include "PsVecMath.h"

#define SUPPORT_PROGRESSIVE_BUILDING
#define SUPPORT_REFIT_BITMASK
#define SUPPORT_UPDATE_ARRAY	128	// PT: consumes SUPPORT_UPDATE_ARRAY*sizeof(PxU32) bytes per AABBTree

namespace physx
{

using namespace shdfnd::aos;

namespace Gu
{
	class Plane;
	class Container;
	class AABBTreeBuilder;

	class BitArray
	{
		public:
										BitArray();
										BitArray(PxU32 nb_bits);
										~BitArray();

						bool			Init(PxU32 nb_bits);

		// Data management
		PX_FORCE_INLINE	void			SetBit(PxU32 bit_number)
										{
											mBits[bit_number>>5] |= 1<<(bit_number&31);
										}
		PX_FORCE_INLINE	void			ClearBit(PxU32 bit_number)
										{
											mBits[bit_number>>5] &= ~(1<<(bit_number&31));
										}
		PX_FORCE_INLINE	void			ToggleBit(PxU32 bit_number)
										{
											mBits[bit_number>>5] ^= 1<<(bit_number&31);
										}

		PX_FORCE_INLINE	void			ClearAll()			{ PxMemZero(mBits, mSize*4);		}
		PX_FORCE_INLINE	void			SetAll()			{ PxMemSet(mBits, 0xff, mSize*4);	}

		// Data access
		PX_FORCE_INLINE	Ps::IntBool		IsSet(PxU32 bit_number)	const
										{
											return mBits[bit_number>>5] & (1<<(bit_number&31));
										}

		PX_FORCE_INLINE	const PxU32*	GetBits()	const	{ return mBits;		}
		PX_FORCE_INLINE	PxU32			GetSize()	const	{ return mSize;		}

		protected:
						PxU32*			mBits;		//!< Array of bits
						PxU32			mSize;		//!< Size of the array in dwords
	};


#ifdef SUPPORT_PROGRESSIVE_BUILDING
	class FIFOStack2;
#endif

	class AABBTreeNode;

#ifndef PX_WIIU
	// template because we rely on the linker to merge duplicate constants from different modules
	template<typename _> struct AABBCompressionConstantsT
	{
		static VecU32V	ff PX_WEAK_SYMBOL;
		static Vec4V	scaleMul4 PX_WEAK_SYMBOL;
		static PxF32	scaleMul1 PX_WEAK_SYMBOL;
	};
	typedef AABBCompressionConstantsT<int> AABBCompressionConstants;

	template<> VecU32V AABBCompressionConstants::ff = VecU32VLoadXYZW(0xFF, 0xFF, 0xFF, 0xFF);
	template<> Vec4V   AABBCompressionConstants::scaleMul4 = V4Load(1.0f/10000.0f);
	template<> PxF32   AABBCompressionConstants::scaleMul1 = 1.0f/10000.0f;
#else
	// since the template is fully specialized above, it is like any other static member definition in a header and causes linker errors on Wii U (multiple definitions).
	// Using the -multiple flag is not an option for every customer, so we fix it for Wii U.
	struct AABBCompressionConstants
	{
		static VecU32V	ff PX_WEAK_SYMBOL;
		static Vec4V	scaleMul4 PX_WEAK_SYMBOL;
		static PxF32	scaleMul1 PX_WEAK_SYMBOL;
	};
#endif

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	User-callback, called for each node by the walking code.
	 *	\param		current		[in] current node
	 *	\param		depth		[in] current node's depth
	 *	\param		user_data	[in] user-defined data
	 *	\return		true to recurse through children, else false to bypass them
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	typedef		bool				(*WalkingCallback)	(const AABBTreeNode* current, PxU32 depth, void* user_data);

	class PX_PHYSX_COMMON_API AABBTree : public Ps::UserAllocated
	{
		public:
											AABBTree();
											~AABBTree();
		// Build
						bool				Build(AABBTreeBuilder* builder);
#ifdef SUPPORT_PROGRESSIVE_BUILDING
						PxU32				ProgressiveBuild(AABBTreeBuilder* builder, PxU32 progress, PxU32 limit);
#endif
						void				Release();

		// Data access
		PX_FORCE_INLINE	const PxU32*		GetIndices()		const	{ return mIndices;		}
		PX_FORCE_INLINE	PxU32*				GetIndices()				{ return mIndices;		}
		PX_FORCE_INLINE	PxU32				GetNbNodes()		const	{ return mTotalNbNodes;	}
		PX_FORCE_INLINE	PxU32				GetTotalPrims()		const	{ return mTotalPrims;	}
		PX_FORCE_INLINE	const AABBTreeNode*	GetNodes()			const	{ return mPool;			}
		PX_FORCE_INLINE	AABBTreeNode*		GetNodes()					{ return mPool;			}

		// Stats
						PxU32				ComputeDepth()		const;
						PxU32				Walk(WalkingCallback callback, void* user_data) const;
						void				Walk2(WalkingCallback callback, void* user_data) const;

						bool				Refit2(AABBTreeBuilder* builder, PxU32* indices);
#ifndef SUPPORT_REFIT_BITMASK
						bool				Refit3(PxU32 nb_objects, const PxBounds3* boxes, const Container& indices);
#endif
#ifdef PX_DEBUG 
						void				Validate() const;
#endif
						void				MarkForRefit(PxU32 index);
						void				RefitMarked(PxU32 nb_objects, const PxBounds3* boxes, PxU32* indices);

						void				ShiftOrigin(const PxVec3& shift);
#if PX_IS_SPU
		PX_FORCE_INLINE	void				SetNodes(AABBTreeNode* lsPool) { mPool = lsPool; }
#endif
		private:
						PxU32*				mIndices;			//!< Indices in the app list. Indices are reorganized during build (permutation).
						AABBTreeNode*		mPool;				//!< Linear pool of nodes for complete trees. NULL otherwise. [Opcode 1.3]
						BitArray			mRefitBitmask;
						PxU32				mRefitHighestSetWord;
#ifdef SUPPORT_UPDATE_ARRAY
						PxU32				mNbRefitNodes;
						PxU32				mRefitArray[SUPPORT_UPDATE_ARRAY];
#endif
		// Stats
						PxU32				mTotalNbNodes;		//!< Number of nodes in the tree.
						PxU32				mTotalPrims;
#ifdef SUPPORT_PROGRESSIVE_BUILDING
						FIFOStack2*			mStack;
#endif
						friend class AABBTreeNode;
	};

	// AP: adding #pragma pack(push,4) makes linux64 crash but saves 4 bytes per node
	// currently on 64 bit platforms the size is 24 bytes
	class AABBTreeNode : public Ps::UserAllocated
	{
		public:	
		PX_FORCE_INLINE					AABBTreeNode() : mBitfield(0) {};
		PX_FORCE_INLINE					~AABBTreeNode() {};
		// Data access
		// Decompress this node's bounds given parent center and parent extents
		PX_INLINE	const PxVec3&		GetAABBCenter() const { return *(PxVec3*)&mCx; }
		PX_INLINE	PxVec3				GetAABBExtents() const { PxI32 scale = (getExtentScale() << 8); return PxVec3(PxReal(mEx & 0xFF), PxReal(mEy & 0xFF), PxReal(mEz & 0xFF)) * PX_FR(scale) * AABBCompressionConstants::scaleMul1; }
		PX_FORCE_INLINE void			GetAABBCenterExtentsV(Vec3V* center, Vec3V* extents) const;

		// Child/parent access
		PX_INLINE	const AABBTreeNode*	GetPos(const AABBTreeNode* base) const { PX_ASSERT(!IsLeaf()); PxU32 pos = getPosOrNodePrimitives(); return base + pos; } // 0 returns base, so backptr to root equivalent to NULL
		PX_INLINE	const AABBTreeNode*	GetNeg(const AABBTreeNode* base) const { const AABBTreeNode* P = GetPos(base); return P != base ? P+1 : base; }
		PX_INLINE	const AABBTreeNode*	GetParent(const AABBTreeNode* base) const { return base + getNbBuildPrimitivesOrParent(); }

		// Leaf flag access
		PX_INLINE	PxU32				IsLeaf()		const	{ return getIsLeaf(); }
		PX_INLINE	void				SetLeaf()				{ setIsLeaf(1); }
		PX_INLINE	void				ClearLeaf()				{ setIsLeaf(0); }

		// Stats
		PX_INLINE	PxU32				GetNodeSize()	const	{ return sizeof(*this);	}
		PX_INLINE	void				SetPos(PxU32 val) { PX_ASSERT(!IsLeaf()); setPosOrNodePrimitives(val); }
		PX_INLINE	void				SetNodePrimitives(PxU32 val) { PX_ASSERT(!IsLeaf()); setPosOrNodePrimitives(val); }
		PX_INLINE	void				SetParent(PxU32 val) { setNbBuildPrimitivesOrParent(val); }
		PX_INLINE	void				SetNbBuildPrimitives(PxU32 val) { setNbBuildPrimitivesOrParent(val); }

		// WARNING: typically you want to use Compress<writeBack=1>
		// calling this function with writeBack=0 must be followed by a call to WriteBack
		// passing the values previously returned from Compress
		template<int writeBack>
		PX_FORCE_INLINE	void			Compress(const Vec3V exactBoundsMin, const Vec3V exactBoundsMax, VecU32V* result1 = NULL, Vec3V* result2 = NULL);
		PX_FORCE_INLINE void			WriteBack(const VecU32V& scale, const Vec3V& pos)
										{
											V3StoreU(pos, *(PxVec3*)&mCx);
											setExtentScale( (((PxU32*)&scale)[1] >> 8) );
										}
		// Data access
		PX_INLINE	const PxU32*		GetPrimitives(const PxU32* treeIndices) const { return treeIndices + getPosOrNodePrimitives(); }
		PX_INLINE	PxU32*				GetPrimitives(PxU32* treeIndices) const { return treeIndices + getPosOrNodePrimitives(); }
		PX_INLINE	PxU32				GetNbRuntimePrimitives() const { return getNbRuntimePrimitives(); }
		PX_INLINE	PxU32				GetNodePrimitives() const { PX_ASSERT(IsLeaf()); return getPosOrNodePrimitives(); }
		PX_INLINE	PxU32				GetNbBuildPrimitives() const { return getNbBuildPrimitivesOrParent(); }

					// Compressed node AABB center and extents
					// upper 24 bits of mCx,mCy,mCz are used for the float representation of center's xyz
					// lower 8 bits of mEx,mEy,mEz are used for extents.
					// The extents are computed from compressed representation by multiplying by mExtentScale*GetExtentScaleMultiplier()
					// mExtentScale is stored as a clipped float (lower 8 bits of mantissa are dropped)
					// see GetAABBCenter(), GetAABBExtents() for decompression code and Compress() for compression code
					// GetAABBCenterExtentsV() is a SIMD optimized version that decompresses both center and extents in one shot
					union { PxF32 mCx; PxU32 mEx; };
					union { PxF32 mCy; PxU32 mEy; };
					union { PxF32 mCz; PxU32 mEz; };

					// the bitfield layout is, in this order, most significant bits first:
					// mExtentScale,
					// mPosOrNodePrimitives,
					// mNbBuildPrimitivesOrParent,
					// mNbRuntimePrimitives,
					// mIsLeaf
					enum { EXSC_BITS = 24, POSNODE_BITS = 19, BUILDPARENT_BITS = 19, NBRUNTIME_BITS = 1, ISLEAF_BITS = 1 };
					enum { EXSC_OFFS = 63, POSNODE_OFFS = EXSC_OFFS-EXSC_BITS, BUILDPARENT_OFFS = POSNODE_OFFS-POSNODE_BITS,
						NBRUNTIME_OFFS = BUILDPARENT_OFFS-BUILDPARENT_BITS, ISLEAF_OFFS = NBRUNTIME_OFFS-NBRUNTIME_BITS };

					// start is the index of the bit in mBitfield, count the number of bits down from there
					PX_INLINE PxU32 getBits(PxU32 start, PxU32 count) const
					{
						PX_ASSERT(count >= 1 && count <= 63);
						PX_ASSERT(start <= 63); // check that start and count are within range
						PX_ASSERT(PxI32(start+1) - PxI32(count) >= 0);
						return Ps::to32((mBitfield << (63-start)) >> (64-count));
					}

					PX_INLINE void setBits(PxU64 val, PxU32 start, PxU32 count)
					{
						PX_ASSERT(val < PxU32(PxU64(1)<<count));
						PX_ASSERT(count >= 1 && count <= 63);
						PX_ASSERT(start <= 63);
						PX_ASSERT(PxI32(start+1) - PxI32(count) >= 0);
								#ifdef PX_DEBUG // verify that we preserved the bits on the boundaries
								PxU32 oldHighBit = (start < 63) ? getBits(start+1, 1) : 0;
								PxU32 oldLowBit = (PxI32(start)-PxI32(count) >= 0) ? getBits(start-count, 1) : 0;
								#endif
						PxU64 clearMask = ~(((PxU64(1)<<count)-1)<<(start+1-count));
						mBitfield = (mBitfield & clearMask) | (val << (start+1-count));
						PX_ASSERT(getBits(start, count) == val); // verify that we read what we just wrote
								#ifdef PX_DEBUG
								// verify that we preserved bits on the boundaries
								PX_ASSERT(start < 63 ? oldHighBit == getBits(start+1, 1) : true);
								PX_ASSERT(PxI32(start)-PxI32(count) >= 0 ? oldLowBit == getBits(start-count, 1) : true);
								#endif
					}

					PX_INLINE PxU32	getExtentScale() const					
					{ 						
						#if PX_IS_INTEL
						return ((const PxU32*)&mBitfield)[1]>>8;
						#else
						return getBits(EXSC_OFFS, EXSC_BITS);
						#endif
					}
					PX_INLINE void	setExtentScale(PxU32 val)				{ setBits(val, EXSC_OFFS, EXSC_BITS); }
					PX_INLINE PxU32	getPosOrNodePrimitives() const			{ return getBits(POSNODE_OFFS, POSNODE_BITS); }
					PX_INLINE void	setPosOrNodePrimitives(PxU32 val)		{ setBits(val, POSNODE_OFFS, POSNODE_BITS); }
					PX_INLINE PxU32	getNbBuildPrimitivesOrParent() const	{ return getBits(BUILDPARENT_OFFS, BUILDPARENT_BITS); }
					PX_INLINE void	setNbBuildPrimitivesOrParent(PxU32 val)	{ setBits(val, BUILDPARENT_OFFS, BUILDPARENT_BITS); }
					PX_INLINE PxU32	getNbRuntimePrimitives() const			{ return getBits(NBRUNTIME_OFFS, NBRUNTIME_BITS); }
					PX_INLINE void	setNbRunTimePrimitives(PxU32 val)		{ setBits((val & ((1<<NBRUNTIME_BITS)-1)), NBRUNTIME_OFFS, NBRUNTIME_BITS); }
					PX_INLINE PxU32	getIsLeaf() const						{ return getBits(ISLEAF_OFFS, ISLEAF_BITS); }
					PX_INLINE void	setIsLeaf(PxU32 val)					{ setBits(val, ISLEAF_OFFS, ISLEAF_BITS); }

					PxU64			mBitfield; 

					// Internal methods
					PX_INLINE PxU32		Split(const PxBounds3& exactBounds, PxU32 axis, AABBTreeBuilder* builder, PxU32* indices);
					PX_INLINE bool		Subdivide(const PxBounds3& exactBounds, AABBTreeBuilder* builder, PxU32* indices);
					PX_INLINE void		_BuildHierarchy(AABBTreeBuilder* builder, PxU32* indices);
	};

#define PX_IS_DENORM(x) (x != 0.0f && PxAbs(x) < FLT_MIN)
#define PX_AABB_COMPRESSION_MAX 1e33f // max value supported by compression

// The specs for compression are:
// 1. max(abs(bounds.min.xyz), abs(bounds.max.xyz))<PX_AABB_COMPRESSION_MAX
// 2. decompressedOutput.contains(originalInput) for extents >= 0
// 3. if originalInput.getExtents().xyz < 0 then decompressedOutput.getExtents() will also be negative but there are no inclusion or any other guarantees
template<int writeBack>
PX_FORCE_INLINE void AABBTreeNode::Compress(
	const Vec3V aExactBoundsMin, const Vec3V aExactBoundsMax, VecU32V* result1, Vec3V* result2)
{
	// assert that aabb min is under compression max and aabb max is over minus compression min to save clamping instructions
	PX_ASSERT(FStore(V3ExtractMax(aExactBoundsMin))<PX_AABB_COMPRESSION_MAX && "Bounds out of range in AABB::Compress");
	PX_ASSERT(FStore(V3ExtractMin(aExactBoundsMax))>-PX_AABB_COMPRESSION_MAX && "Bounds out of range in AABB::Compress");
	static const FloatV one256th = FLoad(1.0f/254.999f);
	// original scale*=(1+1/255), bake this into invExtentScaleMul
	static const Vec3V invExtentScaleMul = V3Recip(Vec3V_From_Vec4V(AABBCompressionConstants::scaleMul4));
	static const FloatV half = FHalf();
	static const FloatV ep4 = FLoad(1e-4f);
	static const FloatV fmin4 = FLoad(2.0f*FLT_MIN);
	static const VecU32V ff00 = VecU32VLoadXYZW(0xFFffFF00, 0xFFffFF00, 0xFFffFF00, 0xFFffFF00);
	static const FloatV ones = FOne();
	static const Vec3V clampBounds = Vec3V_From_FloatV(FLoad(PX_AABB_COMPRESSION_MAX)); // clamp the bounds to compression allowed max
	Vec3V exactBoundsMin = V3Max(aExactBoundsMin, V3Neg(clampBounds)); // only clamp the min from below to save 2 instructions
	Vec3V exactBoundsMax = V3Min(aExactBoundsMax, clampBounds); // only clamp the max from above to save 2 instructions

	Vec3V exactCenter = V3Scale(V3Add(exactBoundsMin, exactBoundsMax), half); // compute exact center
	Vec3V exactExtents = V3Scale(V3Sub(exactBoundsMax, exactBoundsMin), half); // compute exact extents

	// see how much in float space we can be most off by in max(x,y,z) after overwriting the low 8 bits
	// errCenter = maximum error introduced by clipping the low 8 bits of exactCenter
	// add ep4 do avoid a divide by zero later
	Vec3V errCenter = V3Add(V3Abs(V3Mul(exactCenter, Vec3V_From_FloatV(ep4))), Vec3V_From_FloatV(ep4));

	// forward formula:
	// PxI32 scale = PxI32(mExtentScale) << 8;
	// return PxVec3(mEx & 0xFF, mEy & 0xFF, mEz & 0xFF) * PX_FR(scale) * GetExtentScaleMultiplier(); }

	// we now try to convert 3x float extents to 3x8 bit values with a common 24-bit multiplier
	// we do this by finding the maximum of 3 extents and a multiplier such that dequantization produces conservative bounds
	Vec3V errCenterExactExtents = V3Add(errCenter, exactExtents); // we incorporate error from center compression by inflating extents
	Vec3V splatExtentX = Vec3V_From_Vec4V(V4SplatElement<0>(Vec4V_From_Vec3V(errCenterExactExtents))); // includes center error
	Vec3V splatExtentY = Vec3V_From_Vec4V(V4SplatElement<1>(Vec4V_From_Vec3V(errCenterExactExtents))); // includes center error
	Vec3V splatExtentZ = Vec3V_From_Vec4V(V4SplatElement<2>(Vec4V_From_Vec3V(errCenterExactExtents))); // includes center error
	Vec3V maxExtentXYZ = V3Max(splatExtentX, V3Max(splatExtentY, splatExtentZ)); // find the maximum of extent.x, extent.y, extent.z
	Vec3V maxExtent255 = V3Scale(maxExtentXYZ, one256th); // divide maxExtent by 255 so we can fit into 0..255 range
	// add error, inv scale maxExtent255*= 1.0f + ep4. scaleMul4 is to improve precision
	Vec3V maxExtent255e = V3Mul(V3Add(maxExtent255, V3Mul(maxExtent255, Vec3V_From_FloatV(ep4))), invExtentScaleMul);
	VecU32V u;
	if (writeBack)
		V4U32StoreAligned(VecU32V_ReinterpretFrom_Vec4V(Vec4V_From_Vec3V(maxExtent255e)), &u);
	else
		V4U32StoreAligned(VecU32V_ReinterpretFrom_Vec4V(Vec4V_From_Vec3V(maxExtent255e)), result1);

	Vec3V valXYZ = V3Mul(V3Add(exactExtents, errCenter), V3Recip(maxExtent255));
	VecI32V valXYZi = VecI32V_From_Vec4V(Vec4V_From_Vec3V(V3Add(valXYZ, Vec3V_From_FloatV(ones))));
	valXYZ = Vec3V_From_Vec4V(Vec4V_ReinterpretFrom_VecI32V(valXYZi));


#ifdef PX_DEBUG
	PX_ALIGN_PREFIX(16) PxU32 test[4] PX_ALIGN_SUFFIX(16) = {255, 255, 255, 255};
	V4U32StoreAligned(VecU32V_ReinterpretFrom_Vec4V(Vec4V_From_Vec3V(valXYZ)), (VecU32V*)test);
	PX_ASSERT(test[0]<=255 && test[1]<=255 && test[2]<=255 && test[3]<=255);
#endif
	// the input (exact center in floats) has to be correct meaning it's either zero or non-denorm
	// if its a non-denorm we can put anything into the low 8 bits and maintain non-denorm
	// if it's a strict zero we replace it with fmin4
	// next we mask off the low 8 bits of mantissa, logical or with quantized 8-bit xyz extents
	Vec3V zero = V3Sub(Vec3V_From_FloatV(ones), Vec3V_From_FloatV(ones)); // should be faster than loading FZero() i think
	Vec3V nonzeroExactCenter = V3Sel(V3IsEq(exactCenter, zero), Vec3V_From_FloatV(fmin4), exactCenter); 
	VecU32V resu32 =
		V4U32or(
			V4U32and(VecU32V_ReinterpretFrom_Vec4V(Vec4V_From_Vec3V( nonzeroExactCenter )), ff00),
			VecU32V_ReinterpretFrom_Vec4V(Vec4V_From_Vec3V(valXYZ)));
	Vec3V res = Vec3V_From_Vec4V(Vec4V_ReinterpretFrom_VecU32V(resu32));
	if (writeBack)
	{
		V3StoreU(res, *(PxVec3*)&mCx);
		PX_ASSERT(!PX_IS_DENORM(mCx) && !PX_IS_DENORM(mCy) && !PX_IS_DENORM(mCz));
		setExtentScale( (((PxU32*)&u)[1] >> 8) );
	} else
		*result2 = res;

#ifdef PX_DEBUG
	if (!writeBack)
		return;
	else
	{
		// verify that compression produced conservative bounds
		PxI32 es = getExtentScale() << 8;
		PX_ASSERT(!PX_IS_DENORM(PX_FR(es)));
		PxF32 escale = PX_FR(es) * AABBCompressionConstants::scaleMul1; PX_UNUSED(escale);
		PxVec3 deqCenter = GetAABBCenter(), deqExtents = GetAABBExtents();
		PxBounds3 exactBounds;
		V3StoreU(exactBoundsMin, exactBounds.minimum);
		V3StoreU(exactBoundsMax, exactBounds.maximum);
		PxBounds3 deqBounds = PxBounds3::centerExtents(deqCenter, deqExtents);
		PX_ASSERT(((exactBounds.minimum.x > exactBounds.maximum.x) && (deqBounds.minimum.x > deqBounds.maximum.x)) || exactBounds.isInside(deqBounds));

		PX_ASSERT(!PX_IS_DENORM(mCx) && !PX_IS_DENORM(mCy) && !PX_IS_DENORM(mCz));
	}
#endif
}


#if (defined(PX_X86) || defined (PX_X64))
PX_FORCE_INLINE Vec4V convertScale(PxI32 scale)
{
#if COMPILE_VECTOR_INTRINSICS
	return _mm_castsi128_ps(_mm_cvtsi32_si128(scale));
#else
	PX_ASSERT(false);
	return Vec4V();
#endif
}
#endif

#if (defined(PX_X86) || defined (PX_X64))
PX_FORCE_INLINE Vec4V splatScale(Vec4V s)
{
#if COMPILE_VECTOR_INTRINSICS
	return _mm_shuffle_ps(s, s, 0);  //isn't this the same as V4SplatElement<0>(s) ?
#else
	PX_ASSERT(false);
	return Vec4V();
#endif
}
#endif

PX_FORCE_INLINE	void AABBTreeNode::GetAABBCenterExtentsV(Vec3V* center, Vec3V* extents) const
{
	Vec3V xyz = V3LoadU(*(PxVec3*)&mCx);
	PX_ASSERT(!PX_IS_DENORM(mCx) && !PX_IS_DENORM(mCy) && !PX_IS_DENORM(mCz));
	*center = xyz;
#if defined(PX_X360) // not the same bitfield layout on PS3 PPU
	// AP: load straight from a bitmask into vec4v. sketchy but works. eliminates a couple LHS. verification code below.
	Vec4V xyzs = V4LoadU(&mCy);
	xyzs = V4Andc(V4SplatElement<3>(xyzs), AABBCompressionConstants::ff); // splat scale
	FloatV multiplier = V4Mul(xyzs, AABBCompressionConstants::scaleMul4);
	#ifdef PX_DEBUG
		PxI32 verify; PxI32_From_VecI32V(VecI32V_ReinterpretFrom_Vec4V(xyzs), &verify);
		PX_ASSERT((PxI32(getExtentScale()) << 8) == verify); // verify that the bitfield layout is in sync with load code and hasn't changed
	#endif
#else
	PxI32 scale = PxI32(getExtentScale()) << 8;
	PX_ASSERT(!PX_IS_DENORM(scale));

	#if (defined(PX_X86) || defined (PX_X64))
		Vec4V s = convertScale(scale); // AP todo: make this cross-platform
		Vec4V scaleV =splatScale(s);
		Vec4V multiplier = V4Mul(AABBCompressionConstants::scaleMul4, scaleV);
	#else
		FloatV multiplier = FLoad(PX_FR(scale) * AABBCompressionConstants::scaleMul1);
	#endif
#endif
	VecU32V toInt = V4U32and(VecU32V_ReinterpretFrom_Vec4V(Vec4V_From_Vec3V(xyz)), AABBCompressionConstants::ff);
	Vec4V extents3 = Vec4V_From_VecI32V((VecI32V&)toInt);
	// we know xyz has a 0 in W, hence so does toInt, hence also extents3. 

	#if (defined(PX_X86) || defined (PX_X64))
		*extents = Vec3V_From_Vec4V_WUndefined(V4Mul(extents3, multiplier));
	#else
		*extents = Vec3V_From_Vec4V_WUndefined(V4Mul(extents3, Vec4V_From_FloatV(multiplier)));
	#endif
	PX_ASSERT(!PX_IS_DENORM(((PxVec3*)extents)->x));
	PX_ASSERT(!PX_IS_DENORM(((PxVec3*)extents)->y));
	PX_ASSERT(!PX_IS_DENORM(((PxVec3*)extents)->z));
	PX_ASSERT(((PxVec4*)extents)->w==0);
}

} // namespace Gu

}

#endif // OPC_AABBTREE_H
