/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "physicsSimpleBodyWrapper.h"
#include "../physics/compiledCollision.h"
#include "../physics/physicsIncludes.h"

#include "../physics/physXEngine.h"

#include "../core/dataError.h"
#include "../physics/physicsSettings.h"
#include "../physics/physicsWorld.h"
#include "../core/mathConversions.h"

#include "../physics/PhysicsWrappersDefinition.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

#pragma warning( disable: 4756 )

DECLARE_PHYSICS_WRAPPER(CPhysicsSimpleWrapper,EPW_Simple,false,false)

CPhysicsSimpleWrapper::CPhysicsSimpleWrapper( CPhysicalCollision collisionTypeAndGroup, CompiledCollisionPtr& compiledCollision )
	: CPhysicsWrapperInterface( collisionTypeAndGroup )
	, m_boundingDimensions( 0.0f )
	, m_linearVelocityClamp( -1.0f )
	, m_dontUpdateBouyancy( false )
{
	GetParentProvider()->GetPhysicsWorld( m_world );

	m_rigidDynamicActor = 0;
	m_simulationType = SM_DYNAMIC;

#ifdef USE_PHYSX

	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();

	PxPhysics& physics = scene->getPhysics();

	const Matrix& initialPose = GetParentProvider()->GetLocalToWorld();
	Vector xAxis = initialPose.V[ 0 ];
	Vector yAxis = initialPose.V[ 1 ];
	Vector zAxis = initialPose.V[ 2 ];
	Vector pos = initialPose.V[ 3 ];
	m_scale.Set3( xAxis.Normalize3(), yAxis.Normalize3(), zAxis.Normalize3() );
	PxMat44 mat( TO_PX_VECTOR( xAxis ), TO_PX_VECTOR( yAxis ), TO_PX_VECTOR( zAxis ), TO_PX_VECTOR( pos ) );
	PxTransform pose( mat );
	pose.q.normalize();

	GetParentProvider()->GetTransform().CalcLocalToWorld( m_componentOffsetInverse );
	m_componentOffsetInverse.Invert();

	PxRigidDynamic* actor = nullptr;
	switch ( m_simulationType )
	{
	case SM_KINEMATIC:
		{
			actor = physics.createRigidDynamic( pose );
			actor->setRigidBodyFlag( PxRigidBodyFlag::eKINEMATIC, true );
			actor->setRigidBodyFlag( PxRigidBodyFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES, true );
			break;
		}

	case SM_DYNAMIC:
		{
			actor = physics.createRigidDynamic( pose );
			break;
		}
	}

	m_rigidDynamicActor = actor;

	if( !m_rigidDynamicActor )
	{
		IScriptable* parent;
		if( GetParent( parent ) )
		{
			GetParentProvider()->DataHalt( parent, TXT("Physical simple wrapper "), TXT( "physical actor creation failure" ) );
		}
		ASSERT( false && "physical actor creation failure " );
		m_rigidDynamicActor = nullptr;
		return;
	}

	if( !RebuildCollision( 0, compiledCollision, m_scale, m_collisionType, m_collisionGroup ) )
	{
		actor->release();
		actor = nullptr;;
		m_rigidDynamicActor = nullptr;
		return;
	}

	actor->setLinearDamping( SPhysicsSettings::m_simpleBodyLinearDamper );
	actor->setAngularDamping( SPhysicsSettings::m_simpleBodyAngularDamper );
	actor->setSleepThreshold( SPhysicsSettings::m_actorSleepThreshold );
	Uint32 minPositionIters = SPhysicsSettings::m_rigidbodyPositionIters;
	Uint32 minVelocityIters = SPhysicsSettings::m_rigidbodyVelocityIters;
	actor->setSolverIterationCounts( minPositionIters, minVelocityIters );

	Float mass = actor->getMass();
	actor->setContactReportThreshold( SPhysicsSettings::m_contactReportsThreshold * mass );

	Uint32 actorCount = GetActorsCount();
	float totalShapes = 0;
	m_floatingRatio = 0;
	for( Uint32 i = 0; i != actorCount; ++i )
	{
		Uint32 shapesCount = GetShapesCount( i );
		for( Uint32 j = 0; j != shapesCount; ++j )
		{
			physx::PxShape* shape = ( physx::PxShape* ) GetShape( j, i );
			if( !shape ) continue;

			SPhysicalFilterData filter( shape->getSimulationFilterData() );
			filter.SetFlags( filter.GetFlags() | SPhysicalFilterData::EPFDF_CountactSoundable | SPhysicalFilterData::EPFDF_DetailedConntactInfo );
			shape->setSimulationFilterData( filter.m_data );
			shape->setQueryFilterData( filter.m_data );

			PxMaterial* material;
			shape->getMaterials( &material, 1 );
			if( !material ) continue;

			SPhysicalMaterial* physicalMaterial = ( SPhysicalMaterial* ) material->userData;
			m_floatingRatio += physicalMaterial->m_floatingRatio;
			totalShapes += 1;

			if(m_floatingRatio == 0)
				SetFlag(PRBW_DisableBuoyancy, true);
		}
	}
	m_floatingRatio /= totalShapes;
	const float piDiv2 = 1.570796326794897f;
	Matrix rotMat;
	switch(compiledCollision->GetSwimmingRotationAxis())
	{
	case 0:
		rotMat = Matrix::IDENTITY;
		break;
	case 1:
		rotMat = rotMat.SetRotX33(piDiv2);
		break;
	case 2:
		rotMat = rotMat.SetRotY33(piDiv2);
		break;
	case 3:
		rotMat = rotMat.SetRotZ33(piDiv2);
		break;
	default:
		rotMat = Matrix::IDENTITY;
	}
	const auto quatFromRot = rotMat.ToQuat();
	m_swimmingRotationQuaternion = RedQuaternion(quatFromRot.X, quatFromRot.Y, quatFromRot.Z, quatFromRot.W);
	
	actor->userData = this;

	Box box = CalcLocalBounds( 0 );
	m_boundingDimensions = box.Max.DistanceTo( box.Min );

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	if( !position )
	{
		int a = 0;
	}
	position->m_x = pos.X;
	position->m_y = pos.Y;
	position->m_resultDistanceSquared = FLT_MAX;
	position->m_visibilityQueryId = 0;

	PHYSICS_STATISTICS_INC(SimpleBodiesInstanced)
#ifndef NO_EDITOR
	m_debugName = UNICODE_TO_ANSI( GetParentProvider()->GetFriendlyName().AsChar() );
	actor->setName( m_debugName.AsChar() );
#endif

	m_compiledCollision = compiledCollision;

#endif


	SetOcclusionParameters( 0, compiledCollision->GetOcclusionDiagonalLimit(), compiledCollision->GetOcclusionAttenuation() );
}

