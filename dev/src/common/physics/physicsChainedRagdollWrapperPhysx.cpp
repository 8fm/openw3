#include "build.h"
#include "../physics/physicsIncludes.h"
#include "../physics/physXEngine.h"
#include "PhysicsChainedRagdollWrapper.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "../core/dataError.h"
#include "../physics/physicsSettings.h"
#include "../physics/physicsMemory.h"
#include "../physics/physicsSettings.h"
#include "../physics/PhysicsWrappersDefinition.h"

#ifdef USE_PHYSX
using namespace physx;
#endif

DECLARE_PHYSICS_WRAPPER(CPhysicsChainedRagdollWrapper,EPW_Chained,true,false)

#ifdef EP2_SHEEP_RAGDOLL_ANGULAR_DAMPING_HACK
CPhysicsChainedRagdollWrapper::CPhysicsChainedRagdollWrapper( SPhysicsRagdollState& ragdollState, const DataBuffer& buffer, TDynArray< BoneInfo >& bones, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint32 visibiltyId, Float angularDamping )
#else
CPhysicsChainedRagdollWrapper::CPhysicsChainedRagdollWrapper( SPhysicsRagdollState& ragdollState, const DataBuffer& buffer, TDynArray< BoneInfo >& bones, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, Uint32 visibiltyId )
#endif
	: CPhysicsRagdollWrapper()
	, m_articulation( nullptr )
#ifdef EP2_SHEEP_RAGDOLL_ANGULAR_DAMPING_HACK
	, m_angularDamping( angularDamping )
#endif

{
#ifdef USE_PHYSX

	using namespace physx;

	GetParentProvider()->GetPhysicsWorld( m_world );

	m_simulationType = SM_KINEMATIC;
	PxPhysics* physics = GPhysXEngine->GetPxPhysics();

	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();

	m_buffer = buffer;

	const physx::PxCollection* materialsCollection = GPhysXEngine->GetMaterialsCollection();

	PxCollection* collection = PxSerialization::createCollectionFromBinary( m_buffer.GetData(), *GPhysXEngine->GetSerializationRegistry(), materialsCollection );
	m_collection = collection;

	if( !collection->getNbObjects() )
	{
		return;
	}

	PxMaterial* fleshMaterial = GPhysXEngine->GetMaterial( CNAME( flesh ) );

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
			if( physx::PxRigidDynamic* dynamic = actor->isRigidDynamic() )
			{
				dynamic->setSolverIterationCounts( 1, 1 );
				dynamic->setRigidDynamicFlag( PxRigidBodyFlag::eKINEMATIC, true );
			}
			
			const Uint32 nbShapes = actor->getNbShapes();
			for( Uint16 j = 0; j != nbShapes; ++j )
			{
				physx::PxShape* shape;
				actor->getShapes( &shape, 1, j );
				SPhysicalFilterData data( collisionType, collisionGroup );
				shape->setSimulationFilterData( data.m_data );
				shape->setQueryFilterData( data.m_data );

				PxMaterial* material;
				shape->getMaterials( &material, 1 );
				if( !material ) continue;

				SPhysicalMaterial* physicalMaterial = ( SPhysicalMaterial* ) material->userData;
				floatingRatios[i] = physicalMaterial->m_floatingRatio;
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
			m_transformsBuffer.PushBack( BoneTransformBuffer( b, actor ) );
			break;
		}
	}
	const Uint32 mappingCount = m_mappings.Size();
	PxAggregate* aggregate = GPhysXEngine->GetPxPhysics()->createAggregate( m_mappings.Size(), false );
	m_aggregate = aggregate;
	for ( Uint16 i = 0; i < mappingCount; ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];
		physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_rigidDynamic;

		UpdateMass( i, ragdollState.m_densityScaler );
		Float mass = dynamic->getMass();
		dynamic->setContactReportThreshold( SPhysicsSettings::m_contactReportsThreshold * mass );

		const Uint32 nbShapes = dynamic->getNbShapes();
		for( Uint16 j = 0; j != nbShapes; ++j )
		{
			physx::PxShape* shape = nullptr;
			dynamic->getShapes( &shape, 1, j );
			( ( SActorShapeIndex& )shape->userData ) = SActorShapeIndex( i, j );
		}
		aggregate->addActor( *dynamic );
	}

	for ( Uint16 b = 0; b < bonesCount; ++b )
	{
		BoneInfo& bi = bones[b];
		if( bi.m_boneName.Empty() )	continue;
		m_mappings.PushBack( BoneMapping( b, bi.m_parentIndex, bi.m_initLocalPose ) );
		m_transformsBuffer.PushBack( BoneTransformBuffer( b ) );
	}
	PxArticulation* articulation = physics->createArticulation();
	m_articulation = articulation;
	Int32 resourceProjection = ragdollState.m_projectionIterations;
	if( resourceProjection > -1 )
	{
		articulation->setMaxProjectionIterations( resourceProjection );
	}

	for ( Uint16 i = 0; i < mappingCount; ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];
		physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_rigidDynamic;
		if( !dynamic )  continue;

		BoneTransformBuffer& transformBuffer = m_transformsBuffer[ i ];
		const physx::PxTransform& transform = dynamic->getGlobalPose();
		const physx::PxQuat& quat = transform.q;
		transformBuffer.m_quatX = quat.x;
		transformBuffer.m_quatY = quat.y;
		transformBuffer.m_quatZ = quat.z;
		transformBuffer.m_quatW = quat.w;
		const physx::PxVec3& vec = transform.p;
		transformBuffer.m_posX = vec.x;
		transformBuffer.m_posY = vec.y;
		transformBuffer.m_posZ = vec.z;
	
		BoneMapping* parentMapping = nullptr;
		PxD6Joint* joint = nullptr;

		{
			const PxU32 collectionObjectsCount = collection->getNbObjects();
			for ( Uint16 i = 0; i < collectionObjectsCount; i++ )
			{
				PxBase& base = collection->getObject( i );
				if( base.is< PxD6Joint >() )
				{
					PxD6Joint* j = base.is< PxD6Joint >();
					PxRigidActor* actor0 = nullptr;
					PxRigidActor* actor1 = nullptr;
					j->getActors( actor0, actor1 );
					PxRigidActor* other = nullptr;
					if( actor0 == dynamic )
					{
						other = actor1;
					}
					else if( actor1 == dynamic)
					{
						other = actor0;
					}

					if( !other )
					{
						continue;
					}
					for( Uint16 j = 0; j != mappingCount; ++j )
					{
						BoneMapping& otherMapping = m_mappings[ j ];
						if( otherMapping.m_rigidDynamic == other && otherMapping.m_boneIndex < mapping.m_boneIndex )
						{
							parentMapping = &m_mappings[ j ];
							break;
						}
					}


					if( parentMapping )
					{
						if( ( actor0 == parentMapping->m_rigidDynamic && actor1 == dynamic )
							|| ( actor0 == dynamic && actor1 == parentMapping->m_rigidDynamic ) )
						{
							joint = j;
							break;
						}
					}

				}
			}

		}
		if( !parentMapping && articulation->getNbLinks() )
		{
			m_mappings.ClearFast();
			return;
		}
		PxArticulationLink* link = articulation->createLink( parentMapping ? ( PxArticulationLink* ) parentMapping->m_articulationLink : nullptr, dynamic->getGlobalPose() );
		mapping.m_articulationLink = link;
		if( mapping.m_articulationLink )
		{
			link->setName( dynamic->getName() );
			link->userData = this;
			Uint32 shapesCount = dynamic->getNbShapes();
			for( Uint32 i = 0; i != shapesCount; ++i )
			{
				physx::PxShape* shape = nullptr;
				dynamic->getShapes( &shape, 1, i );
				PxMaterial* material = nullptr;
				shape->getMaterials( &material, 1 );
				physx::PxShape* newShape = link->createShape( shape->getGeometry().any(), &material, 1, shape->getLocalPose() );
				newShape->userData = shape->userData;
				newShape->setQueryFilterData( shape->getQueryFilterData() );
				newShape->setSimulationFilterData( shape->getSimulationFilterData() );
			}
			float mass = dynamic->getMass();
			link->setMass( mass );
			mapping.m_linearDamper = SPhysicsSettings::m_ragdollChainedLinearDamper;
#ifdef EP2_SHEEP_RAGDOLL_ANGULAR_DAMPING_HACK
			if( m_angularDamping < 0.0f )
			{
				m_angularDamping = SPhysicsSettings::m_ragdollChainedAngularDamper;
			}
			mapping.m_angularDamper = m_angularDamping;
#else
			mapping.m_angularDamper = SPhysicsSettings::m_ragdollChainedAngularDamper;
#endif
			if( joint )
			{
				physx::PxArticulationJoint* articulationJoint = link->getInboundJoint();
				PxTransform parentPose = joint->getLocalPose( PxJointActorIndex::eACTOR0 );

				articulationJoint->setParentPose( parentPose );
				if( joint->getMotion( PxD6Axis::eTWIST ) == PxD6Motion::eFREE || joint->getTwistLimit().lower == joint->getTwistLimit().upper)
				{
					articulationJoint->setTwistLimitEnabled( false );
				}
				else
				{
					articulationJoint->setTwistLimitEnabled( true );
					float lower = joint->getTwistLimit().lower;
					float upper = joint->getTwistLimit().upper;

					lower += ragdollState.m_modifyTwistLower;
					upper += ragdollState.m_modifyTwistUpper;
					if( lower > upper )
					{
						lower = upper;
					}
					if( lower <= -PxPi )
					{
						lower = -PxPi + 0.01f;
					}
					if( upper >= PxPi )
					{
						upper = PxPi - 0.01f;
					}
					articulationJoint->setTwistLimit( lower, upper );
				}
				if( ( joint->getMotion( PxD6Axis::eSWING1 ) == PxD6Motion::eFREE ) && ( joint->getMotion( PxD6Axis::eSWING2 ) == PxD6Motion::eFREE ) )
				{
					articulationJoint->setSwingLimitEnabled( false );
				}
				else
				{
					articulationJoint->setSwingLimitEnabled( true );
					float yAngle = joint->getSwingLimit().yAngle - 0.01f;

					yAngle += ragdollState.m_modifySwingY;
					if( yAngle <=  0.0f )
					{
						yAngle = 0.01;
					}
					if( yAngle >= PxPi )
					{
						yAngle = PxPi - 0.01f;
					}

					float zAngle = joint->getSwingLimit().zAngle - 0.01f;
					zAngle += ragdollState.m_modifySwingZ;
					if( zAngle <= 0.0f )
					{
						zAngle = 0.01;
					}
					if( zAngle >= PxPi )
					{
						zAngle = PxPi - 0.01f;
					}

					articulationJoint->setSwingLimit( yAngle, zAngle );
				}
			}
			
		}
	}

	articulation->setSleepThreshold( SPhysicsSettings::m_ragdollChainedSleepThreshold );

