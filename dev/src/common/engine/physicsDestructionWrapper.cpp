/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "physicsDestructionWrapper.h"
#include "..\physics\physicsWorldPhysXImpl.h"
#include "..\physics\compiledCollision.h"
#include "physicsDestructionResource.h"

#ifdef USE_PHYSX
#include "PxGeometry.h"

using namespace physx;
#endif
#include "..\core\dataError.h"
#include "..\physics\physicsEngine.h"
#include "..\physics\physicsSettings.h"
#include "collisionCache.h"
#include "collisionMesh.h"
#include "destructionComponent.h"
#include "utils.h"

DECLARE_PHYSICS_WRAPPER(CPhysicsDestructionWrapper,EPW_Destruction,true, true)


SPhysicsDestructionParameters::SPhysicsDestructionParameters()
	: m_dynamic( false )
	, m_kinematic( false )
	, m_baseResource( nullptr )
	, m_fracturedResource( nullptr )
	, m_debrisMaxSeparation( 10.0f )
	, m_simulationDistance( 25.0f )
	, m_fadeOutTime( 1.0f )
	, m_forceToDamage( 0 )
	, m_damageThreshold( 0 )
	, m_damageEndurance( 0 )
	, m_useWorldSupport( false )
	, m_initialBaseVelocity( Vector::ZEROS )
	, m_hasInitialFractureVelocity( false )
	, m_maxVelocity( -1.0f )
	, m_maxAngFractureVelocity( -1.0f )
	, m_debrisTimeout( false )
	, m_debrisTimeoutMin( 0.f )
	, m_debrisTimeoutMax( 0.f )
	, m_accumulateDamage( false )
	, m_parent( 0 )
	, m_physicalCollisionType( CNAME( Destructible ) )
	, m_fracturedPhysicalCollisionType( CNAME( Destructible ) )
{
}

CPhysicsDestructionWrapper::CPhysicsDestructionWrapper( const SPhysicsDestructionParameters& parameters, CPhysicsWorld* world, TRenderVisibilityQueryID visibiltyId
													   , CompiledCollisionPtr compiledCollision, CompiledCollisionPtr compiledCollisionFractured)
	: CPhysicsWrapperInterface( parameters.m_physicalCollisionType )
	, m_actorsCount( 0 )
	, m_debrisLifetimeAcc( 0.f )
	, m_accumulatedDamage( 0.f )
	, m_maxVelocity( 0.f )
	, m_forceOnDestruction( Vector::ZEROS )
	, m_scale( Vector::ONES )
	, m_updateKinematicStateDecision( false )
	, m_isDissolveEnabled( false )
{
	m_world = static_cast< CPhysicsWorldPhysXImpl* >( world );
	m_world->AddRef();

	SWrapperContext* wrapperContext = m_world->GetWrappersPool< CPhysicsDestructionWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );

	wrapperContext->m_x = parameters.m_pose.GetTranslationRef().X;
	wrapperContext->m_y = parameters.m_pose.GetTranslationRef().Y;
	wrapperContext->m_resultDistanceSquared = FLT_MAX;
	wrapperContext->m_visibilityQueryId = visibiltyId;

	m_bounds = Box();

	SetDestructionFlag( EPDF_RequestSceneRemove, true );
	SetDestructionFlag( EPDF_IsFractured, false );
	SetDestructionFlag( EPDF_IsReady, false);
	SetDestructionFlag( EPDF_UpdateBounds, true );
	SetDestructionFlag( EPDF_ScheduleUpdateSkinning, false );
	SetDestructionFlag( EPDF_ScheduleEnableDissolve, false );
}

CPhysicsDestructionWrapper::~CPhysicsDestructionWrapper( )
{
	Destroy();
}

void CPhysicsDestructionWrapper::Destroy()
{
	PHYSICS_STATISTICS_DEC(DestructionsInstanced)

	for( Uint32 i = 0; i < m_actorsCount; ++i)
	{
		PxActor* actor =  m_actors[i];
		if( actor )
		{
			ASSERT( actor->getScene() == nullptr, TXT( "Destruction actor's still on scene in destructor. This shouldn't happen.") )
			actor->release();
			m_actors[i] = nullptr;
		}
	}

	m_actors.Clear();
	m_world->ReleaseRef();
	m_world = nullptr;
}

void CPhysicsDestructionWrapper::Release(Uint32 actorIndex /*= 0*/)
{
	m_world->GetWrappersPool< CPhysicsDestructionWrapper, SWrapperContext >()->PushWrapperToRemove( this );
}

Bool CPhysicsDestructionWrapper::IsReady() const
{
	return GetDestructionFlag( EPDF_IsReady );
}


Matrix CPhysicsDestructionWrapper::GetPose(Uint32 actorIndex /*= 0*/) const
{ 
	return actorIndex < m_actorsCount ? TO_MAT( PxMat44( m_actors[actorIndex]->getGlobalPose() ) ) : Matrix::IDENTITY;
}

void CPhysicsDestructionWrapper::SetPose(const Matrix& localToWorld, Uint32 actorIndex /*= 0*/)
{
#ifdef USE_PHYSX

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


	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsDestructionWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );

	position->m_x = pos.X;
	position->m_y = pos.Y;

	Int32 flags = m_flags.GetValue();
	if( ( flags & EPDF_IsReady ) && !( flags & EPDF_IsFractured ) && (m_simulationType != SM_STATIC) ) 
	{
		PxMat44 mat( TO_PX_VECTOR( xAxis ), TO_PX_VECTOR( yAxis ), TO_PX_VECTOR( zAxis ), TO_PX_VECTOR( pos ) );
		PxTransform pose( mat );

		m_actors[0]->setGlobalPose( pose, m_simulationType == SM_KINEMATIC );
		SetDestructionFlag( EPDF_UpdateBounds, true );
	}
#endif
}

void* CPhysicsDestructionWrapper::GetActor(Uint32 actorIndex /*= 0 */) const
{
#ifndef USE_PHYSX
	return 0;
#else
	return actorIndex < m_actorsCount ? m_actors[ actorIndex ] : 0;
#endif
}

CPhysicsWorld* CPhysicsDestructionWrapper::GetPhysicsWorld()
{
	return m_world;
}

Float CPhysicsDestructionWrapper::GetMass(Uint32 actorIndex /*= 0 */) const
{
	if( !GetDestructionFlag( EPDF_IsFractured ) )
	{
		if( m_actorsAdditionalInfo[ 0 ].GetFlag( EDAF_IsStatic ) )
		{
			return 1.0f;
		}
	}
	return CPhysicsWrapperInterface::GetMass( actorIndex );
}

