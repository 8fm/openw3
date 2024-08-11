#include "build.h"
#include "../physics/physXEngine.h"
#include "PhysicsJointedRagdollWrapper.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "../core/dataError.h"
#include "../physics/physicsSettings.h"
#include "../physics/physicsMemory.h"
#include "../physics/physXEngine.h"
#include "../physics/PhysicsWrappersDefinition.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

DECLARE_PHYSICS_WRAPPER(CPhysicsJointedRagdollWrapper,EPW_Jointed,true,false)

CPhysicsJointedRagdollWrapper::CPhysicsJointedRagdollWrapper( SPhysicsRagdollState& ragdollState, const DataBuffer& buffer, TDynArray< BoneInfo >& bones, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint32 visibiltyId  )
	: CPhysicsRagdollWrapper()
	, m_speedLimitForSleep( 0.0f )
	, m_windScaler( 1.0f )

{
	GetParentProvider()->GetPhysicsWorld( m_world );

#ifdef USE_PHYSX
	m_buffer = buffer;

	m_previousPosition = Vector::ZEROS;
	m_speedLimitForSleep = 0.0f;
	m_windAdaptationTarget = 0.0f;
	m_windAdaptationCurrent = 0.0f;
	m_windScaler = ragdollState.m_windScaler;
	m_forceWakeUp = ragdollState.m_forceWakeUpOnAttach;

	m_simulationType = SM_DYNAMIC;
	PxPhysics* physics = GPhysXEngine->GetPxPhysics();

	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();

	m_buffer = buffer;

	const physx::PxCollection* materialsCollection = GPhysXEngine->GetMaterialsCollection();

	PxCollection* collection = PxSerialization::createCollectionFromBinary( m_buffer.GetData(), *GPhysXEngine->GetSerializationRegistry(), materialsCollection );
	m_collection = collection;

	PxMaterial* fleshMaterial = GPhysXEngine->GetMaterial( CNAME( flesh ) );

	// get^normalize pose
	Matrix componentPose = GetParentProvider()->GetLocalToWorld();

	// IMPORTANT
	// we need to flip X and Y vectors because scene transform from 3ds max is rotated by 180 by Z axis.
	// So until we wont have ragdoll editor in engine and we will export it from 3ds max we need to handle correct transform 
	// by flipping those 2 vectors here.
	componentPose.V[ 0 ] = -componentPose.V[ 0 ];
	componentPose.V[ 1 ] = -componentPose.V[ 1 ];
	// END OF IMPORTANT NOTE
	componentPose.V[ 2 ].Normalize3();
	componentPose.V[ 0 ].Normalize3();
	componentPose.V[ 1 ].Normalize3();
	PxMat44 mat( TO_PX_VECTOR( componentPose.V[ 0 ] ), TO_PX_VECTOR( componentPose.V[ 1 ] ), TO_PX_VECTOR( componentPose.V[ 2 ] ), TO_PX_VECTOR( componentPose.V[ 3 ] ) );

	const PxU32 collectionObjectsCount = collection->getNbObjects();
	TDynArray<Float> floatingRatios;
	floatingRatios.Reserve(collectionObjectsCount);
	floatingRatios.Resize(collectionObjectsCount);
	for ( Uint16 i = 0; i < collectionObjectsCount; i++ )
	{
		PxBase& base = collection->getObject( i );

		if( base.is< PxD6Joint >() )
		{
			PxD6Joint* j = base.is< PxD6Joint >();

			if( ragdollState.m_disableConstrainsTwistAxis )	j->setMotion( PxD6Axis::eTWIST, PxD6Motion::eFREE );
			if( ragdollState.m_disableConstrainsSwing1Axis ) j->setMotion( PxD6Axis::eSWING1, PxD6Motion::eFREE );
			if( ragdollState.m_disableConstrainsSwing2Axis ) j->setMotion( PxD6Axis::eSWING2, PxD6Motion::eFREE );

			PxConstraint* constraint = j->getConstraint();
			constraint->setFlags( PxConstraintFlag::ePROJECTION | PxConstraintFlag::eVISUALIZATION );
			j->setProjectionLinearTolerance( SPhysicsSettings::m_ragdollProjectionLinearTolerance );


			if( j->getMotion( PxD6Axis::eSWING1 ) == PxD6Motion::eLOCKED || j->getMotion( PxD6Axis::eSWING2 ) == PxD6Motion::eLOCKED )
				j->setProjectionAngularTolerance( 3.141 );
			else
				j->setProjectionAngularTolerance( SPhysicsSettings::m_ragdollProjectionAngularTolerance );

		}
		if( base.is< PxRigidActor >() )
		{
			PxRigidActor* actor = base.is<PxRigidActor>();
			actor->userData = this;

			// compute pose
			PxTransform trans( mat );
			PxTransform pose = actor->getGlobalPose();
			trans = trans * pose;
			actor->setGlobalPose( trans );

			const Uint32 nbShapes = actor->getNbShapes();
			for( Uint16 j = 0; j != nbShapes; ++j )
			{
				physx::PxShape* shape;
				actor->getShapes( &shape, 1, j );
				SPhysicalFilterData data( collisionType, collisionGroup );
				shape->setSimulationFilterData( data.m_data );
				shape->setQueryFilterData( data.m_data );
				( ( SActorShapeIndex& )shape->userData ) = SActorShapeIndex( i, j );

				PxMaterial* material;
				shape->getMaterials( &material, 1 );
				if( !material ) continue;

				SPhysicalMaterial* physicalMaterial = ( SPhysicalMaterial* ) material->userData;
				floatingRatios[i] = physicalMaterial->m_floatingRatio;
			}

			UpdateMass( i, ragdollState.m_densityScaler );

			physx::PxRigidDynamic* dynamic = actor->isRigidDynamic();
			if( dynamic )
			{
				dynamic->setLinearDamping( SPhysicsSettings::m_ragdollJointedLinearDamper );
				dynamic->setAngularDamping( SPhysicsSettings::m_ragdollJointedAngularDamper );
				dynamic->setSleepThreshold( SPhysicsSettings::m_ragdollJointedSleepThreshold );
				const Uint32 minPositionIters = SPhysicsSettings::m_ragdollMinPositionIters;
				const Uint32 minVelocityIters = SPhysicsSettings::m_ragdollMinVelocityIters;
				dynamic->setSolverIterationCounts( minPositionIters, minVelocityIters );
				const Float mass = dynamic->getMass();
				dynamic->setContactReportThreshold( SPhysicsSettings::m_contactReportsThreshold * mass );
			}
		}
	}

	const Uint32 bonesCount = bones.Size();
	m_mappings.Reserve( bonesCount );
	for ( Uint16 b = 0; b < bonesCount; ++b )
	{
		BoneInfo& bi = bones[ b ];

		for ( PxU32 i = 0; i < collectionObjectsCount; i++ )
		{
			PxBase& base = collection->getObject( i );
			physx::PxRigidDynamic* actor = base.is< physx::PxRigidDynamic >();
			if( !actor ) continue;

			const char* name = actor->getName();
			if( !name )	continue;
			if ( bi.m_boneName != name ) continue;

			bi.m_boneName.Clear();
			const Float & ratio = floatingRatios[i];
			m_mappings.PushBack( BoneMapping( b, bi.m_parentIndex, bi.m_initLocalPose, actor, ratio ) );
			break;
		}
	}
	const Uint32 mappingCount = m_mappings.Size();
	PxAggregate* aggreate = GPhysXEngine->GetPxPhysics()->createAggregate( m_mappings.Size(), false );
	m_aggregate = aggreate;
	for ( Uint16 i = 0; i < mappingCount; ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];
		physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_actor;
		aggreate->addActor( *dynamic );
	}

	for ( Uint16 b = 0; b < bonesCount; ++b )
	{
		BoneInfo& bi = bones[b];
		if( bi.m_boneName.Empty() )	continue;
		m_mappings.PushBack( BoneMapping( b, bi.m_parentIndex, bi.m_initLocalPose ) );
	}

	for ( Uint16 i = 0; i < mappingCount; ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];
		physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_actor;
		if( !dynamic )  continue;

		PxTransform trans = dynamic->getGlobalPose();
		mapping.m_outTranslation.X = trans.p.x;
		mapping.m_outTranslation.Y = trans.p.y;
		mapping.m_outTranslation.Z = trans.p.z;
		mapping.m_outRotation.Quat.X = trans.q.x;
		mapping.m_outRotation.Quat.Y = trans.q.y;
		mapping.m_outRotation.Quat.Z = trans.q.z;
		mapping.m_outRotation.Quat.W = trans.q.w;
	}