#endif

	CPhysicsEngine::CollisionMask group;
	ragdollState.m_customDynamicGroup.RetrieveCollisionMasks( m_customDynamicGroupMask, group );
	m_originalGroupMask = GetGroup();

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	const Vector& trans = GetParentProvider()->GetLocalToWorld().GetTranslationRef();
	position->m_x = trans.X;
	position->m_y = trans.Y;
	position->m_desiredDistanceSquared = SPhysicsSettings::m_ragdollSimulationDynamicDistanceLimit;
	position->m_desiredDistanceSquared *= position->m_desiredDistanceSquared;
	position->m_resultDistanceSquared = FLT_MAX;
	position->m_visibilityQueryId = visibiltyId;

	PHYSICS_STATISTICS_INC(RagdollsInstanced)
}

CPhysicsChainedRagdollWrapper::~CPhysicsChainedRagdollWrapper()
{
#ifdef USE_PHYSX
	if( m_aggregate )
	{
		PxAggregate* aggregate = ( PxAggregate* ) m_aggregate;
		aggregate->release();
	}

	if( m_articulation )
	{
		PxArticulation* articulation = ( PxArticulation* ) m_articulation;
		articulation->release();
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

void CPhysicsChainedRagdollWrapper::Release( Uint32 actorIndex )
{
	if( actorIndex )
	{
#ifdef USE_PHYSX
		Uint32 mappingsCount = m_transformsBuffer.Size();
		for ( Uint16 i = 0; i < mappingsCount; i++ )
		{
			BoneTransformBuffer& mapping = m_transformsBuffer[ i ];
			PxRigidActor* rigidActor = ( PxRigidActor* ) mapping.m_rigidDynamic;
			if( !rigidActor ) continue;
			if( rigidActor->getNbShapes() == 0 ) continue;
			physx::PxShape* shape = 0;
			rigidActor->getShapes( &shape, 1, 0 );
			SActorShapeIndex index = ( SActorShapeIndex& ) shape->userData; 
			if( index.m_actorIndex != ( Int16 ) actorIndex ) continue;
			mapping.m_boneIndex = -1;
			break;
		}
#endif
	}
	else
	{
		RED_ASSERT( m_ref.GetValue() > 0 )
		if( !m_ref.Decrement() )
		{
			m_world->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->PushWrapperToRemove( this );
		}

	}
}

Bool CPhysicsChainedRagdollWrapper::MakeReadyToDestroy( TDynArray< void* >* toRemove )
{
	Bool result = true;
#ifdef USE_PHYSX

	PxScene* scene = nullptr;
	if( m_aggregate )
	{
		PxAggregate* aggregate = ( PxAggregate* ) m_aggregate;
		scene = aggregate->getScene();
		if( scene )
		{
			scene->removeAggregate( *aggregate );
			result = false;
		}
	}

	if( m_articulation )
	{
		PxArticulation* articulation = ( PxArticulation* ) m_articulation;
		scene = articulation->getScene();
		if( scene )
		{
			scene->removeArticulation( *articulation );
			result = false;
		}
	}
#endif
	return result;
}

void CPhysicsChainedRagdollWrapper::PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd )
{
#ifdef USE_PHYSX
	PC_SCOPE_PHYSICS(CPhysicsRagdollChainWrapper PreSimulation )

	if( !GetParentProvider()->HasParent() ) return;

	Uint32 flags = m_flags.GetValue();
	if( flags & PRBW_FlagsAreDirty )
	{
		UpdateFlags();
	}

	const Uint32 mappingsCount = m_mappings.Size();
	PxAggregate* aggregate = ( PxAggregate* ) m_aggregate;
	PxArticulation* articulate = ( PxArticulation* ) m_articulation;
	Bool attached = aggregate->getScene() != 0 || articulate->getScene() != 0;
	const EVisibilityResult visibilityQueryResult = ( const EVisibilityResult ) simulationContext->m_visibilityQueryResult;
	const float distanceFromViewportSquared = simulationContext->m_resultDistanceSquared;

	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();
	Float distanceLimit;
	if( m_simulationType != SM_DYNAMIC )
	{
		if( visibilityQueryResult != EVisibilityResult::EVR_NotVisible && visibilityQueryResult != EVisibilityResult::EVR_PartialyVisible )
		{
			distanceLimit = SPhysicsSettings::m_ragdollSimulationKinematicDistanceLimit * SPhysicsSettings::m_ragdollSimulationKinematicDistanceLimit;
		}
		else distanceLimit = 0.0f;
	}
	else distanceLimit = simulationContext->m_desiredDistanceSquared;

	static Uint64 currentTickMarker = 0;

	if( attached && distanceFromViewportSquared > distanceLimit )
	{
		if( currentTickMarker < tickMarker  ) 
		{
			currentTickMarker = tickMarker;
			attached = false;

			if( aggregate->getScene() )
			{
				PC_SCOPE_PHYSICS(CPhysRagWra removeAggregate )
				scene->removeAggregate( *aggregate );
			}
			if( articulate->getScene() )
			{
				PC_SCOPE_PHYSICS(CPhysRagWra removeArticulation )
				scene->removeArticulation( *articulate );
			}
			PHYSICS_STATISTICS_DEC(RagdollsSimulated)
		}
		else
		{
			SWrapperContext* position = m_world->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
			position->m_requestProcessingFlag = true;
		}
	}
	else if( !attached && distanceFromViewportSquared <= distanceLimit && m_ref.GetValue() > 0 )
	{
		if( allowAdd && ( !articulate || !articulate->getScene() ) )
		{
			if( currentTickMarker < tickMarker ) 
			{
				currentTickMarker = tickMarker;
				attached = true;
				if( m_simulationType != SM_DYNAMIC )
				{

					PC_SCOPE_PHYSICS(CPhysRagWra addAggregate )
					scene->addAggregate( *aggregate );

					for ( Uint16 i = 0; i < mappingsCount; i++ )
					{
						const BoneTransformBuffer& mapping = m_transformsBuffer[ i ];
						physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_rigidDynamic;
						if( !dynamic ) continue;
						PxTransform trans;
						PxQuat& quat = trans.q;
						quat.x = mapping.m_quatX;
						quat.y = mapping.m_quatY;
						quat.z = mapping.m_quatZ;
						quat.w = mapping.m_quatW;
						PxVec3& pos = trans.p;
						pos.x = mapping.m_posX;
						pos.y = mapping.m_posY;
						pos.z = mapping.m_posZ;
						dynamic->setKinematicTarget( trans );
					}
				}
				else
				{
					PC_SCOPE_PHYSICS(CPhysRagWra addArticulation )

					for ( Uint16 i = 0; i < mappingsCount; i++ )
					{
						BoneTransformBuffer& mapping = m_transformsBuffer[ i ];
						physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_rigidDynamic;
						if( !dynamic ) continue;
						if( physx::PxArticulationLink* link = ( physx::PxArticulationLink* ) m_mappings[ i ].m_articulationLink )
						{
							PxTransform trans;
							PxQuat& quat = trans.q;
							quat.x = mapping.m_quatX;
							quat.y = mapping.m_quatY;
							quat.z = mapping.m_quatZ;
							quat.w = mapping.m_quatW;
							PxVec3& pos = trans.p;
							pos.x = mapping.m_posX;
							pos.y = mapping.m_posY;
							pos.z = mapping.m_posZ;
							link->setGlobalPose( trans );
							link->setLinearVelocity( dynamic->getLinearVelocity() );
							link->setAngularVelocity( dynamic->getAngularVelocity() );
						}
					}
					scene->addArticulation( *articulate );
				}
				PHYSICS_STATISTICS_INC(RagdollsSimulated);
			}
		}
	}

	PHYSICS_STATISTICS_INC_IF(RagdollsSimulated,attached)
	if( !attached ) return;

	if( m_simulationType != SM_DYNAMIC )
	{
		if( ( flags & PRBW_PoseIsDirty ) && ( aggregate->getScene() ) )
		{
			for ( Uint16 i = 0; i < mappingsCount; i++ )
			{
				const BoneTransformBuffer& mapping = m_transformsBuffer[ i ];
				if( mapping.m_boneIndex < 0 ) continue;
				physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_rigidDynamic;
				if( !dynamic ) continue;
				PxTransform trans;
				PxQuat& quat = trans.q;
				quat.x = mapping.m_quatX;
				quat.y = mapping.m_quatY;
				quat.z = mapping.m_quatZ;
				quat.w = mapping.m_quatW;
				PxVec3& pos = trans.p;
				pos.x = mapping.m_posX;
				pos.y = mapping.m_posY;
				pos.z = mapping.m_posZ;
				dynamic->setKinematicTarget( trans );
			}

			SetFlag( PRBW_PoseIsDirty, false );
		}
	}

	if( flags & PRBW_StateIsDirty )
	{
		if( currentTickMarker >= tickMarker )
		{
			if( m_simulationType != SM_DYNAMIC )
			{
				if( aggregate->getScene() == nullptr ) return;
			} 
			else if( articulate->getScene() == nullptr ) return;		
		}
		else
		{
			PC_SCOPE_PHYSICS(CPhysRagWra statedirty )

			SetFlag( PRBW_StateIsDirty, false );

			Bool isNonKinematic = articulate->getScene() != 0;

			if( m_simulationType == SM_STATIC )
			{
				SWrapperContext* position = m_world->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
				position->m_desiredDistanceSquared = 0.0f;
				scene->removeAggregate( *aggregate );
				scene->removeArticulation( *articulate );
			}
			else if( ( m_simulationType == SM_KINEMATIC && isNonKinematic ) ||
				( m_simulationType != SM_KINEMATIC && !isNonKinematic ) )
			{
				m_previousPosition = Vector::ZEROS;

				if( m_simulationType != SM_DYNAMIC )
				{
					// custom dynamic group enabled? - restore original group
					if ( m_customDynamicGroupMask != m_originalGroupMask )
					{
						SetGroup( m_originalGroupMask );
					}
				}
				else
				{
					PxPhysics* physics = GPhysXEngine->GetPxPhysics();

					// custom dynamic group enabled? - set custom group
					if ( m_customDynamicGroupMask && ( m_customDynamicGroupMask != m_originalGroupMask ) )
					{
						SetGroup( m_customDynamicGroupMask );
					}
				}

				PxCollection* collection = ( PxCollection* ) m_collection;

				for ( Uint16 i = 0; i < collection->getNbObjects(); i++ )
				{
					PxBase& base = collection->getObject( i );
					physx::PxRigidDynamic* dynamic = base.is<physx::PxRigidDynamic>();
					if( dynamic )
					{
						Uint32 shapesCount = GetShapesCount( i );
						for( Uint32 j = 0; j != shapesCount; ++j )
						{
							physx::PxShape* shape = ( physx::PxShape* ) GetShape( j, i );
							if( !shape ) continue;

							SPhysicalFilterData filter( shape->getSimulationFilterData() );
							if( m_simulationType == SM_DYNAMIC )
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
						if( m_simulationType == SM_DYNAMIC ) 
						{
							flags |= PxConstraintFlag::ePROJECTION;
						}
						constraint->setFlags( flags );
					}

				}

				if( m_simulationType == SM_DYNAMIC )
				{
					for ( Uint16 i = 0; i < mappingsCount; i++ )
					{
						BoneTransformBuffer& mapping = m_transformsBuffer[ i ];
						physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_rigidDynamic;
						if( !dynamic ) continue;
						if( physx::PxArticulationLink* link = ( physx::PxArticulationLink* ) m_mappings[ i ].m_articulationLink )
						{
							PxTransform trans;
							PxQuat& quat = trans.q;
							quat.x = mapping.m_quatX;
							quat.y = mapping.m_quatY;
							quat.z = mapping.m_quatZ;
							quat.w = mapping.m_quatW;
							PxVec3& pos = trans.p;
							pos.x = mapping.m_posX;
							pos.y = mapping.m_posY;
							pos.z = mapping.m_posZ;
							link->setGlobalPose( trans );
							PxVec3 linearVelocity = dynamic->getLinearVelocity();
							PxVec3 angularVelocity = dynamic->getAngularVelocity();
							link->setLinearVelocity( linearVelocity );
							link->setAngularVelocity( angularVelocity );
						}
					}
					{
						PC_SCOPE_PHYSICS(CPhysRagWra removeAggregate )
							scene->removeAggregate( *aggregate );
					}

					{
						PC_SCOPE_PHYSICS(CPhysRagWra addArticulation )
						scene->addArticulation( *articulate );
					}

				}
				else
				{
					{
						PC_SCOPE_PHYSICS(CPhysRagWra removeArticulation )
							scene->removeArticulation( *articulate );
					}
					{
						PC_SCOPE_PHYSICS(CPhysRagWra addAggregate )
							scene->addAggregate( *aggregate );
						for ( Uint16 i = 0; i < mappingsCount; i++ )
						{
							const BoneTransformBuffer& mapping = m_transformsBuffer[ i ];
							physx::PxRigidDynamic* dynamic = ( physx::PxRigidDynamic* ) mapping.m_rigidDynamic;
							if( !dynamic ) continue;
							PxTransform trans;
							PxQuat& quat = trans.q;
							quat.x = mapping.m_quatX;
							quat.y = mapping.m_quatY;
							quat.z = mapping.m_quatZ;
							quat.w = mapping.m_quatW;
							PxVec3& pos = trans.p;
							pos.x = mapping.m_posX;
							pos.y = mapping.m_posY;
							pos.z = mapping.m_posZ;
							dynamic->setKinematicTarget( trans );
						}
					}
				}

				currentTickMarker = tickMarker;

			}
			for ( Uint16 i = 0; i < mappingsCount; i++ )
			{
				if( m_transformsBuffer[ i ].m_boneIndex >= 0 ) continue;

				currentTickMarker = tickMarker;

				PxRigidActor* actor = ( PxRigidActor* ) m_transformsBuffer[ i ].m_rigidDynamic;
				if( actor )
				{
					PxScene* scene = aggregate->getScene();
					if( scene )
					{
						PC_SCOPE_PHYSICS(CPhysRagWra removeAggregate )
							scene->removeAggregate( *aggregate );
					}

					for( Uint32 i = aggregate->getNbActors(); i != 0; --i )
					{
						physx::PxActor* rigidDynamic = nullptr;
						aggregate->getActors( &rigidDynamic, 1, i - 1 );
						if( rigidDynamic == actor )
						{
							aggregate->removeActor( *rigidDynamic );
						}
					}
					if( scene )
					{
						PC_SCOPE_PHYSICS(CPhysRagWra addAggregate )
							scene->addAggregate( *aggregate );
					}
				}

				if( physx::PxArticulationLink* link = ( physx::PxArticulationLink* ) m_mappings[ i ].m_articulationLink )
				{
					scene->removeArticulation( *articulate );

					static void (*releaseHierarchy) ( int, TDynArray<BoneMapping>& ) = []( int index, TDynArray<BoneMapping>& m_mappings )
					{
						physx::PxArticulationLink* articulationLink = ( physx::PxArticulationLink* ) m_mappings[ index ].m_articulationLink;
						if( !articulationLink ) return;
						Uint32 childsCount = articulationLink->getNbChildren();
						physx::PxArticulationLink* childArticulationLinkBuffer[ 16 ];
						articulationLink->getChildren( childArticulationLinkBuffer, 16 );

						for( Uint32 i = 0; i != childsCount; ++i )
						{
							PxShape* shape = nullptr;
							childArticulationLinkBuffer[ i ]->getShapes( &shape, 1, 0 );
							SActorShapeIndex index = ( SActorShapeIndex& ) shape->userData; 
							releaseHierarchy( index.m_actorIndex, m_mappings );
						}

						articulationLink->release();
						m_mappings[ index ].m_articulationLink = nullptr;
					};
					releaseHierarchy( i, m_mappings );

					scene->addArticulation( *articulate );

				}
			}
		}
	}

	if( m_simulationType != SM_DYNAMIC )
	{
		return;
	}

	Bool buoyancyProcessing = false;
	if( timeDelta > 0.0f )
	{
		if( !( flags & PRBW_DisableBuoyancy ) )
		{
			Bool dontUpdateBouyancy;
#ifdef EP2_SHEEP_RAGDOLL_ANGULAR_DAMPING_HACK
			if( ApplyBuoyancyForce( SPhysicsSettings::m_ragdollChainedLinearDamper, m_angularDamping, dontUpdateBouyancy ) )
#else
			if( ApplyBuoyancyForce( SPhysicsSettings::m_ragdollChainedLinearDamper, SPhysicsSettings::m_ragdollChainedAngularDamper, dontUpdateBouyancy ) )
#endif
			{
				buoyancyProcessing = true;
				if ( GetGroup() == m_customDynamicGroupMask)
				{
					SetGroup( m_originalGroupMask );
				}
			}
			else if ( m_customDynamicGroupMask && ( m_originalGroupMask != m_customDynamicGroupMask ) && ( GetGroup() == m_originalGroupMask ) )
			{
				SetGroup( m_customDynamicGroupMask );
			}
		}

		ApplyDampers( timeDelta );
	}

#endif
}

Bool CPhysicsChainedRagdollWrapper::IsReady() const
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

void CPhysicsChainedRagdollWrapper::SyncToAnimation( const TDynArray<Matrix>& bones, const Matrix& localToWorld )
{
	PC_SCOPE( CPhysicsRagr SyncToAnimation );
	if( m_simulationType != SM_KINEMATIC ) return;
#ifdef PHYSICS_NAN_CHECKS
	{
		PC_SCOPE_PHYSICS( CPhysicsRagW nans check );
		if( !localToWorld.IsOk() )
		{
			RED_FATAL_ASSERT( localToWorld.IsOk(), "NANS" );
#ifndef RED_PLATFORM_ORBIS
			for( Uint32 i = 0; i != 16; ++i )
			{
				const Float* temp = &localToWorld.V[ 0 ].X;
				int mask = _fpclass( *( temp + i ) );
				String message = String::Printf( TXT( "%f %x" ), *( temp + i ), *(unsigned int*)( temp + i ) );
				if( mask & _FPCLASS_SNAN ) message += TXT( " _FPCLASS_SNAN Signaling NaN " );
				if( mask & _FPCLASS_QNAN ) message += TXT( " _FPCLASS_QNAN Quiet NaN " );
				if( mask & _FPCLASS_NINF ) message += TXT( " _FPCLASS_NINF Negative infinity ( –INF) " );
				if( mask & _FPCLASS_NN ) message += TXT( " _FPCLASS_NN Negative normalized non-zero " );
				if( mask & _FPCLASS_ND ) message += TXT( " _FPCLASS_ND Negative denormalized " );
				if( mask & _FPCLASS_NZ ) message += TXT( " _FPCLASS_NZ Negative zero ( – 0) " );
				if( mask & _FPCLASS_PZ ) message += TXT( " _FPCLASS_PZ Positive 0 (+0) " );
				if( mask & _FPCLASS_PD ) message += TXT( " _FPCLASS_PD Positive denormalized " );
				if( mask & _FPCLASS_PN ) message += TXT( " _FPCLASS_PN Positive normalized non-zero " );
				if( mask & _FPCLASS_PINF ) message += TXT( " _FPCLASS_PINF Positive infinity (+INF) " );
				RED_LOG( RED_LOG_CHANNEL( NaNCheck ), message.AsChar() );
			}
#endif
			return;
		}

		Uint32 count = bones.Size();
		for( Uint32 i = 0; i != count; ++i )
		{
			const Matrix& mat = bones[ i ];
			if( !mat.GetTranslationRef().IsOk() )
			{
				RED_FATAL_ASSERT( mat.IsOk(), "NANS" );
#ifndef RED_PLATFORM_ORBIS
				for( Uint32 i = 0; i != 16; ++i )
				{
					const Float* temp = &mat.V[ 0 ].X;
					int mask = _fpclass( *( temp + i ) );
					String message = String::Printf( TXT( "%f %x" ), *( temp + i ), *(unsigned int*)( temp + i ) );
					if( mask & _FPCLASS_SNAN ) message += TXT( " _FPCLASS_SNAN Signaling NaN " );
					if( mask & _FPCLASS_QNAN ) message += TXT( " _FPCLASS_QNAN Quiet NaN " );
					if( mask & _FPCLASS_NINF ) message += TXT( " _FPCLASS_NINF Negative infinity ( –INF) " );
					if( mask & _FPCLASS_NN ) message += TXT( " _FPCLASS_NN Negative normalized non-zero " );
					if( mask & _FPCLASS_ND ) message += TXT( " _FPCLASS_ND Negative denormalized " );
					if( mask & _FPCLASS_NZ ) message += TXT( " _FPCLASS_NZ Negative zero ( – 0) " );
					if( mask & _FPCLASS_PZ ) message += TXT( " _FPCLASS_PZ Positive 0 (+0) " );
					if( mask & _FPCLASS_PD ) message += TXT( " _FPCLASS_PD Positive denormalized " );
					if( mask & _FPCLASS_PN ) message += TXT( " _FPCLASS_PN Positive normalized non-zero " );
					if( mask & _FPCLASS_PINF ) message += TXT( " _FPCLASS_PINF Positive infinity (+INF) " );
					RED_LOG( RED_LOG_CHANNEL( NaNCheck ), message.AsChar() );
				}
#endif
				return;
			}
		}
	}
#endif


	Bool modified = false;

	{
#ifdef USE_PHYSX
		PxTransform localToWorldTransform(*reinterpret_cast<const PxMat44*>( &localToWorld ) );

		for ( Uint32 i = 0; i < m_mappings.Size(); ++i )
		{
			BoneTransformBuffer& mapping = m_transformsBuffer[ i ];

			const Int16 boneIndex = m_mappings[i].m_boneIndex;
			if( boneIndex < 0 ) continue;

			const Matrix& parentBone = bones[ boneIndex ];

			const PxTransform boneTransform( *reinterpret_cast<const PxMat44*>( &parentBone ) ) ;
			const PxTransform outTransform = localToWorldTransform.transform( boneTransform );

			const PxVec3& p = outTransform.p;
			mapping.m_posX = p.x;
			mapping.m_posY = p.y;
			mapping.m_posZ = p.z;

			PxQuat q = outTransform.q;
			q.normalize();

			mapping.m_quatX = q.x;
			mapping.m_quatY = q.y;
			mapping.m_quatZ = q.z;
			mapping.m_quatW = q.w;
			modified = true;
		}
#endif

	}
	const Vector& pos = localToWorld.GetTranslationRef();

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	RED_FATAL_ASSERT( position, "SWrapperContext* position is null ptr" );

	//const Bool differ = position->m_x != pos.X || position->m_y != pos.Y;
	position->m_x = pos.X;
	position->m_y = pos.Y;

	if( modified )
	{
		SetFlag( PRBW_PoseIsDirty, true );
	}
}

Int32 CPhysicsChainedRagdollWrapper::GetIndex( const char* actorName )
{
#ifdef USE_PHYSX
	for ( Uint32 i = 0; i < m_mappings.Size(); ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];

		PxRigidBody* actor;
		if( m_simulationType != SM_DYNAMIC ) actor = ( physx::PxRigidDynamic* ) mapping.m_rigidDynamic;
		else actor = ( PxRigidBody* ) mapping.m_articulationLink;
		if( !actor ) continue;

		if( Red::System::StringCompare( actor->getName(), actorName ) == 0 )
		{
			return i;
		}
	}
#endif
	return -1;
}

SPhysicalMaterial* CPhysicsChainedRagdollWrapper::GetMaterial( Int32 actorIndex )
{
#ifndef USE_PHYSX
	return 0;
#else

	size_t mappingsCount = m_mappings.Size();
	if( !mappingsCount ) return 0;
	if( mappingsCount <= actorIndex ) return 0;

	BoneMapping& mapping = m_mappings[ actorIndex ];

	PxRigidActor* actor;
	if( m_simulationType != SM_DYNAMIC ) actor = ( physx::PxRigidDynamic* ) mapping.m_rigidDynamic;
	else actor = ( PxRigidActor* ) mapping.m_articulationLink;
	if( !actor ) return 0;

	physx::PxShape* shape = 0;
	actor->getShapes( &shape, 1, 0 );

	PxMaterial* material = 0;
	shape->getMaterials( &material, 1 );

	return ( SPhysicalMaterial* ) material->userData;
#endif
}

Uint32 CPhysicsChainedRagdollWrapper::GetActorsCount() const
{
#ifndef USE_PHYSX
	return 0;
#else
	return m_mappings.Size();
#endif
}

void* CPhysicsChainedRagdollWrapper::GetActor( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
	return 0;
#else
	size_t mappingsCount = m_mappings.Size();
	if( !mappingsCount ) return 0;
	if( mappingsCount <= actorIndex ) return 0;
	if( m_simulationType != SM_DYNAMIC )
	{
		return m_mappings[ actorIndex ].m_rigidDynamic;
	}
	else 
	{
		return m_mappings[ actorIndex ].m_articulationLink;
	}

#endif
}

void CPhysicsChainedRagdollWrapper::SampleBonesModelSpace( TDynArray<Matrix>& poseInOut ) const
{
#ifdef USE_PHYSX
	PC_SCOPE_PHYSICS(CPhysicsRagdollChainWrapper SampleBonesModelSpace )

	const Uint32 numMappings = m_mappings.Size();

	if( !numMappings ) return;

	if( m_simulationType == SM_KINEMATIC) return;

	Matrix invertWorldMat = GetParentProvider()->GetLocalToWorld().FullInverted();

	PxRigidActor* baseActor = nullptr;

	PxArticulation* articulate = ( PxArticulation* ) m_articulation;
	if ( articulate->getScene() != nullptr )
	{
		baseActor = ( PxRigidActor* ) m_mappings[ 0 ].m_articulationLink;

		if( baseActor )
		{
			PxTransform trans = baseActor->getGlobalPose();
			if( !trans.isValid() )
			{
				SWrapperContext* simulationContext = m_world->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
				simulationContext->m_desiredDistanceSquared = 0;
				simulationContext->m_requestProcessingFlag = true;
				return;
			}
		}

		for ( Uint32 i = 0; i < numMappings; ++i )
		{
			const BoneMapping& map = m_mappings[i];
			BoneTransformBuffer& mapping = const_cast< BoneTransformBuffer& >( m_transformsBuffer[ i ] );

			const Int16 boneIndex = map.m_boneIndex;
			if ( boneIndex >= 0 )
			{
				if (map.m_articulationLink )
				{
					PxTransform globalPose = ( ( PxRigidActor* ) map.m_articulationLink )->getGlobalPose();
					PxMat44 mat = PxMat44( globalPose );
#ifdef _DEBUG
					Matrix globalPartPose = TO_MAT( mat );
					Matrix modelPartPoseInverted = globalPartPose * invertWorldMat;
					poseInOut[ boneIndex ] = modelPartPoseInverted;
#else
					PxMat44 world = TO_PX_MAT( invertWorldMat ) * mat;
					poseInOut[ boneIndex ] = TO_MAT( world );

					const PxVec3& p = globalPose.p;
					mapping.m_posX = p.x;
					mapping.m_posY = p.y;
					mapping.m_posZ = p.z;

					const PxQuat q = globalPose.q;
					mapping.m_quatX = q.x;
					mapping.m_quatY = q.y;
					mapping.m_quatZ = q.z;
					mapping.m_quatW = q.w;
#endif
				}
				else
				{
					const Int16 parentBoneIndex = map.m_parentIndex;
					if( parentBoneIndex >= 0 )
					{
						poseInOut[ boneIndex ] = map.m_initLocalPose * poseInOut[ parentBoneIndex ];
					}
					else
					{
						poseInOut[ boneIndex ] = map.m_initLocalPose;
					}
				}
			}
		}


	}
	else
	{
		for ( Uint32 i = 0; i < numMappings; ++i )
		{
			const BoneMapping& map = m_mappings[i];
			const BoneTransformBuffer& mapping = m_transformsBuffer[ i ];

			const Int16 boneIndex = map.m_boneIndex;
			if ( boneIndex >= 0 )
			{
				if ( map.m_articulationLink )
				{
					PxTransform outTransform;
					outTransform.p.x = mapping.m_posX;
					outTransform.p.y = mapping.m_posY;
					outTransform.p.z = mapping.m_posZ;
					outTransform.q.x = mapping.m_quatX;
					outTransform.q.y = mapping.m_quatY;
					outTransform.q.z = mapping.m_quatZ;
					outTransform.q.w = mapping.m_quatW;

					PxMat44 world = TO_PX_MAT( invertWorldMat ) * PxMat44( outTransform );
					poseInOut[ boneIndex ] = TO_MAT( world );
				}
				else
				{
					const Int16 parentBoneIndex = map.m_parentIndex;
					if( parentBoneIndex >= 0 )
					{
						poseInOut[ boneIndex ] = map.m_initLocalPose * poseInOut[ parentBoneIndex ];
					}
					else
					{
						poseInOut[ boneIndex ] = map.m_initLocalPose;
					}
				}
			}

		}
	}

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	position->m_x = m_transformsBuffer[ 0 ].m_posX;
	position->m_y = m_transformsBuffer[ 0 ].m_posY;
#endif

}

Bool CPhysicsChainedRagdollWrapper::GetCurrentPositionAndDeltaPosition( Vector & outCurrentPosition, Vector & outDeltaPosition ) const
{
#ifndef USE_PHYSX
	return false;
#else
	if( m_simulationType != SM_DYNAMIC ) return false;

	if( m_mappings.Empty() ) return false;

	if( !m_mappings[ 0 ].m_rigidDynamic ) return false;

	PxRigidActor* actor = ( PxRigidActor* )m_mappings[ 0 ].m_articulationLink;

	if( !actor->getScene() ) return false;

	PxTransform trans = actor->getGlobalPose();
	if( !trans.isValid() )
	{
		return false;
	}
	PxMat44 pose = trans;
	Matrix result = TO_MAT( pose );

	outCurrentPosition = result.GetTranslation();
	outDeltaPosition = m_previousPosition == Vector::ZEROS? Vector::ZERO_3D_POINT : outCurrentPosition - m_previousPosition;
	return true;
#endif
};

//////////////////////////////////////////////////////////////////////////

Int32 CPhysicsChainedRagdollWrapper::GetBoneIndex( Int32 actorIndex ) const
{
#ifdef USE_PHYSX
	physx::PxActor* actor = ( physx::PxActor* ) GetActor( actorIndex );
	if ( !actor ) return -1;
	for ( Uint32 i = 0; i < m_mappings.Size(); ++i )
	{
		if ( m_mappings[ i ].m_rigidDynamic == actor )
		{
			return m_mappings[ i ].m_boneIndex;
		}
	}
#endif
	return -1;
}

//////////////////////////////////////////////////////////////////////////

Bool CPhysicsChainedRagdollWrapper::DisableRagdollParts( const TDynArray< Uint32 > & boneIndices )
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
		if ( mappingIndex > -1 )
		{
			res = true;
			Release( mappingIndex );
		}
	}
	return res;
}