CPhysicsSimpleWrapper::CPhysicsSimpleWrapper( CPhysicsEngine::CollisionMask collisionTypeMask, CPhysicsEngine::CollisionMask collisionGroupMask, CompiledCollisionPtr& compiledCollision, const Vector* boxDimensions, Float* sphereRadius, const Matrix& localOffset  )
	: CPhysicsWrapperInterface()
	, m_boundingDimensions( 0.0f )
	, m_linearVelocityClamp( -1.0f )
	, m_dontUpdateBouyancy( false )
{
	GetParentProvider()->GetPhysicsWorld( m_world );

	m_collisionGroup = collisionGroupMask;
	m_collisionType = collisionTypeMask;

	m_rigidDynamicActor = 0;
	m_floatingRatio = 1;

	const Matrix& initialPose = GetParentProvider()->GetLocalToWorld();
	Vector xAxis = initialPose.V[ 0 ];
	Vector yAxis = initialPose.V[ 1 ];
	Vector zAxis = initialPose.V[ 2 ];
	Vector pos = initialPose.V[ 3 ];
	m_scale.Set3( xAxis.Normalize3(), yAxis.Normalize3(), zAxis.Normalize3() );
#ifdef USE_PHYSX

	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();
	PxPhysics& physics = scene->getPhysics();

	PxMat44 mat( TO_PX_VECTOR( xAxis ), TO_PX_VECTOR( yAxis ), TO_PX_VECTOR( zAxis ), TO_PX_VECTOR( pos ) );
	PxTransform pose( mat );
	pose.q.normalize();

	GetParentProvider()->GetTransform().CalcLocalToWorld( m_componentOffsetInverse );
	m_componentOffsetInverse.Invert();

	PxRigidDynamic* rigidDynamicActor = physics.createRigidDynamic( pose );
	m_rigidDynamicActor = rigidDynamicActor;

	rigidDynamicActor->userData = this;

	Int16 shapeIndex = -1;

	const CName* defaultMaterialName = &GPhysicEngine->GetMaterial()->m_name;
	if( compiledCollision )
	{
		if( !RebuildCollision( 0, compiledCollision, m_scale, m_collisionType, m_collisionGroup ) )
		{
			rigidDynamicActor->release();
			rigidDynamicActor = nullptr;
			m_rigidDynamicActor = nullptr;
			return;
		}
		else
		{
			shapeIndex = 0;
		}
	}
	else if( boxDimensions )
	{
		PxVec3 box( boxDimensions->X / 2, boxDimensions->Y / 2, boxDimensions->Z / 2 );
		PxBoxGeometry geometry( box );
		shapeIndex = CreateScaledShape( 0, &geometry, defaultMaterialName, 1, localOffset, m_scale );
	}
	else if( sphereRadius )
	{
		PxSphereGeometry geometry( *sphereRadius );
		shapeIndex = CreateScaledShape( 0, &geometry, defaultMaterialName, 1, localOffset, m_scale );
	}

	if( shapeIndex >= 0 )
	{
		physx::PxShape* shape = ( physx::PxShape* ) GetShape( shapeIndex, 0 );

		SPhysicalFilterData data( m_collisionType, m_collisionGroup );
		shape->setSimulationFilterData( data.m_data );
		shape->setQueryFilterData( data.m_data );

	}

#endif
	Box box = CalcLocalBounds( 0 );
	m_boundingDimensions = box.Max.DistanceTo( box.Min );

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	if( !position )
	{
		int a = 0;
	}
	position->m_x = pos.X;
	position->m_y = pos.Y;
	position->m_desiredDistanceSquared = SPhysicsSettings::m_simpleBodySimulationDistanceLimit + m_boundingDimensions;
	position->m_desiredDistanceSquared *= position->m_desiredDistanceSquared;
	position->m_resultDistanceSquared = FLT_MAX;
	position->m_visibilityQueryId = 0;

	m_compiledCollision = compiledCollision;

	PHYSICS_STATISTICS_INC(SimpleBodiesInstanced)
}

