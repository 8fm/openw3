#include "RTdef.h"
#if RT_COMPILE
#ifndef CONVEX_BASE
#define CONVEX_BASE

#include <PxPhysics.h>
#include <PxCooking.h>
#include <PxVec3.h>
#include <PxPlane.h>
#include <PxBounds3.h>
#include <PxTransform.h>
#include <PsArray.h>
#include <PsUserAllocated.h>

namespace physx
{
	class PxShape;
	class PxActor;
	class PxScene;
	class PxConvexMesh;

namespace fracture
{
namespace base
{

class Compound;
class CompoundGeometry;
class MeshClipper;
class SimScene;

// ----------------------------------------------------------------------------
class Convex : public ::physx::shdfnd::UserAllocated
{
	friend class SimScene;
protected:
	Convex(SimScene* scene);
public:
	virtual ~Convex();

	void createFromConvex(const Convex *convex, const PxTransform *trans = NULL);
	void createFromGeometry(const CompoundGeometry &geom, int convexNr, const PxMat44 *trans = NULL);
	//bool createFromXml(XMLParser *p, float scale, bool ignoreVisualMesh = false);

	void transform(const PxMat44 &trans);
	PxVec3 centerAtZero();
	PxVec3 getCenter() const;
	void setTexScale(float texScale) { mTexScale = texScale; }
	void increaseRefCounter() { mRefCounter++; }
	int  decreaseRefCounter() { mRefCounter--; return mRefCounter; }

	bool rayCast(const PxVec3 &ray, const PxVec3 &dir, float &dist, PxVec3 &normal) const;
	bool collide(const PxVec3 &pos, float r, float &penetration, PxVec3 &surfaceNormal, PxVec3 &surfaceVel) const;

	void intersectWithConvex(const PxPlane *planes, int numPlanes, const PxMat44 &trans, bool &empty);

	virtual void draw(bool /*debug*/ = false) {}

	PxConvexMesh* createPxConvexMesh(Compound *parent, PxPhysics *pxPhysics, PxCooking *pxCooking);
	void setPxActor(PxRigidActor *actor);
	void setLocalPose(const PxTransform &pose);

	// accessors
	Compound *getParent() { return mParent; }
	const Compound *getParent() const { return mParent; }
	const PxConvexMesh *getPxConvexMesh() const { return mPxConvexMesh; }
	PxConvexMesh *getPxConvexMesh() { return mPxConvexMesh; }

	const shdfnd::Array<PxPlane> &getPlanes() const { return mPlanes; };
	const PxBounds3 &getBounds() const { return mBounds; }
	void getWorldBounds(PxBounds3 &bounds) const;
	void getLocalBounds(PxBounds3 &bounds) const;
	float getVolume() const;
	void removeInvisibleFacesFlags();
	void updateFaceVisibility(const float *faceCoverage);
	void clearFraceFlags(unsigned int flag);

	struct Face {
		void init() { 
			firstIndex = 0; numIndices = 0; flags = 0; firstNormal = 0;
		}
		int firstIndex;
		int numIndices;
		int flags;
		int firstNormal;


	};

	const shdfnd::Array<Face> &getFaces() const { return mFaces; }
	const shdfnd::Array<int> &getIndices() const { return mIndices; }
	const shdfnd::Array<PxVec3> &getVertices() const { return mVertices; }

	const shdfnd::Array<PxVec3> &getVisVertices() const { return mVisVertices; }
	const shdfnd::Array<PxVec3> &getVisNormals() const { return mVisNormals; }
	const shdfnd::Array<PxVec3> &getVisTangents() const { return mVisTangents; }
	const shdfnd::Array<float> &getVisTexCoords() const { return mVisTexCoords; }
	const shdfnd::Array<int> &getVisTriIndices() const { return mVisTriIndices; }

	const shdfnd::Array<int> &getVisPolyStarts() const { return mVisPolyStarts; }
	const shdfnd::Array<int> &getVisPolyIndices() const { return mVisPolyIndices; }
	const shdfnd::Array<int> &getVisPolyNeighbors() const { return mVisPolyNeighbors; }

	PxVec3 getMaterialOffset() const { return mMaterialOffset; }
	void   setMaterialOffset(const PxVec3 &offset);
	PxTransform getGlobalPose() const;
	PxTransform getLocalPose() const;

	bool isGhostConvex() const { return mIsGhostConvex; }