//////////////////////////////////////////////////////////////////////////
//
// set collision group type
void CPhysicsChainedRagdollWrapper::SetGroup( const CName& groupType )
{
	#ifdef USE_PHYSX
	CPhysicsEngine::CollisionMask newTypeBit = GPhysicEngine->GetCollisionTypeBit( groupType );
	PxCollection* collection = ( PxCollection* ) m_collection;
	const physx::PxU32 objectsCount = collection->getNbObjects();
	for ( physx::PxU32 i = 0; i < objectsCount; i++ )
	{
		PxBase& base = collection->getObject( i );
		PxRigidActor* actor = base.is<PxRigidActor>();
		if( !actor ) continue;
		const Uint32 nbShapes = actor->getNbShapes();
		for( Uint32 j = 0; j != nbShapes; ++j )
		{
			physx::PxShape* shape;
			actor->getShapes( &shape, 1, j );
			SPhysicalFilterData old( shape->getSimulationFilterData() );

			// set new group
			SPhysicalFilterData newGroups( newTypeBit, GPhysicEngine->GetCollisionGroupMask( groupType ), old.GetFlags() );
			shape->setSimulationFilterData( newGroups.m_data );
			shape->setQueryFilterData( newGroups.m_data );
		}
	}
	#endif
}
//////////////////////////////////////////////////////////////////////////
//
// set collision group type
void CPhysicsChainedRagdollWrapper::SetGroup( const CPhysicsEngine::CollisionMask& groupTypeMask )
{
#ifdef USE_PHYSX
	PxCollection* collection = ( PxCollection* ) m_collection;
	const physx::PxU32 objectsCount = collection->getNbObjects();
	for ( physx::PxU32 i = 0; i < objectsCount; i++ )
	{
		PxBase& base = collection->getObject( i );
		PxRigidActor* actor = base.is<PxRigidActor>();
		if( !actor ) continue;
		const Uint32 nbShapes = actor->getNbShapes();
		for( Uint32 j = 0; j != nbShapes; ++j )
		{
			physx::PxShape* shape;
			actor->getShapes( &shape, 1, j );
			SPhysicalFilterData old( shape->getSimulationFilterData() );

			// set new group
			SPhysicalFilterData newGroups( groupTypeMask, GPhysicEngine->GetCollisionGroupMask( groupTypeMask ), old.GetFlags() );
			shape->setSimulationFilterData( newGroups.m_data );
			shape->setQueryFilterData( newGroups.m_data );
		}
	}
#endif
}
//////////////////////////////////////////////////////////////////////////
//
// return collision group type
CPhysicsEngine::CollisionMask CPhysicsChainedRagdollWrapper::GetGroup()
{
#ifdef USE_PHYSX
	PxCollection* collection = ( PxCollection* ) m_collection;
	const physx::PxU32 objectsCount = collection->getNbObjects();
	for ( physx::PxU32 i = 0; i < objectsCount; i++ )
	{
		PxBase& base = collection->getObject( i );
		PxRigidActor* actor = base.is<PxRigidActor>();
		if( !actor ) continue;
		const Uint32 nbShapes = actor->getNbShapes();
		for( Uint32 j = 0; j != nbShapes; ++j )
		{
			physx::PxShape* shape;
			actor->getShapes( &shape, 1, j );
			SPhysicalFilterData old( shape->getSimulationFilterData() );

			return old.GetCollisionType();
		}
	}
#endif

	// def result
	return 0;
}