CPhysicsSimpleWrapper::~CPhysicsSimpleWrapper()
{
#ifdef USE_PHYSX

	if ( !m_rigidDynamicActor ) return;

	Bool hasTriggerShape = false;

	PxRigidDynamic* dynamic = ( PxRigidDynamic* ) m_rigidDynamicActor;
	Uint32 count = dynamic->getNbShapes();
	for( Uint32 i = 0; i != count; ++i )
	{
		physx::PxShape* shape = 0;
		if( !dynamic->getShapes( &shape, 1, i ) ) continue;
		if( shape->getFlags() & physx::PxShapeFlag::eTRIGGER_SHAPE )
		{
			shape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, false );
			hasTriggerShape = true;
		}
	}
	dynamic->userData = 0;
	PxScene* scene = dynamic->getScene();
	if( scene )
	{
		scene->removeActor( *dynamic );

		PHYSICS_STATISTICS_DEC(SimpleBodiesSimulated)
	}

	dynamic->release();
	m_compiledCollision.Reset();
	m_rigidDynamicActor = 0;

#endif

	PHYSICS_STATISTICS_DEC(SimpleBodiesInstanced)

}

Bool CPhysicsSimpleWrapper::MakeReadyToDestroy( TDynArray< void* >* toRemove )
{
#ifndef USE_PHYSX
	return false;
#else
	if ( !m_rigidDynamicActor ) return true;

	PxRigidDynamic* dynamic = ( PxRigidDynamic* ) m_rigidDynamicActor;

	if(	dynamic->getScene() )
	{
		toRemove->PushBack( dynamic );
		return false;
	}
#endif
	return true;
}