#endif

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsJointedRagdollWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	if( !position )
	{
		int a = 0;
	}
	position->m_x = GetParentProvider()->GetLocalToWorld().GetTranslationRef().X;
	position->m_y = GetParentProvider()->GetLocalToWorld().GetTranslationRef().Y;
	position->m_desiredDistanceSquared = SPhysicsSettings::m_ragdollSimulationDynamicDistanceLimit;
	position->m_desiredDistanceSquared *= position->m_desiredDistanceSquared;
	position->m_resultDistanceSquared = FLT_MAX;
	position->m_visibilityQueryId = visibiltyId;

	PHYSICS_STATISTICS_INC(RagdollsInstanced)
}

CPhysicsJointedRagdollWrapper::~CPhysicsJointedRagdollWrapper()
{
#ifdef USE_PHYSX
	if( m_aggregate )
	{
		PxAggregate* aggregate = ( PxAggregate* ) m_aggregate;
		aggregate->release();
	}

	if ( m_collection )
	{
		PxCollection* collection = ( PxCollection* ) m_collection;

		for ( Uint16 i = 0; i < collection->getNbObjects(); i++ )
		{
			PxBase& base = collection->getObject( i );
			physx::PxShape* actor = base.is< physx::PxShape >();
			if( actor )
			{
				actor->userData = 0;
			}
		}

		for ( PxU32 i = 0; i < collection->getNbObjects(); i++ )
		{
			PxBase& base = collection->getObject( i );
			RemoveSerializable( &base );
		}

		collection->release();
	}
#endif
	PHYSICS_STATISTICS_DEC(RagdollsInstanced)
}

void CPhysicsJointedRagdollWrapper::Release( Uint32 actorIndex )
{
	if( actorIndex )
	{
#ifdef USE_PHYSX
		Uint32 mappingsCount = m_mappings.Size();
		for ( Uint16 i = 0; i < mappingsCount; i++ )
		{
			BoneMapping& mapping = m_mappings[ i ];
			PxRigidActor* rigidActor = ( PxRigidActor* ) mapping.m_actor;
			if( !rigidActor ) continue;
			if( rigidActor->getNbShapes() == 0 ) continue;
			physx::PxShape* shape = 0;
			rigidActor->getShapes( &shape, 1, 0 );
			SActorShapeIndex index = ( SActorShapeIndex& ) shape->userData; 
			if( index.m_actorIndex != ( Int16 ) actorIndex ) continue;
			mapping.m_boneIndex = -1;
			SetFlag( PRBW_StateIsDirty, true );
			break;
		}
#endif
	}
	else
	{
		RED_ASSERT( m_ref.GetValue() > 0 )
			if( !m_ref.Decrement() )
			{
				m_world->GetWrappersPool< CPhysicsJointedRagdollWrapper, SWrapperContext >()->PushWrapperToRemove( this );
			}

	}
}

Bool CPhysicsJointedRagdollWrapper::MakeReadyToDestroy( TDynArray< void* >* toRemove )
{
	Bool result = true;
#ifdef USE_PHYSX

	PxAggregate* aggregate = ( PxAggregate* ) m_aggregate;
	PxScene* scene = aggregate->getScene();
	if( scene )
	{
		scene->removeAggregate( *aggregate );
		result = false;
	}
#endif
	return result;
}

