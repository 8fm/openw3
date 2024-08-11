/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "physicsWrapper.h"
#include "../physics/compiledCollision.h"
#include "../physics/physicsIncludes.h"

#ifdef USE_PHYSX
#include "../physics/physXEngine.h"
#include "../physics/physicsWorldPhysXImpl.h"
#endif

#include "../core/dataError.h"
#include "../physics/physicsSettings.h"
#include "../physics/physicsWorld.h"
#include "../core/mathConversions.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

#pragma warning( disable: 4756 )

CPhysicsWrapperInterface::CPhysicsWrapperInterface( CPhysicalCollision collisionTypeAndGroup )
	: m_flags( 0 )
	, m_ref( 1 )
{
	collisionTypeAndGroup.RetrieveCollisionMasks( m_collisionType, m_collisionGroup );
}

CPhysicsWrapperInterface::~CPhysicsWrapperInterface()
{
}


Bool CPhysicsWrapperInterface::GetFlag( EPhysicsRigidBodyWrapperFlags flag ) const
{
	return ( m_flags.GetValue() & flag ) > 0;
}

void CPhysicsWrapperInterface::SetFlag( EPhysicsRigidBodyWrapperFlags flag, Bool decision )
{
	if( decision )
	{
		m_flags.Or( flag );
	}
	else
	{
		m_flags.And( 0xFFFFFFFF ^ flag );
	}

	if( flag & EPRW_SHAPE_FLAGS )
	{
		m_flags.Or( PRBW_FlagsAreDirty );
	}
}

Vector CPhysicsWrapperInterface::GetPosition( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
	return Vector::ZEROS;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return Vector::ZEROS;
	PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) return Vector::ZEROS;

	PxMat44	mat = rigidActor->getGlobalPose();
	return Vector( mat.column3.x, mat.column3.y, mat.column3.z, 1.0f );
#endif
}

Vector CPhysicsWrapperInterface::GetShapePosition( Uint32 actorIndex, Uint32 shapeIndex ) const
{
#ifndef USE_PHYSX
	return Vector::ZEROS;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return Vector::ZEROS;
	PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) return Vector::ZEROS;

	physx::PxShape* shape = ( physx::PxShape* ) GetShape( shapeIndex, actorIndex );
	if( !shape ) return Vector::ZEROS;

	PxTransform trans = rigidActor->getGlobalPose() * shape->getLocalPose();

	PxMat44	mat( trans );

	return Vector( mat.column3.x, mat.column3.y, mat.column3.z, 1.0f );
#endif

}

Vector CPhysicsWrapperInterface::GetCenterOfMassPosition( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
	return Vector::ZEROS;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return Vector::ZEROS;
	PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) return Vector::ZEROS;

	PxMat44	mat = rigidActor->getGlobalPose();
	physx::PxRigidDynamic* dynamic = rigidActor->isRigidDynamic();
	if( dynamic )
	{
		mat = mat * dynamic->getCMassLocalPose();
	}
	return Vector( mat.column3.x, mat.column3.y, mat.column3.z, 1.0f );
#endif
}

Box CPhysicsWrapperInterface::GetWorldBounds( Int32 actorIndex )
{
#ifndef USE_PHYSX
	return Box();
#else
	PxBounds3 bounds;
	bounds.setEmpty();

	if( actorIndex != -1 )
	{
		physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
		if( !actor ) return Box();
		PxRigidActor* rigidActor = actor->isRigidActor();
		if( !rigidActor ) return Box();

		bounds = rigidActor->getWorldBounds();
	}
	else
	{
		Uint32 actorCount = GetActorsCount();
		for( Uint32 i = 0; i != actorCount; ++i )
		{
			physx::PxActor* actor = ( physx::PxActor* ) GetActor( i );
		
			if( !actor || !actor->getScene() ) continue;

			Uint32 shapesCount = GetShapesCount( i );
			for( Uint32 j = 0; j != shapesCount; ++j )
			{
				physx::PxShape* shape = ( physx::PxShape* ) GetShape( j, i );
				if( !shape ) continue;

				bounds.include( physx::PxShapeExt::getWorldBounds( *shape, *shape->getActor() ) );
			}
		}
	}
	return Box( TO_VECTOR( bounds.minimum ), TO_VECTOR( bounds.maximum ) );
#endif
}