void CPhysicsSimpleWrapper::SetPose( const Matrix& localToWorld, Uint32 actorIndex )
{
#ifdef USE_PHYSX
	if( !m_rigidDynamicActor ) return;

#ifdef PHYSICS_NAN_CHECKS
	if( !localToWorld.IsOk() )
	{
		RED_FATAL_ASSERT( localToWorld.IsOk(), "NANS" );
		return;
	}
#endif

	SetFlag( PRBW_PoseIsDirty, false );

	Vector xAxis = localToWorld.V[ 0 ];
	Vector yAxis = localToWorld.V[ 1 ];
	Vector zAxis = localToWorld.V[ 2 ];
	const Vector& pos = localToWorld.V[ 3 ];
	m_scale.Set3( xAxis.Normalize3(), yAxis.Normalize3(), zAxis.Normalize3() );
	PxMat44 mat( TO_PX_VECTOR( xAxis ), TO_PX_VECTOR( yAxis ), TO_PX_VECTOR( zAxis ), TO_PX_VECTOR( pos ) );
	PxTransform pose( mat );

	PxRigidDynamic* dynamic = ( PxRigidDynamic* ) m_rigidDynamicActor;
	dynamic->setGlobalPose( pose, m_simulationType == SM_KINEMATIC );

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	if( !position )
	{
		int a = 0;
	}
	position->m_x = pos.X;
	position->m_y = pos.Y;

	if( dynamic->getScene() && ( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) )
	{
		dynamic->setKinematicTarget( pose );
	}

#endif
}

Uint32 CPhysicsSimpleWrapper::GetActorsCount() const
{
#ifndef USE_PHYSX
	return 0;
#else
	return m_rigidDynamicActor ? 1 : 0;
#endif
}

void* CPhysicsSimpleWrapper::GetActor( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
	return 0;
#else
	return m_rigidDynamicActor;
#endif
}

void* CPhysicsWrapperInterface::GetShape( Uint32 shapeIndex, Uint32 actorIndex ) const
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
	return shape;
#endif
}

Matrix CPhysicsSimpleWrapper::GetPose( Uint32 actorIndex ) const
{
	if( !m_rigidDynamicActor ) return Matrix::IDENTITY;

#ifndef USE_PHYSX
	return Matrix::IDENTITY;
#else
	PxMat44	mat = ( ( PxRigidDynamic* )m_rigidDynamicActor )->getGlobalPose();
	Matrix result = TO_MAT( mat );
	result.SetScale33( m_scale );
	return result;
#endif
}

void CPhysicsSimpleWrapper::SetFlag( EPhysicsRigidBodyWrapperFlags flag, Bool decision )
{
	CPhysicsWrapperInterface::SetFlag( flag, decision );
	if( decision && ( flag & PRBW_PoseIsDirty ) )
	{
		m_world->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->PushDirtyWrapper( this );
	}
}