void CPhysicsJointedRagdollWrapper::PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd )
{
#ifdef USE_PHYSX
	PC_SCOPE_PHYSICS(CPhysicsJointedRagdollWrapper PreSimulation )

	if( !GetParentProvider()->HasParent() ) return;

	Uint32 flags = m_flags.GetValue();
	if( flags & PRBW_FlagsAreDirty )
	{
		UpdateFlags();
	}

	PxAggregate* aggregate = ( PxAggregate* ) m_aggregate;
	PxCollection* collection = ( PxCollection* ) m_collection;
	const Uint32 mappingsCount = m_mappings.Size();
	Bool attached = aggregate->getScene() != 0;
	const EVisibilityResult visibilityQueryResult = ( const EVisibilityResult ) simulationContext->m_visibilityQueryResult;
	const float distanceFromViewportSquared = simulationContext->m_resultDistanceSquared;
	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();
	Float distanceLimit;
	if( m_simulationType == SM_KINEMATIC )
	{
		if( visibilityQueryResult != EVisibilityResult::EVR_NotVisible && visibilityQueryResult != EVisibilityResult::EVR_PartialyVisible )
		{
			distanceLimit = SPhysicsSettings::m_ragdollSimulationKinematicDistanceLimit * SPhysicsSettings::m_ragdollSimulationKinematicDistanceLimit;
		}
		else distanceLimit = 0.0f;
	}
	else distanceLimit = SPhysicsSettings::m_ragdollSimulationDynamicDistanceLimit * SPhysicsSettings::m_ragdollSimulationDynamicDistanceLimit;

	static Uint64 currentTickMarker = 0;

	if( attached && distanceFromViewportSquared > distanceLimit )
	{
		if( currentTickMarker < tickMarker ) 
		{
			currentTickMarker = tickMarker;
			PC_SCOPE_PHYSICS(CPhysRagWra removeAggregate )
			attached = false;
			scene->removeAggregate( *aggregate );
			PHYSICS_STATISTICS_DEC(RagdollsSimulated)
		}
		else
		{
			SWrapperContext* position = m_world->GetWrappersPool< CPhysicsJointedRagdollWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
			position->m_requestProcessingFlag = true;
		}
	}
	else if( !attached && distanceFromViewportSquared <= distanceLimit && m_ref.GetValue() > 0 )
	{
		if( allowAdd && currentTickMarker < tickMarker ) 
		{
			currentTickMarker = tickMarker;
			PC_SCOPE_PHYSICS(CPhysRagWra addAggregate )
			attached = true;
			scene->addAggregate( *aggregate );

			if( m_forceWakeUp )
			{
				for ( Uint16 i = 0; i < mappingsCount; i++ )
				{
					if( m_mappings[ i ].m_boneIndex < 0 ) continue;
					PxRigidActor* actor = ( PxRigidActor* ) m_mappings[ i ].m_actor;
					if( !actor ) continue;
					physx::PxRigidDynamic* dynamic = actor->is<physx::PxRigidDynamic>();
					if( !dynamic ) continue;
					if( ( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) ) continue;
					dynamic->wakeUp();
				}
			}
			else
			{
				for ( Uint16 i = 0; i < mappingsCount; i++ )
				{
					if( m_mappings[ i ].m_boneIndex < 0 ) continue;
					PxRigidActor* actor = ( PxRigidActor* ) m_mappings[ i ].m_actor;
					if( !actor ) continue;
					physx::PxRigidDynamic* dynamic = actor->is<physx::PxRigidDynamic>();
					if( !dynamic ) continue;
					if( ( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) ) continue;
					dynamic->putToSleep();
				}
			}

			PHYSICS_STATISTICS_INC(RagdollsSimulated);
		}
	}

	PHYSICS_STATISTICS_INC_IF(RagdollsSimulated,attached)
	if( !attached ) return;

	if( flags & PRBW_StateIsDirty )
	{
		PC_SCOPE_PHYSICS(CPhysRagWra statedirty )

		SetFlag( PRBW_StateIsDirty, false );

		Bool isNonKinematic = false;
		for ( Uint16 i = 0; i < collection->getNbObjects(); i++ )
		{
			PxBase& base = collection->getObject( i );
			physx::PxRigidDynamic* dynamic = base.is<physx::PxRigidDynamic>();

			if( !dynamic ) continue;

			if( ( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) ) continue;

			isNonKinematic = true;
		}

		for ( Uint16 i = 0; i < mappingsCount; i++ )
		{
			if( m_mappings[ i ].m_boneIndex >= 0 ) continue;
			PxRigidActor* actor = ( PxRigidActor* ) m_mappings[ i ].m_actor;
			if( !actor ) continue;

			PxScene* scene = aggregate->getScene();
			if( scene )
			{
				scene->removeAggregate( *aggregate );
			}

			for( Uint32 i = aggregate->getNbActors(); i != 0; --i )
			{
				physx::PxActor* articulationActor = nullptr;
				aggregate->getActors( &articulationActor, 1, i - 1 );
				if( articulationActor == actor )
				{
					aggregate->removeActor( *articulationActor );
				}
			}
			if( scene )
			{
				scene->addAggregate( *aggregate );
			}
		}

		if( ( m_simulationType == SM_KINEMATIC && isNonKinematic ) ||
			( m_simulationType != SM_KINEMATIC && !isNonKinematic ) )
		{
			m_previousPosition = Vector::ZEROS;

			for ( Uint16 i = 0; i < collection->getNbObjects(); i++ )
			{
				PxBase& base = collection->getObject( i );
				physx::PxRigidDynamic* dynamic = base.is<physx::PxRigidDynamic>();
				if( dynamic )
				{
					if( m_simulationType == SM_KINEMATIC )
					{
						dynamic->setSolverIterationCounts( 1, 1 );
						dynamic->setRigidDynamicFlag( PxRigidBodyFlag::eKINEMATIC, true );
					}
					else
					{
						dynamic->setRigidDynamicFlag( PxRigidBodyFlag::eKINEMATIC, false );
						dynamic->wakeUp();
					}

					Uint32 shapesCount = GetShapesCount( i );
					for( Uint32 j = 0; j != shapesCount; ++j )
					{
						physx::PxShape* shape = ( physx::PxShape* ) GetShape( j, i );
						if( !shape ) continue;

						SPhysicalFilterData filter( shape->getSimulationFilterData() );
						if( m_simulationType != SM_KINEMATIC )
						{
							filter.SetFlags( ( filter.GetFlags() | SPhysicalFilterData::EPFDF_CountactSoundable ) );
						}
						else
						{
							filter.SetFlags( filter.GetFlags() & ( 0xFFFF ^ SPhysicalFilterData::EPFDF_CountactSoundable ) );
						}
						shape->setSimulationFilterData( filter.m_data );
						shape->setQueryFilterData( filter.m_data );
					}
					continue;
				}

				PxD6Joint* j = base.is< PxD6Joint >();
				if( j )
				{
					PxConstraint* constraint = j->getConstraint();
					PxConstraintFlags flags = PxConstraintFlag::eVISUALIZATION;
					if( m_simulationType != SM_KINEMATIC ) 
					{
						flags |= PxConstraintFlag::ePROJECTION;
					}
					constraint->setFlags( flags );
				}

			}
		}
	}

	if( timeDelta > 0.0f )
	{
		if( m_windAdaptationTarget > m_windAdaptationCurrent )
		{
			m_windAdaptationCurrent += timeDelta;
			if( m_windAdaptationTarget < m_windAdaptationCurrent )
			{
				m_windAdaptationCurrent = m_windAdaptationTarget;
			}
		}
		else if( m_windAdaptationTarget < m_windAdaptationCurrent )
		{
			m_windAdaptationCurrent -= timeDelta;
			if( m_windAdaptationTarget > m_windAdaptationCurrent )
			{
				m_windAdaptationCurrent = m_windAdaptationTarget;
			}
		}
		else
		{
			PC_SCOPE_PHYSICS(CPhysRagWra GetRand )
			static CStandardRand randomGenerator;
			m_windAdaptationTarget = randomGenerator.Get< Float >( 0.5f , 1.0f );
		}
	}

	if( m_simulationType == SM_KINEMATIC )
	{
		if( flags & PRBW_PoseIsDirty )
		{
			PC_SCOPE_PHYSICS(CPhysRagWra PoseIsDirty 2 )

				for ( Uint16 i = 0; i < mappingsCount; i++ )
				{
					const BoneMapping& mapping = m_mappings[ i ];
					if( mapping.m_boneIndex < 0 ) continue;
					physx::PxRigidDynamic* dynamic = ( PxRigidDynamic* ) mapping.m_actor;
					if( !dynamic ) continue;
					const RedVector4& translation( mapping.m_outTranslation );
					const RedQuaternion& rotation( mapping.m_outRotation );
					dynamic->setKinematicTarget( PxTransform( PxVec3( translation.X, translation.Y, translation.Z ), PxQuat( rotation.Quat.X, rotation.Quat.Y, rotation.Quat.Z, rotation.Quat.W ) ) );
				}

				SetFlag( PRBW_PoseIsDirty, false );
		}
		return;
	}
	else
	{
		PC_SCOPE_PHYSICS(CPhysRagWra querry )

		Uint32 minPos = 0;
		Uint32 minVel = 0;

		for ( Uint16 i = 0; i < mappingsCount; i++ )
		{
			const physx::PxRigidDynamic* dynamic = ( const physx::PxRigidDynamic* ) m_mappings[ i ].m_actor;
			if( !dynamic ) continue;
			dynamic->getSolverIterationCounts( minPos, minVel );
			break;
		}

		if ( visibilityQueryResult != EVisibilityResult::EVR_NotVisible )
		{
			if( minPos != SPhysicsSettings::m_ragdollInVieportMinPositionIters || minVel != SPhysicsSettings::m_ragdollInVieportMinVelocityIters )
			{
				for ( Uint16 i = 0; i < mappingsCount; i++ )
				{
					physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) m_mappings[ i ].m_actor;
					if( !dynamic ) continue;
					dynamic->setSolverIterationCounts( SPhysicsSettings::m_ragdollInVieportMinPositionIters, SPhysicsSettings::m_ragdollInVieportMinVelocityIters );
					dynamic->setSleepThreshold( SPhysicsSettings::m_ragdollInVieportSleepThreshold );
				}
			}
		}
		else if( minPos != SPhysicsSettings::m_ragdollMinPositionIters || minVel != SPhysicsSettings::m_ragdollMinVelocityIters )
			for ( Uint16 i = 0; i < mappingsCount; i++ )
			{
				physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) m_mappings[ i ].m_actor;
				if( !dynamic ) continue;
				dynamic->setSolverIterationCounts( SPhysicsSettings::m_ragdollMinPositionIters, SPhysicsSettings::m_ragdollMinVelocityIters );
				dynamic->setSleepThreshold( SPhysicsSettings::m_actorSleepThreshold );
			}

	}

	Bool buoyancyProcessing = false;
	if( timeDelta > 0.0f )
	{
		if( !( flags & PRBW_DisableBuoyancy ) )
		{
			Bool dontUpdateBouyancy;
			if( ApplyBuoyancyForce( SPhysicsSettings::m_ragdollJointedLinearDamper, SPhysicsSettings::m_ragdollJointedAngularDamper, dontUpdateBouyancy ) )
			{
				buoyancyProcessing = true;
			}
		}

		if( !buoyancyProcessing && visibilityQueryResult != EVisibilityResult::EVR_NotVisible && visibilityQueryResult != EVisibilityResult::EVR_PartialyVisible ) 
		{
			UpdateWindOnActors();
		}
	}

	if( buoyancyProcessing || m_speedLimitForSleep <= 0.0f ) return;

	// init
	Bool fullySleeping = true;
	Float maxActorsSpeed = 0.0f;

	const physx::PxU32 objectsCount = collection->getNbObjects();
	// check actor max speed
	for ( physx::PxU32 i = 0; i < objectsCount; i++ )
	{
		PxBase& base = collection->getObject( i );
		PxRigidActor* actor = base.is<PxRigidActor>();
		if( !actor ) continue;
		physx::PxRigidDynamic* dynamic = actor->is<physx::PxRigidDynamic>();
		if( !dynamic ) continue;
		if( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) continue;

		Float speed = dynamic->getLinearVelocity().magnitude();
		if( speed > maxActorsSpeed ) maxActorsSpeed = speed;
	}

	if ( maxActorsSpeed < m_speedLimitForSleep )
	{
		for ( physx::PxU32 i = 0; i < objectsCount; i++ )
		{
			PxBase& base = collection->getObject( i );
			PxRigidActor* actor = base.is<PxRigidActor>();
			if( !actor ) continue;
			physx::PxRigidDynamic* dynamic = actor->is<physx::PxRigidDynamic>();
			if( !dynamic ) continue;
			if( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) continue;

			if ( maxActorsSpeed < 0.01f )
			{
				dynamic->putToSleep();
			}
			else if( maxActorsSpeed < m_speedLimitForSleep )
			{
				Float linearDamper = dynamic->getLinearDamping() + timeDelta * SPhysicsSettings::m_ragdollSleepFallSpeed;
				Float angularDamper = dynamic->getAngularDamping() + timeDelta * SPhysicsSettings::m_ragdollSleepFallSpeed;
				dynamic->setLinearDamping( linearDamper );
				dynamic->setAngularDamping( angularDamper );
				fullySleeping = false;
			}
		}
	}