Bool CPhysicsWrapperInterface::SetVelocityLinear( const Vector& linear, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	physx::PxRigidDynamic* rigidDynamic = actor->isRigidDynamic();
	if( !rigidDynamic ) return false;
	if( rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	rigidDynamic->setLinearVelocity( TO_PX_VECTOR( linear ) );
	return true;
#endif
}

Bool CPhysicsWrapperInterface::SetVelocityAngular( const Vector& angular, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	physx::PxRigidDynamic* rigidDynamic = actor->isRigidDynamic();
	if( !rigidDynamic ) return false;
	if( rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	rigidDynamic->setAngularVelocity( TO_PX_VECTOR( angular ) );
	return true;
#endif
}

Bool CPhysicsWrapperInterface::SetMaxVelocityAngular( Float maxVelocity, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	physx::PxRigidDynamic* rigidDynamic = actor->isRigidDynamic();
	if( !rigidDynamic ) return false;
	if( rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	rigidDynamic->setMaxAngularVelocity( maxVelocity );
	return true;
#endif
}

Bool CPhysicsWrapperInterface::GetVelocity( Vector& linear, Vector& angular, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	physx::PxRigidDynamic* rigidDynamic = actor->isRigidDynamic();
	if( !rigidDynamic ) return false;
	angular = TO_VECTOR( rigidDynamic->getAngularVelocity() );
	linear = TO_VECTOR( rigidDynamic->getLinearVelocity() );
	return true;
#endif
}

Bool CPhysicsWrapperInterface::GetLinearVelocityAtPos( const Vector& pos, Vector & out, Uint32 actorIndex  )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	physx::PxRigidDynamic* rigidDynamic = actor->isRigidDynamic();
	if( !rigidDynamic ) return false;
	out = TO_VECTOR( PxRigidBodyExt::getVelocityAtPos( *rigidDynamic, TO_PX_VECTOR(pos) ) );
	return true;
#endif
}

Float CPhysicsWrapperInterface::GetDampingLinear( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
    return -1.0f;
#else
    physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
    if( !actor )
        return -1.0f;

    physx::PxRigidDynamic* rigidDynamic = actor->isRigidDynamic();
    if( !rigidDynamic )
        return -1.0f;

    if( rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC )
        return -1.0f;

    return rigidDynamic->getLinearDamping();
#endif
}

Float CPhysicsWrapperInterface::GetDampingAngular( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
    return -1.0f;
#else
    physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
    if( !actor )
        return -1.0f;

    physx::PxRigidDynamic* rigidDynamic = actor->isRigidDynamic();
    if( !rigidDynamic )
        return -1.0f;

    if( rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC )
        return -1.0f;

    return rigidDynamic->getAngularDamping();
#endif
}

Bool CPhysicsWrapperInterface::SetDampingLinear( float linear, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	physx::PxRigidDynamic* rigidDynamic = actor->isRigidDynamic();
	if( !rigidDynamic ) return false;
	if( rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	rigidDynamic->setLinearDamping( linear );
	return true;
#endif
}

Bool CPhysicsWrapperInterface::SetDampingAngular( float angular, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	physx::PxRigidDynamic* rigidDynamic = actor->isRigidDynamic();
	if( !rigidDynamic ) return false;
	if( rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	rigidDynamic->setAngularDamping( angular );
	return true;
#endif
}

Uint32 CPhysicsWrapperInterface::GetShapesCount( Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return 0;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return 0;
	PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) return 0;
	return rigidActor->getNbShapes();
#endif
}

CPhysicsEngine::CollisionMask CPhysicsWrapperInterface::GetCollisionTypesBits( Uint32 actorIndex, Uint32 shapeIndex ) const
{
#ifndef USE_PHYSX
	return 0;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return 0;

	PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) return 0;

	physx::PxShape* shape = 0;
	rigidActor->getShapes( &shape, 1, shapeIndex );
	if( !shape ) return 0;

	PxFilterData filter = shape->getSimulationFilterData();
	SPhysicalFilterData& data = ( SPhysicalFilterData& ) filter;
	return data.GetCollisionType();
#endif
}

void CPhysicsWrapperInterface::SetScriptCallback( EPhysicalScriptCallbackType type, const THandle< IScriptable >& object, CName onEventName )
{
	while( m_callbacks.Size() <= ( Uint32 ) type )
	{
		m_callbacks.PushBack( SCallbackData() );
	}
	m_callbacks[ type ].m_scriptReciverObject = object;
	m_callbacks[ type ].m_scriptReciversOnEventName = onEventName;

	while( m_callbacks.Size() )
	{
		if( !m_callbacks[ m_callbacks.Size() - 1 ].isEmpty() ) break;
		m_callbacks.PopBackFast();
	}

	if( IsReady() )
	{
		SetFlag( PRBW_StateIsDirty, true );
	}
	else
	{
		UpdateTriggerShapes();
	}
}

void CPhysicsWrapperInterface::SetCodeCallback( EPhysicalCodeCallbackType type, IPhysicalCollisionTriggerCallback* callback, const THandle< IScriptable >& object )
{
	while( m_callbacks.Size() <= ( Uint32 ) type )
	{
		m_callbacks.PushBack( SCallbackData() );
	}

	m_callbacks[ type ].m_codeReceiverObject = callback;
	m_callbacks[ type ].m_parentObject = object;

	while( m_callbacks.Size() )
	{
		if( !m_callbacks[ m_callbacks.Size() - 1 ].isEmpty() ) break;
		m_callbacks.PopBackFast();
	}

	if( IsReady() )
	{
		SetFlag( PRBW_StateIsDirty, true );
	}
	else
	{
		UpdateTriggerShapes();
	}
}

void CPhysicsWrapperInterface::SwitchToKinematic( Bool decision )
{
	SetFlag( PRBW_StateIsDirty, true );
	m_simulationType = decision ? SM_KINEMATIC : SM_DYNAMIC;
}

void CPhysicsWrapperInterface::SwitchToStatic( )
{
	SetFlag( PRBW_StateIsDirty, true );
	m_simulationType = SM_STATIC;
}

Float CPhysicsWrapperInterface::GetMass( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
	return 0.0f;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return 0.0f;
	PxRigidBody* rigidBody = actor->isRigidBody();
	if( !rigidBody ) return 0.0f;
	return rigidBody->getMass();
#endif
}

Vector CPhysicsWrapperInterface::GetLocalInertiaTensor( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
    return Vector::ZEROS;
#else
    physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
    if( !actor )
        return Vector::ZEROS;

    PxRigidBody* rigidBody = actor->isRigidBody();
    if( !rigidBody )
        return Vector::ZEROS;

    return TO_VECTOR( rigidBody->getMassSpaceInertiaTensor() );
#endif
}

Bool CPhysicsWrapperInterface::ApplyImpulse( const Vector& impulse, const Vector& point, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	PxRigidBody* rigidBody = actor->isRigidBody();
	if( !rigidBody ) return false;
	if( !rigidBody->getScene() ) return false;
	if( rigidBody->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	PxRigidBodyExt::addForceAtPos( *rigidBody, TO_PX_VECTOR( impulse ), TO_PX_VECTOR( point ), PxForceMode::eIMPULSE );
	return true;
#endif
}

Bool CPhysicsWrapperInterface::ApplyForce( const Vector& force, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	PxRigidBody* rigidBody = actor->isRigidBody();
	if( !rigidBody ) return false;
	if( !rigidBody->getScene() ) return false;
	if( rigidBody->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	rigidBody->addForce( TO_PX_VECTOR( force ), PxForceMode::eFORCE );
	return true;
#endif
}

Bool CPhysicsWrapperInterface::ApplyForce( const Vector& force, const Vector& point, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	PxRigidBody* rigidBody = actor->isRigidBody();
	if( !rigidBody ) return false;
	if( !rigidBody->getScene() ) return false;
	if( rigidBody->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	PxRigidBodyExt::addForceAtPos( *rigidBody, TO_PX_VECTOR( force ), TO_PX_VECTOR( point ), PxForceMode::eFORCE );
	return true;
#endif
}

Bool CPhysicsWrapperInterface::ApplyAcceleration( const Vector& accel, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	PxRigidBody* rigidBody = actor->isRigidBody();
	if( !rigidBody ) return false;
	if( !rigidBody->getScene() ) return false;
	if( rigidBody->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	rigidBody->addForce( TO_PX_VECTOR( accel ), PxForceMode::eACCELERATION );
	return true;
#endif
}

Bool CPhysicsWrapperInterface::ApplyTorque( const Vector& torque, Uint32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	PxRigidBody* rigidBody = actor->isRigidBody();
	if( !rigidBody ) return false;
	if( !rigidBody->getScene() ) return false;
	if( rigidBody->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	rigidBody->addTorque( TO_PX_VECTOR( torque ), PxForceMode::eFORCE );
	return true;
#endif
}

Bool CPhysicsWrapperInterface::ApplyTorqueImpulse( const Vector& torque, Int32 actorIndex )
{
#ifndef USE_PHYSX
	return false;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	PxRigidBody* rigidBody = actor->isRigidBody();
	if( !rigidBody ) return false;
	if( !rigidBody->getScene() ) return false;
	if( rigidBody->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;
	rigidBody->addTorque( TO_PX_VECTOR( torque ), PxForceMode::eIMPULSE );
	return true;
#endif
}

void CPhysicsWrapperInterface::UpdateTriggerShapes()
{
#ifdef USE_PHYSX
	Bool shouldBeTrigger = false;
	for( Uint32 i = 0; i != m_callbacks.Size(); ++i )
	{
		if( m_callbacks[ i ].isEmpty() ) continue;
		if( i > EPSCT_OnTriggerFocusLost && i < EPCCT_OnTriggerFocusFound ) continue;
		shouldBeTrigger = true;
		break;
	}
	Uint32 actorCount = GetActorsCount();
	for( Uint32 i = 0; i != actorCount; ++i )
	{
		physx::PxActor* actor = ( physx::PxActor* ) GetActor( i );
		if( !actor ) continue;
		PxRigidActor* rigidActor = actor->isRigidActor();
		if( !rigidActor ) continue;
		physx::PxShape* shape = 0;
		Uint32 count = rigidActor->getNbShapes();
		for( Uint32 i = 0; i != count; ++i )
		{
			if( !rigidActor->getShapes( &shape, 1, i ) ) continue;
			Bool wasTrigger = shape->getFlags() & physx::PxShapeFlag::eTRIGGER_SHAPE;
			if( wasTrigger == shouldBeTrigger ) continue;
			if( shouldBeTrigger )
			{
				shape->setFlag( physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false );
				shape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, false );
				shape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, true );
			}
			else
			{
				shape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, false );
				shape->setFlag( physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true );
				shape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, true );
			}
		}
	}
#endif
}

void CPhysicsWrapperInterface::UpdateMass( Uint32 actorIndex, float densityScaler )
{
#ifdef USE_PHYSX
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return;
	physx::PxRigidDynamic* dynamic = actor->isRigidDynamic();
	if( !dynamic ) return;

	Uint32 shapesCount = dynamic->getNbShapes();

	TDynArray< PxReal > densities;
	densities.Reserve( shapesCount );
	for ( Uint32 i = 0; i != shapesCount; ++i )
	{
		physx::PxShape* shape;
		dynamic->getShapes( &shape, 1, i );

		densities.PushBack( 1.0f );
		PxMaterial* material;
		shape->getMaterials( &material, 1 );
		if( !material ) continue;

		SPhysicalMaterial* physicalMaterial = ( SPhysicalMaterial* ) material->userData;
		densities[ i ] = physicalMaterial->m_density * densityScaler;
	}

	if ( densities.Size() )
	{
		physx::PxRigidBodyExt::updateMassAndInertia( *dynamic, &densities[ 0 ], shapesCount );
	}
#endif
}

void CPhysicsWrapperInterface::UpdateFlags()
{
	m_flags.And( 0xFFFF ^ PRBW_FlagsAreDirty );
	Uint32 flags = m_flags.GetValue() & EPhysicsRigidBodyWrapperFlags::EPRW_SHAPE_FLAGS;

#ifdef USE_PHYSX
	const Uint32 actorCount = GetActorsCount();
	for( Uint32 i = 0; i != actorCount; ++i )
	{
		physx::PxActor* actor = ( physx::PxActor* ) GetActor( i );
		if ( actor )
		{
			Bool decison = ( flags & PRBW_DisableGravity ) != 0;
			actor->setActorFlag( physx::PxActorFlag::eDISABLE_GRAVITY, decison );
		}

		const Uint32 shapesCount = GetShapesCount( i );
		for( Uint32 j = 0; j != shapesCount; ++j )
		{
			physx::PxShape* shape = ( physx::PxShape* ) GetShape( j, i );
			if( !shape ) continue;

			SPhysicalFilterData filter( shape->getSimulationFilterData() );

			Uint16 shapeFlags = filter.GetFlags();

			shapeFlags &= 0xFFFF ^ SPhysicalFilterData::EPFDF_SHAPE_FLAGS;
			shapeFlags |= flags;
			filter.SetFlags( shapeFlags );
			shape->setSimulationFilterData( filter.m_data );
			shape->setQueryFilterData( filter.m_data );
		}
	}

#endif
}

Bool CPhysicsWrapperInterface::ApplyBuoyancyForce( Uint32 actorIndex, float baseLinearDamper, float baseAngularDamper, Bool& waterLevelToDeep, float floatingRatio, Uint32 visibilityQueryResult )
{
	PC_SCOPE_PHYSICS( CPhyWraInt_ApplyBuoyancyForce );

#ifdef USE_PHYSX
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;
	physx::PxRigidDynamic* dynamic = actor->isRigidDynamic();
	if( !dynamic ) return false;
	if( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;

	CPhysicsWorld* world = nullptr;
	GetPhysicsWorld( world );
	IPhysicsWorldParentProvider* provider = world->GetWorldParentProvider();
	if( !provider ) return false;
	if( !provider->IsWaterAvailable() ) return false;

	//PC_SCOPE_PHYSICS( CPhyWraInt_ApplyBuoyancy_GetGlobalPose );
	// getGlobalPose seems to be 3x times faster than getWorldBounds()	
	PxTransform pTransform = dynamic->getGlobalPose();

	// TODO expose this as LODing parameters globally		
	const Vector& eyePos = world->GetEyePosition();
	const Float distX = pTransform.p.x - eyePos.X;
	const Float distY = pTransform.p.y - eyePos.Y;
	const Float sqrDist2D = distX*distX + distY*distY;
	
	// default height for sinking the body
	Float halfHeight = 0.3f;
	Uint32 waterLevelApproximation = 2;	

	const PxBounds3& bounds = dynamic->getWorldBounds();
	const PxVec3 position = bounds.getCenter();
	halfHeight = ( bounds.maximum.z - bounds.minimum.z ) * 0.5f;
	if(visibilityQueryResult == EVR_Visible )
	{
		waterLevelApproximation = 0;
	}
	else if( visibilityQueryResult == EVR_PartialyVisible )
	{
		waterLevelApproximation = 1;		
	}
	//PC_SCOPE_PHYSICS( CPhyWraInt_ApplyBuoyancy_GetWaterLevel );
	Float depthHeight = 0.0f;
	const Float waterLevel = provider->GetWaterLevel( TO_VECTOR( position ), waterLevelApproximation, &depthHeight );
	
	waterLevelToDeep = false;
	if ( position.z - halfHeight > waterLevel )
	{
		//PC_SCOPE_PHYSICS( CPhyWraInt_ApplyBuoyancy_ResetDamping );
		{
			if( dynamic->getActorFlags() & physx::PxActorFlag::eDISABLE_GRAVITY )
			{
				//PC_SCOPE_PHYSICS( CPhyWraInt_ApplyBuoyancy_ResetDamping_DisableGravity );

				dynamic->wakeUp();
				dynamic->setActorFlag( physx::PxActorFlag::eDISABLE_GRAVITY, false );
			}

			if( dynamic->getLinearDamping() != baseLinearDamper )
			{
				//PC_SCOPE_PHYSICS( CPhyWraInt_ApplyBuoyancy_ResetDamping_SetLinear );

				dynamic->wakeUp();
				dynamic->setLinearDamping( baseLinearDamper );
			}

			if( dynamic->getAngularDamping() != baseAngularDamper )
			{
				//PC_SCOPE_PHYSICS( CPhyWraInt_ApplyBuoyancy_ResetDamping_SetAngular );

				dynamic->wakeUp();
				dynamic->setAngularDamping( baseAngularDamper );
			}
		}

		if( position.z - halfHeight > waterLevel + 5.0f )
		{
			waterLevelToDeep = true;
		}
	}
	else
	{
		//PC_SCOPE_PHYSICS( CPhyWraInt_ApplyBuoyancy_UnderWater );

		if( depthHeight < waterLevel && depthHeight - waterLevel > -SPhysicsSettings::m_fluidBuoyancyMinimalDepth ) return false;

		if( !( dynamic->getActorFlags() & physx::PxActorFlag::eDISABLE_GRAVITY ) && (floatingRatio > 0.5f) )
		{
			dynamic->setActorFlag( physx::PxActorFlag::eDISABLE_GRAVITY, true );
		}

		Float manualGravity = 9.81f * SPhysicsSettings::m_fluidLinearForceMultipler;
		Float linearDamper = baseLinearDamper + SPhysicsSettings::m_fluidLinearDamping;
		Float angularDamper = baseAngularDamper + SPhysicsSettings::m_fluidAngularDamping;

		if( position.z > waterLevel ) 
		{ //object falls into the water from above, we slow down the impact
			manualGravity = (floatingRatio > 0.5f) ? -9.81f : 9.81f/2.0f;
			const Float ratio = fabsf( position.z - waterLevel ) / halfHeight;

			linearDamper -= SPhysicsSettings::m_fluidLinearDamping * ratio;
			angularDamper -= SPhysicsSettings::m_fluidAngularDamping * ratio;
			manualGravity *= ratio;		
		}
		else if( position.z > waterLevel - halfHeight && floatingRatio > 0.5f )
		{
			const Float ratio = (1.0f/floatingRatio) * (fabsf( ( waterLevel - halfHeight ) - position.z ) / halfHeight);

			manualGravity = manualGravity - manualGravity * ratio;	
		}
		else
		{
			//We try to push back object to the surface if it's too deep
			manualGravity = manualGravity  * (floatingRatio)/SPhysicsSettings::m_fluidLinearForceMultipler;

		}

		{
			//PC_SCOPE_PHYSICS( CPhyWraInt_ApplyBuoyancy_UnderWater_AddForce );
			dynamic->addForce( PxVec3( 0.0f, 0.0f, manualGravity ), PxForceMode::eACCELERATION );
		}
		{
			//PC_SCOPE_PHYSICS( CPhyWraInt_ApplyBuoyancy_UnderWater_Damping );
			dynamic->setLinearDamping( linearDamper );
			dynamic->setAngularDamping( angularDamper );
		}

		return true;
	}
#endif
	return false;
}

void CPhysicsWrapperInterface::ApplyBuoyancyTorque( Uint32 actorIndex,const RedQuaternion & rotation, Uint32 visibilityQueryResult  )
{
#ifdef USE_PHYSX

	PC_SCOPE_PHYSICS(CPhyWraInt ApplyBuoyancyTorque )

	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return;
	physx::PxRigidDynamic* dynamic = actor->isRigidDynamic();
	if( !dynamic ) return;

	if( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return;

	CPhysicsWorld* world = nullptr;
	GetPhysicsWorld( world );
	IPhysicsWorldParentProvider* provider = world->GetWorldParentProvider();
	if( !provider ) return;
	if( !provider->IsWaterAvailable() ) return;

	PxMat44 mat( dynamic->getGlobalPose() * PxTransform(TO_PX_QUAT(rotation.Quat)));
	Matrix pose = TO_MAT( mat ) ;

	const PxBounds3& boundsTransf = dynamic->getWorldBounds();

	Float radiusX = boundsTransf.getDimensions().x * 0.5f;
	Float radiusY = boundsTransf.getDimensions().y * 0.5f;

	PxVec3 position = boundsTransf.getCenter(); 
	Vector posVec = TO_VECTOR(position);

	Vector left		= Vector( -radiusX, 0.0f, 0.0f, 0.0f ) + posVec;
	Vector right	= Vector( radiusX, 0.0f, 0.0f, 0.0f ) + posVec;
	Vector top		= Vector( 0.0f, -radiusY, 0.0f, 0.0f )  + posVec;
	Vector down		= Vector( 0.0f, radiusY, 0.0f, 0.0f )  + posVec;

	Vector eyePos = world->GetEyePosition();
	Float distX = position.x - eyePos.X;
	Float distY = position.y - eyePos.Y;

	Float sqrDist2D = distX*distX + distY*distY;
	Uint32 waterLevelApproximation = 2;

	if(visibilityQueryResult == EVR_Visible )
	{
		waterLevelApproximation = 0;
	}
	else if( visibilityQueryResult == EVR_PartialyVisible )
	{
		waterLevelApproximation = 1;
	}

	float levelleft		= provider->GetWaterLevel( left, waterLevelApproximation );
	float levelright	= provider->GetWaterLevel( right, waterLevelApproximation );
	float leveltop		= provider->GetWaterLevel( top, waterLevelApproximation );
	float leveldown		= provider->GetWaterLevel( down, waterLevelApproximation );

	levelleft	-= left.Z;
	levelright	-= right.Z;
	leveltop	-= top.Z;
	leveldown	-= down.Z;


	float difX = ( ( levelleft - levelright ) / radiusX ) * 0.5f;
	float difY = ( ( leveldown - leveltop ) / radiusY ) * 0.5f;

	PxVec3 torque( -( mat[ 1 ].z - difY ) * SPhysicsSettings::m_fluidAngularForceMultipler, ( mat[ 0 ].z + difX ) * SPhysicsSettings::m_fluidAngularForceMultipler, 0.0f );

	float mag = torque.magnitude();
	if( mag >= SPhysicsSettings::m_fluidAngularForceMaxClamp )
	{
		mag = SPhysicsSettings::m_fluidAngularForceMaxClamp;
	}

	torque *= mag;

	mat.setPosition( PxVec3( 0.0f, 0.0f, 0.0f ) );
	torque = mat.transform( torque );

	dynamic->addTorque( torque, PxForceMode::eACCELERATION );
#endif
}

Int16 CPhysicsWrapperInterface::CreateScaledShape( Uint32 actorIndex, void* geometry, const CName* materialsNames, Uint16 materialsCount, const Matrix& localPose, const Vector& scale )
{
#ifndef USE_PHYSX
	return -1;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return -1;

	PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) return -1;

	TDynArray< PxMaterial* > materials;
	materials.Reserve( materialsCount );

	for ( Uint32 j = 0; j != materialsCount; ++j )
	{
		PxMaterial* material = GPhysXEngine->GetMaterial( materialsNames[ j ] );
		if ( !material ) material = GPhysXEngine->GetMaterial();
		materials.PushBack( material ? material : GPhysXEngine->GetMaterial() );
	}

	// Scaling is applied in the mesh's space, so we need to translate that into the shape's space. Unfortunately,
	// we can't do it perfectly (we can't deal with shearing or anything), but we'll do our best. Works fine for
	// uniform scaling, and mostly fine for shapes oriented to 90 degrees (although capsules and spheres must retain
	// their circular shapes). Arbitrary orientations with non-uniform scaling are less than ideal.
	Matrix scaleMtx = Matrix::IDENTITY;
	scaleMtx.SetScale33( scale );

	Matrix scaledTransform = localPose * scaleMtx;

	// Just use the scaling factor from this resulting matrix. The rest of the matrix might be invalid (scaling a rotation
	// could result in some shearing, which PhysX is not happy with), so we don't use that.
	Vector finalScale = scaledTransform.GetScale33();
	// Actually, the above isn't entirely true. We can use the translation from the scaled matrix as well, so that the
	// shape is properly shifted with the scale.
	Matrix finalTransform = localPose;
	finalTransform.SetTranslation( scaledTransform.GetTranslationRef() );

	PxTransform pxPose = PxTransform( TO_PX_MAT(finalTransform) );
	// Try to fix the pose if we have a non-unit quaternion
	{
		PC_SCOPE_PHYSICS(CreateScaledShape issane )

		if ( pxPose.q.isFinite() && !pxPose.q.isSane() )
		{
			pxPose.q.normalize();
		}

		// Check if the pose is sane
		if( !pxPose.q.isSane() )
		{
#ifndef RED_FINAL_BUILD
			IScriptable* parent;
			if( GetParent( parent, actorIndex ) )
			{
				GetParentProvider()->DataHalt( parent, TXT("Physical collision"), TXT(" pose is fucked up ") );
			}
#endif
			return -1;
		}
	}


#ifndef RED_FINAL_BUILD
	 CPhysXLogger::ClearLastError();
#endif

	PxGeometry* phxGeometry = static_cast< PxGeometry* >( geometry );
	physx::PxShape* shape = 0;
	// Modify geometry based on what type of shape it is, to add in scaling factor.
	switch ( phxGeometry->getType() )
	{
	case PxGeometryType::eBOX:
		{
			PC_SCOPE_PHYSICS(CreateScaledShape box )

			PxBoxGeometry box = *( PxBoxGeometry* )phxGeometry;
			box.halfExtents = box.halfExtents.multiply( TO_PX_VECTOR( finalScale ) );
			shape = rigidActor->createShape( box, &materials[ 0 ], materialsCount, pxPose );
			break;
		}

	case PxGeometryType::eCAPSULE:
		{
			PC_SCOPE_PHYSICS(CreateScaledShape capsule )

			// Capsules must have a circular cross-section, so we'll just pick the greatest of the Y/Z factors. We can stretch
			// it lengthwise no problem.
			PxCapsuleGeometry capsule = *( PxCapsuleGeometry* )phxGeometry;
			capsule.radius *= Max( finalScale.Y, finalScale.Z );
			capsule.halfHeight *= finalScale.X;
			shape = rigidActor->createShape( capsule, &materials[ 0 ], materialsCount, pxPose );
			break;
		}

	case PxGeometryType::eSPHERE:
		{
			PC_SCOPE_PHYSICS(CreateScaledShape sphere )

			// Spheres must remain spherical, so we just pick the greatest scale factor.
			PxSphereGeometry sphere = *( PxSphereGeometry* )phxGeometry;
			sphere.radius *= Max( finalScale.X, finalScale.Y, finalScale.Z );
			shape = rigidActor->createShape( sphere, &materials[ 0 ], materialsCount, pxPose );
			break;	
		}

	case PxGeometryType::eCONVEXMESH:
		{
			PC_SCOPE_PHYSICS(CreateScaledShape convex )

			PxConvexMeshGeometry convex = *( PxConvexMeshGeometry* )phxGeometry;
			convex.scale.scale = convex.scale.scale.multiply( TO_PX_VECTOR(finalScale) );
			shape = rigidActor->createShape( convex, &materials[ 0 ], materialsCount, pxPose );
			break;
		}

	case PxGeometryType::eTRIANGLEMESH:
		{
			PC_SCOPE_PHYSICS(CreateScaledShape triangle )

			PxTriangleMeshGeometry trimesh = *( PxTriangleMeshGeometry* )phxGeometry;
			trimesh.scale.scale = trimesh.scale.scale.multiply( TO_PX_VECTOR(finalScale) );
			shape = rigidActor->createShape( trimesh, &materials[ 0 ], materialsCount, pxPose );
			break;
		}

	case PxGeometryType::eHEIGHTFIELD:
	case PxGeometryType::ePLANE:
		HALT( "Unsupported Geometry Type?" );
		break;
	}

#ifndef RED_FINAL_BUILD
	if( CPhysXLogger::GetLastErrorString().Size() && CPhysXLogger::IsLastErrorFromSameThread() )
	{
		IScriptable* parent;
		if( GetParent( parent, actorIndex ) )
		{
			GetParentProvider()->DataHalt( parent, TXT("Physical collision"), CPhysXLogger::GetLastErrorString().AsChar() );
		}
		CPhysXLogger::ClearLastError();
	}
#endif

	if( !shape ) return -1;

	Int16 shapeIndex = ( Int16 ) rigidActor->getNbShapes() - 1;
	SActorShapeIndex& actorShapeIndex = ( SActorShapeIndex& ) shape->userData;
	actorShapeIndex = SActorShapeIndex( ( Uint16 ) actorIndex, ( Uint16 ) shapeIndex );
	return shapeIndex;
#endif
}

Bool CPhysicsWrapperInterface::RebuildCollision( Uint32 actorIndex, CompiledCollisionPtr collisionShape, const Vector& scale, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup )
{
	PC_SCOPE_PHYSICS(CreateScaledShape RebuildCollision )

#ifdef USE_PHYSX
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return false;

	PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) return false;

	TDynArray< physx::PxShape* > oldShapes;
	oldShapes.Resize( rigidActor->getNbShapes() );
	rigidActor->getShapes( oldShapes.TypedData(), oldShapes.Size(), 0 );

	physx::PxRigidDynamic* dynamicRigidActor = actor->isRigidDynamic();

	TDynArray< PxReal > densities;

	Bool isSucceed = true;
	if( collisionShape )
	{
		auto & geometries = collisionShape->GetGeometries();
		for ( Uint32 i = 0; i < geometries.Size(); ++i )
		{
			SCachedGeometry& geometry = geometries[ i ];
			if( !geometry.GetGeometry() ) continue;

			PxGeometry* phxGeometry = ( PxGeometry* ) geometry.GetGeometry();
			//check if somebody haven't made dynamic physics with trimesh geometry
			if ( dynamicRigidActor && phxGeometry->getType() == PxGeometryType::eTRIANGLEMESH )
			{
				IScriptable* parent;
				if( GetParent( parent, actorIndex ) )
				{
					GetParentProvider()->DataHalt( parent, TXT("Physical dynamics"), TXT("Physical dynamic actor is created with trimesh geometry, physx doesn't support this so actor WONT be created. SOLUTION: convert geometry to some primitive or convex") );
				}
				isSucceed = false;
			}
			else if ( dynamicRigidActor && geometry.m_densityScaler == 0.0f )
			{
				IScriptable* parent;
				if( GetParent( parent, actorIndex ) )
				{
					GetParentProvider()->DataHalt( parent, TXT("Physical dynamics"), TXT("Density scaler on dynamic physic instance is set to zero which is WRONG") );
				}
				isSucceed = false;
			}
			else
			{
				Uint16 materialsCount;
				const CName* materialNames;
				if ( geometry.m_physicalMultiMaterials.Empty() )
				{
					materialsCount = 1;
					materialNames = &geometry.m_physicalSingleMaterial;
					PxMaterial* material = GPhysXEngine->GetMaterial( geometry.m_physicalSingleMaterial );
					if ( !material )
					{
						if ( geometry.m_physicalSingleMaterial != CName::NONE )
						{
							String text = TXT("Physical geometry has undefined physical material \"%s\" ");
							text += geometry.m_physicalSingleMaterial.AsString().AsChar();
							IScriptable* parent;
							if( GetParent( parent, actorIndex ) )
							{
								GetParentProvider()->DataHalt( parent, TXT("Physical collision"), text.AsChar() );
							}
						}
					}
					if( material == GPhysXEngine->GetMaterial() && dynamicRigidActor )
					{
						IScriptable* parent;
						if( GetParent( parent, actorIndex ) )
						{
							GetParentProvider()->DataHalt( parent, TXT("Physical dynamics"), TXT("Physical dynamic actor is created with default material which is dissalowed ") );
						}
					}
				}
				else
				{
					materialsCount = ( Uint16 ) geometry.m_physicalMultiMaterials.Size();
					materialNames = &geometry.m_physicalMultiMaterials[ 0 ];
					for ( Uint32 j = 0; j != materialsCount; ++j )
					{
						PxMaterial* material = GPhysXEngine->GetMaterial( geometry.m_physicalMultiMaterials[ j ] );
						if ( !material )
						{
							if ( geometry.m_physicalMultiMaterials[ j ] != CName::NONE )
							{
								String text = TXT("Physical geometry has undefined physical material \"%s\" ");
								text += geometry.m_physicalMultiMaterials[ j ].AsString().AsChar();
								IScriptable* parent;
								if( GetParent( parent, actorIndex ) )
								{
									GetParentProvider()->DataHalt( parent, TXT("Physical dynamics"), text.AsChar() );
								}
							}
						}
					}
				}
				Int16 shapeIndex = CreateScaledShape( actorIndex, geometry.GetGeometry(), materialNames, materialsCount, geometry.m_pose, scale );
				if( shapeIndex >= 0 )
				{
					physx::PxShape* shape = ( physx::PxShape* ) GetShape( shapeIndex, actorIndex );
					SPhysicalFilterData data( collisionType, collisionGroup );
					shape->setSimulationFilterData( data.m_data );
					shape->setQueryFilterData( data.m_data );
					SPhysicalMaterial* physicalMaterial = GetMaterial( actorIndex );
					Float resultDensity = physicalMaterial->m_density * geometry.m_densityScaler;
					densities.PushBack( resultDensity );
					shape->setContactOffset( SPhysicsSettings::m_contactOffset );
					shape->setRestOffset( SPhysicsSettings::m_restOffset );

				}
				else
				{
					isSucceed = false;
				}
			}
		}
	}

	// Destroy any old shapes that existed before.
	for ( Uint32 i = 0; i < oldShapes.Size(); ++i )
	{
		oldShapes[i]->release();
	}

	// update mass/density/etc.
	if ( dynamicRigidActor && densities.Size() )
	{
		physx::PxRigidBodyExt::updateMassAndInertia( *dynamicRigidActor, &densities[ 0 ], densities.Size() );
		if( dynamicRigidActor->getMass() < 0.1f )
		{
			dynamicRigidActor->setMass( 0.1f );
		}
	}

	return isSucceed;
#else
	return false;
#endif
}

Box CPhysicsWrapperInterface::CalcLocalBounds( Uint32 actorIndex )
{
	Box result;
	result.Clear();
#ifdef USE_PHYSX
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) return result;

	PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) return result;

	Uint32 shapesCount = rigidActor->getNbShapes();
	for( Uint32 i = 0; i != shapesCount; ++i )
	{
		physx::PxShape* shape = 0;
		rigidActor->getShapes( &shape, 1, i );
		PxGeometryHolder geometry = shape->getGeometry();
		switch ( geometry.getType() )
		{
		case PxGeometryType::eBOX:
			{
				PxBoxGeometry box = geometry.box();
				result.AddBox( Box( TO_VECTOR( -box.halfExtents ), TO_VECTOR( box.halfExtents ) ) );
				break;
			}

		case PxGeometryType::eCAPSULE:
			{
				PxCapsuleGeometry capsule = geometry.capsule();
				result.AddBox( Box( Vector( -capsule.radius, -capsule.radius, -capsule.radius - capsule.halfHeight ), Vector( capsule.radius, capsule.radius, capsule.radius + capsule.halfHeight ) ) );
				break;
			}
		case PxGeometryType::eSPHERE:
			{
				PxSphereGeometry sphere = geometry.sphere();
				result.AddBox( Box( Vector( -sphere.radius, -sphere.radius, -sphere.radius ), Vector( sphere.radius, sphere.radius, sphere.radius ) ) );
				break;
			}

		case PxGeometryType::eCONVEXMESH:
			{
				PxConvexMeshGeometry convex = geometry.convexMesh();
				PxBounds3 bounds = convex.convexMesh->getLocalBounds();
				bounds.scaleSafe( convex.scale.scale.magnitude() );
				result.AddBox( Box( TO_VECTOR( bounds.minimum ), TO_VECTOR( bounds.maximum ) ) );
				break;
			}

		case PxGeometryType::eTRIANGLEMESH:
			{
				PxTriangleMeshGeometry trimesh = geometry.triangleMesh();
				PxBounds3 bounds = trimesh.triangleMesh->getLocalBounds();
				bounds.scaleSafe( trimesh.scale.scale.magnitude() );
				result.AddBox( Box( TO_VECTOR( bounds.minimum ), TO_VECTOR( bounds.maximum ) ) );
				break;
			}
		}
	}
#endif
	return result;
}

Float CPhysicsWrapperInterface::GetDiagonal( Uint32 actorIndex )
{
	Box box = CalcLocalBounds( actorIndex );
	Vector size = box.CalcSize();
	return size.Mag3();
}

void CPhysicsWrapperInterface::SetMass( Float mass, Uint32 actorIndex )
{
#ifdef USE_PHYSX
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( actor == nullptr )
		return;

	PxRigidActor* rigidActor = actor->isRigidActor();
	if( rigidActor == nullptr )
		return;

	physx::PxRigidDynamic* dynamic = rigidActor->isRigidDynamic();
	if( dynamic == nullptr )
		return;

	PxTransform pose = dynamic->getCMassLocalPose();
	PxRigidBodyExt::setMassAndUpdateInertia( *dynamic, mass, &pose.p );
#endif
}

void CPhysicsWrapperInterface::OnContactModify( void* pairPtr )
{
#ifdef USE_PHYSX
	const physx::PxRigidDynamic* dynamicKinematicActor = nullptr;
	const PxRigidBody* rigidBody = nullptr;

	physx::PxContactModifyPair* const pair = ( physx::PxContactModifyPair* const ) pairPtr;

	float reverse = 1.0f;
	if( const physx::PxRigidDynamic* actor = pair->actor[ 0 ]->isRigidDynamic() )
	{
		if( actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC )
		{
			dynamicKinematicActor = actor;
			rigidBody = pair->actor[ 1 ]->isRigidBody();
			reverse *= -1.0f;
		}
	}

	if( !dynamicKinematicActor || !rigidBody )
	{
		if( const physx::PxRigidDynamic* actor = pair->actor[ 1 ]->isRigidDynamic() )
		{
			if( actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC )
			{
				dynamicKinematicActor = actor;
				rigidBody = pair->actor[ 0 ]->isRigidBody();
			}
		}
	}

	//be sure we have two dynamic actors and one of them with kinematic state
	if( !dynamicKinematicActor || !rigidBody ) return;

	PxContactSet& contact = pair->contacts;
	Uint32 contactCount = contact.size();
	for( Uint32 j = 0; j != contactCount; ++j )
	{
		const PxVec3& point = contact.getPoint( j );
		PxVec3 force = contact.getNormal( j );
		force.z = 0.0f;
		force *= SPhysicsSettings::m_rigidbodyContactMultipler;
		force /= ( float ) contactCount;
		{
			Float magnitude = force.magnitude();
			if( magnitude > SPhysicsSettings::m_rigidbodyContactClamp )
			{
				//force aplied shouldnt be to big
				force *= SPhysicsSettings::m_rigidbodyContactClamp/magnitude;
			}
		}

		//fuck current impulse from contact
		contact.ignore( j ); 

		//designers dont want to bother with actor masses....cool
		force *= rigidBody->getMass(); 

		//normal direction depends on relation of first actor to second on reverse...
		force *= reverse;
		PxRigidBodyExt::addForceAtPos( *const_cast< PxRigidBody* >( rigidBody ), force, point, PxForceMode::eIMPULSE );
	}
#endif
}

void CPhysicsWrapperInterface::SetMassPosition( const Vector& localPosition, Uint32 actorIndex )
{
#ifdef USE_PHYSX
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( actor == nullptr )
		return;

	PxRigidActor* rigidActor = actor->isRigidActor();
	if( rigidActor == nullptr )
		return;

	physx::PxRigidDynamic* dynamic = rigidActor->isRigidDynamic();
	if( dynamic == nullptr )
		return;

	PxVec3 vec = TO_PX_VECTOR( localPosition );
	PxRigidBodyExt::setMassAndUpdateInertia( *dynamic, dynamic->getMass(), &vec );
#endif
}

SPhysicalMaterial* CPhysicsWrapperInterface::GetMaterial( Int32 actorIndex, Int32 shapeIndex )
{
#ifndef USE_PHYSX
	return 0;
#else
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if( !actor ) 
	{
		return 0;
	}
	PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) 
	{
		return 0;
	}

	physx::PxShape* shape = 0;
	rigidActor->getShapes( &shape, 1, shapeIndex );

	PxMaterial* material = 0;
	if(!shape)
	{
		return 0;
	}
	shape->getMaterials( &material, 1 );

	return ( SPhysicalMaterial* ) material->userData;
#endif
}
