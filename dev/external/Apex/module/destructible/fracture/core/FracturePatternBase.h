#include "RTdef.h"
#if RT_COMPILE
#ifndef FRACTURE_PATTERN_BASE
#define FRACTURE_PATTERN_BASE

#include "Delaunay3dBase.h"
#include "Delaunay2dBase.h"
#include "CompoundGeometryBase.h"
#include "PxTransform.h"
#include "PxBounds3.h"
#include <PsUserAllocated.h>

namespace physx
{
namespace fracture
{
namespace base
{

class Convex;
class Compound;
class CompoundGeometry;

//------------------------------------------------------------------------------------------
class FracturePattern : public ::physx::shdfnd::UserAllocated 
{
	friend class SimScene;
protected:
	FracturePattern(SimScene* scene);
public:
	virtual ~FracturePattern() {}

	void create3dVoronoi(const PxVec3 &dims, int numCells, float biasExp = 1.0f, float maxDist = PX_MAX_F32);
	void create2dVoronoi(const PxVec3 &dims, int numCells, float biasExp = 1.0f, int numRays = 0);

	void createRegularCubeMesh(const PxVec3 &dims, float cubeSize);
	void createRegular3dVoronoi(const PxVec3 &dims, float pieceSize, float relRandOffset = 0.1f);
	void createLocal3dVoronoi(const PxVec3 &dims, int numCells, float radius);
	void createGlass(float radius, float thickness, int numSectors, float sectorRand, float firstSegmentSize, float segmentScale, float segmentRand);
	void createCrack(const PxVec3 &dims, float spacing, float zRand);
	void createCrosses(const PxVec3 &dims, float spacing);

	// radius = 0.0f : complete fracture
	// radius > 0.0f : partial (local) fracture
	void getCompoundIntersection(const Compound *compound, const PxMat44 &trans, float radius, float minConvexSize,
		shdfnd::Array<int> &compoundSizes, shdfnd::Array<Convex*> &newConvexes) const;

	const CompoundGeometry &getGeometry() { return mGeom; }

protected:
	void addBox(const PxBounds3 &bounds);
	void clear();
	void finalize();

	void getConvexIntersection(const Convex *convex, const PxMat44 &trans, float minConvexSize, int numFitDirections = 3) const;

	SimScene* mScene;

	CompoundGeometry mGeom;

	// cells can have multiple connected convexes
	// if this array is empty, each convex is a separate cell
	// size of array must be num cells + 1
	shdfnd::Array<int> mFirstConvexOfCell;	

	PxBounds3 mBounds;
	shdfnd::Array<float> mConvexVolumes;

	// temporary for intersection computation
	mutable shdfnd::Array<int> mFirstPiece;
	mutable shdfnd::Array<bool> mSplitPiece;
	struct Piece {
		Convex* convex;
		int next;
	};
	mutable shdfnd::Array<Piece> mPieces;
};

}
}
}

#endif
#endif