#endif
}

Bool CPhysicsJointedRagdollWrapper::IsReady() const
{
#ifndef USE_PHYSX
	return false;
#else
	PxCollection* collection = ( PxCollection* ) m_collection;
	if( collection->getNbObjects() == 0 ) return false;
	Uint32 collectionCount = collection->getNbObjects(); 
	for ( Uint16 i = 0; i < collectionCount; i++ )
	{
		PxBase& base = collection->getObject( i );
		PxRigidActor* actor = base.is<PxRigidActor>();
		if( !actor ) continue;
		if ( actor->getScene() != 0 ) return true;
	}
	return false;
#endif
}

void CPhysicsJointedRagdollWrapper::SyncToAnimation( const TDynArray<Matrix>& bones, const Matrix& localToWorld )
{
	PC_SCOPE( CPhysicsRagr SyncToAnimation );

#ifdef PHYSICS_NAN_CHECKS
	{
		PC_SCOPE_PHYSICS( CPhysicsRagW nans check );
		if( !localToWorld.IsOk() )
		{
			RED_FATAL_ASSERT( localToWorld.IsOk(), "NANS" );
			return;
		}

		Uint32 count = bones.Size();
		for( Uint32 i = 0; i != count; ++i )
		{
			const Matrix& mat = bones[ i ];
			if( !mat.GetTranslationRef().IsOk() )
			{
				RED_FATAL_ASSERT( mat.IsOk(), "NANS" );
				return;
			}
		}
	}
#endif


	Bool modified = false;

	if ( !GetFlag( PRBW_PoseIsDirty ) )
	{
#ifdef USE_PHYSX
		PxTransform localToWorldTransform(*reinterpret_cast<const PxMat44*>( &localToWorld ) );
		//++ TODO We should not need it
		localToWorldTransform.q.normalize();
		//---

		for ( Uint32 i = 0; i < m_mappings.Size(); ++i )
		{
			BoneMapping& mapping = m_mappings[ i ];

			const Int16 boneIndex = m_mappings[i].m_boneIndex;
			physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_actor;

			// TODO - We do not use dynamic stuff here so we don't need to have
			// condition: dynamic && ( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) && dynamic->getScene()
			// todo -> cache flag per ragdoll bone that we need to sync transform or not

			if ( boneIndex >= 0 && dynamic && ( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) && dynamic->getScene() )
			{
				const Matrix& parentBone = bones[ boneIndex ];

				//++ TODO This is too slow
				PxTransform boneTransform( *reinterpret_cast<const PxMat44*>( &parentBone ) ) ;
				//boneTransform.q.normalize();
				//--

				//++ TODO We should not need it
				PxTransform outTransform = localToWorldTransform.transform( boneTransform );
				//outTransform.q.normalize();
				//--

				mapping.m_outTranslation.X = outTransform.p.x;
				mapping.m_outTranslation.Y = outTransform.p.y;
				mapping.m_outTranslation.Z = outTransform.p.z;
				mapping.m_outRotation.Quat.X = outTransform.q.x;
				mapping.m_outRotation.Quat.Y = outTransform.q.y;
				mapping.m_outRotation.Quat.Z = outTransform.q.z;
				mapping.m_outRotation.Quat.W = outTransform.q.w;
				modified = true;
			}
		}
#endif

	}
	const Vector& pos = localToWorld.GetTranslationRef();

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsJointedRagdollWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	RED_FATAL_ASSERT( position, "SWrapperContext* position is null ptr" );

	//const Bool differ = position->m_x != pos.X || position->m_y != pos.Y;
	position->m_x = pos.X;
	position->m_y = pos.Y;

	if( modified )
	{
		SetFlag( PRBW_PoseIsDirty, true );
	}
}

