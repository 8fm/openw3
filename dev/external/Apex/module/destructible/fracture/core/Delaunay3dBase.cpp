#include "RTdef.h"
#if RT_COMPILE
#include "Delaunay3dBase.h"
#include "PsSort.h"
#include <PxAssert.h>

#include "PxBounds3.h"

namespace physx
{
namespace fracture
{
namespace base
{

// side orientation points outwards
const int Delaunay3d::Tetra::sideIndices[4][3] = {{1,2,3},{2,0,3},{0,1,3},{2,1,0}};

// ------ singleton pattern -----------------------------------------------------------
//
//static Delaunay3d *gDelaunay3d = NULL;
//
//Delaunay3d* Delaunay3d::getInstance() 
//{
//	if (gDelaunay3d == NULL) {
//		gDelaunay3d = PX_NEW(Delaunay3d)();
//	}
//	return gDelaunay3d;
//}
//
//void Delaunay3d::destroyInstance() 
//{
//	if (gDelaunay3d != NULL) {
//		PX_DELETE(gDelaunay3d);
//	}
//	gDelaunay3d = NULL;
//}

// -----------------------------------------------------------------------------
void Delaunay3d::tetrahedralize(const PxVec3 *vertices,  int numVerts, int byteStride, bool removeExtraVerts)
{
	clear();

	PxU8 *vp = (PxU8*)vertices;
	for (int i = 0; i < numVerts; i++) {
		mVertices.pushBack(*(PxVec3*)vp);
		vp += byteStride;
	}

	delaunayTetrahedralization();

	compressTetrahedra(removeExtraVerts);
	if (removeExtraVerts) {
		mVertices.resize(mFirstFarVertex);
	}

	for (int i = 0; i < (int)mTetras.size(); i++) {
		Tetra &t = mTetras[i];
		mIndices.pushBack(t.ids[0]);
		mIndices.pushBack(t.ids[1]);
		mIndices.pushBack(t.ids[2]);
		mIndices.pushBack(t.ids[3]);
	}
}

// -----------------------------------------------------------------------------
void Delaunay3d::clear()
{
	mVertices.clear();
	mIndices.clear();
	mTetras.clear();
	mFirstFarVertex = 0;
	mLastFarVertex = 0;

	mGeom.clear();

	mTetMarked.clear();
	mTetMark = 0;
}

// -----------------------------------------------------------------------------
void Delaunay3d::delaunayTetrahedralization()
{
	int i,j;

	shdfnd::Array<Edge> edges;
	Edge edge;

	PxBounds3 bounds;
	bounds.setEmpty();
	for (int i = 0; i < (int)mVertices.size(); i++) 
		bounds.include(mVertices[i]);

	// start with large tetrahedron
	mTetras.clear();
	float a = 3.0f * bounds.getDimensions().magnitude();
	float x  = 0.5f * a;
	float y0 = x / sqrt(3.0f);
	float y1 = x * sqrt(3.0f) - y0;
	float z0 = 0.25f * sqrt(6.0f) * a;
	float z1 = a * sqrt(6.0f) / 3.0f - z0;

	PxVec3 center = bounds.getCenter();

	mFirstFarVertex = mVertices.size();
	PxVec3 p0(-x,-y0,-z1); mVertices.pushBack(center + p0);
	PxVec3 p1( x,-y0,-z1); mVertices.pushBack(center + p1);
	PxVec3 p2( 0, y1,-z1); mVertices.pushBack(center + p2);
	PxVec3 p3( 0,  0, z0); mVertices.pushBack(center + p3);
	mLastFarVertex = mVertices.size()-1;

	Tetra tetra;
	tetra.init(mFirstFarVertex, mFirstFarVertex+1, mFirstFarVertex+2, mFirstFarVertex+3);
	mTetras.pushBack(tetra);

	// build Delaunay triangulation iteratively

	int lastVertex = (int)mVertices.size()-4;
	for (i = 0; i < lastVertex; i++) {
		PxVec3 &v = mVertices[i];

		int tNr = findSurroundingTetra(mTetras.size()-1, v);	// fast walk
		if (tNr < 0)
			continue;
		// assert(tNr >= 0);

		int newTetraNr = mTetras.size();
		retriangulate(tNr, i);
		edges.clear();
		for (j = newTetraNr; j < (int)mTetras.size(); j++) {
			Tetra &newTet = mTetras[j];
			edge.init(newTet.ids[2], newTet.ids[3], j,1);
			edges.pushBack(edge);
			edge.init(newTet.ids[3], newTet.ids[1], j,2);
			edges.pushBack(edge);
			edge.init(newTet.ids[1], newTet.ids[2], j,3);
			edges.pushBack(edge);
		}
		shdfnd::sort(edges.begin(), edges.size());
		for (j = 0; j < (int)edges.size(); j += 2) {
			Edge &edge0 = edges[j];
			Edge &edge1 = edges[j+1];
			PX_ASSERT(edge0 == edge1);
			mTetras[edge0.tetraNr].neighborNrs[edge0.neighborNr] = edge1.tetraNr;
			mTetras[edge1.tetraNr].neighborNrs[edge1.neighborNr] = edge0.tetraNr;
		}
	}
}

// -----------------------------------------------------------------------------
int Delaunay3d::findSurroundingTetra(int startTetra, const PxVec3 &p) const
{
	// find the tetrahedra which contains the vertex
	// by walking through mesh O(n ^ (1/3))

	mTetMark++;

	if (mTetras.size() <= 0) return -1;
	int tetNr = startTetra;
	PX_ASSERT(!mTetras[startTetra].deleted);

	if (mTetMarked.size() < mTetras.size())
		mTetMarked.resize(mTetras.size(), -1);

	bool loop = false;

	while (tetNr >= 0) {
		if (mTetMarked[tetNr] == mTetMark) {
			loop = true;
			break;
		}
		mTetMarked[tetNr] = mTetMark;

		const Tetra &tetra = mTetras[tetNr];
		const PxVec3 &p0 = mVertices[tetra.ids[0]];
		PxVec3 q  = p-p0;
		PxVec3 q0 = mVertices[tetra.ids[1]] - p0;
		PxVec3 q1 = mVertices[tetra.ids[2]] - p0;
		PxVec3 q2 = mVertices[tetra.ids[3]] - p0;
		PxMat33 m;
		m.column0 = q0;
		m.column1 = q1;
		m.column2 = q2;
		float det = m.getDeterminant();
		m.column0 = q;
		float x = m.getDeterminant();
		if (x < 0.0f && tetra.neighborNrs[1] >= 0) {
			tetNr = tetra.neighborNrs[1];
			continue;
		}
		m.column0 = q0; m.column1 = q;
		float y = m.getDeterminant();
		if (y < 0.0f && tetra.neighborNrs[2] >= 0) {
			tetNr = tetra.neighborNrs[2];
			continue;
		}
		m.column1 = q1; m.column2 = q;
		float z = m.getDeterminant();
		if (z < 0.0f && tetra.neighborNrs[3] >= 0) {
			tetNr = tetra.neighborNrs[3];
			continue;
		}
		if (x + y + z > det  && tetra.neighborNrs[0] >= 0) {
			tetNr = tetra.neighborNrs[0];
			continue;
		}
		return tetNr;
	}
	if (loop) {		// search failed, brute force:
		for (int i = 0; i < (int)mTetras.size(); i++) {
			const Tetra &tetra = mTetras[i];
			if (tetra.deleted)
				continue;

			const PxVec3 &p0 = mVertices[tetra.ids[0]];
			const PxVec3 &p1 = mVertices[tetra.ids[1]];
			const PxVec3 &p2 = mVertices[tetra.ids[2]];
			const PxVec3 &p3 = mVertices[tetra.ids[3]];

			PxVec3 n;
			n = (p1-p0).cross(p2-p0);
			if (n.dot(p) < n.dot(p0)) continue;
			n = (p2-p0).cross(p3-p0);
			if (n.dot(p) < n.dot(p0)) continue;
			n = (p3-p0).cross(p1-p0);
			if (n.dot(p) < n.dot(p0)) continue;
			n = (p3-p1).cross(p2-p1);
			if (n.dot(p) < n.dot(p1)) continue;
			return i;
		}
		return -1;
	}
	else
		return -1;
}

// -----------------------------------------------------------------------------
void Delaunay3d::updateCircumSphere(Tetra &tetra)
{
	if (!tetra.circumsphereDirty)  
		return;
	PxVec3 p0 = mVertices[tetra.ids[0]];
	PxVec3 b  = mVertices[tetra.ids[1]] - p0;
	PxVec3 c  = mVertices[tetra.ids[2]] - p0;
	PxVec3 d  = mVertices[tetra.ids[3]] - p0;
	float det = b.x*(c.y*d.z - c.z*d.y) - b.y*(c.x*d.z - c.z*d.x) + b.z*(c.x*d.y-c.y*d.x);
	if (det == 0.0f) {
		tetra.center = p0; tetra.radiusSquared = 0.0f; 
		return; // singular case
	}
	det *= 2.0f;
	PxVec3 v = c.cross(d)*b.dot(b) + d.cross(b)*c.dot(c) +  b.cross(c)*d.dot(d);
	v /= det;
	tetra.radiusSquared = v.magnitudeSquared();
	tetra.center = p0 + v;
	tetra.circumsphereDirty = false;
}

// -----------------------------------------------------------------------------
bool Delaunay3d::pointInCircumSphere(Tetra &tetra, const PxVec3 &p)
{
	updateCircumSphere(tetra);
	return (tetra.center - p).magnitudeSquared() < tetra.radiusSquared;
}

// -----------------------------------------------------------------------------
void Delaunay3d::retriangulate(int tetraNr, int vertNr)
{
	Tetra &tetra = mTetras[tetraNr];
	if (tetra.deleted) return;
	Tetra tNew;
	PxVec3 &v = mVertices[vertNr];
	tetra.deleted = true;
	for (int i = 0; i < 4; i++) {
		int n = mTetras[tetraNr].neighborNrs[i];
		if (n >= 0 && mTetras[n].deleted)
			continue;
		if (n >= 0 && pointInCircumSphere(mTetras[n],v))
			retriangulate(n, vertNr);
		else {
			Tetra &t = mTetras[tetraNr];
			tNew.init(vertNr, 
				t.ids[Tetra::sideIndices[i][0]], 
				t.ids[Tetra::sideIndices[i][1]], 
				t.ids[Tetra::sideIndices[i][2]]
			);
			tNew.neighborNrs[0] = n;
			if (n >= 0) {
				mTetras[n].neighborOf(
					tNew.ids[1], tNew.ids[2], tNew.ids[3]) 
					= mTetras.size();
			}
			mTetras.pushBack(tNew);
		}
	}
}

// ----------------------------------------------------------------------
void Delaunay3d::compressTetrahedra(bool removeExtraVerts)
{
	shdfnd::Array<int> oldToNew(mTetras.size(), -1);

	int num = 0;
	for (int i = 0; i < (int)mTetras.size(); i++) {
		Tetra &t = mTetras[i];

		if (removeExtraVerts) {
			if (t.ids[0] >= mFirstFarVertex ||
				t.ids[1] >= mFirstFarVertex ||
				t.ids[2] >= mFirstFarVertex ||
				t.ids[3] >= mFirstFarVertex)
				continue;
		}

		if (t.deleted) 
			continue;

		oldToNew[i] = num;
		mTetras[num] = t;
		num++;
	}
	mTetras.resize(num);

	for (int i = 0; i < num; i++) {
		Tetra &t = mTetras[i];
		for (int j = 0; j < 4; j++) {
			if (t.neighborNrs[j] >= 0)
				t.neighborNrs[j] = oldToNew[t.neighborNrs[j]];
		}
	}
}

// ----------------------------------------------------------------------
void Delaunay3d::computeVoronoiMesh()
{
	mGeom.clear();

	if (mTetras.empty())
		return;

	// vertex -> tetras links
	int numVerts = mVertices.size();

	shdfnd::Array<int> firstVertTet(numVerts+1, 0);
	shdfnd::Array<int> vertTets;

	shdfnd::Array<int> tetMarks(mTetras.size(), -1);
	int tetMark = 0;

	for (int i = 0; i < (int)mTetras.size(); i++) {
		Tetra &t = mTetras[i];
		firstVertTet[t.ids[0]]++;
		firstVertTet[t.ids[1]]++;
		firstVertTet[t.ids[2]]++;
		firstVertTet[t.ids[3]]++;
	}

	int numLinks = 0;
	for (int i = 0; i < numVerts; i++) {
		numLinks += firstVertTet[i];
		firstVertTet[i] = numLinks;
	}
	firstVertTet[numVerts] = numLinks;
	vertTets.resize(numLinks);

	for (int i = 0; i < (int)mTetras.size(); i++) {
		Tetra &t = mTetras[i];
		int &i0 = firstVertTet[t.ids[0]];
		int &i1 = firstVertTet[t.ids[1]];
		int &i2 = firstVertTet[t.ids[2]];
		int &i3 = firstVertTet[t.ids[3]];
		i0--; vertTets[i0] = i;
		i1--; vertTets[i1] = i;
		i2--; vertTets[i2] = i;
		i3--; vertTets[i3] = i;
	}

	shdfnd::Array<int> convexOfFace(numVerts, -1);
	shdfnd::Array<int> vertOfTet(mTetras.size(), -1);
	shdfnd::Array<int> convexOfVert(numVerts, -1);

	for (int i = 0; i < numVerts; i++) {
		int i0 = i;

		int firstTet = firstVertTet[i];
		int lastTet = firstVertTet[i+1];

		// debug
		//bool ok = true;
		//for (int j = firstTet; j < lastTet; j++) {
		//	int tetNr = vertTets[j];
		//	Tetra &t = mTetras[tetNr];
		//	int num = 0;
		//	for (int k = 0; k < 4; k++) {
		//		int n = t.neighborNrs[k];
		//		bool in = false;
		//		for (int l = firstTet; l < lastTet; l++) {
		//			if (vertTets[l] == n)
		//				in = true;
		//		}
		//		if (in)
		//			num++;
		//	}
		//	if (num != 3)
		//		ok = false;
		//}
		//if (ok)
		//	int foo = 0;


		// new convex
		int convexNr = i;
		CompoundGeometry::Convex c;
		mGeom.initConvex(c);
		c.numVerts = lastTet - firstTet;

		bool rollBack = false;

		// create vertices
		for (int j = firstTet; j < lastTet; j++) {
			int tetNr = vertTets[j];
			vertOfTet[tetNr] = j - firstTet;
			updateCircumSphere(mTetras[tetNr]);
			mGeom.vertices.pushBack(mTetras[tetNr].center);
		}

		// create indices
		for (int j = firstTet; j < lastTet; j++) {
			int tetNr = vertTets[j];
			Tetra &t = mTetras[tetNr];

			for (int k = 0; k < 4; k++) {
				int i1 = t.ids[k];
				if (i1 == i0)
					continue;
				if (convexOfFace[i1] == convexNr)
					continue;
				convexOfFace[i1] = convexNr;
				mGeom.neighbors.pushBack(i1);
				c.numNeighbors++;

				// new face
				c.numFaces++;
				PxVec3 faceN = mVertices[i1] - mVertices[i0];
				PxVec3 faceC = (mVertices[i1] + mVertices[i0]) * 0.5f;
				int faceStart = mGeom.indices.size();
				mGeom.indices.pushBack(0);
				mGeom.indices.pushBack(0);		// face attrib

				int faceSize = 0;
				int currNr = tetNr;
				int prevNr = -1;
				tetMark++;

				do {
					mGeom.indices.pushBack(vertOfTet[currNr]);
					faceSize++;
					
					if (tetMarks[currNr] == tetMark) {	// safety
						rollBack = true;
						break;
					}
					tetMarks[currNr] = tetMark;

					// find proper neighbor tet
					int nextTet = -1;
					for (int l = 0; l < 4; l++) {
						int nr = mTetras[currNr].neighborNrs[l];
						if (nr < 0)
							continue;
						if (nr == prevNr)
							continue;

						Tetra &tn = mTetras[nr];
						bool hasEdge = 
							(tn.ids[0] == i0 || tn.ids[1] == i0 || tn.ids[2] == i0 || tn.ids[3] == i0) &&
							(tn.ids[0] == i1 || tn.ids[1] == i1 || tn.ids[2] == i1 || tn.ids[3] == i1);
						if (!hasEdge)
							continue;

						//if (prevNr < 0) {	// correct winding
						//	if ((t.center - faceC).cross(tn.center - faceC).dot(faceN) < 0.0f)
						//		continue;
						//}
						nextTet = nr;
						break;
					}

					if (nextTet < 0) {		// not proper convex, roll back
						rollBack = true;
						break;
					}

					prevNr = currNr;
					currNr = nextTet;

				} while (currNr != tetNr);

				if (rollBack)
					break;

				mGeom.indices[faceStart] = faceSize;
			}
			if (rollBack)
				break;
		}
		if (rollBack) {
			mGeom.indices.resize(c.firstIndex);
			mGeom.vertices.resize(c.firstVert);
		}
		else {
			convexOfVert[i] = mGeom.convexes.size();
			mGeom.convexes.pushBack(c);
		}
	}

	// fix face orientations
	for (int i = 0; i < (int)mGeom.convexes.size(); i++) {
		CompoundGeometry::Convex &c = mGeom.convexes[i];
		int *ids = &mGeom.indices[c.firstIndex];
		PxVec3 *vertices = &mGeom.vertices[c.firstVert];
		PxVec3 cc(0.0f, 0.0f, 0.0f);
		for (int j = 0; j < c.numVerts; j++)
			cc += vertices[j];
		cc /= (float)c.numVerts;
		
		int *faceIds = ids;
		for (int j = 0; j < c.numFaces; j++) {
			int faceSize = *faceIds++;
			*faceIds++; //int faceAttr = *faceIds++;
			PxVec3 fc(0.0f, 0.0f, 0.0f);
			for (int k = 0; k < faceSize; k++)
				fc += vertices[faceIds[k]];
			fc /= (float)faceSize;
			PxVec3 n = fc - cc;
			if ((vertices[faceIds[1]] - vertices[faceIds[0]]).cross(vertices[faceIds[2]] - vertices[faceIds[0]]).dot(n) < 0.0f) {
				for (int k = 0; k < faceSize/2; k++) {
					int d = faceIds[k]; faceIds[k] = faceIds[faceSize-1-k]; faceIds[faceSize-1-k] = d;
				}
			}
			faceIds += faceSize;
		}
	}

	// fix neighbors
	for (int i = 0; i < (int)mGeom.neighbors.size(); i++) {
		if (mGeom.neighbors[i] >= 0)
			mGeom.neighbors[i] = convexOfVert[mGeom.neighbors[i]];
	}
}

}
}
}
#endif