Bool CPhysicsDestructionWrapper::Create(const SPhysicsDestructionParameters& parameters, CompiledCollisionPtr compiledCollision, CompiledCollisionPtr compiledCollisionFractured)
{
	PC_SCOPE_PHYSICS(CPhysicsDestructionWrapper Create )
		
	Bool isSucceed = true;
	// If m_actorsCount == 0 it means we just created the wrapper and we should initialize it's params as well as create the base actor.
	if( !m_actorsCount )
	{
		m_basePhysicalCollisionType			= parameters.m_physicalCollisionType;
		m_fracturedPhysicalCollisionType	= parameters.m_fracturedPhysicalCollisionType;

		m_simulationType					= parameters.m_dynamic ? SM_DYNAMIC : (parameters.m_kinematic ? SM_KINEMATIC : SM_STATIC );
		m_maxVelocity						= parameters.m_maxVelocity;
		m_maxAngFractureVelocity			= parameters.m_maxAngFractureVelocity;

		SetDestructionFlag( EPDF_AccumulatesDamage,				parameters.m_accumulateDamage );
		SetDestructionFlag( EPDF_HasDebrisTimeout,				parameters.m_debrisTimeout );
		SetDestructionFlag( EPDF_HasInitialFractureVelocity,	parameters.m_hasInitialFractureVelocity );
		SetDestructionFlag( EPDF_HasMaxVelocity,				m_maxVelocity > 0.f ); // by default -1.f means disabled

		m_forceToDamage						= parameters.m_forceToDamage;
		m_damageThreshold					= parameters.m_damageThreshold;

#ifdef USE_PHYSX
		m_actorsCount = 0;
		Uint32 actorsCount = ( ( compiledCollision ? compiledCollision->GetGeometries().Size() : 0 )+ ( compiledCollisionFractured ? compiledCollisionFractured->GetGeometries().Size() : 0 ));
		m_actors.Resize( actorsCount );
		m_boundsFractured.Resize( actorsCount );
		m_actorsAdditionalInfo.Resize( actorsCount );

		CComponent* component = nullptr;
		if( !GetParent( component ) ) 
		{
			return false;
		}

		m_componentOffsetInverse = Matrix::IDENTITY;
		if( parameters.m_parent )
		{
			component->GetTransform().CalcLocalToWorld(m_componentOffsetInverse);
			m_componentOffsetInverse.Invert();
		}

		// if we have compiled collision, then create the base actor.
		if( compiledCollision )
		{
			isSucceed &= CreateActors(compiledCollision, parameters, m_actorsCount);
		}
		else 
		{
			RED_LOG( CPhysicsDestructionWrapper, TXT( "No compiled collision for base actor of destruction wrapper - check collision cache!" ) );
			isSucceed = false;
		}
	}
	// if we have compiled collision, then create the fractured actor.
	// this might be called multiple times over few frames, depending on the number of actors to create
	if( compiledCollisionFractured )
	{
		isSucceed &= CreateActors(compiledCollisionFractured, parameters, m_actorsCount);
	}
	else 
	{
		RED_LOG( CPhysicsDestructionWrapper, TXT( "No compiled collision for fractured actors of destruction wrapper - check collision cache!" ) );
		isSucceed = false;
	}
#endif

	return isSucceed;
}