Int32 CPhysicsJointedRagdollWrapper::GetIndex( const char* actorName )
{
#ifdef USE_PHYSX
	PxCollection* collection = ( PxCollection* ) m_collection;
	Uint32 count = collection->getNbObjects();
	for( Uint32 i = 0; i != count; ++i )
	{
		PxBase& base = collection->getObject( i );
		PxRigidActor* actor = base.is<PxRigidActor>();
		if( !actor ) continue;

		if( Red::System::StringCompare( actor->getName(), actorName ) == 0 )
		{
			return i;
		}
	}
#endif
	return -1;
}

SPhysicalMaterial* CPhysicsJointedRagdollWrapper::GetMaterial( Int32 actorIndex )
{
#ifndef USE_PHYSX
	return 0;
#else
	PxCollection* collection = ( PxCollection* ) m_collection;
	if( ( ( Int32 ) collection->getNbObjects() <= actorIndex ) || ( actorIndex == -1 ) )
	{
		return 0;
	}

	PxBase& base = collection->getObject( actorIndex );
	PxRigidActor* actor = base.is<PxRigidActor>();

	if( !actor ) return 0;

	physx::PxShape* shape = 0;
	actor->getShapes( &shape, 1, 0 );

	PxMaterial* material = 0;
	shape->getMaterials( &material, 1 );

	return ( SPhysicalMaterial* ) material->userData;
#endif
}

