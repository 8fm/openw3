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
#include "GuRTreeCooking.h"
#include "PxBounds3.h"
#include "PxBounds3.h"
#include "PsSort.h"
#include "PsMathUtils.h"
#include "PxMathUtils.h"

#define PRINT_RTREE_COOKING_STATS 0

using namespace physx::Gu;

namespace physx
{

struct RTreeNodeNQ
{
	PxBounds3	bounds;
	PxI32		childPageFirstNodeIndex;
	PxI32		leafCount; // -1 for empty nodes, 0 for non-terminal nodes, number of enclosed tris if non-zero (LeafTriangles), also means a terminal node

	RTreeNodeNQ() : bounds(PxBounds3::empty()), childPageFirstNodeIndex(-1), leafCount(0)
	{}
};

static const PxU32 RT_PAGESIZE = RTreePage::SIZE;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define EXPORT_TO_MAXSCRIPT
#ifdef EXPORT_TO_MAXSCRIPT
#include "stdio.h"
static void exportRTreeToMaxscript(
	PxU32 pageSize, const RTreeNodeNQ* root, const RTreeNodeNQ* page, PxU32 level = 0, FILE* f = NULL, char* path = NULL)
{
	//FILE * f = fopen("/app_home/rtree.ms", "wt");
	if (f == NULL)
		f = fopen("c:\\rtree.ms", "wt");

	static char path0[256] = "";
	if (!path)
		path = path0;
	size_t pathLen = strlen(path);

	for (PxU32 i = 0; i < RT_PAGESIZE; i++)
	{
		const RTreeNodeNQ& n = page[i];
		if (n.leafCount == -1)
			continue;
		sprintf(path, "%s_%02d", path, i);
		PxVec3 dims = n.bounds.getDimensions();
		PxVec3 center = n.bounds.getCenter();
		// in MAX width is x, length is y and height is z
		// Also MAX Box() has a center in the middle of the x and y and at the base of the box in z
		// in PhysX height is y
		fprintf(f, "box name: \"rtree%s\" width: %.4f length: %.4f height: %.4f pos:[%.4f, %.4f, %.4f]",
			path, dims.x, dims.z, dims.y, center.x, center.z, center.y-dims.y*0.5f);
		path[pathLen] = 0; // convert back to parent name
		if (level > 0)
			fprintf(f, " parent:(getNodeByName(\"rtree%s\"))\n", path);
		else
			fprintf(f, "\n");
		path[pathLen] = '_'; // restore child name

		if (n.leafCount == 0 && n.childPageFirstNodeIndex != -1)
			exportRTreeToMaxscript(pageSize, root, root+n.childPageFirstNodeIndex, level+1, f, path);

		// strip the last 3 characters from path
		path[pathLen] = 0;
	}

	if (level == 0)
		fclose(f);
}
#endif

/////////////////////////////////////////////////////////////////////////
void RTreeCooker::buildFromTriangles(
	Gu::RTree& result, const PxVec3* verts, PxU32 numVerts, const PxU16* tris16, const PxU32* tris32, PxU32 numTris,
	PxU32 numTrisPerLeaf, Array<PxU32>& resultPermute, RTreeCooker::RemapCallback* rc)
{
	PX_UNUSED(numVerts);
	Array<PxBounds3> allBounds;
	Array<PxU32> boundedData;
	Array<PxU32> bigBounds;
	allBounds.reserve(numTris);
	boundedData.reserve(numTris);
	bigBounds.reserve(numTris);
	PxF32 avg = 0.0f;
	PxBounds3 allTriBounds;
	PxU32 i;
	Array<PxVec3> tris;
	for (i = 0; i < numTris; i ++)
	{
		PxU32 i0 = (tris16 ? tris16[i*3+0] : tris32[i*3+0]);
		PxU32 i1 = (tris16 ? tris16[i*3+1] : tris32[i*3+1]);
		PxU32 i2 = (tris16 ? tris16[i*3+2] : tris32[i*3+2]);
		PX_ASSERT(i0 < numVerts && i1 < numVerts && i2 < numVerts && "Input mesh triangle's vertex index exceeds specified numVerts.");
		PxVec3 vert0 = verts[i0], vert1 = verts[i1], vert2 = verts[i2];
		PxBounds3 & b = allBounds.pushBack(PxBounds3::boundsOfPoints(vert0, vert1));
		b.include(vert2);
		boundedData.pushBack(i);
		if (i == 0)
			allTriBounds = b;
		else
			allTriBounds.include(b);
		tris.pushBack(vert0); tris.pushBack(vert1); tris.pushBack(vert2);
	}

	const PxU32 numHist = 20;
	PxU32 sizeHistogram[numHist];
	memset(sizeHistogram, 0, numHist*sizeof(sizeHistogram[0]));
	PxF32 maxDim = allTriBounds.getExtents().maxElement();
	for (i = 0; i < numTris; i++)
	{
		const PxBounds3& b = allBounds[i];
		PxReal size = b.getExtents().magnitude();
		avg += size;
		PxU32 histIndex = PxMin<PxU32>(PxU32(numHist*PxF32(size/maxDim)), numHist-1);
		sizeHistogram[histIndex] ++;
		if (histIndex > 4)
			bigBounds.pushBack( 1 );
		else
			bigBounds.pushBack( 0 );
	}
	avg /= numTris;

	buildFromBounds(
		result, allBounds.begin(), boundedData.begin(), bigBounds.begin(), numTris,
		numTrisPerLeaf, resultPermute, rc, tris.begin());

}

/////////////////////////////////////////////////////////////////////////
struct SortBoundsPredicateLR
{
	PxU32 coordIndex;
	const PxBounds3* allBounds;
	PxBounds3 clusterBounds;
	SortBoundsPredicateLR(PxU32 coordIndex, const PxBounds3* allBounds, const PxBounds3& clusterBounds)
		: coordIndex(coordIndex), allBounds(allBounds), clusterBounds(clusterBounds)
	{}