void CPhysicsChainedRagdollWrapper::OnContactModify( void* pairPtr )
{
	if( m_simulationType != SM_DYNAMIC ) return;
#ifdef USE_PHYSX
	
	physx::PxContactModifyPair* const pair = ( physx::PxContactModifyPair* const ) pairPtr;
	const physx::PxRigidDynamic* dynamicActor = nullptr;
	const PxRigidBody* articulationActor = pair->actor[ 0 ]->isArticulationLink();
	float reverse = 1.0f;
	if( !articulationActor )
	{
		articulationActor = pair->actor[ 1 ]->isArticulationLink();
		dynamicActor = pair->actor[ 0 ]->isRigidDynamic();
		reverse *= -1.0f;
	}
	else
	{
		dynamicActor = pair->actor[ 1 ]->isRigidDynamic();
	}

	//be sure we have one articulation and onde dynamic actor
	if( !articulationActor || !dynamicActor ) return;

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );

	Int8& visibilityResult = position->m_visibilityQueryResult;
	Bool applyForce = visibilityResult & EVisibilityResult::EVR_PartialyVisible || visibilityResult & EVisibilityResult::EVR_NotTested;

	PxContactSet& contact = pair->contacts;
	Uint32 contactCount = contact.size();
	for( Uint32 j = 0; j != contactCount; ++j )
	{
		const PxVec3& point = contact.getPoint( j );
		PxVec3 force = contact.getNormal( j );

		//fuck current impulse from contact
		contact.ignore( j );

		if( !applyForce ) 
		{
			continue;
		}

		force.z = 0.0f;
		PxVec3 velocity = dynamicActor->getLinearVelocity();
		velocity.z = 0.0f;
		float multipler = velocity.magnitude();
		force *= SPhysicsSettings::m_ragdollContactMultipler;
		force *= multipler;
		force /= ( float ) contactCount;
		{
			Float magnitude = force.magnitude();
			if( magnitude > SPhysicsSettings::m_ragdollContactClamp )
			{
				force *= SPhysicsSettings::m_ragdollContactClamp/magnitude;
			}
		}


		//normal direction depends on relation of first actor to second on reverse...
		force *= reverse;
		PxRigidBodyExt::addForceAtPos( *const_cast< PxRigidBody* >( articulationActor ), force, point, PxForceMode::eIMPULSE );
	}