	// explicit visual mesh
	bool hasExplicitVisMesh() const { return mHasExplicitVisMesh; }
	bool setExplicitVisMeshFromTriangles(int numVertices, const PxVec3 *vertices, const PxVec3 *normals, const PxVec2 *texcoords,
								int numIndices, const PxU32 *indices, PxTransform *trans = NULL, const PxVec3* scale = NULL);
	bool setExplicitVisMeshFromPolygons(int numVertices, const PxVec3 *vertices, const PxVec3 *normals, 
								const PxVec3 *tangents, const float *texCoords, 
								int numPolygons, const int *polyStarts, // numPolygons+1 entries
								int numIndices, const int *indices, PxTransform *trans = NULL, const PxVec3* scale = NULL);
	void createVisTrisFromPolys();
	void createVisMeshFromConvex();
	void transformVisualMesh(const PxTransform &trans);
	bool insideVisualMesh(const PxVec3 &pos) const;

	bool clipVisualMesh(MeshClipper *clipper, const PxTransform &trans, shdfnd::Array<Convex*> &newConvexes);
	void fitToVisualMesh(bool &cutEmpty, int numFitDirections = 3);

	bool isOnConvexSurface(const PxVec3 pts) const;
	bool check();
	
	PxActor* getActor();
	bool insideFattened(const PxVec3 &pos, float r) const;

	bool use2dTexture() const { return mUse2dTexture; }
	bool isIndestructible() const { return mIndestructible; }
	int  getMaterialId() const { return mMaterialId; }
	int  getSurfaceMaterialId() const { return mSurfaceMaterialId; }
	void setSurfaceMaterialId(int id) { mSurfaceMaterialId = id; }

	void setModelIslandNr(int nr) { mModelIslandNr = nr; }
	int  getModelIslandNr() const { return mModelIslandNr; } 

	void setConvexRendererInfo(int groupNr, int groupPos) const { mConvexRendererGroupNr = groupNr; mConvexRendererGroupPos = groupPos; }
	int getConvexRendererGroupNr() const { return mConvexRendererGroupNr; }
	int getConvexRendererGroupPos() const { return mConvexRendererGroupPos; }

	void setIsFarConvex(bool v) { mIsFarConvex = v; }
	bool getIsFarConvex() { return mIsFarConvex; }

protected:
	void clear();
	void finalize();
	void updateBounds();
	void updatePlanes();

	bool computeVisMeshNeighbors();
	void computeVisTangentsFromPoly();
	bool cutVisMesh(const PxVec3 &localPlaneN, float localPlaneD, bool &cutEmpty);
	bool cut(const PxVec3 &localPlaneN, float localPlaneD, bool &cutEmpty, bool setNewFaceFlag = true);

	bool rayCastConvex(const PxVec3 &orig, const PxVec3 &dir, float &dist, PxVec3 &normal) const;
	bool rayCastVisMesh(const PxVec3 &orig, const PxVec3 &dir, float &dist, PxVec3 &normal) const;

	SimScene* mScene;

	shdfnd::Array<Face> mFaces;
	shdfnd::Array<int> mIndices;
	shdfnd::Array<PxVec3> mVertices;
	shdfnd::Array<PxVec3> mNormals;
	shdfnd::Array<PxPlane> mPlanes;

	shdfnd::Array<PxVec3> mVisVertices;
	shdfnd::Array<PxVec3> mVisNormals;
	shdfnd::Array<PxVec3> mVisTangents;
	shdfnd::Array<float> mVisTexCoords;
	shdfnd::Array<int> mVisTriIndices;

	int mRefCounter;
	bool  mHasExplicitVisMesh;
	bool  mIsGhostConvex;
	shdfnd::Array<int> mVisPolyStarts;	// for explicit mesh only
	shdfnd::Array<int> mVisPolyIndices;
	shdfnd::Array<int> mVisPolyNeighbors;

	Convex *mNewConvex;		// temporary buffer for cut operations

	Compound *mParent;
	PxRigidActor *mPxActor;
	PxTransform  mLocalPose;
	PxConvexMesh *mPxConvexMesh;

	PxBounds3 mBounds;
	mutable float mVolume;
	mutable bool mVolumeDirty;
	PxVec3 mMaterialOffset;
	float mTexScale;
	int mModelIslandNr;

	// material
	bool mUse2dTexture;
	bool mIndestructible;
	int mMaterialId;
	int mSurfaceMaterialId;

	bool mIsFarConvex;

	mutable int mConvexRendererGroupNr;
	mutable int mConvexRendererGroupPos;
};

}
}
}

#endif
#endif