Bool CPhysicsDestructionWrapper::CreateActors(CompiledCollisionPtr compiledCollision, const SPhysicsDestructionParameters &parameters, Uint16 &actorIndexOut)
{
	PC_SCOPE_PHYSICS(CPhysicsDestructionWrapper CreateActors )

	auto & geometries		= compiledCollision->GetGeometries();
	PxScene* scene			= m_world->GetPxScene();
	PxPhysics& physics		= scene->getPhysics();

	Bool isSucceed = true;
	
	Matrix transform = parameters.m_pose;
	CStandardRand frandom;

	CPhysicsEngine::CollisionMask collisionType = 0;
	CPhysicsEngine::CollisionMask collisionGroup = 0;

	CPhysicsEngine::CollisionMask collisionTypeSt = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) );
	CPhysicsEngine::CollisionMask collisionGroupSt = GPhysicEngine->GetCollisionGroupMask( CNAME( Static )  );

	if( actorIndexOut == 0 )
	{
		m_basePhysicalCollisionType.RetrieveCollisionMasks( collisionType, collisionGroup );
	}
	else
	{
		m_fracturedPhysicalCollisionType.RetrieveCollisionMasks( collisionType, collisionGroup );
	}

	SPhysicalFilterData dataSt( collisionTypeSt, collisionGroupSt );
	SPhysicalFilterData data( collisionType, collisionGroup );
	data.SetFlags( SPhysicalFilterData::EPFDF_DetailedConntactInfo | data.GetFlags() );

	Int32 flags = m_flags.GetValue();

	CPhysicsDestructionResource* res = actorIndexOut ? parameters.m_fracturedResource : parameters.m_baseResource;
	const auto& additionalInfo = res->GetAdditionalInfoArray();

	Uint16 actorIndex = actorIndexOut;
	Uint16 count = 0;
	Float maxAngVeloSq = m_maxAngFractureVelocity * m_maxAngFractureVelocity;

	for ( Uint16 chunkResIndex = actorIndex ? actorIndex - 1 : actorIndex; chunkResIndex < geometries.Size(); ++chunkResIndex, ++count )
	{
		if( count > MAX_CREATED_PER_FRAME )
		{
			return false;
		}
		SCachedGeometry& geometry = geometries[ chunkResIndex ];
		void* geometryPtr = geometry.GetGeometry();
		if( !geometryPtr ) 
		{
			continue;
		}

		PxGeometry* phxGeometry = ( PxGeometry* ) geometryPtr;

		if ( geometry.m_densityScaler == 0.0f )
		{
			DATA_HALT( DES_Major, res, TXT("DestructionWrapper"),  TXT("Density scaler on dynamic physic instance No.[%i] is set to zero which is WRONG"), actorIndex );
			isSucceed = false;
		}
		else
		{
			PxMaterial* material = GPhysXEngine->GetMaterial( geometry.m_physicalSingleMaterial );
			if ( !material )
			{
				if ( geometry.m_physicalSingleMaterial != CName::NONE )
				{
					DATA_HALT( DES_Major, res, TXT("DestructionWrapper"),  TXT("Physical geometry No.[%i] has undefined physical material \"%s\" "), actorIndex, geometry.m_physicalSingleMaterial.AsString().AsChar() );
				}
				// Apparently the script exporting from max can put a single material in this array, so we're checking for that to avoid crashing
				if( geometry.m_physicalMultiMaterials.Size() > 0 )
				{
					CName matName = geometry.m_physicalMultiMaterials[0];
					material = GPhysXEngine->GetMaterial( geometry.m_physicalSingleMaterial );							
				}	
				if( !material )
				{
					DATA_HALT( DES_Major, res, TXT("DestructionWrapper"),  TXT("Physical geometry No.[%i] has no material defined."), actorIndex);
					// We take default material to prevent crashes...
					material = GPhysXEngine->GetMaterial();
				}
			}
			else if( material == GPhysXEngine->GetMaterial() )
			{
				DATA_HALT( DES_Major, res, TXT("DestructionWrapper"),  TXT("Physical destruction actor No.[%i] is created with default material which is dissalowed "), actorIndex );
			}

			Bool isDestroyed = parameters.m_isDestroyed && (actorIndexOut != 0);
			actorIndex = actorIndexOut;

			CPhysicsEngine::CollisionMask collType;
			CPhysicsEngine::CollisionMask collGroup;

			PxTransform pxTransform = PxTransform( TO_PX_MAT( transform ) );

			if ( pxTransform.q.isFinite() && !pxTransform.q.isSane() )
			{
				pxTransform.q.normalize();
			}

			// If we set the sim type to dynamic in component - disregard resource settings, make all actors dynamic.
			if( m_simulationType == SM_DYNAMIC )
			{
				PxRigidDynamic* actor(nullptr);
				SPhysicalFilterData dataToSet = data;
				if( additionalInfo[ chunkResIndex ].m_overrideCollisionMasks )
				{
					additionalInfo[ chunkResIndex ].m_collisionType.RetrieveCollisionMasks( collType, collGroup );
					dataToSet = SPhysicalFilterData( collType, collGroup );
				}
				if( !isDestroyed )
				{			
					actor = physics.createRigidDynamic( pxTransform );
					ASSERT( actor != NULL, TXT("PhysX actor wasn't created") );

					if( actor )
					{
						m_actors[ actorIndex ] = actor;	
						m_actorsAdditionalInfo[ actorIndex ].m_initialVelocity = additionalInfo[ chunkResIndex ].m_initialVelocity;
						if( m_maxAngFractureVelocity > 0.f )
						{
							Vector angVelo = Vector( frandom.Get(m_maxAngFractureVelocity), frandom.Get(m_maxAngFractureVelocity), frandom.Get(m_maxAngFractureVelocity) );
							Float mag = angVelo.SquareMag3();
							if( mag > maxAngVeloSq )
							{
								angVelo = angVelo.Normalized3() * m_maxAngFractureVelocity;
							}
							m_actorsAdditionalInfo[ actorIndex ].m_initialAngularVelocity = angVelo;
						}
						if( flags & EPDF_HasDebrisTimeout )
						{
							m_actorsAdditionalInfo[ actorIndex ].m_debrisChunkTimeout = frandom.Get< Float >( parameters.m_debrisTimeoutMin, parameters.m_debrisTimeoutMax );
						}

						actor->userData = this;
						m_boundsFractured[ actorIndex ] = Box::EMPTY;
					}
					else
					{
						isSucceed = false;
					}
				}
				else
				{
					m_actorsAdditionalInfo[ actorIndex ].SetFlag( EDAF_IsDeleted, true );
				}
				++actorIndexOut;

				if( !isDestroyed )
				{	
					PxShape* shape = CreateShapeSingleMaterial( actorIndex, actor, phxGeometry, material, geometry.m_pose );		
					if( shape )
					{
						shape->setSimulationFilterData( dataToSet.m_data );
						shape->setQueryFilterData( dataToSet.m_data );

						SPhysicalMaterial* physicalMaterial = GetMaterial( actorIndex );
						Float resultDensity = physicalMaterial->m_density * geometry.m_densityScaler;
						shape->setContactOffset( SPhysicsSettings::m_contactOffset );
						shape->setRestOffset( SPhysicsSettings::m_restOffset );

						PxRigidBodyExt::updateMassAndInertia( *actor, &resultDensity, 1 );
						if( actor->getMass() < 0.1f )
						{
							actor->setMass( 0.1f );
						}
					}
					else
					{
						isSucceed = false;
					}
				}
			}
			// if we set the sim type to not dynamic, then all fractured actors are based on resource settings but the first actor is forced to be static
			// unless the m_kinematic flag was set.
			else
			{
				if( (m_simulationType == SM_KINEMATIC || actorIndex ) && additionalInfo[ chunkResIndex ].m_simType == EPDST_Dynamic )
				{
					PxRigidDynamic* actor(nullptr);

					SPhysicalFilterData dataToSet = data;
					if( additionalInfo[ chunkResIndex ].m_overrideCollisionMasks )
					{
						additionalInfo[ chunkResIndex ].m_collisionType.RetrieveCollisionMasks( collType, collGroup );
						dataToSet = SPhysicalFilterData( collType, collGroup );
					}

					if( !isDestroyed )
					{			
						actor = physics.createRigidDynamic( pxTransform );
						ASSERT( actor != NULL, TXT("PhysX actor wasn't created") )

						if( actor )
						{
							m_actors[ actorIndex ] = actor;	
							m_actorsAdditionalInfo[ actorIndex ].m_initialVelocity = additionalInfo[ chunkResIndex ].m_initialVelocity;
							m_actorsAdditionalInfo[ actorIndex ].m_debrisChunkTimeout = frandom.Get< Float >( parameters.m_debrisTimeoutMin, parameters.m_debrisTimeoutMax );

							if( actorIndex == 0 )
							{
								actor->setRigidBodyFlag( PxRigidBodyFlag::eKINEMATIC, true );
							}

							if( m_maxAngFractureVelocity > 0.f )
							{
								Vector angVelo = Vector( frandom.Get(m_maxAngFractureVelocity), frandom.Get(m_maxAngFractureVelocity), frandom.Get(m_maxAngFractureVelocity) );
								Float mag = angVelo.SquareMag3();
								if( mag > maxAngVeloSq )
								{
									angVelo = angVelo.Normalized3() * m_maxAngFractureVelocity;
								}
								m_actorsAdditionalInfo[ actorIndex ].m_initialAngularVelocity = angVelo;
							}

							actor->userData = this;
							m_boundsFractured[ actorIndex ] = Box::EMPTY;
						}
						else
						{
							isSucceed = false;
						}
					}
					else
					{
						m_actorsAdditionalInfo[ actorIndex ].SetFlag( EDAF_IsDeleted, true );
					}
					++actorIndexOut;

					if( !isDestroyed && actor )
					{	
						PxShape* shape = CreateShapeSingleMaterial( actorIndex, actor, phxGeometry, material, geometry.m_pose );		
						if( shape )
						{
							shape->setSimulationFilterData( dataToSet.m_data );
							shape->setQueryFilterData( dataToSet.m_data );

							SPhysicalMaterial* physicalMaterial = GetMaterial( actorIndex );
							Float resultDensity = physicalMaterial->m_density * geometry.m_densityScaler;
							shape->setContactOffset( SPhysicsSettings::m_contactOffset );
							shape->setRestOffset( SPhysicsSettings::m_restOffset );

							PxRigidBodyExt::updateMassAndInertia( *actor, &resultDensity, 1 );
							if( actor->getMass() < 0.1f )
							{
								actor->setMass( 0.1f );
							}
						}
						else
						{
							isSucceed = false;
						}
					}
				}
				//static
				else
				{
					
					PxRigidStatic* actor = physics.createRigidStatic( pxTransform );
					ASSERT( actor != NULL, TXT("PhysX actor wasn't created") )

					if( actor )
					{
						SPhysicalFilterData dataToSet = dataSt;
						if( additionalInfo[ chunkResIndex ].m_overrideCollisionMasks )
						{
							additionalInfo[ chunkResIndex ].m_collisionType.RetrieveCollisionMasks( collType, collGroup );
							dataToSet = SPhysicalFilterData( collType, collGroup );
						}

						m_actors[ actorIndex ] = actor;	
						m_actorsAdditionalInfo[ actorIndex ].SetFlag( EDAF_IsStatic, true );
						actor->userData = this;
						m_boundsFractured[ actorIndex ] = Box::EMPTY;
						++actorIndexOut;

						PxShape* shape = CreateShapeSingleMaterial( actorIndex, actor, phxGeometry, material, geometry.m_pose );
						if( shape )
						{
							shape->setSimulationFilterData( dataToSet.m_data );
							shape->setQueryFilterData( dataToSet.m_data );

							SPhysicalMaterial* physicalMaterial = GetMaterial( actorIndex );
							shape->setContactOffset( SPhysicsSettings::m_contactOffset );
							shape->setRestOffset( SPhysicsSettings::m_restOffset );
						}
						else
						{
							isSucceed = false;
						}
					}
					else
					{
						isSucceed = false;
					}
				}
			}
		}
	}
	return isSucceed;
}