#endif
}

Bool CPhysicsChainedRagdollWrapper::ApplyBuoyancyForce( float baseLinearDamper, float baseAngularDamper, Bool& waterLevelToDeep )
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
			PxRigidActor* actor = ( PxRigidActor* ) mapping.m_articulationLink;
			if( !actor ) continue;

			///This is to accomodate for the rare situation when we can be thrown out of water quite rapidly
			//by a player and freeze in the air due to gravity being disabled and not turned on again
			if( ( actor->getActorFlags() & physx::PxActorFlag::eDISABLE_GRAVITY ) )
			{
				actor->setActorFlag( physx::PxActorFlag::eDISABLE_GRAVITY, false );
			}

			elementsToCount = i;

			PxVec3 vec = actor->getGlobalPose().p;
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
		PxRigidActor* actor = ( PxRigidActor* ) mapping.m_articulationLink;

		if(mapping.m_floatingRatio < 0.5f) forceMultiplier += 1.0f;

		if( !actor ) continue;

		elementsToCount = i + 1;
		PxBounds3 bounds;
		{
			PC_SCOPE_PHYSICS(CPhyWraInt ApplyBuoyancyForce bounds )
			bounds = actor->getWorldBounds();
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
	PxArticulation* articulate = ( PxArticulation* ) m_articulation;
	for( Uint32 i = 0; i != elementsToCount; ++i )
	{
		BoneMapping* mapping = &m_mappings[ i ];

		physx::PxArticulationLink* actor = ( physx::PxArticulationLink* ) mapping->m_articulationLink;
		if( !actor ) continue;
		if( mapping->m_boneIndex < 0 ) continue;

		if( mapping->m_buffer2 - mapping->m_halfBoundsHeight > mapping->m_buffer3 )
		{
			if( actor->getActorFlags() & physx::PxActorFlag::eDISABLE_GRAVITY )
			{
				articulate->wakeUp();
				actor->setActorFlag( physx::PxActorFlag::eDISABLE_GRAVITY, false );
			}

			mapping->m_linearDamper = baseLinearDamper;
			mapping->m_angularDamper = baseAngularDamper;
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

			mapping->m_linearDamper = linearDamper;
			mapping->m_angularDamper = angularDamper;
		}
	}
#endif

	return true;
}

