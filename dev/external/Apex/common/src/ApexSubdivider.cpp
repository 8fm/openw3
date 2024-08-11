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

#include "ApexSubdivider.h"

#include "NiApexSDK.h"

#include "PsSort.h"

namespace physx
{
namespace apex
{

ApexSubdivider::ApexSubdivider() :
	mMarkedVertices(0),
	mTriangleListEmptyElement(-1)
{
	mBound.setEmpty();
	mRand.setSeed(0);
}




void ApexSubdivider::clear()
{
	mBound.setEmpty();
	mRand.setSeed(0);
}



void ApexSubdivider::registerVertex(const PxVec3& v, PxU32 bitFlagPayload)
{
	SubdividerVertex vertex(v, bitFlagPayload);
	mVertices.pushBack(vertex);
	mBound.include(v);
}



void ApexSubdivider::registerTriangle(PxU32 i0, PxU32 i1, PxU32 i2)
{
	PX_ASSERT(i0 < mVertices.size());
	PX_ASSERT(i1 < mVertices.size());
	PX_ASSERT(i2 < mVertices.size());

	SubdividerTriangle t;
	t.init(i0, i1, i2);
	PxU32 triangleNumber = mTriangles.size();
	mTriangles.pushBack(t);
	addTriangleToVertex(i0, triangleNumber);
	addTriangleToVertex(i1, triangleNumber);
	addTriangleToVertex(i2, triangleNumber);
}



void ApexSubdivider::endRegistration()
{

}



void ApexSubdivider::mergeVertices(IProgressListener* progress)
{
	PX_UNUSED(progress);
	if (mVertices.empty())
	{
		APEX_INVALID_OPERATION("no vertices available");
		return;
	}

	const PxF32 MERGE_THRESHOLD = 1.0e-6f;

	const PxF32 d = (mBound.minimum - mBound.maximum).magnitude() * MERGE_THRESHOLD;
	const PxF32 d2 = d * d;

	Array<SubdividerVertexRef> refs;
	refs.reserve(mVertices.size());

	for (PxU32 i = 0; i < mVertices.size(); i++)
	{
		SubdividerVertex& v = mVertices[i];
		v.marked = false;
		SubdividerVertexRef vr(v.pos, i);
		refs.pushBack(vr);
	}
	mMarkedVertices = 0;

	shdfnd::sort(refs.begin(), refs.size(), SubdividerVertexRef());

	for (PxU32 i = 0; i < refs.size() - 1; i++)
	{
		PxU32 iNr = refs[i].vertexNr;
		SubdividerVertex& vi = mVertices[iNr];

		if (vi.marked)
		{
			continue;
		}

		const PxVec3 pos = refs[i].pos;
		PxU32 j = i + 1;
		while (j < refs.size() && fabs(refs[j].pos.x - pos.x) < MERGE_THRESHOLD)
		{
			if ((refs[j].pos - pos).magnitudeSquared() < d2)
			{
				PxU32 jNr = refs[j].vertexNr;
				SubdividerVertex& vj = mVertices[jNr];

				const PxU32 payload = vi.payload | vj.payload;
				vi.payload = vj.payload = payload;

				if (vj.firstTriangle != -1)
				{
					// find vi's last triangle, add the others there
					PxI32 lastTri = vi.firstTriangle;

					for (;;)
					{
						TriangleList& t = mTriangleList[lastTri];
						if (t.nextTriangle == -1)
						{
							break;
						}

						PX_ASSERT(t.triangleNumber < mTriangles.size());

						lastTri = t.nextTriangle;
					}
					PX_ASSERT(lastTri != -1);
					PX_ASSERT((PxU32)lastTri < mTriangleList.size());
					PX_ASSERT(mTriangleList[lastTri].nextTriangle == -1);
					mTriangleList[lastTri].nextTriangle = vj.firstTriangle;
					vj.firstTriangle = -1;
					lastTri = mTriangleList[lastTri].nextTriangle;
					while (lastTri != -1)
					{
						PxU32 tNr = mTriangleList[lastTri].triangleNumber;
						PX_ASSERT(tNr < mTriangles.size());
						mTriangles[tNr].replaceVertex(refs[j].vertexNr, refs[i].vertexNr);

						lastTri = mTriangleList[lastTri].nextTriangle;
					}
				}
				vj.marked = true;
				mMarkedVertices++;
			}
			j++;
		}
	}

	if (mMarkedVertices > 0)
	{
		compress();
	}
}



void ApexSubdivider::closeMesh(IProgressListener* progress)
{
	PX_UNUSED(progress);
	Array<SubdividerEdge> edges, borderEdges;
	SubdividerEdge edge;

	edges.reserve(mTriangles.size() * 3);

	for (PxU32 i = 0; i < mTriangles.size(); i++)
	{
		SubdividerTriangle& t = mTriangles[i];
		edge.init(t.vertexNr[0], t.vertexNr[1], i);
		edges.pushBack(edge);
		edge.init(t.vertexNr[1], t.vertexNr[2], i);
		edges.pushBack(edge);
		edge.init(t.vertexNr[2], t.vertexNr[0], i);
		edges.pushBack(edge);
	}

	shdfnd::sort(edges.begin(), edges.size(), SubdividerEdge());

	for (PxU32 i = 0; i < edges.size(); i++)
	{
		SubdividerEdge& ei = edges[i];
		PxU32 j = i + 1;
		while (j < edges.size() && edges[j] == ei)
		{
			j++;
		}
		if (j == i + 1)
		{
			borderEdges.pushBack(ei);
		}
		i = j - 1;
	}

	// find border circles
	Array<PxU32> borderVertices;
	borderVertices.reserve(borderEdges.size());
	while (!borderEdges.empty())
	{
		edge = borderEdges.back();
		borderEdges.popBack();

		borderVertices.clear();
		// find orientation

		const SubdividerTriangle& triangle = mTriangles[edge.triangleNr];
		PX_ASSERT(triangle.containsVertex(edge.v0));
		PX_ASSERT(triangle.containsVertex(edge.v1));
		bool swap = triangle.vertexNr[0] == edge.v0 || (triangle.vertexNr[1] == edge.v0 && triangle.vertexNr[0] != edge.v1);

		if (swap)
		{
			borderVertices.pushBack(edge.v1);
			borderVertices.pushBack(edge.v0);
		}
		else
		{
			borderVertices.pushBack(edge.v0);
			borderVertices.pushBack(edge.v1);
		}
		PxU32 currentV = borderVertices.back();
		PxI32 nextV = -1;
		do
		{
			nextV = -1;
			PxU32 i = 0;
			for (; i < borderEdges.size(); i++)
			{
				SubdividerEdge& ei = borderEdges[i];
				if (ei.v0 == currentV)
				{
					nextV = ei.v1;
					break;
				}
				else if (ei.v1 == currentV)
				{
					nextV = ei.v0;
					break;
				}
			}
			if (nextV < 0)
			{
				break;    // chain ended
			}

			PX_ASSERT(i < borderEdges.size());
			borderEdges.replaceWithLast(i);
			borderVertices.pushBack(nextV);
			currentV = nextV;
		}
		while (nextV >= 0);

		if (borderVertices[0] == borderVertices[borderVertices.size() - 1])
		{
			borderVertices.popBack();
		}

		closeHole(borderVertices.begin(), borderVertices.size());
	}
}



void ApexSubdivider::subdivide(PxU32 subdivisionGridSize, IProgressListener*)
{
	PX_ASSERT(subdivisionGridSize > 0);
	if (subdivisionGridSize == 0)
	{
		return;
	}

	const PxF32 maxLength = (mBound.minimum - mBound.maximum).magnitude() / (PxF32)subdivisionGridSize;
	const PxF32 threshold = 2.0f * maxLength;
	const PxF32 threshold2 = threshold * threshold;

	PX_ASSERT(threshold2 > 0.0f);
	if (threshold2 <= 0.0f)
	{
		return;
	}

	Array<SubdividerEdge> edges;
	edges.reserve(mTriangles.size() * 3);
	for (PxU32 i = 0; i < mTriangles.size(); i++)
	{
		SubdividerEdge edge;
		SubdividerTriangle& t = mTriangles[i];
		edge.init(t.vertexNr[0], t.vertexNr[1], i);
		edges.pushBack(edge);
		edge.init(t.vertexNr[1], t.vertexNr[2], i);
		edges.pushBack(edge);
		edge.init(t.vertexNr[2], t.vertexNr[0], i);
		edges.pushBack(edge);
	}

	shdfnd::sort(edges.begin(), edges.size(), SubdividerEdge());

	PxU32 i = 0;
	while (i < edges.size())
	{
		SubdividerEdge& ei = edges[i];
		PxU32 newI = i + 1;
		while (newI < edges.size() && edges[newI] == ei)
		{
			newI++;
		}

		const PxVec3 p0 = mVertices[ei.v0].pos;
		const PxVec3 p1 = mVertices[ei.v1].pos;
		const PxF32 d2 = (p0 - p1).magnitudeSquared();

		if (d2 < threshold2)
		{
			i = newI;
			continue;
		}

		PxU32 newVertex = mVertices.size();
		const PxF32 eps = 1.0e-4f;
		SubdividerVertex vertex;
		vertex.pos = (p0 + p1) * 0.5f + PxVec3(mRand.getNext(), mRand.getNext(), mRand.getNext()) * eps;
		vertex.payload = mVertices[ei.v0].payload | mVertices[ei.v1].payload;
		mVertices.pushBack(vertex);

		PxU32 newEdgeNr = edges.size();
		for (PxU32 j = i; j < newI; j++)
		{
			SubdividerEdge ej = edges[j];
			SubdividerTriangle tj = mTriangles[ej.triangleNr];

			PxU32 v2 = 0; // the vertex not contained in the edge
			if (tj.vertexNr[1] != ej.v0 && tj.vertexNr[1] != ej.v1)
			{
				v2 = 1;
			}
			else if (tj.vertexNr[2] != ej.v0 && tj.vertexNr[2] != ej.v1)
			{
				v2 = 2;
			}

			const PxU32 v0 = (v2 + 1) % 3;
			const PxU32 v1 = (v0 + 1) % 3;

			// generate new triangle
			const PxU32 newTriangle = mTriangles.size();

			SubdividerTriangle tNew;
			tNew.init(tj.vertexNr[v0], newVertex, tj.vertexNr[v2]);
			mTriangles.pushBack(tNew);
			addTriangleToVertex(tNew.vertexNr[0], newTriangle);
			addTriangleToVertex(tNew.vertexNr[1], newTriangle);
			addTriangleToVertex(tNew.vertexNr[2], newTriangle);

			// modify existing triangle
			removeTriangleFromVertex(tj.vertexNr[v0], ej.triangleNr);
			tj.vertexNr[v0] = newVertex;
			mTriangles[ej.triangleNr].vertexNr[v0] = newVertex;
			addTriangleToVertex(newVertex, ej.triangleNr);

			// update edges
			PxI32 k = binarySearchEdges(edges, tNew.vertexNr[2], tNew.vertexNr[0], ej.triangleNr);
			PX_ASSERT(k >= 0);
			edges[k].triangleNr = newTriangle;

			SubdividerEdge edge;
			edge.init(tj.vertexNr[v2], tj.vertexNr[v0], ej.triangleNr);
			edges.pushBack(edge);
			edge.init(tj.vertexNr[v0], tj.vertexNr[v1], ej.triangleNr);
			edges.pushBack(edge);
			edge.init(tNew.vertexNr[0], tNew.vertexNr[1], newTriangle);
			edges.pushBack(edge);
			edge.init(tNew.vertexNr[1], tNew.vertexNr[2], newTriangle);
			edges.pushBack(edge);
		}
		i = newI;
		PX_ASSERT(newEdgeNr < edges.size());

		shdfnd::sort(edges.begin() + newEdgeNr, edges.size() - newEdgeNr, SubdividerEdge());
	}
}



PxU32 ApexSubdivider::getNumVertices() const
{
	PX_ASSERT(mMarkedVertices == 0);
	return mVertices.size();
}



PxU32 ApexSubdivider::getNumTriangles() const
{
	return mTriangles.size();
}



void ApexSubdivider::getVertex(PxU32 i, PxVec3& v, PxU32& bitFlagPayload) const
{
	PX_ASSERT(mMarkedVertices == 0);
	PX_ASSERT(i < mVertices.size());
	v = mVertices[i].pos;
	bitFlagPayload = mVertices[i].payload;
}



void ApexSubdivider::getTriangle(PxU32 i, PxU32& i0, PxU32& i1, PxU32& i2) const
{
	PX_ASSERT(i < mTriangles.size());
	PX_ASSERT(mTriangles[i].isValid());

	const SubdividerTriangle& t = mTriangles[i];
	i0 = t.vertexNr[0];
	i1 = t.vertexNr[1];
	i2 = t.vertexNr[2];
}



void ApexSubdivider::compress()
{
	Array<PxU32> oldToNew;
	oldToNew.resize(mVertices.size());
	Array<SubdividerVertex> newVertices;
	newVertices.reserve(mVertices.size() - mMarkedVertices);

	for (PxU32 i = 0; i < mVertices.size(); i++)
	{
		if (mVertices[i].marked)
		{
			oldToNew[i] = (PxU32) - 1;
			mMarkedVertices--;
		}
		else
		{
			oldToNew[i] = newVertices.size();
			newVertices.pushBack(mVertices[i]);
		}
	}

	mVertices.resize(newVertices.size());
	for (PxU32 i = 0; i < newVertices.size(); i++)
	{
		mVertices[i] = newVertices[i];
	}

	for (PxU32 i = 0; i < mTriangles.size(); i++)
	{
		if (mTriangles[i].isValid())
		{
			SubdividerTriangle& t = mTriangles[i];
			t.vertexNr[0] = oldToNew[t.vertexNr[0]];
			t.vertexNr[1] = oldToNew[t.vertexNr[1]];
			t.vertexNr[2] = oldToNew[t.vertexNr[2]];
		}
		else
		{
			mTriangles.replaceWithLast(i);
			i--;
		}
	}

	PX_ASSERT(mMarkedVertices == 0);
}



void ApexSubdivider::closeHole(PxU32* indices, PxU32 numIndices)
{
	if (numIndices < 3)
	{
		return;
	}

	SubdividerTriangle triangle;
	triangle.init(0, 0, 0);

	// fill hole
	while (numIndices > 3)
	{
		PxVec3 normal(0.0f);
		for (PxU32 i = 0; i < numIndices; i++)
		{
			const PxVec3& p0 = mVertices[indices[i]].pos;
			const PxVec3& p1 = mVertices[indices[(i + 1) % numIndices]].pos;
			const PxVec3& p2 = mVertices[indices[(i + 2) % numIndices]].pos;

			PxVec3 normalI = (p0 - p1).cross(p2 - p1);
			normalI.normalize();
			normal += normalI;
		}
		normal.normalize();

		PxF32 maxQuality = -1.0f;
		PxI32 bestI = -1;
		for (PxU32 i = 0; i < numIndices; i++)
		{
			const PxU32 i2 = (i + 2) % numIndices;

			const PxU32 b0 = indices[i];
			const PxU32 b1 = indices[(i + 1) % numIndices];
			const PxU32 b2 = indices[i2];
			const PxU32 b3 = indices[(i + 3) % numIndices];
			const PxU32 b4 = indices[(i + 4) % numIndices];

			if (getTriangleNr(b1, b2, b3) != -1)
			{
				continue;
			}


			// init best
			//if (i == 0)
			//{
			//	t.init(b1,b2,b3);
			//	bestI = i2;
			//}

			// check whether triangle is an ear
			PxVec3 normalI = (mVertices[b1].pos - mVertices[b2].pos).cross(mVertices[b3].pos - mVertices[b2].pos);
			normalI.normalize(); ///< \todo, remove again, only for debugging
			if (normalI.dot(normal) < 0.0f)
			{
				continue;
			}

			PxF32 quality = qualityOfTriangle(b1, b2, b3) - qualityOfTriangle(b0, b1, b2) - qualityOfTriangle(b2, b3, b4);
			if (maxQuality < 0.0f || quality > maxQuality)
			{
				maxQuality = quality;
				triangle.init(b1, b2, b3);
				bestI = i2;
			}
		}
		PX_ASSERT(bestI != -1);
		PX_ASSERT(triangle.isValid());


		// remove ear vertex from temporary border
		for (PxU32 i = bestI; i < numIndices - 1; i++)
		{
			indices[i] = indices[i + 1];
		}

		numIndices--;

		// do we have the triangle already?
		//if (getTriangleNr(triangle.vertexNr[0], triangle.vertexNr[1], triangle.vertexNr[2]) >= 0)
		//	continue;

		// TODO:   triangle is potentially uninitialized.
		// do we have to subdivide the triangle?
		PxVec3& p0 = mVertices[triangle.vertexNr[0]].pos;
		PxVec3& p2 = mVertices[triangle.vertexNr[2]].pos;
		PxVec3 dir = p2 - p0;
		//PxF32 d = dir.normalize();
		PxU32 triangleNr = mTriangles.size();
		mTriangles.pushBack(triangle);
		addTriangleToVertex(triangle.vertexNr[0], triangleNr);
		addTriangleToVertex(triangle.vertexNr[1], triangleNr);
		addTriangleToVertex(triangle.vertexNr[2], triangleNr);
	}

	triangle.init(indices[0], indices[1], indices[2]);
	if (getTriangleNr(triangle.vertexNr[0], triangle.vertexNr[1], triangle.vertexNr[2]) < 0)
	{
		PxU32 triangleNr = mTriangles.size();
		mTriangles.pushBack(triangle);
		addTriangleToVertex(triangle.vertexNr[0], triangleNr);
		addTriangleToVertex(triangle.vertexNr[1], triangleNr);
		addTriangleToVertex(triangle.vertexNr[2], triangleNr);
	}
}



PxF32 ApexSubdivider::qualityOfTriangle(PxU32 v0, PxU32 v1, PxU32 v2) const
{
	const PxVec3& p0 = mVertices[v0].pos;
	const PxVec3& p1 = mVertices[v1].pos;
	const PxVec3& p2 = mVertices[v2].pos;

	const PxF32 a = (p0 - p1).magnitude();
	const PxF32 b = (p1 - p2).magnitude();
	const PxF32 c = (p2 - p0).magnitude();

	if (a > b && a > c) // a is biggest
	{
		return b + c - a;
	}
	else if (b > c)
	{
		return a + c - b;    // b is biggest
	}
	return a + b - c; // c is biggest
}



PxI32 ApexSubdivider::getTriangleNr(const PxU32 v0, const PxU32 v1, const PxU32 v2) const
{
	const PxU32 num = mVertices.size();
	if (v0 >= num || v1 >= num || v2 >= num)
	{
		return -1;
	}

	PxI32 triangleListIndex = mVertices[v0].firstTriangle;
	while (triangleListIndex != -1)
	{
		const TriangleList& tl = mTriangleList[triangleListIndex];
		const PxU32 triangleIndex = tl.triangleNumber;
		const SubdividerTriangle& triangle = mTriangles[triangleIndex];
		if (triangle.containsVertex(v1) && triangle.containsVertex(v2))
		{
			return triangleIndex;
		}

		triangleListIndex = tl.nextTriangle;
	}

	return -1;
}



PxI32 ApexSubdivider::binarySearchEdges(const Array<SubdividerEdge>& edges, PxU32 v0, PxU32 v1, PxU32 triangleNr) const
{
	if (edges.empty())
	{
		return -1;
	}

	SubdividerEdge edge;
	edge.init(v0, v1, (PxU32) - 1);

	PxU32 l = 0;
	PxU32 r = edges.size() - 1;
	PxI32 m = 0;
	while (l <= r)
	{
		m = (l + r) / 2;
		if (edges[m] < edge)
		{
			l = m + 1;
		}
		else if (edge < edges[m])
		{
			r = m - 1;
		}
		else
		{
			break;
		}
	}
	if (!(edges[m] == edge))
	{
		return -1;
	}

	while (m >= 0 && edges[m] == edge)
	{
		m--;
	}
	m++;

	PX_ASSERT(m >= 0);
	PX_ASSERT((PxU32)m < edges.size());
	while ((PxU32)m < edges.size() && edges[m] == edge && edges[m].triangleNr != triangleNr)
	{
		m++;
	}

	if (edges[m] == edge && edges[m].triangleNr == triangleNr)
	{
		return m;
	}

	return -1;
}



void ApexSubdivider::addTriangleToVertex(PxU32 vertexNumber, PxU32 triangleNumber)
{
	PX_ASSERT(vertexNumber < mVertices.size());
	PX_ASSERT(triangleNumber < mTriangles.size());

	TriangleList& t = allocateTriangleElement();

	t.triangleNumber = triangleNumber;
	t.nextTriangle = mVertices[vertexNumber].firstTriangle;

	mVertices[vertexNumber].firstTriangle = (PxI32)(&t - &mTriangleList[0]);
	//int a = 0;
}



void ApexSubdivider::removeTriangleFromVertex(PxU32 vertexNumber, PxU32 triangleNumber)
{
	PX_ASSERT(vertexNumber < mVertices.size());
	PX_ASSERT(triangleNumber < mTriangles.size());

	PxI32* lastPointer = &mVertices[vertexNumber].firstTriangle;
	PxI32 triangleListIndex = *lastPointer;
	while (triangleListIndex != -1)
	{
		if (mTriangleList[triangleListIndex].triangleNumber == triangleNumber)
		{
			*lastPointer = mTriangleList[triangleListIndex].nextTriangle;

			freeTriangleElement(triangleListIndex);

			break;
		}
		lastPointer = &mTriangleList[triangleListIndex].nextTriangle;
		triangleListIndex = *lastPointer;
	}
}



ApexSubdivider::TriangleList& ApexSubdivider::allocateTriangleElement()
{
	if (mTriangleListEmptyElement == -1)
	{
		return mTriangleList.insert();
	}
	else
	{
		PX_ASSERT((PxU32)mTriangleListEmptyElement < mTriangleList.size());
		TriangleList& elem = mTriangleList[mTriangleListEmptyElement];
		mTriangleListEmptyElement = elem.nextTriangle;
		elem.nextTriangle = -1;

		return elem;
	}
}



void ApexSubdivider::freeTriangleElement(PxU32 index)
{
	PX_ASSERT(index < mTriangleList.size());
	mTriangleList[index].nextTriangle = mTriangleListEmptyElement;
	mTriangleListEmptyElement = index;
}

}
} // end namespace physx::apex
