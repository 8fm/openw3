#include "RTdef.h"
#if RT_COMPILE
#ifndef COMPOUNDBASE
#define COMPOUNDBASE

#define DEMO_MODE 1
#define TECHNICAL_MODE 0

#include <PxVec3.h>
#include <PxPlane.h>
#include <PxBounds3.h>
#include <PxTransform.h>
#include <PsArray.h>
#include <PxRigidDynamic.h>
#include <PsUserAllocated.h>


namespace physx
{
namespace fracture
{
namespace base
{

class Convex;
class FracturePattern;
class Mesh;
class SimScene;
class CompoundGeometry;
class Actor;

// -----------------------------------------------------------------------------------
class Compound : public ::physx::shdfnd::UserAllocated
{
	friend class SimScene;
	friend class Actor;
protected:
	Compound(SimScene* scene, const FracturePattern *pattern, const FracturePattern *secondaryPattern = NULL, PxReal contactOffset = 0.005f, PxReal restOffset = -0.001f);
public:
	virtual ~Compound();

	virtual bool createFromConvexes(Convex** convexes, int numConvexes, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, bool copyConvexes = true);
	bool createFromConvex(Convex* convex, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, bool copyConvexes = true);
	bool createFromGeometry(const CompoundGeometry &geom, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega);
	void createFromMesh(const Mesh *mesh, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, int submeshNr = -1, const PxVec3& scale = PxVec3(1.f));
	//bool createFromXml(XMLParser *p, float scale, const PxTransform &trans, bool ignoreVisualMesh = false);

	virtual void applyShapeTemplate(PxShape* /*shape*/) {}

	bool rayCast(const PxVec3 &orig, const PxVec3 &dir, float &dist, int &convexNr, PxVec3 &normal);
	void setLifeFrames(int frames) { mLifeFrames = frames == 0 ? 1 : frames; }
	int  getLifeFrames() { return mLifeFrames; }

	bool patternFracture(const PxVec3 &pos, float minConvexSize, shdfnd::Array<Compound*> &compounds, 
		const PxMat33 &patternTransform, shdfnd::Array<PxVec3>& debugPoints, float impactRadius, float radialImpulse, const PxVec3 &directedImpulse);
	// impactRadius controls partial fracture. For impactRadius == 0.0f the fracture is global

	// event notification
	virtual void spawnParticles(shdfnd::Array<PxVec3>& /*crackNormals*/,shdfnd::Array<PxVec3>& /*debugPoints*/,int /*debugPointsStart*/, float /*radialImpulse*/) {}
	virtual void convexAdded(Convex* /*c*/) {}
	virtual void convexRemoved(Convex* /*c*/) {}

	bool decompose(FracturePattern *pattern, shdfnd::Array<Compound*> &compounds);
	void attach(const shdfnd::Array<PxBounds3> &bounds);
	void attachLocal(const shdfnd::Array<PxBounds3> &bounds);

	const shdfnd::Array<Convex*>& getConvexes() const { return mConvexes; }
	const shdfnd::Array<PxBounds3>& getAttachmentBounds() const { return mAttachmentBounds; }

	PxRigidDynamic* getPxActor() { return mPxActor; }
	void getWorldBounds(PxBounds3 &bounds) const;
	void getLocalBounds(PxBounds3 &bounds) const;
	void getRestBounds(PxBounds3 &bounds) const;

	//void setShader(Shader* shader, const ShaderMaterial &mat) { mShader = shader; mShaderMat = mat; }
	//Shader* getShader() const { return mShader; }
	//const ShaderMaterial& getShaderMat() { return mShaderMat; }

	void setKinematic(const PxVec3 &vel);
	void step(float dt);

	virtual void draw(bool /*useShader*/, bool /*debug*/ = false) {}

	virtual void clear();

	void setAdditionalImpactImpulse(float radial, float normal) { 
		mAdditionalImpactRadialImpulse = radial; mAdditionalImpactNormalImpulse = normal;
	}
	float getAdditionalImpactRadialImpulse() const { return mAdditionalImpactRadialImpulse; }
	float getAdditionalImpactNormalImpulse() const { return  mAdditionalImpactNormalImpulse; }

	virtual void copyShaders(Compound*) {}

protected:
	void createPointSamples(float area);
	
	bool isAttached();

	bool createPxActor(shdfnd::Array<PxShape*> &shapes, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega);

	virtual void addDust(shdfnd::Array<Convex*>& /*newConvexes*/,shdfnd::Array<PxVec3>& /*debugPoints*/,int& /*convexNr*/,int& /*num*/) {}

	static void appendUniformSamplesOfConvexPolygon(PxVec3* vertices, int numV, float area, shdfnd::Array<PxVec3>& samples, shdfnd::Array<PxVec3>* normals = NULL);

	virtual float getSleepingThresholdRB() {return 0.1f;}

	struct Edge {
		void init(int c0, int c1) {
			this->c0 = c0; this->c1 = c1;
			restLen = 0.0f;
			deleted = false;
		}
		int c0, c1;
		float restLen;
		bool deleted;
	};

	shdfnd::Array<Convex*> mConvexes;
	shdfnd::Array<Edge> mEdges;

	SimScene *mScene;
	Actor *mActor;
	PxRigidDynamic *mPxActor;
	const FracturePattern *mFracturePattern;
	const FracturePattern *mSecondardFracturePattern;
	PxVec3 mKinematicVel;
	shdfnd::Array<PxBounds3> mAttachmentBounds;

	//Shader *mShader;
	//ShaderMaterial mShaderMat;

	PxReal mContactOffset;
	PxReal mRestOffset;

	int mLifeFrames;

	float mAdditionalImpactNormalImpulse;
	float mAdditionalImpactRadialImpulse;

	shdfnd::Array<PxVec3> pointSamples;

	PxU32 mDepth; // fracture depth
	PxVec3 mNormal; // normal use with mSheetFracture
};

}}}

#endif
#endif