Uint32 CPhysicsJointedRagdollWrapper::GetActorsCount() const
{
#ifndef USE_PHYSX
	return 0;
#else
	if( !m_collection ) return 0;
	PxCollection* collection = ( PxCollection* ) m_collection;
	return collection->getNbObjects();	
#endif
}

void* CPhysicsJointedRagdollWrapper::GetActor( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
	return 0;
#else
	if( !m_collection ) return 0;
	PxCollection* collection = ( PxCollection* ) m_collection;
	PxBase& base = collection->getObject( actorIndex );
	return base.is<physx::PxActor>();
#endif
}

void* CPhysicsJointedRagdollWrapper::GetMappingActor( Uint32 index ) const
{
#ifndef USE_PHYSX
	return 0;
#else
	if( m_mappings.Size() <= index ) return 0;
	return m_mappings[ index ].m_actor;
#endif
}

Int16 CPhysicsJointedRagdollWrapper::GetMappingActorIndex( Uint32 index ) const
{
#ifndef USE_PHYSX
	return -1;
#else
	if( m_mappings.Size() <= index ) return -1;
	const BoneMapping& mapping = m_mappings[ index ];
	PxRigidActor* rigidActor = ( PxRigidActor* ) mapping.m_actor;
	if( !rigidActor ) return -1;
	if( rigidActor->getNbShapes() == 0 ) return -1;
	physx::PxShape* shape = 0;
	rigidActor->getShapes( &shape, 1, 0 );
	SActorShapeIndex actorShapeIndex = ( SActorShapeIndex& ) shape->userData; 
	return actorShapeIndex.m_actorIndex;
#endif
}

void CPhysicsJointedRagdollWrapper::SampleBonesModelSpace( TDynArray<Matrix>& poseInOut ) const
{
	PC_SCOPE_PHYSICS(CPhysicsJointedRagdollWrapper SampleBonesModelSpace )

		if ( !m_mappings.Empty() && m_simulationType == SM_DYNAMIC )
		{
			//++ TODO - Matrix full invert inside
			Matrix invertWorldMat = GetParentProvider()->GetLocalToWorld().FullInverted();
			//--

#ifdef USE_PHYSX
			const Uint32 numMappings = m_mappings.Size();
			for ( Uint32 i = 0; i < numMappings; ++i )
			{
				const BoneMapping& map = m_mappings[i];
				const Int16 boneIndex = map.m_boneIndex;
				if ( boneIndex >= 0 )
				{
					if ( map.m_actor )
					{
						const physx::PxRigidDynamic* dynamic = ( const physx::PxRigidDynamic* ) map.m_actor;

						if( !( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) )
						{
							//++ TODO to many conversions/mem copy
							PxMat44 mat = PxMat44( dynamic->getGlobalPose() );
#ifdef _DEBUG
							Matrix globalPartPose = TO_MAT( mat );
							Matrix modelPartPoseInverted = globalPartPose * invertWorldMat;
							poseInOut[ boneIndex ] = modelPartPoseInverted;
#else
							PxMat44 world = TO_PX_MAT( invertWorldMat ) * mat;
							poseInOut[ boneIndex ] = TO_MAT( world );
#ifdef PHYSICS_NAN_CHECKS
							if( !poseInOut[ boneIndex ].GetTranslationRef().IsOk() )
							{
								RED_FATAL_ASSERT( poseInOut[ boneIndex ].IsOk(), "NANS" );
								return;
							}
#endif
#endif
						}
					}
					else
					{
						const Int16 parentBoneIndex = map.m_parentIndex;
						if( parentBoneIndex >= 0 )
						{
#ifdef PHYSICS_NAN_CHECKS
							if( !poseInOut[ parentBoneIndex ].IsOk() )
							{
								RED_FATAL_ASSERT( poseInOut[ parentBoneIndex ].IsOk(), "NANS" );
								return;
							}
#endif
							poseInOut[ boneIndex ] = map.m_initLocalPose * poseInOut[ parentBoneIndex ];
							const Matrix& mat = poseInOut[ boneIndex ];
#ifdef PHYSICS_NAN_CHECKS
							if( !mat.GetTranslationRef().IsOk() )
							{
								RED_FATAL_ASSERT( mat.IsOk(), "NANS" );
								return;
							}
#endif
						}
						else
						{
							poseInOut[ boneIndex ] = map.m_initLocalPose;
							const Matrix& mat = poseInOut[ boneIndex ];
#ifdef PHYSICS_NAN_CHECKS
							if( !mat.GetTranslationRef().IsOk() )
							{
								RED_FATAL_ASSERT( mat.IsOk(), "NANS" );
								return;
							}
#endif
						}
					}
				}
			}

			//++ TODO - HACK
			const PxRigidActor* actor = ( const PxRigidActor* ) m_mappings[ 0 ].m_actor;
			if( actor )
			{
				//++ TODO - this is too slow - we need to have only position. Too many mem copy.
				PxTransform trans = actor->getGlobalPose();
				PxMat44 pose = trans;
				Matrix result = TO_MAT( pose );
				const_cast< CPhysicsJointedRagdollWrapper* >( this )->m_previousPosition = result.GetTranslation();
				//--
			}
			//--
#endif
		}
}

