#include "RTdef.h"
#if RT_COMPILE
#include "Delaunay2dBase.h"
#include "PxBounds3.h"
#include <PxMath.h>

namespace physx
{
namespace fracture
{
namespace base
{

// -------------------------------------------------------------------------------------
Delaunay2d::Delaunay2d(SimScene* scene):
	mScene(scene)
{
}

// -------------------------------------------------------------------------------------
Delaunay2d::~Delaunay2d() 
{
}

// -------------------------------------------------------------------------------------
void Delaunay2d::clear()
{
	mVertices.clear();
	mIndices.clear();
	mTriangles.clear();
	mFirstFarVertex = 0;

	mConvexVerts.clear();
	mConvexes.clear();
	mConvexNeighbors.clear();
}

// -------------------------------------------------------------------------------------
void Delaunay2d::triangulate(const PxVec3 *vertices, int numVerts, int byteStride, bool removeFarVertices)
{
	clear();

	PxU8 *vp = (PxU8*)vertices;
	mVertices.resize(numVerts);
	for (int i = 0; i < numVerts; i++) {
		mVertices[i] = (*(PxVec3*)vp);
		mVertices[i].z = 0.0f;
		vp += byteStride;
	}

	delaunayTriangulation();

	for (int i = 0; i < (int)mTriangles.size(); i++) {
		Triangle &t = mTriangles[i];
		if (!removeFarVertices || (t.p0 < mFirstFarVertex && t.p1 < mFirstFarVertex && t.p2 < mFirstFarVertex)) {
			mIndices.pushBack(t.p0);
			mIndices.pushBack(t.p1);
			mIndices.pushBack(t.p2);
		}
	}

	if (removeFarVertices)
		mVertices.resize(mFirstFarVertex);
}


// -------------------------------------------------------------------------------------
void Delaunay2d::delaunayTriangulation()
{
	PxBounds3 bounds;
	bounds.setEmpty();

	for (int i = 0; i < (int)mVertices.size(); i++) 
		bounds.include(mVertices[i]);

	bounds.fattenSafe(bounds.getDimensions().magnitude());

	// start with two triangles
	//float scale = 10.0f;
	PxVec3 p0(bounds.minimum.x, bounds.minimum.y, 0.0f);
	PxVec3 p1(bounds.maximum.x, bounds.minimum.y, 0.0f);
	PxVec3 p2(bounds.maximum.x, bounds.maximum.y, 0.0f);
	PxVec3 p3(bounds.minimum.x, bounds.maximum.y, 0.0f);

	mFirstFarVertex = mVertices.size();
	mVertices.pushBack(p0);
	mVertices.pushBack(p1);
	mVertices.pushBack(p2);
	mVertices.pushBack(p3);

	mTriangles.clear();
	addTriangle(mFirstFarVertex, mFirstFarVertex+1, mFirstFarVertex+2);
	addTriangle(mFirstFarVertex, mFirstFarVertex+2, mFirstFarVertex+3);

	// insert other points
	for (int i = 0; i < mFirstFarVertex; i++) {
		mEdges.clear();
		int j = 0;
		while (j < (int)mTriangles.size()) {
			Triangle &t = mTriangles[j];
			if ((t.center - mVertices[i]).magnitudeSquared() > t.circumRadiusSquared) {
				j++;
				continue;
			}
			Edge e0(t.p0, t.p1);
			Edge e1(t.p1, t.p2);
			Edge e2(t.p2, t.p0);
			bool found0 = false;
			bool found1 = false;
			bool found2 = false;

			int k = 0;
			while (k < (int)mEdges.size()) {
				Edge &e = mEdges[k];
				bool found = false;
				if (e == e0) { found0 = true; found = true; }
				if (e == e1) { found1 = true; found = true; }
				if (e == e2) { found2 = true; found = true; }
				if (found) {
					mEdges[k] = mEdges[mEdges.size()-1];
					mEdges.popBack();
				}
				else k++;
			}
			if (!found0) mEdges.pushBack(e0);
			if (!found1) mEdges.pushBack(e1);
			if (!found2) mEdges.pushBack(e2);
			mTriangles[j] = mTriangles[mTriangles.size()-1];
			mTriangles.popBack();
		}
		for (j = 0; j < (int)mEdges.size(); j++) {
			Edge &e = mEdges[j];
			addTriangle(e.p0, e.p1, i);
		}
	}
}

// -------------------------------------------------------------------------------------
void Delaunay2d::addTriangle(int p0, int p1, int p2)
{
	Triangle triangle;
	triangle.p0 = p0;
	triangle.p1 = p1;
	triangle.p2 = p2;
	getCircumSphere(mVertices[p0], mVertices[p1], mVertices[p2], triangle.center, triangle.circumRadiusSquared);
	mTriangles.pushBack(triangle);
}

// -------------------------------------------------------------------------------------
void Delaunay2d::getCircumSphere(const PxVec3 &p0, const PxVec3 &p1, const PxVec3 &p2,
		PxVec3 &center, float &radiusSquared)
{
	float x1 = p1.x - p0.x;
	float y1 = p1.y - p0.y;
	float x2 = p2.x - p0.x;
	float y2 = p2.y - p0.y;

	float det = x1 * y2 - x2 * y1;
	if (det == 0.0f) {
		center = p0; radiusSquared = 0.0f;
		return;
	}
	det = 0.5f / det;
	float len1 = x1*x1 + y1*y1;
	float len2 = x2*x2 + y2*y2;
	float cx = (len1 * y2 - len2 * y1) * det;
	float cy = (len2 * x1 - len1 * x2) * det;
	center.x = p0.x + cx;
	center.y = p0.y + cy;
	center.z = 0.0f;
	radiusSquared = cx * cx + cy * cy;
}

// -------------------------------------------------------------------------------------
void Delaunay2d::computeVoronoiMesh()
{
	mConvexes.clear(); 
	mConvexVerts.clear(); 
	mConvexNeighbors.clear(); 

	int numVerts = mVertices.size();
	int numTris = mIndices.size() / 3;

	// center positions
	shdfnd::Array<PxVec3> centers(numTris);
	for (int i = 0; i < numTris; i++) {
		PxVec3 &p0 = mVertices[mIndices[3*i]];
		PxVec3 &p1 = mVertices[mIndices[3*i+1]];
		PxVec3 &p2 = mVertices[mIndices[3*i+2]];
		float r2;
		getCircumSphere(p0,p1,p2, centers[i], r2);
	}

	// vertex -> triangles links
	shdfnd::Array<int> firstVertTri(numVerts+1, 0);
	shdfnd::Array<int> vertTris;

	for (int i = 0; i < numTris; i++) {
		firstVertTri[mIndices[3*i]]++;
		firstVertTri[mIndices[3*i+1]]++;
		firstVertTri[mIndices[3*i+2]]++;
	}

	int numLinks = 0;
	for (int i = 0; i < numVerts; i++) {
		numLinks += firstVertTri[i];
		firstVertTri[i] = numLinks;
	}
	firstVertTri[numVerts] = numLinks;
	vertTris.resize(numLinks);

	for (int i = 0; i < numTris; i++) {
		int &i0 = firstVertTri[mIndices[3*i]];
		int &i1 = firstVertTri[mIndices[3*i+1]];
		int &i2 = firstVertTri[mIndices[3*i+2]];
		i0--; vertTris[i0] = i;
		i1--; vertTris[i1] = i;
		i2--; vertTris[i2] = i;
	}

	// convexes
	Convex c;
	shdfnd::Array<int> nextVert(numVerts, -1);
	shdfnd::Array<int> vertVisited(numVerts, -1);
	shdfnd::Array<int> triOfVert(numVerts, -1);
	shdfnd::Array<int> convexOfVert(numVerts, -1);

	for (int i = 0; i < numVerts; i++) {
		int first = firstVertTri[i];
		int last = firstVertTri[i+1];
		int num = last - first;
		if (num < 3)
			continue;

		int start = -1;

		for (int j = first; j < last; j++) {
			int triNr = vertTris[j];

			int k = 0;
			while (k < 3 && mIndices[3*triNr+k] != i)
				k++;

			int j0 = mIndices[3*triNr + (k+1)%3];
			int j1 = mIndices[3*triNr + (k+2)%3];

			if (j == first)
				start = j0;

			nextVert[j0] = j1;
			vertVisited[j0] = 2*i;
			triOfVert[j0] = triNr;
		}

		c.firstNeighbor = mConvexNeighbors.size();
		c.firstVert = mConvexVerts.size();
		c.numVerts = num;
		bool rollback = false;
		int id = start;
		do {
			if (vertVisited[id] != 2*i) {
				rollback = true;
				break;
			}
			vertVisited[id] = 2*i+1;

			mConvexVerts.pushBack(centers[triOfVert[id]]);
			mConvexNeighbors.pushBack(id);
			id = nextVert[id];
		} while (id != start);

		if (rollback) {
			mConvexVerts.resize(c.firstVert);
			mConvexNeighbors.resize(c.firstNeighbor);
			continue;
		}

		c.numVerts = mConvexVerts.size() - c.firstVert;
		c.numNeighbors = mConvexNeighbors.size() - c.firstNeighbor;
		convexOfVert[i] = mConvexes.size();
		mConvexes.pushBack(c);
	}

	// compute neighbors
	int newPos = 0;
	for (int i = 0; i < (int)mConvexes.size(); i++) {
		Convex &c = mConvexes[i];
		int pos = c.firstNeighbor;
		int num = c.numNeighbors;
		c.firstNeighbor = newPos;
		c.numNeighbors = 0;
		for (int j = 0; j < num; j++) {
			int n = convexOfVert[mConvexNeighbors[pos+j]];
			if (n >= 0) {
				mConvexNeighbors[newPos] = n;
				newPos++;
				c.numNeighbors++;
			}
		}
	}
	mConvexNeighbors.resize(newPos);
}

}
}
}
#endif