	bool operator()(const PxU32 & idx1, const PxU32 & idx2) const
	{
		//PxF32 center1 = allBounds[idx1].getCenter()[coordIndex];
		//PxF32 center2 = allBounds[idx2].getCenter()[coordIndex];
		//return (center1 < center2);
		PX_ASSERT(allBounds[idx1].maximum[coordIndex] <= clusterBounds.maximum[coordIndex]);
		PX_ASSERT(allBounds[idx1].minimum[coordIndex] >= clusterBounds.minimum[coordIndex]);
		PX_ASSERT(allBounds[idx2].maximum[coordIndex] <= clusterBounds.maximum[coordIndex]);
		PX_ASSERT(allBounds[idx2].minimum[coordIndex] >= clusterBounds.minimum[coordIndex]);
		{
			PxF32 distLeft1 = (allBounds[idx1].minimum[coordIndex]-clusterBounds.minimum[coordIndex]);
			PxF32 distLeft2 = (allBounds[idx2].minimum[coordIndex]-clusterBounds.minimum[coordIndex]);
			PxF32 distRight1 = (clusterBounds.maximum[coordIndex]-allBounds[idx2].maximum[coordIndex]);
			PxF32 distRight2 = (clusterBounds.maximum[coordIndex]-allBounds[idx2].maximum[coordIndex]);
			return (distLeft1-distRight1 < distLeft2-distRight2);
		}
	}
};

/////////////////////////////////////////////////////////////////////////
struct SortBoundsPredicate
{
	PxU32 coordIndex;
	const PxBounds3* allBounds;
	SortBoundsPredicate(PxU32 coordIndex, const PxBounds3* allBounds)
		: coordIndex(coordIndex), allBounds(allBounds)
	{}