PxShape* CPhysicsDestructionWrapper::CreateShapeSingleMaterial( Uint32 actorIndex, PxRigidActor* rigidActor, PxGeometry* phxGeometry, PxMaterial* material, const Matrix& localPose )
{
#ifndef USE_PHYSX
	return nullptr;
#else

	// Scaling taken from  CPhysicsWrapperInterface::CreateScaledShape()
	// Scaling is applied in the mesh's space, so we need to translate that into the shape's space. Unfortunately,
	// we can't do it perfectly (we can't deal with shearing or anything), but we'll do our best. Works fine for
	// uniform scaling, and mostly fine for shapes oriented to 90 degrees (although capsules and spheres must retain
	// their circular shapes). Arbitrary orientations with non-uniform scaling are less than ideal.
	Matrix scaleMtx = Matrix::IDENTITY;
	scaleMtx.SetScale33( m_scale );

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
		PC_SCOPE_PHYSICS(CreateShapeSingleMaterial issane )

			if ( pxPose.q.isFinite() && !pxPose.q.isSane() )
			{
				pxPose.q.normalize();
			}

			// Check if the pose is sane
			if( !pxPose.q.isSane() )
			{
#ifndef RED_FINAL_BUILD
				CComponent* parent = nullptr;
				GetParent( parent, actorIndex );
				DATA_HALT( DES_Major, CResourceObtainer::GetResource( parent->GetParent() ), TXT("Physical collision"), TXT(" pose is fucked up ") );
#endif
				return nullptr;
			}
	}


#ifndef RED_FINAL_BUILD
	CPhysXLogger::ClearLastError();
#endif

	PxShape* shape = 0;
	// Modify geometry based on what type of shape it is, to add in scaling factor.
	switch ( phxGeometry->getType() )
	{
	case PxGeometryType::eBOX:
		{
			PC_SCOPE_PHYSICS(CreateShapeSingleMaterial box )

			PxBoxGeometry box = *( PxBoxGeometry* )phxGeometry;
			box.halfExtents = box.halfExtents.multiply( TO_PX_VECTOR( finalScale ) );
			shape = rigidActor->createShape( box, &material, 1, pxPose );
			break;
		}

	case PxGeometryType::eCAPSULE:
		{
			PC_SCOPE_PHYSICS(CreateShapeSingleMaterial capsule )

			// Capsules must have a circular cross-section, so we'll just pick the greatest of the Y/Z factors. We can stretch
			// it lengthwise no problem.
			PxCapsuleGeometry capsule = *( PxCapsuleGeometry* )phxGeometry;
			capsule.radius *= Max( finalScale.Y, finalScale.Z );
			capsule.halfHeight *= finalScale.X;
			shape = rigidActor->createShape( capsule, &material, 1, pxPose );
			break;
		}

	case PxGeometryType::eSPHERE:
		{
			PC_SCOPE_PHYSICS(CreateShapeSingleMaterial sphere )

			// Spheres must remain spherical, so we just pick the greatest scale factor.
			PxSphereGeometry sphere = *( PxSphereGeometry* )phxGeometry;
			sphere.radius *= Max( finalScale.X, finalScale.Y, finalScale.Z );
			shape = rigidActor->createShape( sphere, &material, 1, pxPose );
			break;	
		}

	case PxGeometryType::eCONVEXMESH:
		{
			PC_SCOPE_PHYSICS(CreateShapeSingleMaterial convex )

			PxConvexMeshGeometry convex = *( PxConvexMeshGeometry* )phxGeometry;
			convex.scale.scale = convex.scale.scale.multiply( TO_PX_VECTOR(finalScale) );
			shape = rigidActor->createShape( convex, &material, 1, pxPose );
			break;
		}

	case PxGeometryType::eTRIANGLEMESH:
		{
			PC_SCOPE_PHYSICS(CreateShapeSingleMaterial triangle )
			
			PxTriangleMeshGeometry trimesh = *( PxTriangleMeshGeometry* )phxGeometry;
			trimesh.scale.scale = trimesh.scale.scale.multiply( TO_PX_VECTOR(finalScale) );
			shape = rigidActor->createShape( trimesh, &material, 1, pxPose );
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
		CComponent* parent = nullptr;
		GetParent( parent, actorIndex );
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( parent->GetParent() ), TXT("Physical collision"), CPhysXLogger::GetLastErrorString().AsChar() );
		CPhysXLogger::ClearLastError();
	}
#endif

	if( shape ) 
	{
		Int16 shapeIndex = ( Int16 ) rigidActor->getNbShapes() - 1;
		SActorShapeIndex& actorShapeIndex = ( SActorShapeIndex& ) shape->userData;
		actorShapeIndex = SActorShapeIndex( ( Uint16 ) actorIndex, ( Uint16 ) shapeIndex );
	}

	return shape;
#endif
}