void CPhysicsSimpleWrapper::PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd )
{
	PC_SCOPE_PHYSICS(CPhysicsSimple PreSimulation );

	if( !m_rigidDynamicActor ) return;

	if( !GetParentProvider()->HasParent() ) return;

#ifdef USE_PHYSX
	PxRigidDynamic* dynamic = ( PxRigidDynamic* ) m_rigidDynamicActor;
	PxScene* actorScene = dynamic->getScene();

	Bool isKinematic = dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC;

	const Float distanceFromViewportSquared = simulationContext->m_resultDistanceSquared;

	const Float distanceLimit = simulationContext->m_desiredDistanceSquared * simulationContext->m_desiredDistanceSquared;

	Uint32 flags = m_flags.GetValue();
	if( flags & PRBW_FlagsAreDirty )
	{
		UpdateFlags();
	}

	if( flags & PRBW_PoseIsDirty )
	{
		SetPose( GetParentProvider()->GetLocalToWorld() );
	}

	if( actorScene && distanceFromViewportSquared > distanceLimit )
	{
		toRemove->PushBack( m_rigidDynamicActor );
		return;
	}
	else if( !actorScene && distanceFromViewportSquared <= distanceLimit && m_ref.GetValue() > 0 )
	{
		if( allowAdd )
		{
			{
				PC_SCOPE_PHYSICS(CPhysSimp PreSim_AddActor );
				CPhysicsWorldPhysXImpl* world = nullptr;
				GetPhysicsWorld( world );
				PxScene* scene = world->GetPxScene();
				scene->addActor( *dynamic );
				if( isKinematic )
				{
					PxTransform trans = dynamic->getGlobalPose();
					dynamic->setKinematicTarget( trans );
				}
				else
				{
					dynamic->putToSleep();
				}
			}
			SetFlag( PRBW_UpdateEntityPose, GetParentProvider()->isRoot() );
		}
	}

	actorScene = dynamic->getScene();
	PHYSICS_STATISTICS_INC_IF(SimpleBodiesSimulated,actorScene!=0)

	if( !actorScene ) return;

	{
		if( flags & PRBW_StateIsDirty )
		{
			PC_SCOPE_PHYSICS(CPhysSimp PreSim_Dynamics );
			SetFlag( PRBW_StateIsDirty, false );

			if( m_simulationType == SM_KINEMATIC && !isKinematic )
			{
				if( actorScene )
				{
					dynamic->putToSleep();
				}
				dynamic->setRigidBodyFlag( PxRigidBodyFlag::eKINEMATIC, true );
				dynamic->setRigidBodyFlag( PxRigidBodyFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES, true );
				isKinematic = true;
				PxTransform trans = dynamic->getGlobalPose();
				dynamic->setKinematicTarget( trans );

			}
			else if( m_simulationType == SM_DYNAMIC && isKinematic )
			{
				dynamic->setRigidBodyFlag( PxRigidBodyFlag::eKINEMATIC, false );
				dynamic->setRigidBodyFlag( PxRigidBodyFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES, false );
				isKinematic = false;

				if( actorScene )
				{
					dynamic->setWakeCounter( 1.0 );
					dynamic->wakeUp();
				}
			}

			if( m_simulationType == SM_KINEMATIC )
			{
				UpdateTriggerShapes();
			}

			physx::PxShape* firstShape = ( physx::PxShape* ) GetShape( 0, 0 );
			if( firstShape )
			{
				SPhysicalFilterData filter = firstShape->getSimulationFilterData();
				SPhysicalFilterData data( m_collisionType, m_collisionGroup, filter.GetFlags() );
				if( filter.m_data.word0 != data.m_data.word0 || 
					filter.m_data.word1 != data.m_data.word1 ||
					filter.m_data.word2 != data.m_data.word2 || 
					filter.m_data.word3 != data.m_data.word3 )
				{
					Uint32 shapesCount = GetShapesCount();
					for( Uint32 j = 0; j != shapesCount; ++j )
					{
						physx::PxShape* shape = ( physx::PxShape* ) GetShape( j );
						if( !shape ) continue;

						shape->setSimulationFilterData( data.m_data );
						shape->setQueryFilterData( data.m_data );
					}
				}
			}

		}

		if( isKinematic ) return;

		if( timeDelta == 0.0f ) return;
		Bool isSleeping = dynamic->isSleeping();
		if( !isSleeping )
		{
			PxVec3 velocity = dynamic->getLinearVelocity();
			Float magSquared = velocity.x * velocity.x + velocity.y * velocity.y;
			if( velocity.z > 0.0f )	magSquared += velocity.z * velocity.z;

			const Float velocityClampSquared = m_linearVelocityClamp != -1.0f ? m_linearVelocityClamp * m_linearVelocityClamp : SPhysicsSettings::m_rigidbodyLinearVelocityClamp * SPhysicsSettings::m_rigidbodyLinearVelocityClamp;
			if( magSquared > velocityClampSquared )
			{
				const Float mag = sqrtf( magSquared );
				const Float ratio = ( m_linearVelocityClamp != -1.0f ? m_linearVelocityClamp : SPhysicsSettings::m_rigidbodyLinearVelocityClamp ) / mag;
				velocity.x *= ratio;
				velocity.y *= ratio;

				if( velocity.z > 0.0f )
					velocity.z *= ratio;

				dynamic->setLinearVelocity( velocity );
			}
		}

		if( flags & PRBW_DisableBuoyancy ) return;

		if( !m_dontUpdateBouyancy )
		{
			EVisibilityResult visibilityQueryResult = EVisibilityResult::EVR_NotTested;
			if( simulationContext->m_visibilityQueryId != 0 )
			{
				visibilityQueryResult = ( EVisibilityResult ) simulationContext->m_visibilityQueryResult;
			}			
			if( ApplyBuoyancyForce( 0, SPhysicsSettings::m_simpleBodyLinearDamper, SPhysicsSettings::m_simpleBodyAngularDamper, m_dontUpdateBouyancy, m_floatingRatio, visibilityQueryResult) )
			{
				if( simulationContext->m_visibilityQueryId == 0 )
				{
					static Uint64 currentTickMarker = 0;
					if( currentTickMarker < tickMarker || !tickMarker ) 
					{
						simulationContext->m_visibilityQueryId = GetParentProvider()->GetVisibilityQuerty();
					}
				}
				ApplyBuoyancyTorque( 0, m_swimmingRotationQuaternion, visibilityQueryResult  );
			}
		}
	}
#endif
}