	bool operator()(const PxU32 & idx1, const PxU32 & idx2) const
	{
		PxF32 center1 = allBounds[idx1].getCenter()[coordIndex];
		PxF32 center2 = allBounds[idx2].getCenter()[coordIndex];
		return (center1 < center2);
	}
};


/////////////////////////////////////////////////////////////////////////
struct Interval
{
	PxU32 start, count;
	Interval(PxU32 start, PxU32 count) : start(start), count(count) {}
};

static PX_FORCE_INLINE PxReal PxSAH(const PxVec3& v)
{
	return v.x*v.y + v.y*v.z + v.x*v.z;
}

/////////////////////////////////////////////////////////////////////////
struct SubSort
{
	PxU32* permuteStart, *tempPermute;
	const PxBounds3* allBounds;
	const PxVec3* allTris;
	PxF32* metricL;
	PxF32* metricR;
	SubSort(PxU32* permute, PxU32 * tempPermute, const PxBounds3 * allBounds, const PxVec3* allTris, PxU32 numBounds)
		: permuteStart(permute), tempPermute(tempPermute), allBounds(allBounds), allTris(allTris)
	{
		metricL = new PxF32[numBounds];
		metricR = new PxF32[numBounds];
	}
	~SubSort()
	{
		delete [] metricL; metricL = NULL;
		delete [] metricR; metricR = NULL;
	}

	////////////////////////////////////////////////////////////////////
	// returns split position for second array start relative to permute ptr
	PxU32 split(PxU32* permute, PxU32 clusterSize, PxU32 boundsPerLeaf)
	{
		PX_ASSERT(boundsPerLeaf > 0);
		PX_UNUSED(boundsPerLeaf);

		if (clusterSize <= 1)
			return 0;
		if (clusterSize == 2)
			return 1;

		PxBounds3 clusterBounds(PxBounds3::empty());
		for (PxU32 i = 0; i < clusterSize; i ++)
			clusterBounds.include(allBounds[permute[i]]);

		PxI32 minCount = clusterSize >= 4 ? 2 : 1;
		PxI32 splitStartL = minCount; // range=[startL->endL)
		PxI32 splitEndL = clusterSize-minCount;
		PxI32 splitStartR = clusterSize-splitStartL; // range=(endR<-startR], startR > endR
		PxI32 splitEndR = clusterSize-splitEndL;
		PX_ASSERT(splitEndL - splitStartL == splitStartR-splitEndR);
		PX_ASSERT(splitStartL <= splitEndL);
		PX_ASSERT(splitStartR >= splitEndR);
		PX_ASSERT(splitEndR >= 1);
		PX_ASSERT(splitEndL < PxI32(clusterSize));

		// pick the best axis with some splitting metric
		PxF32 minMetric[3];
		PxU32 minMetricSplit[3];
		for (PxU32 coordIndex = 0; coordIndex <= 2; coordIndex++)
		{
			PxBounds3 boundsL, boundsR;
			SortBoundsPredicateLR sortPredicateLR(coordIndex, allBounds, clusterBounds);
			memcpy(tempPermute, permute, sizeof(tempPermute[0])*clusterSize);
			Ps::sort(tempPermute, clusterSize, sortPredicateLR);

			boundsL = allBounds[tempPermute[0]]; // init with 0th bound
			PxI32 ii;
			for (ii = 1; ii < splitStartL; ii++) // sweep right to include all bounds up to splitStartL-1
				boundsL.include(allBounds[tempPermute[ii]]);
			PxU32 countL = 0;
			for (ii = splitStartL; ii <= splitEndL; ii++) // compute metric for inclusive bounds from splitStartL to splitEndL
			{
				boundsL.include(allBounds[tempPermute[ii]]);
				PxVec3 extentsL = boundsL.getExtents();
				metricL[countL++] = PxSAH(extentsL);
			}

			boundsR = allBounds[tempPermute[clusterSize-1]]; // init with last bound
			for (ii = clusterSize-2; ii > splitStartR; ii--) // include bound right of splitEndR
				boundsR.include(allBounds[tempPermute[ii]]);
			PxU32 countR = 0;
			for (ii = splitStartR; ii >= splitEndR; ii--)
			{
				boundsR.include(allBounds[tempPermute[ii]]);
				PxVec3 extentsR = boundsR.getExtents();
				metricR[countR++] = PxSAH(extentsR);
			}

			PX_ASSERT(countL == countR && countL == splitEndL-splitStartL+1);

			PxU32 minMetricSplitPosition = 0;
			PxF32 minMetricLocal = PX_MAX_REAL;
			const PxI32 hsI32 = PxI32(clusterSize/2);
			const PxI32 splitRange = (splitEndL-splitStartL+1);
			for (ii = 0; ii < splitRange; ii++)
			{
				PxU32 countL = ii;
				PxU32 countR = splitRange-ii;
				//const PxF32 p = 0.97f;
				//PxF32 mulL = PxMax(powf(countL, p), 1.0f);
				//PxF32 mulR = PxMax(powf(countR, p), 1.0f);
				PxF32 mulL = PxF32(countL);
				PxF32 mulR = PxF32(countR);
					#if RTREE_PAGE_SIZE == 8
					if (0 && countL+countR > 32)
					{
						const PxU32 penalty[8] = {0, 1, 2, 3, 4, 3, 2, 1};
						mulL += 10.1f * penalty[countL % 8];
						mulR += 10.1f * penalty[countR % 8];
					}
					#else
					if (0 && clusterSize<=16)
					{
						const PxF32 penalty[4] = {1, 2.1, 3.5, 2.1};
						mulL *= penalty[countL % 4];
						mulR *= penalty[countR % 4];
					}
					#endif
				const PxF32 metric = (mulL*metricL[ii]+mulR*metricR[splitRange-ii-1]);
				const PxU32 splitPos = ii+splitStartL;
				const bool forceMidpoint = (clusterSize == 4);
				if (metric < minMetricLocal || forceMidpoint ||
					(metric <= minMetricLocal && // same metric but more even split
						PxAbs(PxI32(splitPos)-hsI32) < PxAbs(PxI32(minMetricSplitPosition)-hsI32)))
				{
					minMetricLocal = metric;
					minMetricSplitPosition = splitPos;
				}
			}

			minMetric[coordIndex] = minMetricLocal;
			minMetricSplit[coordIndex] = minMetricSplitPosition;

			// sum of axis lengths for both left and right AABBs
		}

		PxU32 winIndex = 2;
		if (minMetric[0] <= minMetric[1] && minMetric[0] <= minMetric[2])
			winIndex = 0;
		else if (minMetric[1] <= minMetric[2])
			winIndex = 1;

		SortBoundsPredicateLR sortPredicateLR(winIndex, allBounds, clusterBounds);
		Ps::sort(permute, clusterSize, sortPredicateLR);

		PxU32 splitPoint = minMetricSplit[winIndex];
		if (clusterSize == 3 && splitPoint == 0)
			splitPoint = 1; // special case due to rounding
		return splitPoint;
	}