void CPhysicsDestructionWrapper::PreSimulation(SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd)
{
	PC_SCOPE_PHYSICS(CPhysicsDestructionWrapper PreSimulation )

	CDestructionComponent* destructionComponent = nullptr;
	if ( !GetParent( destructionComponent ) ) 
	{
		return;
	}

	const SPhysicsDestructionParameters& params = destructionComponent->GetParameters();
	PxBounds3 pxBounds;
	static Uint64 currentTickMarker = 0;
	const float distanceFromViewportSquared = simulationContext->m_resultDistanceSquared;
	Bool isInSimulationDistance = simulationContext->m_desiredDistanceSquared > distanceFromViewportSquared;
	Bool isInRenderDistance = m_autohideDistanceSquared > distanceFromViewportSquared;

	Int32 flags = m_flags.GetValue();

	if( flags & PRBW_PoseIsDirty )
	{
		SetPose( destructionComponent->GetLocalToWorld() );
	}

	if ( isInSimulationDistance )
	{
		// If we are in simulation distance, but we are not ready, it means we were just created
		if( ! ( flags & EPDF_IsReady ) )
		{
			if( allowAdd && ( currentTickMarker < tickMarker || !tickMarker ) ) 
			{
				PC_SCOPE_PHYSICS(CApexDest Create2 )

				Bool createResult = Create( params, destructionComponent->GetCompCollisionBase(), destructionComponent->GetCompCollisionFractured() );
				if( createResult )
				{
					toAdd->PushBack( m_actors[0] );
					if( m_actors[0]->isRigidDynamic() )
					{
						PxRigidDynamic* dynactor = (PxRigidDynamic*)m_actors[0];
						if( params.m_initialBaseVelocity != Vector::ZEROS )
						{
							dynactor->setLinearVelocity( TO_PX_VECTOR( params.m_initialBaseVelocity ) );
						}
						else
						{
							dynactor->putToSleep();
						}
					}

					currentTickMarker = tickMarker;

					CComponent* parent = nullptr;
					GetParent( parent );

					Uint32 count = ForEachSumResult< Uint32 >( parent->GetEntity()->GetComponents()
						, []( CComponent* component )-> int 
					{ 
						CPhysicsWrapperInterface* wrapper = component->GetPhysicsRigidBodyWrapper();
						return ( wrapper && wrapper->UpdatesEntity() ) ? 1 : 0; 
					} );
					SetFlag( PRBW_UpdateEntityPose, count == 1 );
					SetFlag( PRBW_DetailedConntactInfo, true );
					SetDestructionFlag( EPDF_UpdateBounds, true );
					SetDestructionFlag( EPDF_IsReady, true );
					SetDestructionFlag( EPDF_ScheduleUpdateTransform, true );
					if( params.m_isDestroyed )
					{
						SetDestructionFlag( EPDF_RequestFracture, true );
					}
				}				
				return;
			}
			else 
			{
				simulationContext->m_requestProcessingFlag = true;
			}
		}
	}

	if( !m_actorsCount )
	{
		// We are waiting to be initialized or there is no shapes in the resource
		return;
	}

	if( flags & EPDF_ScheduleUpdateKinematicState && m_simulationType != SM_STATIC )
	{
		if( !( flags & EPDF_IsFractured ) )
		{
			m_simulationType = m_updateKinematicStateDecision ? SM_KINEMATIC : SM_DYNAMIC;
			PxRigidDynamic* actor = (PxRigidDynamic*)m_actors[0];
			actor->setRigidBodyFlag( PxRigidBodyFlag::eKINEMATIC,  m_simulationType == SM_KINEMATIC );
			actor->setRigidBodyFlag( PxRigidBodyFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES,  m_simulationType == SM_KINEMATIC );
		}
		SetDestructionFlag( EPDF_ScheduleUpdateKinematicState, false );
	}

	// Update bounds if needed
	if( ( flags & EPDF_UpdateBounds ) )
	{
		if( CalculateBounds() )
		{
			// Notify component
			destructionComponent->OnUpdateBounds();

			if( flags & EPDF_IsFractured ) 
			{
				// update context position since we updated bounds.
				simulationContext->m_x = m_weightedCenter.X;
				simulationContext->m_y = m_weightedCenter.Y;	

				SetDestructionFlag( EPDF_ScheduleUpdateSkinning, true );
			}

			SetDestructionFlag( EPDF_UpdateBounds, false );
		}
	}

	// Check if we damaged enough to fracture
	if( !( flags & EPDF_IsFractured ) )
	{
		// If the damage exceeds endurance, force destruction
		if( m_accumulatedDamage > params.m_damageEndurance )
		{
			SetDestructionFlag( EPDF_RequestFracture, true );
		}
	}

	if( isInSimulationDistance )
	{
		// We came into simulation distance and need to add actors to scene
		if( flags & EPDF_RequestSceneAdd ) 
		{
			if( PushActorsToUpdate( toAdd, true ) )
			{
				SetDestructionFlag( EPDF_RequestSceneAdd, false );
				SetDestructionFlag( EPDF_RequestSceneRemove, true );
				SetDestructionFlag( EPDF_UpdateBounds, true );
			}		
			return;
		}
	}
	else
	{
		// We came out of simulation distance and need to take actors out of scene
		if( flags & EPDF_RequestSceneRemove )
		{
			if( PushActorsToUpdate( toRemove, false ) )
			{
				SetDestructionFlag( EPDF_RequestSceneRemove, false );
			}
			else
			{
				// We are out of sim distance, but we didn't manage to remove all actors due to limits.
				// Request processing of context next frame to finish
				simulationContext->m_requestProcessingFlag = true;
			}
			SetDestructionFlag( EPDF_RequestSceneAdd, true );
		}

		// If we are not in sim distance, end here
		return;
	}

	if( ( flags & EPDF_ScheduleUpdateSkinning ) && ( flags & EPDF_IsFractured ) )
	{
		SetDestructionFlag( EPDF_ScheduleUpdateSkinning, !destructionComponent->UpdateSkinning() );
	}


	if( flags & EPDF_ScheduleEnableDissolve )
	{
		if( !m_isDissolveEnabled )
		{
			if( destructionComponent->EnableDissolve( true ) )
			{
				m_isDissolveEnabled = true;
				SetDestructionFlag( EPDF_ScheduleEnableDissolve, false );
			}
		}
	}


	// Solid 
	if( !( flags & EPDF_IsFractured ) )
	{
		if( flags & EPDF_ScheduleUpdateTransform )
		{
			destructionComponent->ScheduleUpdateTransformNode();

			if( GetFlag( PRBW_UpdateEntityPose ) )
			{
				CEntity* entity = destructionComponent->GetEntity();
				if( !entity ) 
				{
					return;
				}

				PxMat44 mat(  m_actors[0]->getGlobalPose() );
				Matrix transformOrg = TO_MAT( mat);
				Matrix transform = m_componentOffsetInverse * transformOrg;

				const Vector& pos = transform.GetTranslationRef();
				const EulerAngles rot = transform.ToEulerAngles();

#ifndef NO_EDITOR
				if( !GetFlag( PRBW_PoseIsDirty ) )
				{
					entity->SetRawPlacement( &pos, &rot, 0 );
				}
#else
				entity->SetRawPlacement( &pos, &rot, 0 );
#endif
				SetDestructionFlag( EPDF_ScheduleUpdateTransform, false );
			}		
		}

		// Check if we are supposed to begin destruction
		if( flags & EPDF_RequestFracture ) 
		{
			RequestAddingOfFracturedActors( timeDelta, flags);
			SetDestructionFlag( EPDF_IsFractured, true );
			destructionComponent->ScheduleTickPostPhysics();
		}		
	}
	// fractured
	else 
	{
		// if m_toAddIndexes queue is not empty, we are still adding stuff. Stall, to avoid blinking.
		if( m_toAddIndexes.Empty() )
		{
			// We intentionally left those flag up, so we can now refresh proxies once fractured actors should be added to scene
			if( flags & EPDF_RequestFracture ) 
			{
				SetDestructionFlag( EPDF_RequestFracture, false );
				// Rendering mesh changes, so request refresh of proxies
				destructionComponent->RefreshRenderProxies();
				SetDestructionFlag( EPDF_ScheduleUpdateSkinning, true );
				SetDestructionFlag( EPDF_ScheduleUpdateActiveIndices, true );
			}

			// Check if we are supposed to remove debris after a certain timeout
			if( ( flags & EPDF_HasDebrisTimeout ) && ( m_debrisLifetimeAcc > params.m_debrisTimeoutMin ) )
			{
				if( currentTickMarker < tickMarker || !tickMarker )
				{
					if( !m_isDissolveEnabled )
					{
						SetDestructionFlag( EPDF_ScheduleEnableDissolve, true );
					}

					Bool allIsDone = true, updateSkinning = false;
					const PxF32 fadeSpeed = params.m_fadeOutTime <= 0.0f ? PX_MAX_F32 : ( 1.0f / params.m_fadeOutTime );
					for( Uint16 i = 1; i < m_actorsCount; ++i)
					{
						SDestructionActorInfo& addInfo = m_actorsAdditionalInfo[i];
						// if debris chunk is static, it stays on scene
						if( !addInfo.GetFlag( EDAF_IsStatic ) )
						{
							// if it's already been deleted, we omit it as well
							if( !addInfo.GetFlag(EDAF_IsDeleted) )
							{
								// this will be eventually processed, so we update allIsDone flag
								allIsDone = false;
								if( addInfo.m_debrisChunkTimeout < m_debrisLifetimeAcc )
								{				
									// We process fadeout here, separately to the one in UpdateScene, because the timeout fadeout can be different
									// then the regular one (for cases like flying out of sim distance, which needs to be pretty fast)
									if( addInfo.m_chunkAlpha < FLT_EPSILON )
									{									
										m_toRemoveIndexes.Push( i );	
										// fully faded out fragment, update active indices
										SetDestructionFlag( EPDF_ScheduleUpdateActiveIndices, true );
									}
									else
									{
										addInfo.m_chunkAlpha -=  timeDelta * fadeSpeed;
										addInfo.m_chunkAlpha = addInfo.m_chunkAlpha < 0.0f ? 0.0f : addInfo.m_chunkAlpha;
									}
									updateSkinning = true;
								}								
							}
						}
					}
					if( updateSkinning )
					{
						SetDestructionFlag( EPDF_ScheduleUpdateSkinning, true );
					}
					if( allIsDone )
					{
						SetDestructionFlag( EPDF_HasDebrisTimeout, false );
					}
				}
			}
			m_debrisLifetimeAcc += timeDelta;
		}
	}

	// Update toAdd and toRemove arrays based on our gathered data
	UpdateScene(toAdd, toRemove, timeDelta, SPhysicsSettings::m_destructionFadeOutTime );
}