Bool CPhysicsJointedRagdollWrapper::GetCurrentPositionAndDeltaPosition( Vector & outCurrentPosition, Vector & outDeltaPosition ) const
{
#ifndef USE_PHYSX
	return false;
#else
	if( m_mappings.Empty() ) return false;

	if( !m_mappings[ 0 ].m_actor ) return false;

	physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) m_mappings[ 0 ].m_actor;

	if( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) return false;

	PxTransform trans = dynamic->getGlobalPose();
	PxMat44 pose = trans;
	Matrix result = TO_MAT( pose );

	outCurrentPosition = result.GetTranslation();
	outDeltaPosition = m_previousPosition == Vector::ZEROS? Vector::ZERO_3D_POINT : outCurrentPosition - m_previousPosition;
	return true;
#endif
};

//////////////////////////////////////////////////////////////////////////

Int32 CPhysicsJointedRagdollWrapper::GetBoneIndex( Int32 actorIndex ) const
{
#ifdef USE_PHYSX
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if ( !actor ) return -1;
	for ( Uint32 i = 0; i < m_mappings.Size(); ++i )
	{
		if ( m_mappings[ i ].m_actor == actor )
		{
			return m_mappings[ i ].m_boneIndex;
		}
	}
#endif
	return -1;
}

//////////////////////////////////////////////////////////////////////////

Bool CPhysicsJointedRagdollWrapper::DisableRagdollParts( const TDynArray< Uint32 > & boneIndices )
{
	Bool res = false;
	const Uint32 indicesSize = boneIndices.Size();
	const Uint32 boneMappingsSize = m_mappings.Size();
	for ( Uint32 i = 0; i < indicesSize; i++ )
	{
		const Int16 boneIndex = static_cast< Int16 >( boneIndices[ i ] );
		Int32 mappingIndex = -1;
		for ( Uint32 j = 0; j < boneMappingsSize; j++ )
		{
			if ( m_mappings[ j ].m_boneIndex == boneIndex )
			{
				mappingIndex = static_cast< Int32 >( j );
				break;
			}
		}
		if ( mappingIndex != -1 )
		{
			Int16 actorIndex = GetMappingActorIndex( mappingIndex );
			if ( actorIndex > -1 )
			{
				res = true;
				Release( actorIndex );
			}
		}
	}
	return res;
}

void CPhysicsJointedRagdollWrapper::UpdateWindOnActors()
{
	if( m_windScaler == 0 ) return;

	PC_SCOPE_PHYSICS(CPhyWraInt UpdateWindOnActors )

#ifdef USE_PHYSX
	Uint32 mappingsCount = m_mappings.Size();
	if( !mappingsCount ) return;
	IPhysicsWorldParentProvider* provider = m_world->GetWorldParentProvider();
	Uint32 elementsCount = 0;
	for ( Uint16 i = 0; i < mappingsCount; ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];
		if( mapping.m_boneIndex < 0 ) continue;
		physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_actor;
		if( !dynamic ) continue;
		if( !dynamic->getScene() ) continue;
		if( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) continue;
		PxVec3 pos = dynamic->getGlobalPose().p;
		mapping.m_buffer0 = pos.x;
		mapping.m_buffer1 = pos.y;
		elementsCount = i + 1;
	}
	BoneMapping& mapping = m_mappings[ 0 ];
	provider->GetWindAtPoint( elementsCount, &mapping.m_buffer0, &mapping.m_buffer2, sizeof( BoneMapping ) );

	for ( Uint16 i = 0; i < elementsCount; ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];
		PxRigidDynamic* actor = ( PxRigidDynamic* ) mapping.m_actor;
		if( !actor || !actor->getScene() ) continue;
		if( actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) continue;

		if( ( mapping.m_buffer2 * mapping.m_buffer2 + mapping.m_buffer3 * mapping.m_buffer3 ) == 0.0f ) continue;

		PxVec3 wind( mapping.m_buffer2, mapping.m_buffer3, 0.0f );
		wind *= SPhysicsSettings::m_ragdollGlobalWindScaler;
		wind *= m_windScaler;
		wind *= m_windAdaptationCurrent;
		actor->addForce( wind, PxForceMode::eACCELERATION );
	}
#endif
}

