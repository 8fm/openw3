#include "build.h"
#include "cameraFadeoutHelper.h"

#include "../../common/physics/physicsWrapper.h"
#include "../../common/physics/physicsWorldPhysXImpl.h"

#ifdef USE_PHYSX
//////////////////////////////////////////////////////////////////////////////////////
/// Callback for traces
class CCameraFadeOutTraceCallback : public ICustomTraceCallback
{
private:
	ICameraFadeOutOp* m_op;

public:
	CCameraFadeOutTraceCallback( ICameraFadeOutOp* fadeOutOp ) : m_op( fadeOutOp ) {}

	virtual physx::PxQueryHitType::Enum preFilter( const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags )
	{
		const physx::PxFilterData& shapeFilterData = shape->getQueryFilterData();
		Uint64 type = ( ( Uint64& ) shapeFilterData.word0 ) & 0x0000FFFFFFFFFFFF;
		Uint64 include = ( ( Uint64& ) filterData.word0 ) & 0x0000FFFFFFFFFFFF;
		Uint64 exclude = ( Uint64& ) filterData.word2;

		if( exclude )
		{
			if( include & type && !( exclude & type) )
			{
				SActorShapeIndex& actorIndex = ( SActorShapeIndex& ) shape->userData;
				CPhysicsWrapperInterface* physIntf = static_cast< CPhysicsWrapperInterface* > ( shape->getActor()->userData );
				(*m_op)( physIntf, &actorIndex );
			}
		}
		else if( include & type )
		{
			SActorShapeIndex& actorIndex = ( SActorShapeIndex& ) shape->userData;
			CPhysicsWrapperInterface* physIntf = static_cast< CPhysicsWrapperInterface* > ( shape->getActor()->userData );
			(*m_op)(physIntf, &actorIndex);
		}

		return physx::PxSceneQueryHitType::eNONE;
	}

	virtual physx::PxQueryHitType::Enum postFilter( const physx::PxFilterData& filterData, const physx::PxQueryHit& hit )
	{
		return physx::PxSceneQueryHitType::eNONE;
	}
};
#endif

//////////////////////////////////////////////////////////////////////////////////////
/// Traces
Bool CCustomCameraSweepHelper::SphereSweep( const Vector& from, const Vector& to, Float radius, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, CPhysicsWorld *physicsWorld, ICameraFadeOutOp* fadeOutOp )
{
#ifdef USE_PHYSX
	CCameraFadeOutTraceCallback callback( fadeOutOp );
	const ETraceReturnValue results = physicsWorld->SphereSweepTestWithCustomCallbackNoResults( from, to, radius, include, exclude, &callback ); 
	return results == TRV_Hit;
#else
	return false;
#endif
}

Bool CCustomCameraSweepHelper::BoxOverlap( const Vector& position, const Vector3& halfExtents, const Matrix& orientation, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, CPhysicsWorld * physicsWorld, ICameraFadeOutOp* fadeOutOp )
{
#ifdef USE_PHYSX
	CCameraFadeOutTraceCallback callback( fadeOutOp );
	const ETraceReturnValue results = physicsWorld->BoxOverlapWithWithCustomCallbackNoResults( position, halfExtents, orientation, include, exclude, &callback ); 
	return results == TRV_Hit;
#else
	return false;
#endif
}