void CPhysicsChainedRagdollWrapper::ApplyDampers( Float timeDelta )
{
	if( m_simulationType != SM_DYNAMIC ) return;
#ifdef USE_PHYSX

	PxArticulation* articulate = ( PxArticulation* ) m_articulation;
	if( articulate->isSleeping() ) return;

	for ( Uint32 i = 0; i < m_mappings.Size(); ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];

		PxRigidBody* actor = ( PxRigidBody* ) mapping.m_articulationLink;
		if( !actor ) continue;

		const Float linear = 1.0f - ( mapping.m_linearDamper * timeDelta );
		const Float angular = 1.0f - ( mapping.m_angularDamper * timeDelta );

		if( linear != 1.0f )
		{
			PxVec3 linearVelocity = actor->getLinearVelocity();
			linearVelocity *= linear;
			actor->setLinearVelocity( linearVelocity, false );
		}

		if( angular != 1.0f )
		{
			PxVec3 angularVelocity = actor->getAngularVelocity();
			angularVelocity *= angular;
			actor->setAngularVelocity( angularVelocity, false );
		}

	}
#endif
}

Box CPhysicsChainedRagdollWrapper::GetWorldBounds()
{
#ifndef USE_PHYSX
	return Box();
#else
	PxBounds3 bounds;
	bounds.setEmpty();

	for ( Uint32 i = 0; i < m_mappings.Size(); ++i )
	{
		BoneMapping& mapping = m_mappings[ i ];

		PxRigidBody* actor = nullptr;

		if( m_simulationType != SM_DYNAMIC )
		{
			actor = ( physx::PxRigidDynamic* ) mapping.m_rigidDynamic;
		}
		else
		{
			actor = ( PxRigidBody* ) mapping.m_articulationLink;
		}

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