	PxF32 computeSA(const PxU32* permute, const Interval& split) // both permute and i are relative
	{
		PxBounds3 b = allBounds[permute[split.start]];
		for (PxU32 i = 1; i < split.count; i++)
			b.include(allBounds[permute[split.start+i]]);

		return PxSAH(b.getExtents());
	}

	////////////////////////////////////////////////////////////////////
	void sort4(PxU32* permute, PxU32 clusterSize, PxU32 boundsPerLeaf,
		Array<RTreeNodeNQ>& resultTree, PxU32& maxLevels, PxU32 level = 0, RTreeNodeNQ* parentNode = NULL)
	{
		PX_UNUSED(parentNode);

		if (level == 0)
			maxLevels = 1;
		else
			maxLevels = PxMax(maxLevels, level+1);

		PxU32 splitPos[RT_PAGESIZE];
		for (PxU32 j = 0; j < RT_PAGESIZE; j++)
			splitPos[j] = j+1;

		if (clusterSize >= RT_PAGESIZE)
		{
			// split into RT_PAGESIZE regions via RT_PAGESIZE-1 subsequent splits
			Array<Interval> splits;
			splits.pushBack(Interval(0, clusterSize));
			for (PxU32 iSplit = 0; iSplit < RT_PAGESIZE-1; iSplit++)
			{
				PxF32 maxSAH = -FLT_MAX;
				PxU32 maxSplit = 0xFFFFffff;
				for (PxU32 i = 0; i < splits.size(); i++)
				{
					if (splits[i].count == 1)
						continue;
					PxF32 SAH = computeSA(permute, splits[i])*splits[i].count;
					if (SAH > maxSAH)
					{
						maxSAH = SAH;
						maxSplit = i;
					}
				}
				PX_ASSERT(maxSplit != 0xFFFFffff);
				Interval old = splits[maxSplit];
				PX_ASSERT(old.count > 1);
				PxU32 splitLocal = split(permute+old.start, old.count, boundsPerLeaf); // relative split pos
				PX_ASSERT(splitPos > 0);
				PX_ASSERT(splitLocal >= 1);
				PX_ASSERT(old.count-splitLocal >= 1);
				splits.pushBack(Interval(old.start, splitLocal));
				splits.pushBack(Interval(old.start+splitLocal, old.count-splitLocal));
				splits.replaceWithLast(maxSplit);
				splitPos[iSplit] = old.start+splitLocal;
			}

			PX_ASSERT(splits.size() == RT_PAGESIZE);
			PxU32 sum = 0;
			for (PxU32 j = 0; j < RT_PAGESIZE; j++)
				sum += splits[j].count;
			PX_ASSERT(sum == clusterSize);
		}
		else // clusterSize < RT_PAGESIZE
		{
			// make it so splitCounts add up for small cluster sizes
			for (PxU32 i = clusterSize; i < RT_PAGESIZE-1; i++)
				splitPos[i] = clusterSize;
		}

		Ps::sort(splitPos, RT_PAGESIZE-1);
		splitPos[RT_PAGESIZE-1] = clusterSize;

		PxU32 splitStarts[RT_PAGESIZE];
		PxU32 splitCounts[RT_PAGESIZE];
		splitStarts[0] = 0;
		splitCounts[0] = splitPos[0];
		PxU32 sumCounts = splitCounts[0];
		for (PxU32 j = 1; j < RT_PAGESIZE; j++)
		{
			splitStarts[j] = splitPos[j-1];
			PX_ASSERT(splitStarts[j-1]<=splitStarts[j]);
			splitCounts[j] = splitPos[j]-splitPos[j-1];
			PX_ASSERT(splitCounts[j] > 0 || clusterSize < RT_PAGESIZE);
			sumCounts += splitCounts[j];
			PX_ASSERT(splitStarts[j-1]+splitCounts[j-1]<=splitStarts[j]);
		}

		PX_ASSERT(sumCounts == clusterSize);
		PX_ASSERT(splitStarts[RT_PAGESIZE-1]+splitCounts[RT_PAGESIZE-1]<=clusterSize);

		bool terminalClusterByTotalCount = true && (clusterSize <= RT_PAGESIZE*8);
		for (PxU32 s = 0; s < RT_PAGESIZE; s++)
		{
			RTreeNodeNQ rtn;
			if (splitCounts[s] > 0)
			{
				PxBounds3 b = allBounds[permute[splitStarts[s]]];
				PxF32 sahMin = PxSAH(b.getExtents());
				PxF32 sahMax = sahMin;
				for (PxU32 i = 1; i < splitCounts[s]; i++)
				{
					PxU32 localIndex = i + splitStarts[s];
					const PxBounds3& b1 = allBounds[permute[localIndex]];
					PxF32 sah1 = PxSAH(b1.getExtents());
					sahMin = PxMin(sahMin, sah1);
					sahMax = PxMax(sahMax, sah1);
					b.include(b1);
				}

				rtn.bounds = b;
				const PxF32 sahThreshold = 40.0f;
				const PxF32 sahRatio = sahMax/sahMin;
				bool okSAH = (sahRatio < sahThreshold) || (splitCounts[s] == 1);
				if (!okSAH)
					terminalClusterByTotalCount = false;
				if (splitCounts[s] > 16) // LeafTriangles doesn't support more
					terminalClusterByTotalCount = false;

				if ((okSAH || splitCounts[s] <= 2) && (splitCounts[s] <= 3 || terminalClusterByTotalCount))
				//if (okSAH && (splitCounts[s] <= boundsPerLeaf || terminalClusterByTotalCount))
				//if (okSAH && (splitCounts[s] <= 1 || terminalClusterByTotalCount))
				{
					rtn.childPageFirstNodeIndex = PxI32(splitStarts[s]+(permute-permuteStart));
					rtn.leafCount = splitCounts[s];
					PX_ASSERT(splitCounts[s] <= 16); // LeafTriangles doesn't support more
				}
				else
				{
					rtn.childPageFirstNodeIndex = -1;
					rtn.leafCount = 0;
				}
			}
			else
			{
				rtn.bounds.setEmpty();
				rtn.childPageFirstNodeIndex = -1;
				rtn.leafCount = -1;
			}
			resultTree.pushBack(rtn);
		}

		if (terminalClusterByTotalCount)
			return;

		// recurse on subpages
		PxU32 parentIndex = resultTree.size() - RT_PAGESIZE;
		for (PxU32 s = 0; s<RT_PAGESIZE; s++)
		{
			RTreeNodeNQ* sParent = &resultTree[parentIndex+s]; // array can be resized and relocated during recursion
			if (sParent->leafCount == 0)
			{
				// set the child pointer for parent node
				sParent->childPageFirstNodeIndex = resultTree.size();
				sort4(permute+splitStarts[s], splitCounts[s], boundsPerLeaf, resultTree, maxLevels, level+1, sParent);
			}
		}
	}
};

/////////////////////////////////////////////////////////////////////////
// newIndex = resultPermute[oldIndex]
void RTreeCooker::buildFromBounds(
	Gu::RTree& result, const PxBounds3* allBounds, const PxU32* /*boundedData*/, const PxU32* /*bigBounds*/,
	PxU32 numBounds, PxU32 numTrisPerLeaf, Array<PxU32>& permute, RTreeCooker::RemapCallback* rc, const PxVec3* verts3)
{
	Array<PxU32> tempPermute;
	tempPermute.reserve(numBounds);

	// build bounds for the entire tree
	PxBounds3 treeBounds(PxBounds3::empty());
	for (PxU32 i = 0; i < numBounds; i ++)
		treeBounds.include(allBounds[i]);

	// start off with an identity permutation
	permute.clear();
	for (PxU32 j = 0; j < numBounds; j ++)
		permute.pushBack(j);

	int objectsPerLeaf;
	if (numBounds < 8)
		objectsPerLeaf = 1;
	else if (numBounds < 16)
		objectsPerLeaf = 2;
	else
		objectsPerLeaf = numTrisPerLeaf;


	// load sorted nodes into an RTreeNodeNQ tree representation
	// build the tree structure from sorted nodes
	const PxU32 pageSize = RTreePage::SIZE;
	Array<RTreeNodeNQ> resultTree;

	// sort by shuffling the permutation
	SubSort ss(permute.begin(), tempPermute.begin(), allBounds, verts3, numBounds);
	PxU32* permBegin = permute.begin(); PX_ASSERT(numBounds == permute.size());
	PxU32 maxLevels = 0;
	ss.sort4(permBegin, numBounds, objectsPerLeaf, resultTree, maxLevels);

#if PRINT_RTREE_COOKING_STATS
	PxU32 totalLeafTris = 0;
	PxU32 numLeaves = 0;
	PxI32 maxLeafTris = 0;
	for (PxU32 i = 0; i < resultTree.size(); i++)
	{
		PxI32 leafCount = resultTree[i].leafCount;
		if (leafCount > 0)
		{
			numLeaves++;
			totalLeafTris += leafCount;
			if (leafCount > maxLeafTris)
				maxLeafTris = leafCount;
		}
	}

	printf(
		"numLeaves = %d, avgLeafTris = %.2f, maxTrisPerLeaf = %d\n",
		numLeaves, PxF32(totalLeafTris)/numLeaves, maxLeafTris);
#endif

	#ifdef EXPORT_TO_MAXSCRIPT
	// export to maxscript for debugging
	exportRTreeToMaxscript(pageSize, resultTree.begin(), resultTree.begin());
	#endif

	PX_ASSERT(RT_PAGESIZE*sizeof(RTreeNodeQ) == sizeof(RTreePage));
	const int nodePtrMultiplier = sizeof(RTreeNodeQ); // convert offset as count in qnodes to page ptr

	// Quantize the tree
	Array<RTreeNodeQ> qtreeNodes;
	PxVec4 treeBoundsMin(treeBounds.minimum, 0.0f), treeBoundsMax(treeBounds.maximum, 0.0f); // has to be after computeInvDiagUpdateBounds..
	PxU32 firstEmptyIndex = (PxU32)-1;
	PxU32 resultCount = resultTree.size();
	for (PxU32 i = 0; i < resultCount; i++)
	{
		RTreeNodeNQ & u = resultTree[i];
		RTreeNodeQ q;
		q.setLeaf(u.leafCount > 0);
		if (u.childPageFirstNodeIndex == -1)
		{
			if (firstEmptyIndex == (PxU32)-1)
				firstEmptyIndex = qtreeNodes.size();
			q.minx = q.miny = q.minz = FLT_MAX;
			q.maxx = q.maxy = q.maxz = -FLT_MAX;

			q.ptr = firstEmptyIndex*nodePtrMultiplier; PX_ASSERT((q.ptr & 1) == 0);
			q.setLeaf(true);
		} else
		{
			q.minx = u.bounds.minimum.x;
			q.miny = u.bounds.minimum.y;
			q.minz = u.bounds.minimum.z;
			q.maxx = u.bounds.maximum.x;
			q.maxy = u.bounds.maximum.y;
			q.maxz = u.bounds.maximum.z;
			if (u.leafCount > 0)
			{
				q.ptr = PxU32(u.childPageFirstNodeIndex);
				rc->remap(&q.ptr, q.ptr, u.leafCount);
				PX_ASSERT(q.isLeaf()); // remap is expected to set the isLeaf bit
			}
			else
			{
				// verify that all children bounds are included in the parent bounds
				for (PxU32 s = 0; s < RT_PAGESIZE; s++)
				{
					const RTreeNodeNQ& child = resultTree[u.childPageFirstNodeIndex+s];
					PX_UNUSED(child);
					// is a sentinel node or is inside parent's bounds
					PX_ASSERT(child.leafCount == -1 || child.bounds.isInside(u.bounds));
				}

				q.ptr = u.childPageFirstNodeIndex * nodePtrMultiplier;
				PX_ASSERT(q.ptr % RT_PAGESIZE == 0);
				q.setLeaf(false);
			}
		}
		qtreeNodes.pushBack(q);
	}

	// build quantized rtree image
	result.mInvDiagonal = PxVec4(1.0f);
	PX_ASSERT(qtreeNodes.size() % RT_PAGESIZE == 0);
	result.mTotalNodes = qtreeNodes.size();
	result.mTotalPages = result.mTotalNodes / pageSize;
	result.mPages = static_cast<RTreePage*>(
		Ps::AlignedAllocator<128>().allocate(sizeof(RTreePage)*result.mTotalPages, __FILE__, __LINE__));
	result.mBoundsMin = PxVec4(treeBounds.minimum.x, treeBounds.minimum.y, treeBounds.minimum.z, 0.0f);
	result.mBoundsMax = PxVec4(treeBounds.maximum.x, treeBounds.maximum.y, treeBounds.maximum.z, 0.0f);
	result.mDiagonalScaler = (result.mBoundsMax - result.mBoundsMin) / 65535.0f;
	result.mPageSize = pageSize;
	result.mNumLevels = maxLevels;
	PX_ASSERT(result.mTotalNodes % pageSize == 0);
	result.mUnused = 0;
	result.mNumRootPages = 1;

	const PxReal fatten = 1e-5f;
	for (PxU32 j = 0; j < result.mTotalPages; j++)
	{
		RTreePage& page = result.mPages[j];
		for (int k = 0; k < RT_PAGESIZE; k ++)
		{
			const RTreeNodeQ& n = qtreeNodes[j*RT_PAGESIZE+k];
			//PxF32 nodeOverlapRatio = n.computeOverlapRatio();
			page.maxx[k] = n.maxx+PxMax(PxAbs(n.maxx), fatten)*fatten;
			page.maxy[k] = n.maxy+PxMax(PxAbs(n.maxy), fatten)*fatten;
			page.maxz[k] = n.maxz+PxMax(PxAbs(n.maxz), fatten)*fatten;
			page.minx[k] = n.minx-PxMax(PxAbs(n.minx), fatten)*fatten;
			page.miny[k] = n.miny-PxMax(PxAbs(n.miny), fatten)*fatten;
			page.minz[k] = n.minz-PxMax(PxAbs(n.minz), fatten)*fatten;
			page.ptrs[k] = n.ptr;
		}
	}
	//printf("Tree size=%d\n", result.mTotalPages*sizeof(RTreePage));
#ifdef PX_DEBUG
	result.validate();
#endif
}

} // namespace physx
