#include "RTdef.h"
#if RT_COMPILE
#ifndef ISLAND_DETECTOR_BASE_H
#define ISLAND_DETECTOR_BASE_H

// Matthias M�ller-Fischer

#include <PxVec3.h>
#include <PsArray.h>
#include "IceBoxPruningBase.h"
#include <PsUserAllocated.h>

namespace physx
{
namespace fracture
{
namespace base
{

class SimScene;
class Convex;

// ---------------------------------------------------------------------------------------
class IslandDetector : public ::physx::shdfnd::UserAllocated {
	friend class SimScene;
public:
	// singleton pattern
	//static IslandDetector* getInstance();
	//static void destroyInstance();

	void detect(const shdfnd::Array<Convex*> &convexes, bool computeFaceCoverage);

	struct Island {
		int size;
		int firstNr;
	};
	const shdfnd::Array<Island> &getIslands() { return mIslands; }
	const shdfnd::Array<int> &getIslandConvexes() { return mIslandConvexes; }

	const shdfnd::Array<float> &getFaceCoverage() { return mFaceCoverage; }

	class Edge{
	public:
		int n0, n1;
		float area;
	};
	const shdfnd::Array<Edge> &getNeighborEdges();

protected:
	IslandDetector(SimScene* scene);
	virtual ~IslandDetector(){}

	void createConnectivity(const shdfnd::Array<Convex*> &convexes, bool computeFaceCoverage);
	static bool touching(const Convex *c0, int faceNr, const Convex *c1, float eps);
	void createIslands();

	float faceArea(const Convex *c, int faceNr);
	float faceIntersectionArea(const Convex *c0, int faceNr0, const Convex *c1, int faceNr1);

	bool axisOverlap(const Convex* c0, const Convex* c1, PxVec3 &dir);
	bool overlap(const Convex* c0, const Convex* c1);

	void addOverlapConnectivity(const shdfnd::Array<Convex*> &convexes);

	SimScene* mScene;

	// auxiliary
	shdfnd::Array<int> mFirstNeighbor;
	struct Neighbor {
		int convexNr;
		int next;
		float area;
	};
	shdfnd::Array<Neighbor> mNeighbors;
	struct Face {
		int convexNr;
		int faceNr;
		float orderVal;
		int globalNr;
		bool operator < (const Face &f) const { return orderVal < f.orderVal; }
	};
	bool mNeigborPairsDirty;
	shdfnd::Array<Edge> mNeighborPairs;
	shdfnd::Array<float> mNeighborAreaPairs;
	shdfnd::Array<Face> mFaces;
	shdfnd::Array<int> mColors;
	shdfnd::Array<int> mQueue;

	shdfnd::Array<PxVec3> mCutPolys[2];

	// overlaps
	BoxPruning mBoxPruning;
	shdfnd::Array<PxBounds3> mBounds;
	shdfnd::Array<PxU32> mPairs;

	// output
	shdfnd::Array<Island> mIslands;
	shdfnd::Array<int> mIslandConvexes;
	shdfnd::Array<float> mFaceCoverage;
};

}
}
}

#endif
#endif