void CPhysicsDestructionWrapper::RequestAddingOfFracturedActors( Float timeDelta, Int32 flags )
{
	PC_SCOPE_PHYSICS(CPhysicsDestructionWrapper RequestAddingOfFracturedActors )
	
	m_toRemoveIndexes.Push( 0 );
	PxRigidActor* notFracturedActor = m_actors[ 0 ];
	PxVec3 destructionForce = TO_PX_VECTOR( m_forceOnDestruction );

	for( Uint16 i = 1; i < m_actorsCount; ++i)
	{
		if( m_actorsAdditionalInfo[ i ].GetFlag( EDAF_IsDeleted ) )
		{
			continue;
		}
		PxRigidActor* actor = m_actors[i];
		if(  actor->isRigidDynamic() )
		{		
			PxRigidDynamic* dynActor = (PxRigidDynamic*)actor;
			if( !( dynActor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) )
			{
				if( ( flags & EPDF_HasInitialFractureVelocity ) )
				{	
					// Set the initial velocity - this is done to allow making of predefined fracture effects.
					dynActor->setLinearVelocity( TO_PX_VECTOR( m_actorsAdditionalInfo[i].m_initialVelocity ) );
				}		
				else
				{
					// If we don't have predefined initial velocity, we calculate it based on the force that caused fracturing.
					// This is done, to accomodate for the fact, that we throttle the addition of actors to the scene - therefore,
					// if we destroy few walls with a lot of chunks at the same time, it might take multiple frames for them to be added and the force
					// might fade out in that time. This would cause some debris to drop straight down on the ground, unless we set the velocity manually.
					Float mass = dynActor->getMass();
					PxVec3 velo = ( destructionForce * timeDelta ) / mass;

					if( ( flags & EPDF_HasMaxVelocity ) )
					{
						Float mag = velo.magnitudeSquared();
						if( mag > m_maxVelocity*m_maxVelocity )
						{
							velo = velo.getNormalized() * m_maxVelocity;
						}						
					}		

					dynActor->setLinearVelocity( velo );
				}

				if( m_maxAngFractureVelocity > 0.f )
				{		
					dynActor->setAngularVelocity( TO_PX_VECTOR( m_actorsAdditionalInfo[i].m_initialAngularVelocity ));
				}				
			}
		}
		actor->setGlobalPose( notFracturedActor->getGlobalPose() );
		m_toAddIndexes.Push( i );		
	}
}

void CPhysicsDestructionWrapper::PostSimulationUpdateTransform( const Matrix& transformOrg, void* actor )
{
	PC_SCOPE_PHYSICS(CPhysicsDestructionWrapper PostSimulationUpdateTransform )

	if( !m_actorsCount ) 
	{
		return;
	}

	CDestructionComponent* component = nullptr;
	if ( !GetParent( component ) ) 
	{
		return;
	}

	component->ScheduleUpdateTransformNode();
	Vector refPos = m_world->GetReferencePosition();
	const SPhysicsDestructionParameters& params = component->GetParameters();
	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsDestructionWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );

	Int32 flags = m_flags.GetValue();

	PxRigidActor* rigidActor = ( PxRigidActor* ) actor;
	if( rigidActor && rigidActor->getNbShapes() )
	{
		PxShape* shape = 0;
		rigidActor->getShapes( &shape, 1, 0 );
		SActorShapeIndex& index = ( SActorShapeIndex& ) shape->userData;

		// If we have max velocity set, check if a chunk actor is not exceeding it
		if( flags & EPDF_HasMaxVelocity ) 
		{
			PxRigidDynamic* dynActor = (PxRigidDynamic*)actor;
			if( !( dynActor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) )
			{
				PxVec3 velo = dynActor->getLinearVelocity();
				Float mag = velo.magnitudeSquared();
				if( mag > m_maxVelocity*m_maxVelocity )
				{
					PxVec3 maxVelo = velo.getNormalized() * m_maxVelocity;
					dynActor->setLinearVelocity( maxVelo );
				}

			}
		}		
		
		if( index.m_actorIndex > 0 )
		{
			PxVec3 pos = rigidActor->getGlobalPose().p;

			if( flags & EPDF_RequestFracture ) 
			{
				// If not all are added, weighted center is not calculated yet.
				return;
			}

			// Calculate squared distance of the actor from the "center of mass" - a pseudo-center point inside the bbox influenced by position
			// of all active debris actors
			Float distanceSquared = ( (m_weightedCenter.X - pos.x) * (m_weightedCenter.X - pos.x) ) 
				+ ( (m_weightedCenter.Y - pos.y) * (m_weightedCenter.Y - pos.y) ) 
				+ ( (m_weightedCenter.Z - pos.z) * (m_weightedCenter.Z - pos.z) );

			if( distanceSquared >= 0.0f )
			{
				Float desiredDistance = params.m_debrisMaxSeparation*params.m_debrisMaxSeparation;
				desiredDistance = Min( desiredDistance, position->m_desiredDistanceSquared );
				// If we are exceeding max separation, we need to remove and destroy that actor
				if( ( distanceSquared > desiredDistance ) ) 
				{
					if( rigidActor->getScene() )
					{
						m_toRemoveIndexes.Push( index.m_actorIndex );
						if( !m_isDissolveEnabled )
						{
							SetDestructionFlag( EPDF_ScheduleEnableDissolve, true );
						}
						return;
					}
				}
				else
				{
					// We are in the distance, so update this fractured actor bbox and set the flag, that we should recalculate destruction bounds
					if( rigidActor->getScene() )
					{
						PxBounds3 bounds = rigidActor->getWorldBounds();
						m_boundsFractured[ index.m_actorIndex ] = Box( TO_VECTOR( bounds.minimum ), TO_VECTOR( bounds.maximum ) );
						SetDestructionFlag( EPDF_UpdateBounds, true );
					}
				}
			}
			
			return; //only first actor, which is unfractured actor, should have influence on parent entity pose
		}
	}
	else
	{
		return;
	}

	if( !( flags & PRBW_UpdateEntityPose ) ) return;

	CEntity* entity = component->GetEntity();
	if( !entity ) 
	{
		return;
	}

	Matrix transform = m_componentOffsetInverse * transformOrg;

	const Vector& pos = transform.GetTranslationRef();
	const EulerAngles rot = transform.ToEulerAngles();