Bool CPhysicsJointedRagdollWrapper::ApplyBuoyancyForce( float baseLinearDamper, float baseAngularDamper, Bool& waterLevelToDeep )
{
	PC_SCOPE_PHYSICS(CPhyWraInt ApplyBuoyancyForce )

	IPhysicsWorldParentProvider* provider = m_world->GetWorldParentProvider();
	if( !provider ) return false;
	if( !provider->IsWaterAvailable() ) return false;

#ifdef USE_PHYSX

	Uint32 mappingCount = m_mappings.Size();
	if( !mappingCount ) return false;

	PxVec3 position = PxVec3(0.0f, 0.0f, 0.0f);

	Vector lowestZ = Vector::ZERO_3D_POINT;
	lowestZ.Z = FLT_MAX;
	Uint32 elementsToCount = 0;
	{
		for ( Uint32 i = 0; i < m_mappings.Size(); ++i )
		{
			BoneMapping& mapping = m_mappings[ i ];
			physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_actor;
			if( !dynamic ) continue;
			if( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) continue;

			///This is to accomodate for the rare situation when we can be thrown out of water quite rapidly
			//by a player and freeze in the air due to gravity being disabled and not turned on again
			if( ( dynamic->getActorFlags() & physx::PxActorFlag::eDISABLE_GRAVITY ) )
			{
				dynamic->setActorFlag( physx::PxActorFlag::eDISABLE_GRAVITY, false );
			}

			elementsToCount = i;

			PxVec3 vec = dynamic->getGlobalPose().p;
			if( vec.z < lowestZ.Z )
			{
				lowestZ = TO_VECTOR( vec );
				position = vec;
			}
		}
	}
	if( lowestZ.Z == FLT_MAX ) return false;

	// TODO expose this as LODing parameter
	// currently set to 20m
	Vector eyePos = m_world->GetEyePosition();
	Float distX = position.x - eyePos.X;
	Float distY = position.y - eyePos.Y;

	Float sqrDist2D = distX*distX + distY*distY;
	Uint32 waterLevelApproximation = 3;	

	if( sqrDist2D < 400.0f ) waterLevelApproximation = 0;
	else if( sqrDist2D < 1600.0f ) waterLevelApproximation = 1;
	else if( sqrDist2D < 3600.0f ) waterLevelApproximation = 2;	
	// TODO expose this as LODing parameter

	const Float waterLevel = provider->GetWaterLevel( lowestZ, waterLevelApproximation, nullptr );

	waterLevelToDeep = false;
	if( waterLevel < lowestZ.Z )
	{
		if( waterLevel - 5.0f < lowestZ.Z  )
		{
			waterLevelToDeep = true;
		}
		return false;
	}
	Float forceMultiplier = 1;
	for ( Uint32 i = 0; i < m_mappings.Size(); ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];
		physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_actor;

		if(mapping.m_floatingRatio < 0.5f) forceMultiplier += 1.0f;

		if( !dynamic ) continue;
		if( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) continue;

		elementsToCount = i + 1;
		PxBounds3 bounds;
		{
			PC_SCOPE_PHYSICS(CPhyWraInt ApplyBuoyancyForce bounds )
				bounds = dynamic->getWorldBounds();
		}

		PxVec3 center = bounds.getCenter();

		float halfHeight = ( bounds.maximum.z - bounds.minimum.z ) / 2;

		mapping.m_buffer0 = center.x;
		mapping.m_buffer1 = center.y;
		mapping.m_buffer2 = center.z;
		mapping.m_halfBoundsHeight = halfHeight;
	}

	BoneMapping* mapping = &m_mappings[ 0 ];
	if( !provider->GetWaterLevelBurst( elementsToCount, &mapping->m_buffer0, &mapping->m_buffer3, &mapping->m_depthHeight, sizeof( BoneMapping ), Vector(position.x, position.y, position.z), true ) )
	{
		return false;
	}
	for( Uint32 i = 0; i != elementsToCount; ++i )
	{
		BoneMapping* mapping = &m_mappings[ i ];

		physx::PxRigidDynamic* actor = ( physx::PxRigidDynamic* ) mapping->m_actor;
		if( !actor ) continue;
		if( actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) continue;
		if( mapping->m_boneIndex < 0 ) continue;

		if( mapping->m_buffer2 - mapping->m_halfBoundsHeight > mapping->m_buffer3 )
		{
			if( actor->getActorFlags() & physx::PxActorFlag::eDISABLE_GRAVITY )
			{
				actor->wakeUp();
				actor->setActorFlag( physx::PxActorFlag::eDISABLE_GRAVITY, false );
			}

			if( actor->getLinearDamping() != baseLinearDamper )
			{
				actor->wakeUp();
				actor->setLinearDamping( baseLinearDamper );
			}

			if( actor->getAngularDamping() != baseAngularDamper )
			{
				actor->wakeUp();
				actor->setAngularDamping( baseAngularDamper );
			}

		}
		else
		{
			if( mapping->m_depthHeight < mapping->m_buffer3 && mapping->m_depthHeight - mapping->m_buffer3 > -SPhysicsSettings::m_fluidBuoyancyMinimalDepth ) continue;

			float linearDamper = baseLinearDamper + SPhysicsSettings::m_fluidLinearDamping;
			float angularDamper = baseAngularDamper + SPhysicsSettings::m_fluidAngularDamping;

			if( !( actor->getActorFlags() & physx::PxActorFlag::eDISABLE_GRAVITY ) )
			{
				actor->setActorFlag( physx::PxActorFlag::eDISABLE_GRAVITY, true );
			}

			float manualGravity = 9.81f * SPhysicsSettings::m_fluidLinearForceMultipler;

			if( mapping->m_buffer2 > mapping->m_buffer3 )
			{
				manualGravity = -9.81f;
				float ratio = abs( mapping->m_buffer2 - mapping->m_buffer3 ) / mapping->m_halfBoundsHeight;

				linearDamper -= SPhysicsSettings::m_fluidLinearDamping * ratio;
				angularDamper -= SPhysicsSettings::m_fluidAngularDamping * ratio;
				manualGravity *= ratio;
			}
			else if( mapping->m_buffer2 > mapping->m_buffer3 - mapping->m_halfBoundsHeight )
			{
				float ratio = abs( ( mapping->m_buffer3 - mapping->m_halfBoundsHeight ) - mapping->m_buffer2 ) / mapping->m_halfBoundsHeight;

				manualGravity = manualGravity - manualGravity * ratio;
			}

			if(mapping->m_floatingRatio > 0.5f)
				actor->addForce( PxVec3( 0.0f, 0.0f, manualGravity * ( forceMultiplier * mapping->m_floatingRatio) ), PxForceMode::eACCELERATION );
			else
			{
				Float force =  manualGravity * (0.5f -  mapping->m_floatingRatio) ;
				force = force > 0 ? -force : force;
				actor->addForce( PxVec3( 0.0f, 0.0f, force), PxForceMode::eACCELERATION );
			}

			actor->setLinearDamping( linearDamper );
			actor->setAngularDamping( angularDamper );

		}
	}
#endif
	return true;
}

Box CPhysicsJointedRagdollWrapper::GetWorldBounds()
{
#ifndef USE_PHYSX
	return Box();
#else
	PxBounds3 bounds;
	bounds.setEmpty();

	for ( Uint32 i = 0; i < m_mappings.Size(); ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];

		physx::PxActor* actor = ( physx::PxActor* ) mapping.m_actor;

		if( !actor || !actor->getScene() ) continue;

		PxRigidBody* body = actor->isRigidBody();
		if( !body ) continue;

		Uint32 shapesCount = body->getNbShapes();
		for( Uint32 j = 0; j != shapesCount; ++j )
		{
			physx::PxShape* shape = nullptr;
			body->getShapes( & shape, 1, j );
			if( !shape ) continue;

			bounds.include( physx::PxShapeExt::getWorldBounds( *shape, *shape->getActor() ) );
		}
	}
	return Box( TO_VECTOR( bounds.minimum ), TO_VECTOR( bounds.maximum ) );
#endif
}
