#include "RTdef.h"
#if RT_COMPILE
#include "ActorBase.h"

#include <PxMat44.h>
#include "PxRigidBodyExt.h"

#include "SimSceneBase.h"
#include "CompoundBase.h"

namespace physx
{
namespace fracture
{
namespace base
{

Actor::Actor(SimScene* scene):
	mScene(scene),
	mMinConvexSize(scene->mMinConvexSize),
	mDepthLimit(100),
	mDestroyIfAtDepthLimit(false)
{

}

Actor::~Actor()
{
	if(!mScene->mIsGrbScene)
	{
		mScene->getScene()->lockWrite();
	}
	clear();
	if(!mScene->mIsGrbScene)
	{
		mScene->getScene()->unlockWrite();
	}
	mScene->removeActor(this);
}

void Actor::clear()
{
	for (int i = 0; i < (int)mCompounds.size(); i++) {
		PX_DELETE(mCompounds[i]);
	}
	mCompounds.clear();
}

void Actor::addCompound(Compound *c)
{
	mCompounds.pushBack(c);
	PxRigidDynamic *a = c->getPxActor();
    if (a) {
//		a->setContactReportFlags(Px_NOTIFY_ON_TOUCH_FORCE_THRESHOLD | Px_NOTIFY_ON_START_TOUCH_FORCE_THRESHOLD);
		a->setContactReportThreshold(mScene->mFractureForceThreshold);
    }
	c->mActor = this;
	++(mScene->mSceneVersion);
}

void Actor::removeCompound(Compound *c)
{
	int num = 0;
	for (int i = 0; i < (int)mCompounds.size(); i++) {
		if (mCompounds[i] != c) {
			mCompounds[num] = mCompounds[i];
			num++;
		}
	}
	if (mScene->mPickActor == c->getPxActor())
		mScene->mPickActor = NULL;

	c->clear();
	//delCompoundList.push_back(c);
	//delete c;
	mScene->delCompoundList.pushBack(c);
	mCompounds.resize(num);
	++mScene->mSceneVersion;
}

void Actor::preSim(float dt)
{
	int num = 0;
	for (int i = 0; i < (int)mCompounds.size(); i++) {
		mCompounds[i]->step(dt);
		if (mCompounds[i]->getLifeFrames() == 0) {
			mCompounds[i]->clear();
			//delCompoundList.push_back(mCompounds[i]);
			//delete mCompounds[i];
			mScene->delCompoundList.pushBack(mCompounds[i]);
		}
		else {
			mCompounds[num] = mCompounds[i];
			num++;
		}
	}
	mCompounds.resize(num);
}

void Actor::postSim(float /*dt*/)
{
}

bool Actor::rayCast(const PxVec3 &orig, const PxVec3 &dir, float &dist, int &compoundNr, int &convexNr, PxVec3 &normal) const
{
	dist = PX_MAX_F32;
	compoundNr = -1;
	convexNr = -1;

	for (int i = 0; i < (int)mCompounds.size(); i++) {
		float d;
		int cNr;
		PxVec3 n;
		if (mCompounds[i]->rayCast(orig, dir, d, cNr, n)) {
			if (d < dist) {
				dist = d;
				compoundNr = i;
				convexNr = cNr;
				normal = n;
			}
		}
	}
	return compoundNr >= 0;
}

bool Actor::patternFracture(const PxVec3 &orig, const PxVec3 &dir, const PxMat33 patternTransform, float impactRadius, float radialImpulse, float directionalImpulse)
{
	float dist;
	float objectSize = 0.0f;
	int actorNr;
	int compoundNr;
	int convexNr;
	PxVec3 normal; 

	// do global rayCast.
	if (!mScene->rayCast(orig, dir, dist, actorNr, compoundNr, convexNr, normal))
		return false;
	if (mScene->mActors[actorNr] != this)
		return false;

	mScene->debugPoints.clear();
	shdfnd::Array<Compound*> compounds;

	mScene->profileBegin("patternFracture");
	bool OK = mCompounds[compoundNr]->patternFracture(orig + dir * dist, mMinConvexSize, 
		compounds, patternTransform, mScene->debugPoints, impactRadius, radialImpulse, normal * directionalImpulse);
	mScene->profileEnd("patternFracture");
	if (!OK)
		return false;

	if (compounds.empty())
		return false;

	if (mCompounds[compoundNr]->getPxActor() == mScene->mPickActor)
		mScene->mPickActor = NULL;

	PxBounds3 bounds;
	mCompounds[compoundNr]->getWorldBounds(bounds);
	objectSize = bounds.getDimensions().magnitude();

	//delCompoundList.push_back( mCompounds[compoundNr]);
	mScene->delCompoundList.pushBack( mCompounds[compoundNr]);
	mCompounds[compoundNr]->clear();
	//delete mCompounds[compoundNr];
	
	mCompounds[compoundNr] = compounds[0];

	for (int i = 1; i < (int)compounds.size(); i++)
		mCompounds.pushBack(compounds[i]);

	++mScene->mSceneVersion;

	
	// playShatterSound(objectSize);
	//if (fluidSim)
	//fluidSim->mistPS->seedDustParticles((PxVec3*)&debugPoints[0], debugPoints.size(), seedDustRadius, seedDustNumPerSite, dustMinLife, dustMaxLife, 0.0f, 1.0f);

	int numConvexes = 0;
	for (int i = 0; i < (int)mCompounds.size(); i++)
		numConvexes += mCompounds[i]->getConvexes().size();

	//printf("\n------- pattern fracture------\n");
	//printf("i compounds, %i convexes after fracture\n", mCompounds.size(), numConvexes); 

	return true;
}

bool Actor::patternFracture(const PxVec3 &hitLocation, const PxVec3 &normal, const int &compoundNr, const PxMat33 patternTransform,  float impactRadius, float radialImpulse, float directionalImpulse)
{
	float objectSize = 0.0f;

	mScene->debugPoints.clear();
	shdfnd::Array<Compound*> compounds;

	mScene->profileBegin("patternFracture");
	bool OK = mCompounds[compoundNr]->patternFracture(hitLocation, mMinConvexSize, 
		compounds, patternTransform, mScene->debugPoints, impactRadius, radialImpulse, normal * directionalImpulse);
	mScene->profileEnd("patternFracture");
	if (!OK)
		return false;

	if (compounds.empty())
		return false;

	if (mCompounds[compoundNr]->getPxActor() == mScene->mPickActor)
		mScene->mPickActor = NULL;

	PxBounds3 bounds;
	mCompounds[compoundNr]->getWorldBounds(bounds);
	objectSize = bounds.getDimensions().magnitude();

	//delCompoundList.push_back( mCompounds[compoundNr]);
	mScene->delCompoundList.pushBack( mCompounds[compoundNr]);
	mCompounds[compoundNr]->clear();
	//delete mCompounds[compoundNr];
	
	mCompounds[compoundNr] = compounds[0];

	for (int i = 1; i < (int)compounds.size(); i++)
		mCompounds.pushBack(compounds[i]);

	++mScene->mSceneVersion;

	
	// playShatterSound(objectSize);
	//if (fluidSim)
	//fluidSim->mistPS->seedDustParticles((PxVec3*)&debugPoints[0], debugPoints.size(), seedDustRadius, seedDustNumPerSite, dustMinLife, dustMaxLife, 0.0f, 1.0f);

	int numConvexes = 0;
	for (int i = 0; i < (int)mCompounds.size(); i++)
		numConvexes += mCompounds[i]->getConvexes().size();

	//printf("\n------- pattern fracture------\n");
	//printf("i compounds, %i convexes after fracture\n", mCompounds.size(), numConvexes); 

	return true;
}

bool Actor::findCompound(const Compound* c, int& compoundNr)
{
	for(int i = 0; i < (int)mCompounds.size(); i++)
	{
		if(mCompounds[i] == c)
		{
			compoundNr = i;
			return true;
		}
	}
	return false;
}

}
}
}
#endif