#ifdef DEBUG_TRANS_MGR
#pragma optimize("",off)
#endif

void CPhysicsSimpleWrapper::PostSimulationUpdateTransform( const Matrix& transform, void* actor )
{
	if( IsKinematic() ) return;

	m_dontUpdateBouyancy = false;

	if( !GetFlag( PRBW_UpdateEntityPose ) )
	{
		Matrix result = transform;
		result.SetScale33( m_scale );
		
		if( GetParentProvider()->ForceUpdateTransformWithGlobalPose( result ) )
		{
			return;
		}
	}

	const Matrix result = m_componentOffsetInverse * transform;
	const Vector& pos = result.GetTranslationRef();
    GetParentProvider()->SetRawPlacementNoScale( result );
	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	if( !position )
	{
		int a = 0;
	}
	position->m_x = pos.X;
	position->m_y = pos.Y;
}

#ifdef DEBUG_TRANS_MGR
#pragma optimize("",on)
#endif

void CPhysicsSimpleWrapper::Release( Uint32 actorIndex )
{
	RED_ASSERT( m_ref.GetValue() > 0 )
	if( !m_ref.Decrement() )
	{
		m_world->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->PushWrapperToRemove( this );
	}
}

Int32 CPhysicsSimpleWrapper::GetIndex( const char* actorName )
{
#ifndef USE_PHYSX
	return -1;
#else
	if( !m_rigidDynamicActor || !actorName ) return 0;
	const char* name = ( ( PxRigidDynamic* )m_rigidDynamicActor )->getName();
	if ( !name ) return 0;
	return Red::System::StringCompare( name, actorName ) == 0 ? 0 : -1;
#endif
}

void CPhysicsSimpleWrapper::SetLinearVelocityClamp( Float maxLinearVelocity )
{
    if( maxLinearVelocity > 0 )
        m_linearVelocityClamp = maxLinearVelocity;
}

Bool CPhysicsSimpleWrapper::IsReady() const
{
#ifndef USE_PHYSX
	return false;
#else
	if( !m_rigidDynamicActor ) return false;
	return ( ( PxRigidDynamic* )m_rigidDynamicActor )->getScene() != 0;
#endif
}

Bool CPhysicsSimpleWrapper::SetOcclusionParameters( Uint32 actorIndex, Float diagonalLimit, Float attenuation )
{
#ifndef USE_PHYSX
	return false;
#else
	if( !m_rigidDynamicActor ) return false;

	PxBounds3 bounds = ( ( PxRigidDynamic* )m_rigidDynamicActor )->getWorldBounds();

	Float currentDiameter = bounds.getDimensions().magnitude();
	Bool isOccluding = diagonalLimit == -1 && attenuation == -1 ? false : diagonalLimit <= currentDiameter;

	Uint32 shapesCount = GetShapesCount();
	for( Uint32 j = 0; j != shapesCount; ++j )
	{
		physx::PxShape* shape = ( physx::PxShape* ) GetShape( j );
		if( !shape ) continue;

		SPhysicalFilterData filter( shape->getSimulationFilterData() );
		if( isOccluding )
		{
			filter.SetFlags( filter.GetFlags() | SPhysicalFilterData::EPFDF_SoundOccludable );
		}
		else
		{
			filter.SetFlags( filter.GetFlags() & ( 0xFFFF ^ SPhysicalFilterData::EPFDF_SoundOccludable ) );
		}
		shape->setSimulationFilterData( filter.m_data );
		shape->setQueryFilterData( filter.m_data );
	}

	m_occlusionDiameterLimit = isOccluding ? diagonalLimit : -1.0f;
	m_occlusionAttenuation = isOccluding ? attenuation : -1.0f;
	return true;
#endif
}

Bool CPhysicsSimpleWrapper::GetOcclusionParameters( Uint32 actorIndex, Float* diagonalLimit, Float* attenuation )
{
#ifndef USE_PHYSX
	return false;
#else
	if( diagonalLimit ) *diagonalLimit = m_occlusionDiameterLimit;
	if( attenuation ) *attenuation = m_occlusionAttenuation;
	return true;
#endif
}