#ifndef NO_EDITOR
	if( !( flags & PRBW_PoseIsDirty ) )
	{
		entity->SetRawPlacement( &pos, &rot, 0 );
	}
#else
	entity->SetRawPlacement( &pos, &rot, 0 );
#endif

	CalculateBounds();
	position->m_x = pos.X;
	position->m_y = pos.Y;
}

Bool CPhysicsDestructionWrapper::PushActorsToUpdate( TDynArray< void* >* toUpdate, Bool isAdding )
{
	if( !GetDestructionFlag( EPDF_IsFractured ) )
	{
		if( !m_actorsAdditionalInfo[ 0 ].GetFlag( EDAF_IsDeleted ) )
		{
			PxRigidActor* actor = m_actors[ 0 ];
			if( actor )
			{
				Bool isOnScene = actor->getScene() != nullptr ;
				if( ( isAdding != isOnScene ) )
				{
					toUpdate->PushBack( actor );
					if( toUpdate->Size() > MAX_ACTORS_PER_FRAME )
					{
						return false;
					}
				}
			}
		}
	}
	else
	{
		for( Uint16 i = 1; i < m_actorsCount; ++i)
		{
			if( !m_actorsAdditionalInfo[ i ].GetFlag( EDAF_IsDeleted ) )
			{
				PxRigidActor* actor = m_actors[ i ];
				if( actor )
				{
					Bool isOnScene = actor->getScene() != nullptr;
					if( ( isAdding != isOnScene ) )
					{
						toUpdate->PushBack( actor );
						if( !isAdding )
						{
							if( actor->isRigidDynamic() )
							{
								PxRigidDynamic* dynaActor = (PxRigidDynamic*) actor;
								dynaActor->setLinearVelocity(  PxVec3(0) );
								dynaActor->setAngularVelocity( PxVec3(0) );
							}
						}
						if( toUpdate->Size() > MAX_ACTORS_PER_FRAME )
						{
							return false;
						}
					}
				}
			}
		}
	}
	return true;
}

void CPhysicsDestructionWrapper::UpdateScene( TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, Float timeDelta, Float fadeOutTime )
{
	Bool allRequested = true, updateSkinning = false;
	while( !m_toAddIndexes.Empty() )
	{
		Uint16 index = m_toAddIndexes.Front();
		m_toAddIndexes.Pop();
		PxRigidActor* actor = m_actors[ index ];
		if( actor && !actor->getScene() && !m_actorsAdditionalInfo[ index ].GetFlag( EDAF_IsDeleted ) )
		{
			toAdd->PushBack( actor );
			SetDestructionFlag( EPDF_UpdateBounds, true );
			updateSkinning = true;
			if( toAdd->Size() > MAX_ACTORS_PER_FRAME )
			{
				// We reached frame limit for adding
				break;
			}
		}		
	}

	Uint32 size = m_toRemoveIndexes.Size();
	const PxF32 fadeSpeed = fadeOutTime <= 0.0f ? PX_MAX_F32 : ( 1.0f / fadeOutTime );
	while( size > 0 )
	{
		Uint16 index = m_toRemoveIndexes.Front();
		m_toRemoveIndexes.Pop();
		PxRigidActor* actor = m_actors[ index ];
		SDestructionActorInfo& addInfo = m_actorsAdditionalInfo[ index ];
		// Request removing from scene
		if( actor && actor->getScene() && !addInfo.GetFlag( EDAF_IsDeleted ) )
		{
			if( toRemove->Size() < MAX_ACTORS_PER_FRAME )
			{
				toRemove->PushBack( actor );
				addInfo.SetFlag( EDAF_IsFadingOut, true );
				addInfo.SetFlag( EDAF_IsDeleted, true );
				SetDestructionFlag( EPDF_UpdateBounds, true );
				updateSkinning = true;
			}			
		}
			updateSkinning = true;

		// Update fadeout
		if( addInfo.GetFlag( EDAF_IsFadingOut ) )
		{
			updateSkinning = true;
			if( addInfo.m_chunkAlpha < FLT_EPSILON )
			{									
				addInfo.SetFlag( EDAF_IsFadingOut, false);
				// fully faded out fragment, update active indices
				SetDestructionFlag( EPDF_ScheduleUpdateActiveIndices, true );
			}
			else
			{
				addInfo.m_chunkAlpha -=  timeDelta * fadeSpeed;
				addInfo.m_chunkAlpha = addInfo.m_chunkAlpha < 0.0f ? 0.0f : addInfo.m_chunkAlpha;
				m_toRemoveIndexes.Push( index );
			}
		}
		--size;
	}

	if( updateSkinning )
	{
		SetDestructionFlag( EPDF_ScheduleUpdateSkinning, true );
	}
}

Bool CPhysicsDestructionWrapper::MakeReadyToDestroy(TDynArray< void* >* toRemove)
{
	if( !m_actors.Size() ) 
	{
		return true;
	}
	Bool result = true;
	{
		PC_SCOPE_PHYSICS(CApexDestructionWrapper MakeReadyToDestroy )

		for( Uint16 i = 0; i < m_actorsCount; ++i)
		{
			PxActor* actor = m_actors[i];
			if( !actor ) continue;
			if( actor->getScene() )
			{
				toRemove->PushBack( actor );
				result = false;
			}
		}
	}

	return result;
}

void CPhysicsDestructionWrapper::ApplyFractureByVolume(Uint16 triggeredActorIndex, Float applyFractureDamage, CPhantomComponent* triggerComponent, Vector forceOnDestruction )
{
	Int32 flags = m_flags.GetValue();
	if( !( flags & ( EPDF_IsFractured | EPDF_RequestFracture )) )
	{
		m_forceOnDestruction = forceOnDestruction;
		ApplyDamage( applyFractureDamage );	
		CPhysicsWrapperInterface::ApplyForce(forceOnDestruction, triggeredActorIndex);
	}
}

Bool CPhysicsDestructionWrapper::ApplyForce(const Vector& force, Uint32 actorIndex)
{
	Int32 flags = m_flags.GetValue();
	if( !( flags & ( EPDF_IsFractured | EPDF_RequestFracture )) )
	{
		Float damage = m_forceToDamage * force.Mag3();
		m_forceOnDestruction = force;
		ApplyDamage( damage );		
	}
	return CPhysicsWrapperInterface::ApplyForce(force, actorIndex);
}

