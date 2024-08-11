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

#define RTREE_TEXT_DUMP_ENABLE		0
#ifdef PX_X64
#define RTREE_PAGES_PER_POOL_SLAB	16384 // preallocate all pages in first batch to make sure we stay within 32 bits for relative pointers.. this is 2 megs
#else
#define RTREE_PAGES_PER_POOL_SLAB	128
#endif

#define INSERT_SCAN_LOOKAHEAD		1 // enable one level lookahead scan for determining which child page is best to insert a node into

#include "GuRTree.h"
#include "PsSort.h"
#include "CmPhysXCommon.h"
#include "PsAlignedMalloc.h"
#include "../../GeomUtils/Opcode/Ice/IceUtils.h" // for LittleEndian
#include "../../GeomUtils/Opcode/Ice/IceSerialize.h" // for WriteDword etc
#include "CmMemFetch.h"
#include "PsPool.h"
#include "CmUtils.h"

using namespace physx;
using Ps::Array;
using Ps::sort;
using namespace Gu;

namespace physx
{
namespace Gu {

/////////////////////////////////////////////////////////////////////////
#ifdef PX_X360
#define CONVERT_PTR_TO_INT PxU32
#else
#define CONVERT_PTR_TO_INT PxU64
#endif

#ifdef PX_X64
RTreePage* RTree::sFirstPoolPage = NULL; // used for relative addressing on 64-bit platforms
#endif

/////////////////////////////////////////////////////////////////////////
RTree::RTree()
{
	PX_ASSERT((Cm::MemFetchPtr(this) & 15) == 0);
	mFlags = 0;
	mPages = NULL;
	mTotalNodes = 0;
	mNumLevels = 0;
	mPageSize = RTreePage::SIZE;
}

/////////////////////////////////////////////////////////////////////////
PxU32 RTree::mVersion = 1;

bool RTree::save(PxOutputStream& stream) const
{
	/*
		PxVec4		mBoundsMin, mBoundsMax, mInvDiagonal, mDiagonalScaler;
		PxU32		mPageSize;
		PxU32		mNumRootPages;
		PxU32		mNumLevels;
		PxU32		mTotalNodes;
		PxU32		mTotalPages;
		RTreePage*	mPages;
	*/

	bool mismatch = (LittleEndian() == 1);
	WriteChunk('R', 'T', 'R', 'E', stream);
	WriteDword(mVersion, mismatch, stream);
	WriteFloatBuffer(&mBoundsMin.x, 4, mismatch, stream);
	WriteFloatBuffer(&mBoundsMax.x, 4, mismatch, stream);
	WriteFloatBuffer(&mInvDiagonal.x, 4, mismatch, stream);
	WriteFloatBuffer(&mDiagonalScaler.x, 4, mismatch, stream);
	WriteDword(mPageSize, mismatch, stream);
	WriteDword(mNumRootPages, mismatch, stream);
	WriteDword(mNumLevels, mismatch, stream);
	WriteDword(mTotalNodes, mismatch, stream);
	WriteDword(mTotalPages, mismatch, stream);
	WriteDword(mUnused, mismatch, stream);
	for (PxU32 j = 0; j < mTotalPages; j++)
	{
		WriteFloatBuffer(mPages[j].minx, RTreePage::SIZE, mismatch, stream);
		WriteFloatBuffer(mPages[j].miny, RTreePage::SIZE, mismatch, stream);
		WriteFloatBuffer(mPages[j].minz, RTreePage::SIZE, mismatch, stream);
		WriteFloatBuffer(mPages[j].maxx, RTreePage::SIZE, mismatch, stream);
		WriteFloatBuffer(mPages[j].maxy, RTreePage::SIZE, mismatch, stream);
		WriteFloatBuffer(mPages[j].maxz, RTreePage::SIZE, mismatch, stream);
		WriteDwordBuffer(mPages[j].ptrs, RTreePage::SIZE, mismatch, stream);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////
bool RTree::load(PxInputStream& stream, PxU32 meshVersion)
{
	PX_ASSERT((mFlags & IS_DYNAMIC) == 0);
	PX_UNUSED(meshVersion);

	release();

	PxU8 a, b, c, d;
	ReadChunk(a, b, c, d, stream);
	if(a!='R' || b!='T' || c!='R' || d!='E')
		return false;

	bool mismatch = (LittleEndian() == 1);
	if (ReadDword(mismatch, stream) != mVersion)
		return false;

	ReadFloatBuffer(&mBoundsMin.x, 4, mismatch, stream);
	ReadFloatBuffer(&mBoundsMax.x, 4, mismatch, stream);
	ReadFloatBuffer(&mInvDiagonal.x, 4, mismatch, stream);
	ReadFloatBuffer(&mDiagonalScaler.x, 4, mismatch, stream);
	mPageSize = ReadDword(mismatch, stream);
	mNumRootPages = ReadDword(mismatch, stream);
	mNumLevels = ReadDword(mismatch, stream);
	mTotalNodes = ReadDword(mismatch, stream);
	mTotalPages = ReadDword(mismatch, stream);
	mUnused = ReadDword(mismatch, stream);

	mPages = static_cast<RTreePage*>(
		Ps::AlignedAllocator<128>().allocate(sizeof(RTreePage)*mTotalPages, __FILE__, __LINE__));
	for (PxU32 j = 0; j < mTotalPages; j++)
	{
		ReadFloatBuffer(mPages[j].minx, RTreePage::SIZE, mismatch, stream);
		ReadFloatBuffer(mPages[j].miny, RTreePage::SIZE, mismatch, stream);
		ReadFloatBuffer(mPages[j].minz, RTreePage::SIZE, mismatch, stream);
		ReadFloatBuffer(mPages[j].maxx, RTreePage::SIZE, mismatch, stream);
		ReadFloatBuffer(mPages[j].maxy, RTreePage::SIZE, mismatch, stream);
		ReadFloatBuffer(mPages[j].maxz, RTreePage::SIZE, mismatch, stream);
		ReadDwordBuffer(mPages[j].ptrs, RTreePage::SIZE, mismatch, stream);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////
PxU32 RTree::computeBottomLevelCount(PxU32 multiplier) const
{
	PX_ASSERT((mFlags & IS_DYNAMIC) == 0);
	PxU32 topCount = 0, curCount = mNumRootPages;
	const RTreePage* rightMostPage = &mPages[mNumRootPages-1];
	PX_ASSERT(rightMostPage);
	for (PxU32 level = 0; level < mNumLevels-1; level++)
	{
		topCount += curCount;
		PxU32 nc = rightMostPage->nodeCount();
		PX_ASSERT(nc > 0 && nc <= RTreePage::SIZE);
		// old version pointer, up to PX_MESH_VERSION 8
		PxU32 ptr = (rightMostPage->ptrs[nc-1]) * multiplier;
		PX_ASSERT(ptr % sizeof(RTreePage) == 0);
		const RTreePage* rightMostPageNext = mPages + (ptr / sizeof(RTreePage));
		curCount = PxU32(rightMostPageNext - rightMostPage);
		rightMostPage = rightMostPageNext;
	}

	return mTotalPages - topCount;
}

/////////////////////////////////////////////////////////////////////////
RTree::RTree(const PxEMPTY&)
{
	mFlags |= USER_ALLOCATED;
}

/////////////////////////////////////////////////////////////////////////
void RTree::release()
{
	if ((mFlags & USER_ALLOCATED) == 0 && mPages)
	{
		Ps::AlignedAllocator<128>().deallocate(mPages);
		mPages = NULL;
	}
}

// PX_SERIALIZATION
/////////////////////////////////////////////////////////////////////////
void RTree::exportExtraData(PxSerializationContext& stream)
{
	stream.alignData(128);
	stream.writeData(mPages, mTotalPages*sizeof(RTreePage));
}

/////////////////////////////////////////////////////////////////////////
void RTree::importExtraData(PxDeserializationContext& context)
{
	context.alignExtraData(128);
	mPages = context.readExtraData<RTreePage>(mTotalPages);
}

/////////////////////////////////////////////////////////////////////////
void RTree::dequantizeNode(const RTreePage* page, PxU32 nodeIdx, PxVec3& mn, PxVec3& mx) const
{
	mn.x = page->minx[nodeIdx];
	mn.y = page->miny[nodeIdx];
	mn.z = page->minz[nodeIdx];
	mx.x = page->maxx[nodeIdx];
	mx.y = page->maxy[nodeIdx];
	mx.z = page->maxz[nodeIdx];
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE PxU32 RTreePage::nodeCount() const
{
	for (int j = 0; j < RTreePage::SIZE; j ++)
		if (minx[j] == MX)
			return j;

	return RTreePage::SIZE;
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE void RTreePage::clearNode(PxU32 nodeIndex)
{
	PX_ASSERT(nodeIndex < RTreePage::SIZE);
	minx[nodeIndex] = miny[nodeIndex] = minz[nodeIndex] = MX; // initialize empty node with sentinels
	maxx[nodeIndex] = maxy[nodeIndex] = maxz[nodeIndex] = MN;
	ptrs[nodeIndex] = 0;
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE void RTreePage::getNode(const PxU32 nodeIndex, RTreeNodeQ& r) const
{
	PX_ASSERT(nodeIndex < RTreePage::SIZE);
	r.minx = minx[nodeIndex];
	r.miny = miny[nodeIndex];
	r.minz = minz[nodeIndex];
	r.maxx = maxx[nodeIndex];
	r.maxy = maxy[nodeIndex];
	r.maxz = maxz[nodeIndex];
	r.ptr  = ptrs[nodeIndex];
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE PxU32 RTreePage::getNodeHandle(PxU32 index) const
{
	PX_ASSERT(index < RTreePage::SIZE);
	PX_ASSERT(CONVERT_PTR_TO_INT(this) % sizeof(RTreePage) == 0);
	PxU32 result = (RTree::pagePtrTo32Bits(this) | index);
	return result;
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE void RTreePage::setEmpty(PxU32 startIndex)
{
	PX_ASSERT(startIndex < RTreePage::SIZE);
	for (int j = startIndex; j < RTreePage::SIZE; j ++)
		clearNode(j);
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE void RTreePage::computeBounds(RTreeNodeQ& newBounds)
{
	RTreeValue _minx = MX, _miny = MX, _minz = MX, _maxx = MN, _maxy = MN, _maxz = MN;
	for (int j = 0; j < RTreePage::SIZE; j++)
	{
		if (minx[j] > maxx[j])
			continue;
		_minx = PxMin(_minx, minx[j]);
		_miny = PxMin(_miny, miny[j]);
		_minz = PxMin(_minz, minz[j]);
		_maxx = PxMax(_maxx, maxx[j]);
		_maxy = PxMax(_maxy, maxy[j]);
		_maxz = PxMax(_maxz, maxz[j]);
	}
	newBounds.minx = _minx;
	newBounds.miny = _miny;
	newBounds.minz = _minz;
	newBounds.maxx = _maxx;
	newBounds.maxy = _maxy;
	newBounds.maxz = _maxz;
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE void RTreePage::adjustChildBounds(PxU32 index, const RTreeNodeQ& adjChild)
{
	PX_ASSERT(index < RTreePage::SIZE);
	minx[index] = adjChild.minx;
	miny[index] = adjChild.miny;
	minz[index] = adjChild.minz;
	maxx[index] = adjChild.maxx;
	maxy[index] = adjChild.maxy;
	maxz[index] = adjChild.maxz;
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE void RTreePage::growChildBounds(PxU32 index, const RTreeNodeQ& child)
{
	PX_ASSERT(index < RTreePage::SIZE);
	minx[index] = PxMin(minx[index], child.minx);
	miny[index] = PxMin(miny[index], child.miny);
	minz[index] = PxMin(minz[index], child.minz);
	maxx[index] = PxMax(maxx[index], child.maxx);
	maxy[index] = PxMax(maxy[index], child.maxy);
	maxz[index] = PxMax(maxz[index], child.maxz);
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE void RTreePage::copyNode(PxU32 targetIndex, const RTreePage& sourcePage, PxU32 sourceIndex)
{
	PX_ASSERT(targetIndex < RTreePage::SIZE);
	PX_ASSERT(sourceIndex < RTreePage::SIZE);
	minx[targetIndex] = sourcePage.minx[sourceIndex];
	miny[targetIndex] = sourcePage.miny[sourceIndex];
	minz[targetIndex] = sourcePage.minz[sourceIndex];
	maxx[targetIndex] = sourcePage.maxx[sourceIndex];
	maxy[targetIndex] = sourcePage.maxy[sourceIndex];
	maxz[targetIndex] = sourcePage.maxz[sourceIndex];
	ptrs[targetIndex] = sourcePage.ptrs[sourceIndex];
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE void RTreePage::setNode(PxU32 targetIndex, const RTreeNodeQ& sourceNode)
{
	PX_ASSERT(targetIndex < RTreePage::SIZE);
	minx[targetIndex] = sourceNode.minx;
	miny[targetIndex] = sourceNode.miny;
	minz[targetIndex] = sourceNode.minz;
	maxx[targetIndex] = sourceNode.maxx;
	maxy[targetIndex] = sourceNode.maxy;
	maxz[targetIndex] = sourceNode.maxz;
	ptrs[targetIndex] = sourceNode.ptr;
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE void RTreeNodeQ::grow(const RTreePage& page, int nodeIndex)
{
	PX_ASSERT(nodeIndex < RTreePage::SIZE);
	minx = PxMin(minx, page.minx[nodeIndex]);
	miny = PxMin(miny, page.miny[nodeIndex]);
	minz = PxMin(minz, page.minz[nodeIndex]);
	maxx = PxMax(maxx, page.maxx[nodeIndex]);
	maxy = PxMax(maxy, page.maxy[nodeIndex]);
	maxz = PxMax(maxz, page.maxz[nodeIndex]);
}

/////////////////////////////////////////////////////////////////////////
PX_FORCE_INLINE void RTreeNodeQ::grow(const RTreeNodeQ& node)
{
	minx = PxMin(minx, node.minx); miny = PxMin(miny, node.miny); minz = PxMin(minz, node.minz);
	maxx = PxMax(maxx, node.maxx); maxy = PxMax(maxy, node.maxy); maxz = PxMax(maxz, node.maxz);
}

/////////////////////////////////////////////////////////////////////////
void RTree::validateRecursive(PxU32 level, RTreeNodeQ parentBounds, RTreePage* page)
{
	PX_UNUSED(parentBounds);

	static PxU32 validateCounter = 0; // this is to suppress a warning that recursive call has no side effects
	validateCounter++;

	RTreeNodeQ n;
	PxU32 pageNodeCount = page->nodeCount();
	for (PxU32 j = 0; j < pageNodeCount; j++)
	{
		page->getNode(j, n);
		if (n.minx > n.maxx)
			continue;
		PX_ASSERT(n.minx >= parentBounds.minx); PX_ASSERT(n.miny >= parentBounds.miny); PX_ASSERT(n.minz >= parentBounds.minz);
		PX_ASSERT(n.maxx <= parentBounds.maxx); PX_ASSERT(n.maxy <= parentBounds.maxy); PX_ASSERT(n.maxz <= parentBounds.maxz);
		if (!n.isLeaf())
		{
			PX_ASSERT((n.ptr&1) == 0);
			RTreePage* childPage = (RTreePage*)(CONVERT_PTR_TO_INT(get64BitBasePage())+n.ptr);
			validateRecursive(level+1, n, childPage);
		}
	}
	RTreeNodeQ recomputedBounds;
	page->computeBounds(recomputedBounds);
	PX_ASSERT(recomputedBounds.minx == parentBounds.minx);
	PX_ASSERT(recomputedBounds.miny == parentBounds.miny);
	PX_ASSERT(recomputedBounds.minz == parentBounds.minz);
	PX_ASSERT(recomputedBounds.maxx == parentBounds.maxx);
	PX_ASSERT(recomputedBounds.maxy == parentBounds.maxy);
	PX_ASSERT(recomputedBounds.maxz == parentBounds.maxz);
}

/////////////////////////////////////////////////////////////////////////
void RTree::validate()
{
	for (PxU32 j = 0; j < mNumRootPages; j++)
	{
		RTreeNodeQ rootBounds;
		mPages[j].computeBounds(rootBounds);
		validateRecursive(0, rootBounds, mPages+j);
	}
}

/////////////////////////////////////////////////////////////////////////
#if RTREE_TEXT_DUMP_ENABLE
FILE* rtreeLog = NULL;

/////////////////////////////////////////////////////////////////////////
void rtreeDumpRecursive(FILE* f, RTreePage* page, PxU32 level, PxU32 numLevels)
{
	PX_ASSERT(numLevels > 0);
	PxU32 nodeCount = page->nodeCount();
	for (PxU32 j = 0; j < level; j++)
		fprintf(f, "  ");
	fprintf(f, "(%x) ", nodeCount);
	for (PxU32 j = 0; j < nodeCount; j++)
	{
		#if 0
		PxU32 bt = (level == numLevels-1) ? mObjectMap.find(page->ptrs[j])->second : mPageMap.find(page->ptrs[j])->second;
		fprintf(f, "%x[%x] ", page->ptrs[j], bt);
		#else
		fprintf(f, "%x ", page->ptrs[j]);
		#endif
	}
	fprintf(f, "\n");
	if (level < numLevels-1)
		for (PxU32 j = 0; j < nodeCount; j++)
			rtreeDumpRecursive(f, reinterpret_cast<RTreePage*>(page->ptrs[j]), level+1, numLevels);
	fprintf(f, "\n");
	fflush(f);
}

/////////////////////////////////////////////////////////////////////////
void RTree::maxscriptExportRecursive(void* fPtr, RTreePage* page, PxU32 l, PxU32 numLevels)
{
	FILE* f = (FILE*)fPtr;
	PX_ASSERT(numLevels > 0);
	PxU32 nodeCount = page->nodeCount();
	for (PxU32 j = 0; j < nodeCount; j++)
	{
		PxVec3 mn, mx;
		dequantizeNode(page, j, mn, mx);
		PxVec3 dims = mx-mn;
		PxVec3 center = (mn+mx)*0.5f;
		// in MAX width is x, length is y and height is z
		// Also MAX Box() has a center in the middle of the x and y and at the base of the box in z
		// in PhysX height is y
		fprintf(f, "box name: \"rtree_%02d_%06d\" width: %.4f length: %.4f height: %.4f pos:[%.4f, %.4f, %.4f]",
			l, j, dims.x, dims.z, dims.y, center.x, center.z, center.y-dims.y*0.5f);
		if (l > 0)
			fprintf(f, " parent:(getNodeByName(\"rtree_%02d_%06d\"))\n", l-1, j/RTreePage::SIZE);
		else
			fprintf(f, "\n");
	}
	fprintf(f, "\n");
	if (l < numLevels-1)
		for (PxU32 j = 0; j < nodeCount; j++)
			maxscriptExportRecursive(f, reinterpret_cast<RTreePage*>(page->ptrs[j]), l+1, numLevels);
	fprintf(f, "\n");
	fflush(f);
}
#endif

/////////////////////////////////////////////////////////////////////////
void RTree::maxscriptExport()
{
#if RTREE_TEXT_DUMP_ENABLE
	//FILE * f = fopen("/app_home/rtree.ms", "wt");
	FILE * f = fopen("c:\\rtree.ms", "wt");
	PX_ASSERT(mNumRootPages == 1);
	maxscriptExportRecursive(f, mPages, 0, mNumLevels);
	fclose(f);
#endif
}

/////////////////////////////////////////////////////////////////////////
void RTree::openTextDump()
{
#if RTREE_TEXT_DUMP_ENABLE
	rtreeLog = fopen("c:\\rtreelog.txt", "wt");
#endif
}

/////////////////////////////////////////////////////////////////////////
void RTree::closeTextDump()
{
#if RTREE_TEXT_DUMP_ENABLE
	fclose(rtreeLog);
#endif
}

/////////////////////////////////////////////////////////////////////////
void RTree::textDump(const char* prefix)
{
	PX_UNUSED(prefix);

#if RTREE_TEXT_DUMP_ENABLE
	fprintf(rtreeLog, "%s\n", prefix);
	rtreeDumpRecursive(rtreeLog, mPages, 0, mNumLevels);
#endif
}

//~PX_SERIALIZATION
const RTreeValue RTreePage::MN = -PX_MAX_REAL;
const RTreeValue RTreePage::MX = PX_MAX_REAL;

} // namespace Gu

}
