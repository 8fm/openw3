#include "RTdef.h"
#if RT_COMPILE
#include "IslandDetectorBase.h"
#include "ConvexBase.h"
#include "CompoundGeometryBase.h"
#include "PsSort.h"

namespace physx
{
namespace fracture
{
namespace base
{

// -----------------------------------------------------------------------------
IslandDetector::IslandDetector(SimScene* scene):
	mScene(scene)
{
	mNeigborPairsDirty = true;
}

// -----------------------------------------------------------------------------
void IslandDetector::detect(const shdfnd::Array<Convex*> &convexes, bool computeFaceCoverage)
{
	createConnectivity(convexes, computeFaceCoverage);
	addOverlapConnectivity(convexes);
	createIslands();
	mNeigborPairsDirty = true;
}

// -----------------------------------------------------------------------------
bool IslandDetector::touching(const Convex *c0, int faceNr, const Convex *c1, float eps)
{
	const Convex::Face &face = c0->getFaces()[faceNr];
	const shdfnd::Array<int> &indices = c0->getIndices();
	const shdfnd::Array<PxVec3> &verts = c0->getVertices();
	const shdfnd::Array<PxPlane> &planes = c1->getPlanes();

	for (int i = 0; i < face.numIndices; i++) {
		PxVec3 v = verts[indices[face.firstIndex + i]];
		v = v + c0->getMaterialOffset() - c1->getMaterialOffset();	// transform to c1
		bool inside = true;
		for (int j = 0; j < (int)planes.size(); j++) {
			const PxPlane &p = planes[j];
			if (p.n.dot(v) - p.d > eps) {
				inside = false;
				break;
			}
		}
		if (inside)
			return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
void IslandDetector::createConnectivity(const shdfnd::Array<Convex*> &convexes, bool computeFaceCoverage)
{
	mFaces.clear();
	Face face;
	int globalNr = 0;

	for (int i = 0; i < (int)convexes.size(); i++) {
		const Convex *c = convexes[i];
		face.convexNr = i;
		const shdfnd::Array<PxPlane> &planes = c->getPlanes();
		for (int j = 0; j < (int)planes.size(); j++) {
			globalNr++;
			if (c->getFaces()[j].flags & CompoundGeometry::FF_OBJECT_SURFACE)
				continue;
			face.faceNr = j;
			face.globalNr = globalNr-1;
			float d = planes[j].d;
			d = d + c->getMaterialOffset().dot(planes[j].n);	// transform to global
			face.orderVal = fabs(d);
			mFaces.pushBack(face);
		}
	}
	mFaceCoverage.clear();

	if (computeFaceCoverage) {
		mFaceCoverage.resize(globalNr, 0.0f);
	}

	shdfnd::sort(mFaces.begin(), mFaces.size());

	float eps = 1e-3f;
	mFirstNeighbor.clear();
	mFirstNeighbor.resize(convexes.size(), -1);
	mNeighbors.clear();
	Neighbor n;

	for (int i = 0; i < (int)mFaces.size(); i++) {
		Face &fi = mFaces[i];
		int j = i+1;
		while (j < (int)mFaces.size() && mFaces[j].orderVal - fi.orderVal < eps) {
			Face &fj = mFaces[j];
			j++;

			if (fi.convexNr == fj.convexNr)
				continue;

			PxPlane pi = convexes[fi.convexNr]->getPlanes()[fi.faceNr];
			if (convexes[fi.convexNr]->isGhostConvex()) 
				pi.n = -pi.n; 

			PxPlane pj = convexes[fj.convexNr]->getPlanes()[fj.faceNr];
			if (convexes[fj.convexNr]->isGhostConvex()) 
				pj.n = -pj.n; 

			if (pi.n.dot(pj.n) > -1.0f + eps)
				continue;	// need to be opposite

			if (
				touching(convexes[fi.convexNr], fi.faceNr, convexes[fj.convexNr], eps) ||
				touching(convexes[fj.convexNr], fj.faceNr, convexes[fi.convexNr], eps))
			{
				n.area = 0.5f*( fabs(faceArea(convexes[fi.convexNr], fi.faceNr)) + fabs(faceArea(convexes[fj.convexNr], fj.faceNr)));
				if (computeFaceCoverage) {
					float area = faceIntersectionArea(convexes[fi.convexNr], fi.faceNr, convexes[fj.convexNr], fj.faceNr);
					mFaceCoverage[fi.globalNr] += area;
					mFaceCoverage[fj.globalNr] += area;
				}

				n.convexNr = fj.convexNr;
				n.next = mFirstNeighbor[fi.convexNr];
				mFirstNeighbor[fi.convexNr] = mNeighbors.size();
				mNeighbors.pushBack(n);

				n.convexNr = fi.convexNr;
				n.next = mFirstNeighbor[fj.convexNr];
				mFirstNeighbor[fj.convexNr] = mNeighbors.size();
				mNeighbors.pushBack(n);
			}
		}
	}

	if (computeFaceCoverage) {
		int globalNr = 0;

		for (int i = 0; i < (int)convexes.size(); i++) {
			const Convex *c = convexes[i];
			int numFaces = c->getPlanes().size();
			for (int j = 0; j < numFaces; j++) {
				float total = faceArea(c, j);
				if (total != 0.0f)
					mFaceCoverage[globalNr] /= total;
				globalNr++;
			}
		}
	}

	mNeigborPairsDirty = true;
}

// -----------------------------------------------------------------------------
bool IslandDetector::axisOverlap(const Convex* c0, const Convex* c1, PxVec3 &dir)
{
	float off0 = c0->getMaterialOffset().dot(dir);
	float off1 = c1->getMaterialOffset().dot(dir);
	const shdfnd::Array<PxVec3> &verts0 = c0->getVertices();
	const shdfnd::Array<PxVec3> &verts1 = c1->getVertices();
	if (verts0.empty() || verts1.empty())
		return false;

	float d0 = dir.dot(verts0[0]) + off0;
	float d1 = dir.dot(verts1[0]) + off1;
	if (d0 < d1) {
		float max0 = d0;
		for (int i = 1; i < (int)verts0.size(); i++) {
			float d = verts0[i].dot(dir) + off0;
			if (d > max0) max0 = d;
		}
		for (int i = 0; i < (int)verts1.size(); i++) {
			float d = verts1[i].dot(dir) + off1;
			if (d < max0) 
				return true;
		}
	}
	else {
		float max1 = d1;
		for (int i = 1; i < (int)verts1.size(); i++) {
			float d = verts1[i].dot(dir) + off1;
			if (d > max1) max1 = d;
		}
		for (int i = 0; i < (int)verts0.size(); i++) {
			float d = verts0[i].dot(dir) + off0;
			if (d < max1)
				return true;
		}
	}
	return false;
}

// -----------------------------------------------------------------------------
bool IslandDetector::overlap(const Convex* c0, const Convex* c1)
{
	PxVec3 off0 = c0->getMaterialOffset();
	PxVec3 off1 = c1->getMaterialOffset();
	PxVec3 center0 = c0->getCenter() + off0;
	PxVec3 center1 = c1->getCenter() + off1;
	PxVec3 dir = center1 - center0;

	if (!axisOverlap(c0, c1, dir))
		return false;

	// todo: test other axes

	return true;
}

// -----------------------------------------------------------------------------
void IslandDetector::addOverlapConnectivity(const shdfnd::Array<Convex*> &convexes)
{
	mBounds.resize(convexes.size());
	PxVec3 off;

	for (int i = 0; i < (int)convexes.size(); i++) {
		const Convex *c = convexes[i];
		const PxBounds3 &cb = c->getBounds();
		off = c->getMaterialOffset();
		
		PxBounds3 &b = mBounds[i];
		b.minimum = cb.minimum + off;
		b.maximum = cb.maximum + off;
	}

	// broad phase with AABBs
	Axes axes;
	axes.set(0,2,1);
	mBoxPruning.completeBoxPruning(mBounds, mPairs, axes);

	for (int i = 0; i < (int)mPairs.size(); i += 2) {
		int i0 = mPairs[i];
		int i1 = mPairs[i+1];

		if (convexes[i0]->getModelIslandNr() == convexes[i1]->getModelIslandNr())
			continue;

		// already detected?
		bool found = false;
		int nr = mFirstNeighbor[i0];
		while (nr >= 0) {
			Neighbor &n = mNeighbors[nr];
			nr = n.next;
			if (n.convexNr == i1) {
				found = true;
				break;
			}
		}
		if (found)
			continue;

		if (overlap(convexes[i0], convexes[i1])) {
			Neighbor n;

			// add link
			n.convexNr = i0;
			n.next = mFirstNeighbor[i1];
			mFirstNeighbor[i1] = mNeighbors.size();
			mNeighbors.pushBack(n);

			n.convexNr = i1;
			n.next = mFirstNeighbor[i0];
			mFirstNeighbor[i0] = mNeighbors.size();
			mNeighbors.pushBack(n);
		}
		
	}
}

// -----------------------------------------------------------------------------
void IslandDetector::createIslands()
{
	int numConvexes = mFirstNeighbor.size();
	mIslands.clear();
	mIslandConvexes.clear();

	Island island;

	// color convexes
	mColors.clear();
	mColors.resize(numConvexes,-1);
	int color = -1;

	for (int i = 0; i < numConvexes; i++) {
		if (mColors[i] >= 0)
			continue;
		mQueue.clear();
		mQueue.pushBack(i);
		color++;
		island.firstNr = mIslandConvexes.size();
		island.size = 0;

		while (mQueue.size() > 0) {
			int convexNr = mQueue[mQueue.size()-1];
			mQueue.popBack();
			if (mColors[convexNr] >= 0)
				continue;
			mColors[convexNr] = color;
			mIslandConvexes.pushBack(convexNr);
			island.size++;

			int nr = mFirstNeighbor[convexNr];
			while (nr >= 0) {
				int adj = mNeighbors[nr].convexNr;
				nr = mNeighbors[nr].next;
				if (mColors[adj] < 0)
					mQueue.pushBack(adj);
			}
		}
		mIslands.pushBack(island);
	}
}

// -----------------------------------------------------------------------------
float IslandDetector::faceArea(const Convex *c, int faceNr)
{
	const Convex::Face &face = c->getFaces()[faceNr];
	const shdfnd::Array<PxVec3> &verts = c->getVertices();
	const shdfnd::Array<int> &indices = c->getIndices();

	float area = 0.0f;
	PxVec3 p0 = verts[indices[face.firstIndex]];
	for (int i = 1; i < face.numIndices-1; i++) {
		PxVec3 p1 = verts[indices[face.firstIndex+i]];
		PxVec3 p2 = verts[indices[face.firstIndex+i+1]];
		area += (p1-p0).cross(p2-p0).magnitude();
	}
	return 0.5f * area;
}

// -----------------------------------------------------------------------------
float IslandDetector::faceIntersectionArea(const Convex *c0, int faceNr0, const Convex *c1, int faceNr1)
{
	// vertices of the first face
	mCutPolys[0].clear();
	mCutPolys[1].clear();

	const Convex::Face &face0 = c0->getFaces()[faceNr0];
	const shdfnd::Array<PxVec3> &verts0 = c0->getVertices();
	const shdfnd::Array<int> &indices0 = c0->getIndices();

	const Convex::Face &face1 = c1->getFaces()[faceNr1];
	const shdfnd::Array<PxVec3> &verts1 = c1->getVertices();
	const shdfnd::Array<int> &indices1 = c1->getIndices();

	for (int i = 0; i < face0.numIndices; i++) 
		mCutPolys[0].pushBack(verts0[indices0[face0.firstIndex + i]]);

	const PxVec3 &n0 = c0->getPlanes()[faceNr0].n;

	// cut face 0 polygon with face 1 polygon
	for (int i = 0; i < face1.numIndices; i++) {
		const PxVec3 &p0 = verts1[indices1[face1.firstIndex + (i+1)%face1.numIndices]];
		const PxVec3 &p1 = verts1[indices1[face1.firstIndex + i]];
		PxVec3 n = (p1-p0).cross(n0);
		float d = n.dot(p0);

		shdfnd::Array<PxVec3> &currPoly = mCutPolys[i%2];
		shdfnd::Array<PxVec3> &newPoly = mCutPolys[1 - (i%2)];
		newPoly.clear();

		int num = currPoly.size();
		for (int j = 0; j < num; j++) {
			PxVec3 &q0 = currPoly[j];
			PxVec3 &q1 = currPoly[(j+1) % num];

			bool inside0 = q0.dot(n) < d;
			bool inside1 = q1.dot(n) < d;

			if (inside0) 
				newPoly.pushBack(currPoly[j]);	// point inside -> keep

			if (inside0 != inside1) {		// intersection
				float s = (d - q0.dot(n)) / (q1-q0).dot(n);
				PxVec3 ps = q0 + s*(q1-q0);
				newPoly.pushBack(ps);
			}
		}
	}

	// measure area
	float area = 0.0f;
	shdfnd::Array<PxVec3> &currPoly = mCutPolys[face1.numIndices%2];
	if (currPoly.size() < 3)
		return 0.0f;

	PxVec3 &p0 = currPoly[0];
	for (int i = 1; i < (int)currPoly.size()-1; i++) {
		PxVec3 &p1 = currPoly[i];
		PxVec3 &p2 = currPoly[i+1];
		area += (p1-p0).cross(p2-p0).magnitude();
	}
	return 0.5f * area;
}

// -----------------------------------------------------------------------------
const shdfnd::Array<IslandDetector::Edge> &IslandDetector::getNeighborEdges()
{
	if (mNeigborPairsDirty) {
		Edge e;
		mNeighborPairs.clear();
		for (int i = 0; i < (int)mFirstNeighbor.size(); i++) {
			int nr = mFirstNeighbor[i];
			e.n0 = i;
			while (nr >= 0) {
				Neighbor &n = mNeighbors[nr];
				nr = n.next;
				if (n.convexNr > i) {
					e.n1 = n.convexNr;
					e.area = n.area;
					mNeighborPairs.pushBack(e);
					//mNeighborPairs.pushBack(i);
					//mNeighborPairs.pushBack(n.convexNr);
				}
			}
		}
		mNeigborPairsDirty = false;
	}
	return mNeighborPairs;
}

}
}
}
#endif