void CPhysicsDestructionWrapper::ApplyDamage(Float damage)
{
	if( m_damageThreshold < damage )
	{
		if( !GetDestructionFlag( EPDF_AccumulatesDamage ) )
		{
			m_accumulatedDamage = damage;
		}
		else 
		{
			m_accumulatedDamage += damage;
		}
	}	
}

Box CPhysicsDestructionWrapper::GetBounds()
{
	return m_bounds;
}

Bool CPhysicsDestructionWrapper::CalculateBounds()
{
	if( !m_actorsCount ) return false;
	if( !GetDestructionFlag( EPDF_IsFractured ) )
	{
		const PxRigidActor* actor = m_actors[ 0 ];
		if( actor && actor->getScene() )
		{
			PxBounds3 bounds = actor->getWorldBounds();
			Vector min = TO_VECTOR( bounds.minimum );
			Vector max = TO_VECTOR( bounds.maximum );
			m_bounds = Box( min, max );
		}
		else
		{
			return false;
		}
	}
	else if( GetFracturedActorsCount() )
	{
		m_bounds = Box::EMPTY;

		// When the destruction is fractured, the simulation context position could be either in center of bbox or in the 
		// "weighted" center, which is inside the bbox, but closer to where most of debris is. From testing, the second solution
		// gives better results, but it's not set in stone that it'll be final.
		Vector weightedCenter = Vector::ZEROS;
		Int32 count = 0;
		for( Uint32 i = 1; i < m_actorsCount; ++i )
		{
			const PxRigidActor* actor = m_actors[ i ];
			if( actor && actor->getScene() && !m_actorsAdditionalInfo[ i ].GetFlag( EDAF_IsDeleted ))
			{
				Box bounds = m_boundsFractured[ i ];
				if( bounds == Box::EMPTY )
				{
					// Actor hasn't moved or first frame after fracture so post sim wasnt called yet.
					PxBounds3 boundsActor = actor->getWorldBounds();
					bounds = Box( TO_VECTOR( boundsActor.minimum), TO_VECTOR( boundsActor.maximum) );
				}
				weightedCenter += bounds.CalcCenter();
				count += 1;

				m_bounds.AddBox( bounds );
			}
		}
		if( count )
		{
			m_weightedCenter = weightedCenter / (Float)count;
		}
	}
	return true;
}

Bool CPhysicsDestructionWrapper::SampleBonesModelSpace( TDynArray<Matrix>& poseInOut, TDynArray< Bool >& bonesActive ) 
{
	PC_SCOPE_PHYSICS(CPhysicsDestructionWrapper SampleBonesModelSpace )
		
		Int32 flags = m_flags.GetValue();
		if (  flags & EPDF_IsFractured  )
		{
			Matrix invertWorldMat;
			CComponent* parentPtr;
			GetParent( parentPtr );
			if( parentPtr ) 
			{			
				parentPtr->GetWorldToLocal( invertWorldMat );
#ifdef USE_PHYSX
				Int32 numMappings = GetFracturedActorsCount();
				bonesActive.Clear();
				bonesActive.Resize( numMappings );
				if( numMappings <= 0 )
				{
					return false;
				}
				poseInOut.Resize( numMappings );
				for ( Int32 i = 0; i < numMappings; ++i )
				{
					Int32 index = i + 1;
					const PxRigidActor* actor = m_actors[index];
					poseInOut[ i ] = Matrix::IDENTITY;
					
					if( !actor )
					{
						continue;
					}

					if( !actor->isRigidDynamic() )
					{
						PxMat44 mat = PxMat44( actor->getGlobalPose() );
#ifdef _DEBUG
						Matrix globalPartPose = TO_MAT( mat );
						Matrix modelPartPoseInverted = globalPartPose * invertWorldMat;
						poseInOut[ i ] = modelPartPoseInverted;
#else
						poseInOut[ i ] =  TO_MAT( mat) * invertWorldMat;	
						poseInOut[ i ].SetScale33( m_scale );

#endif
					}
					else
					{
						const PxRigidDynamic* dynamic = (PxRigidDynamic*)m_actors[index];

						if( !( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) )
						{
							PxMat44 mat = PxMat44( dynamic->getGlobalPose() );
#ifdef _DEBUG
							Matrix globalPartPose = TO_MAT( mat );
							Matrix modelPartPoseInverted = globalPartPose * invertWorldMat;
							poseInOut[ i ] = modelPartPoseInverted;
#else
							poseInOut[ i ] =  TO_MAT( mat) * invertWorldMat;	
							poseInOut[ i ].SetScale33( m_scale );
#endif
						}
					}

					if( m_isDissolveEnabled )
					{
						Vector row = poseInOut[i].GetRow( 3 );
						row.W = m_actorsAdditionalInfo[index].m_chunkAlpha;
						poseInOut[ i ].SetRow(3, row );
					}

					if( !m_actorsAdditionalInfo[ index ].GetFlag( EDAF_IsDeleted ) || m_actorsAdditionalInfo[ index ].GetFlag( EDAF_IsFadingOut ) )
					{
						bonesActive[ i ] = true;
					}
					else
					{
						bonesActive[ i ] = false;
					}
				}
#endif
			}
		}
		// Reset update indices flag
		flags = m_flags.GetValue();
		SetDestructionFlag(EPDF_ScheduleUpdateActiveIndices, false);
		return (flags & EPDF_ScheduleUpdateActiveIndices) > 0;
}

void CPhysicsDestructionWrapper::ScheduleSkinningUpdate()
{
	SetDestructionFlag( EPDF_ScheduleUpdateSkinning, true );
}

void CPhysicsDestructionWrapper::SetAutoHideDistance(Float desiredDistance )
{
	m_autohideDistanceSquared = desiredDistance * desiredDistance;
}

Bool CPhysicsDestructionWrapper::GetDestructionFlag( EPhysicsDestructionFlags flag ) const
{
	return ( m_flags.GetValue() & flag ) > 0;
}

void CPhysicsDestructionWrapper::SetDestructionFlag( EPhysicsDestructionFlags flag, Bool decision )
{
	if( decision )
	{
		m_flags.Or( flag );
	}
	else
	{
		m_flags.And( 0xFFFFFFFF ^ flag );
	}
}

void CPhysicsDestructionWrapper::SwitchToKinematic(Bool decision)
{	
	m_updateKinematicStateDecision = decision;
	SetDestructionFlag( EPDF_ScheduleUpdateKinematicState, true );
}

Bool CPhysicsDestructionWrapper::ApplyFracture()
{
	if( !GetDestructionFlag( EPDF_IsFractured ) )
	{
		SetDestructionFlag( EPDF_RequestFracture, true );
		return true;
	}
	return false;
}

#ifndef NO_EDITOR 

const TDynArray<Float> CPhysicsDestructionWrapper::GetMasses()
{
	TDynArray<Float> masses;
	for( Uint32 i = 0; i < m_actorsCount; ++i )
	{
		masses.PushBack( GetMass(i) );
	}
	return masses;
}

#endif