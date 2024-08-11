/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "physicsTileWrapper.h"
#include "..\physics\physicsWorldPhysXImpl.h"
#include "..\physics\physicsSettings.h"
#include "foliageInstance.h"
#include "..\core\dataError.h"
#include "../physics/PhysicsWrappersDefinition.h"
#include "physicsDataProviders.h"

DECLARE_PHYSICS_WRAPPER(CPhysicsTileWrapper,EPW_Tile,false,false)
#pragma warning( disable: 4756 )

#ifdef USE_PHYSX

using namespace physx;

class CPhysxStaticActorCreateTask : public physx::PxTask
{
protected:
	struct SData
	{
		CompiledCollisionPtr m_compiledCollision;
		PxRigidStatic* m_staticActor;
		Matrix m_localToWorld;
		Uint32 m_actorIndex;
		CPhysicsEngine::CollisionMask m_collisionType;
		CPhysicsEngine::CollisionMask m_collisionGroup;
		Box2 m_box;
	};
	TDynArray< SData > m_data;
	CPhysicsWorldPhysXImpl* m_world;

	Bool m_processed;
public:
	virtual const char*	getName() const { return "CPhysxStaticActorCreateTask\0"; }

	Red::Threads::CAtomic< Uint32 >* m_backgroundTasksProcessing;

	void release()
	{
		if( PxTaskManager* save = mTm )
		{
			save->taskCompleted( *this );
		}
		m_backgroundTasksProcessing->Decrement();
	}

	void run() override
	{
		m_processed = true;
		Uint32 count = m_data.Size();
		for( Uint32 i = 0; i != count; ++i )
		{
			SData& data = m_data[ i ];
			Vector xAxis = data.m_localToWorld.V[ 0 ];
			Vector yAxis = data.m_localToWorld.V[ 1 ];
			Vector zAxis = data.m_localToWorld.V[ 2 ];
			Vector pos = data.m_localToWorld.V[ 3 ];

			Vector scale( xAxis.Normalize3(), yAxis.Normalize3(), zAxis.Normalize3() );

			PxMat44 mat( TO_PX_VECTOR( xAxis ), TO_PX_VECTOR( yAxis ), TO_PX_VECTOR( zAxis ), TO_PX_VECTOR( pos ) );
			PxTransform pose( mat );
			pose.q.normalize();

			data.m_staticActor->setGlobalPose( pose );

			const auto & geometries = data.m_compiledCollision->GetGeometries();
			for ( Uint32 i = 0; i < geometries.Size(); ++i )
			{
				const SCachedGeometry& geometry = geometries[ i ];
				if( !geometry.GetGeometry() ) continue;

				PxGeometry* phxGeometry = ( PxGeometry* ) geometry.GetGeometry();
				Uint16 materialsCount;
				const CName* materialNames;
				if ( geometry.m_physicalMultiMaterials.Empty() )
				{
					materialsCount = 1;
					materialNames = &geometry.m_physicalSingleMaterial;
				}
				else
				{
					materialsCount = ( Uint16 ) geometry.m_physicalMultiMaterials.Size();
					materialNames = &geometry.m_physicalMultiMaterials[ 0 ];
				}

				TDynArray< PxMaterial* > materials;
				materials.Reserve( materialsCount );

				for ( Uint32 j = 0; j != materialsCount; ++j )
				{
					PxMaterial* material = GPhysXEngine->GetMaterial( materialNames[ j ] );
					if ( !material ) material = GPhysXEngine->GetMaterial();
					materials.PushBack( material ? material : GPhysXEngine->GetMaterial() );
				}

				Matrix scaleMtx = Matrix::IDENTITY;
				scaleMtx.SetScale33( scale );

				Matrix scaledTransform = geometry.m_pose * scaleMtx;
				Vector finalScale = scaledTransform.GetScale33();
				Matrix finalTransform = geometry.m_pose;
				finalTransform.SetTranslation( scaledTransform.GetTranslationRef() );

				PxTransform pxPose = PxTransform( TO_PX_MAT(finalTransform) );
				{
					if ( pxPose.q.isFinite() && !pxPose.q.isSane() )
					{
						pxPose.q.normalize();
					}
				}

				PxShape* shape = 0;
				// Modify geometry based on what type of shape it is, to add in scaling factor.
				switch ( phxGeometry->getType() )
				{
				case PxGeometryType::eBOX:
					{
						PxBoxGeometry box = *( PxBoxGeometry* )phxGeometry;
						box.halfExtents = box.halfExtents.multiply( TO_PX_VECTOR( finalScale ) );
						shape = data.m_staticActor->createShape( box, &materials[ 0 ], materialsCount, pxPose );
						break;
					}

				case PxGeometryType::eCAPSULE:
					{
						PxCapsuleGeometry capsule = *( PxCapsuleGeometry* )phxGeometry;
						capsule.radius *= Max( finalScale.Y, finalScale.Z );
						capsule.halfHeight *= finalScale.X;
						shape = data.m_staticActor->createShape( capsule, &materials[ 0 ], materialsCount, pxPose );
						break;
					}

				case PxGeometryType::eSPHERE:
					{
						PxSphereGeometry sphere = *( PxSphereGeometry* )phxGeometry;
						sphere.radius *= Max( finalScale.X, finalScale.Y, finalScale.Z );
						shape = data.m_staticActor->createShape( sphere, &materials[ 0 ], materialsCount, pxPose );
						break;	
					}

				case PxGeometryType::eCONVEXMESH:
					{
						PxConvexMeshGeometry convex = *( PxConvexMeshGeometry* )phxGeometry;
						convex.scale.scale = convex.scale.scale.multiply( TO_PX_VECTOR(finalScale) );
						shape = data.m_staticActor->createShape( convex, &materials[ 0 ], materialsCount, pxPose );
						break;
					}

				case PxGeometryType::eTRIANGLEMESH:
					{
						PxTriangleMeshGeometry trimesh = *( PxTriangleMeshGeometry* )phxGeometry;
						trimesh.scale.scale = trimesh.scale.scale.multiply( TO_PX_VECTOR(finalScale) );
						shape = data.m_staticActor->createShape( trimesh, &materials[ 0 ], materialsCount, pxPose );
						break;
					}
				}

				Int16 shapeIndex = ( Int16 ) data.m_staticActor->getNbShapes() - 1;
				SActorShapeIndex& actorShapeIndex = ( SActorShapeIndex& ) shape->userData;
				actorShapeIndex = SActorShapeIndex( ( Uint16 ) data.m_actorIndex, ( Uint16 ) shapeIndex );

				SPhysicalFilterData filterData( data.m_collisionType, data.m_collisionGroup );
				shape->setSimulationFilterData( filterData.m_data );
				shape->setQueryFilterData( filterData.m_data );
				PxMaterial* material = 0;
				shape->getMaterials( &material, 1 );
				SPhysicalMaterial* physicalMaterial = ( SPhysicalMaterial* ) material->userData;
				Float resultDensity = physicalMaterial->m_density * geometry.m_densityScaler;
				shape->setContactOffset( SPhysicsSettings::m_contactOffset );
				shape->setRestOffset( SPhysicsSettings::m_restOffset );
			}


			PxBounds3 actorBounds = data.m_staticActor->getWorldBounds();
			Box2 bounds( actorBounds.minimum.x, actorBounds.minimum.y, actorBounds.maximum.x, actorBounds.maximum.y );
			Uint64& sectorMask = ( Uint64& ) data.m_staticActor->userData;
			if( data.m_box.Contains( bounds ) )
			{
				Vector2 dimensions = data.m_box.Max - data.m_box.Min;
				Vector2 sectorSize = dimensions / 8;
				Uint8 counter = 0;
				for( Uint8 y = 0; y != 8 ; ++y )
					for( Uint8 x = 0; x != 8; ++x )
					{
						Box2 sector( data.m_box.Min.X + sectorSize.X * x, data.m_box.Min.Y + sectorSize.Y * y, data.m_box.Min.X + sectorSize.X * ( x + 1 ), data.m_box.Min.Y + sectorSize.Y * ( y + 1 ) );
						if( sector.Contains( bounds ) )
						{
							sectorMask |= 0x1LL << counter;
						}

						if( bounds.Contains( sector ) )
						{
							sectorMask |= 0x1LL << counter;
						}

						if( bounds.Intersects( sector ) )
						{
							sectorMask |= 0x1LL << counter;
						}

						++counter;
					}
			}

			float diagonalLimit = data.m_compiledCollision->GetOcclusionDiagonalLimit();
			float attenuation = data.m_compiledCollision->GetOcclusionAttenuation();
			Float currentDiameter = actorBounds.getDimensions().magnitude();
			Bool isOccluding = diagonalLimit == -1 && attenuation == -1 ? false : diagonalLimit <= currentDiameter;

			Uint32 shapesCount = data.m_staticActor->getNbShapes();
			for( Uint32 j = 0; j != shapesCount; ++j )
			{
				PxShape* shape = nullptr;
				data.m_staticActor->getShapes( &shape, 1, j );
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
		}
	}

public:
	CPhysxStaticActorCreateTask( CPhysicsWorldPhysXImpl* world ) : m_world( world ), m_processed( false )
	{
		m_backgroundTasksProcessing = world->GetFetchStartSyncInterlock();
		m_backgroundTasksProcessing->Increment();
	}

	~CPhysxStaticActorCreateTask()
	{
	}

	void AddData( PxRigidStatic* actor, Uint32 actorIndex, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup, CompiledCollisionPtr& compiledCollision, const Matrix& localToWorld, const Box2& box )
	{
		m_data.PushBack( SData() );
		SData& data = m_data.Back();
		data.m_compiledCollision = compiledCollision;
		data.m_localToWorld = localToWorld;
		data.m_collisionType = collisionType;
		data.m_collisionGroup = collisionGroup;
		data.m_staticActor = actor;
		data.m_actorIndex = actorIndex;
		data.m_box = box;
	}

	CPhysicsWorldPhysXImpl* GetWorld() { return m_world; }

};
#endif

class CPhysxStaticActorCreateTask* CPhysicsTileWrapper::m_task = nullptr;


CPhysicsTileWrapper::CPhysicsTileWrapper( CPhysicsWorld* world, Box2 box )
	: CPhysicsWrapperInterface()
	, m_box( box )
	, m_sectorMaskProcessed( 0 )
{
	m_world = world;

	m_ref.SetValue( 0 );
#ifdef NO_EDITOR
	m_staticBodies.Reserve( 1024 );
#endif

	m_minIndex = 0;

	PHYSICS_STATISTICS_INC(TerrainTilesInstanced)

	m_simulationType = SM_STATIC;
	
	m_collisionType = GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
	m_collisionGroup = GPhysicEngine->GetCollisionGroupMask( m_collisionType );

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	Vector center = box.Max + box.Min;
	position->m_x = box.Max.X + box.Min.X;
	position->m_x *= 0.5;
	position->m_y = box.Max.Y + box.Min.Y;
	position->m_y *= 0.5;
	position->m_desiredDistanceSquared = m_box.CalcSize().SquareMag() * 2;
	position->m_resultDistanceSquared = FLT_MAX;
	position->m_visibilityQueryId = 0;

	Vector2 pos( box.Max );
	Red::Threads::CAtomic< CPhysicsTileWrapper* >* atomic = m_world->GetTerrainTileWrapperAtomic( pos );
	RED_FATAL_ASSERT( atomic->GetValue() == nullptr, "terrain tile atomic already used" );
	atomic->SetValue( this );
}

CPhysicsTileWrapper::~CPhysicsTileWrapper()
{
	PHYSICS_STATISTICS_DEC(TerrainTilesInstanced)

#ifdef USE_PHYSX
	for( Uint32 i = 0; i != m_staticBodies.Size(); ++i )
	{
		PxRigidActor* rigidActor = ( PxRigidActor* ) m_staticBodies[ i ].m_actor;
		if( !rigidActor ) continue;
		rigidActor->userData = 0;
		if( PxScene* scene = rigidActor->getScene() )
		{
			scene->removeActor( *rigidActor );
		}
		rigidActor->release();
		m_staticBodies[ i ].m_actor = 0;
		m_staticBodies[ i ].m_compiledCollision.Reset();
		PHYSICS_STATISTICS_DEC(StaticBodiesInstanced)
	}
	m_staticBodies.ClearFast();
#endif
}

void CPhysicsTileWrapper::PrepereStaticBodyCreationQue( CPhysicsWorld* world )
{
	if( m_task ) return;
#ifdef USE_PHYSX
	CPhysicsWorldPhysXImpl* physicsWorld = static_cast< CPhysicsWorldPhysXImpl* >( world );
	m_task = physicsWorld->PushCompletionTask< CPhysxStaticActorCreateTask >();
#endif
}

void CPhysicsTileWrapper::FlushStaticBodyCreationQue()
{
	if( !m_task ) return;

#ifdef USE_PHYSX
	if( m_task->getTaskManager() )
	{
		m_task->removeReference();
	}
#endif
	m_task = nullptr;
}

void CPhysicsTileWrapper::Release( Uint32 actorIndex )
{
	PC_SCOPE_PHYSICS( Physics Tile Wrapper Release );

#ifdef USE_PHYSX
	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();
	if( m_staticBodies.Size() <= actorIndex ) return;

	CObject* object = nullptr;
	IPhysicsWrapperParentProvider* provider = ( IPhysicsWrapperParentProvider* ) m_staticBodies[ actorIndex ].m_parentHook;
	provider->GetParent( object );

	if( !object ) return;

	SetFlag( PRBW_PoseIsDirty, true );

	*provider = CPhysicsWrapperParentComponentProvider( nullptr );

#endif

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	position->m_desiredDistanceSquared = FLT_MAX;

	RED_ASSERT( m_ref.GetValue() > 0 )
	if( !m_ref.Decrement() )
	{
		Red::Threads::CAtomic< CPhysicsTileWrapper* >* atomic = m_world->GetTerrainTileWrapperAtomic( Vector2( position->m_x, position->m_y ) );
		RED_FATAL_ASSERT( atomic->GetValue() == this, "wrong terrain tile atomic taken" );
		atomic->SetValue( nullptr );
		m_world->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->PushWrapperToRemove( this );
		m_world = nullptr;
	}
}

void CPhysicsTileWrapper::Dispose( const CObject* owner )
{
	if( !owner ) return;
#ifdef USE_PHYSX
	Uint32 count = m_staticBodies.Size();
	for( Uint32 i = 0; i != count; ++i )
	{
		SStaticBodyStruct& body = m_staticBodies[ i ];
		CObject* object = nullptr;
		IPhysicsWrapperParentProvider* provider = ( IPhysicsWrapperParentProvider* ) body.m_parentHook;
		provider->GetParent( object );
		if( object != owner ) continue;
		Release( i );
	}
#endif
}

void CPhysicsTileWrapper::DisposeArea( const CObject* owner, const Box2& box )
{
	if( !owner ) return;
#ifdef USE_PHYSX
	Uint32 count = m_staticBodies.Size();
	for( Uint32 i = 0; i != count; ++i )
	{
		const SStaticBodyStruct& body = m_staticBodies[ i ];
		CObject* object = nullptr;
		IPhysicsWrapperParentProvider* provider = ( IPhysicsWrapperParentProvider* ) body.m_parentHook;
		provider->GetParent( object );
		if( object!= owner ) continue;
		PxRigidStatic * actor = ( PxRigidStatic* )body.m_actor;
		if( !actor ) continue;
		const PxVec3& position = actor->getGlobalPose().p;
		if( box.Contains( position.x, position.y ) )
		{
			Release( i );
		}
	}
#endif
}

Bool CPhysicsTileWrapper::MakeReadyToDestroy( TDynArray< void* >* toRemove )
{
	PC_SCOPE_PHYSICS( Physics Tile Wrapper make ready );

#ifndef USE_PHYSX
	return false;
#else
	Bool result = true;
	Uint32 actorCount = m_staticBodies.Size();
	for( Uint32 i = 0; i != actorCount; ++i )
	{
		PxRigidActor* actor = ( PxRigidActor* ) m_staticBodies[ i ].m_actor;
		if( !actor ) continue;

		if( actor->getScene() )
		{
			result = false;
			toRemove->PushBack( actor );
		}
	}
	return result;
#endif
}

void CPhysicsTileWrapper::PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd )
{
#ifdef USE_PHYSX
	PC_SCOPE_PHYSICS(CPhysicsTileWrapper PreSimulation )

	if( m_ref.GetValue() == 0 ) return;

	if( m_staticBodies.Empty() ) return;

	Uint64 newMask = 0;

	{
		Vector dimensions = m_box.Max - m_box.Min;
		Vector sectorSize = dimensions / 8;

		Vector eyePos = m_world->GetEyePosition();


		float distanceLimitSquared = SPhysicsSettings::m_staticBodiesDistanceLimit + ( sectorSize.X + sectorSize.Y ) * 0.25f;
		if( distanceLimitSquared > 0 )
		{
			if( distanceLimitSquared < sectorSize.X || distanceLimitSquared < sectorSize.Y )
			{
				if( sectorSize.Y > sectorSize.X )
				{
					distanceLimitSquared = sectorSize.Y;
				}
				else
				{
					distanceLimitSquared = sectorSize.X;
				}
			}

			distanceLimitSquared *= distanceLimitSquared;

			for( Uint8 y = 0; y != 8 ; ++y )
				for( Uint8 x = 0; x != 8; ++x )
				{
					Vector2 position( m_box.Min.X + sectorSize.X * ( x + 0.5f ), m_box.Min.Y + sectorSize.Y * ( y + 0.5f ) );
					float distance = eyePos.DistanceSquaredTo2D( position );
					if( distance > distanceLimitSquared ) continue;

					newMask |= 0x1LL << ( x + y * 8 );
				}

			if( m_box.Contains( eyePos ) )
			{
				float xDiv = ( m_box.Max.X - m_box.Min.X ) / 8;
				float yDiv = ( m_box.Max.Y - m_box.Min.Y ) / 8;
				Vector local = eyePos - m_box.Min;
				Uint8 x = ( Uint8 ) ( local.X / xDiv ); 
				Uint8 y = ( Uint8 ) ( local.Y / yDiv );

				Uint8 counter = x + y * 8;
				newMask |= 0x1LL << counter;;
			}
		}
		else newMask = 0xFFFFFFFFFFFFFFFF;
	}

	if( !GetFlag( PRBW_PoseIsDirty ) && ( newMask == m_sectorMaskProcessed ) ) return;
	SetFlag( PRBW_PoseIsDirty, false );

	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();

	const Uint32 actorCount = m_staticBodies.Size();

	float tilesDistanceLimitSquared = SPhysicsSettings::m_tilesDistanceLimit;
	tilesDistanceLimitSquared *= tilesDistanceLimitSquared;

	Bool recycle = false;
	{
		const float distanceFromViewportSquared = simulationContext->m_resultDistanceSquared;

		{
			PC_SCOPE_PHYSICS(CPhysicsTileWrapper pass )
			for( Uint32 i = 0; i != actorCount; ++i )
			{
				SStaticBodyStruct& body = m_staticBodies[ i ];
				PxRigidActor* actor = ( PxRigidActor* ) body.m_actor;
				if( !actor ) continue;

				CObject* object = nullptr;
				IPhysicsWrapperParentProvider* provider = ( IPhysicsWrapperParentProvider* ) body.m_parentHook;
				provider->GetParent( object );

				if( object == nullptr )
				{
					if( body.m_isAttached )
					{
						toRemove->PushBack( actor );
						body.m_isAttached = false;
						recycle = true;
					}
					else
					{
						actor->userData = 0;
						toRelease->PushBack( actor );
						body.m_actor = 0;
						body.m_sectorMask = 0;
						body.m_compiledCollision.Reset();

						if( m_minIndex > ( Int32 ) i )
						{
							m_minIndex = i;
						}
					}
					continue;
				}

				if( body.m_qued )
				{
					body.m_sectorMask =  ( Uint64& ) actor->userData;
					actor->userData = this;
					body.m_qued = false;
				}
	
				Uint64 actorMask = body.m_sectorMask;
				if( body.m_isAttached )
				{ 
					if( ( !( actorMask & newMask ) && ( actorMask != 0 ) ) || ( actorMask == 0 && tilesDistanceLimitSquared < distanceFromViewportSquared ) )
					{
						toRemove->PushBack( actor );
						body.m_isAttached = false;
					}
				}
				else if( ( actorMask & newMask ) || ( actorMask == 0 && tilesDistanceLimitSquared >= distanceFromViewportSquared ) )
				{
					toAdd->PushBack( actor );
					body.m_isAttached = true;
				}
			}
		}

		if( recycle )
		{
			SetFlag( PRBW_PoseIsDirty, true );
			SWrapperContext* position = m_world->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
			position->m_requestProcessingFlag = true;
		}
	}

	m_sectorMaskProcessed = newMask;

#ifndef NO_EDITOR
	if( !GGame->IsActive() && recycle )
	{
		for( Uint32 i = 1; i != actorCount; ++i )
		{
			PxRigidActor* actor = ( PxRigidActor* )m_staticBodies[ i ].m_actor;
			if( !actor ) continue;


			CComponent* component = nullptr;
			IPhysicsWrapperParentProvider* provider = ( IPhysicsWrapperParentProvider* ) m_staticBodies[ i ].m_parentHook;
			if( !provider->GetParent( component ) ) continue;

			Matrix matrix = component->GetLocalToWorld();
			PxTransform pose = actor->getGlobalPose();
			PxMat44 actorPhysxMat( pose );
			Matrix actorMat( TO_MAT( actorPhysxMat ) );
			Vector xAxis = matrix.V[ 0 ];
			Vector yAxis = matrix.V[ 1 ];
			Vector zAxis = matrix.V[ 2 ];
			xAxis.Normalize3();
			yAxis.Normalize3();
			zAxis.Normalize3();

			if( actorMat.GetTranslationRef() == matrix.GetTranslationRef() && 
				Vector::Near4( actorMat.GetAxisX(), xAxis ) && 
				Vector::Near4( actorMat.GetAxisY(), yAxis ) && 
				Vector::Near4( actorMat.GetAxisZ(), zAxis ) ) continue;

			PxMat44 mat( TO_PX_VECTOR( xAxis ), TO_PX_VECTOR( yAxis ), TO_PX_VECTOR( zAxis ), TO_PX_VECTOR( matrix.GetTranslationRef() ) );
			pose = PxTransform( mat );

			actor->setGlobalPose( pose );
		}
	}
#endif
#endif
}

Bool CPhysicsTileWrapper::CountGetSectorMask( Uint32 actorIndex )
{
	PC_SCOPE_PHYSICS( Physics Tile Wrapper count get );

#ifndef USE_PHYSX
	return false;
#else
	PxActor* actor = ( PxActor* )GetActor( actorIndex );
	if( !actor ) return false;
	PxRigidStatic* rigidStatic = actor->isRigidStatic();
	if( !rigidStatic ) return false;
	PxTransform tras = rigidStatic->getGlobalPose();
	PxBounds3 actorBounds = rigidStatic->getWorldBounds();
	Box2 bounds( actorBounds.minimum.x, actorBounds.minimum.y, actorBounds.maximum.x, actorBounds.maximum.y );
	Uint64 sectorMask = 0;
	if( m_box.Contains( bounds ) )
	{
		Vector2 dimensions = m_box.Max - m_box.Min;
		Vector2 sectorSize = dimensions / 8;
		Uint8 counter = 0;
		for( Uint8 y = 0; y != 8 ; ++y )
			for( Uint8 x = 0; x != 8; ++x )
			{
				Box2 sector( m_box.Min.X + sectorSize.X * x, m_box.Min.Y + sectorSize.Y * y, m_box.Min.X + sectorSize.X * ( x + 1 ), m_box.Min.Y + sectorSize.Y * ( y + 1 ) );
				if( sector.Contains( bounds ) )
				{
					sectorMask |= 0x1LL << counter;
				}

				if( bounds.Contains( sector ) )
				{
					sectorMask |= 0x1LL << counter;
				}

				if( bounds.Intersects( sector ) )
				{
					sectorMask |= 0x1LL << counter;
				}
				++counter;
			}
	}
	m_staticBodies[ actorIndex ].m_sectorMask = sectorMask;
	return true;
#endif
}

IPhysicsWrapperParentProvider* CPhysicsTileWrapper::GetParentProvider( Uint32 actorIndex ) const
{
	if( m_staticBodies.Size() <= actorIndex ) return 0;
	IPhysicsWrapperParentProvider* parentHook = ( IPhysicsWrapperParentProvider* ) m_staticBodies[ actorIndex ].m_parentHook;
	return parentHook;
}

Bool CPhysicsTileWrapper::IsReady() const
{
#ifndef USE_PHYSX
	return false;
#else
	if( m_ref.GetValue() <= 0 )
	{
		return false;
	}
	if( m_staticBodies.Empty() )
	{
		return false;
	}
	return true;
#endif
}

void CPhysicsTileWrapper::SetPose( const Matrix& localToWorld, Uint32 actorIndex )
{
#ifdef USE_PHYSX
	if( m_staticBodies.Size() <= actorIndex ) return;
	PxMat44 pose = TO_PX_MAT( localToWorld );
	PxRigidActor* actor = ( PxRigidActor* ) m_staticBodies[ actorIndex ].m_actor;
	if( !actor ) return;
	actor->setGlobalPose( PxTransform( pose ) );
#endif
}

Matrix CPhysicsTileWrapper::GetPose( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
	return Matrix::IDENTITY;
#else
	if( m_staticBodies.Size() <= actorIndex ) return Matrix::IDENTITY;
	PxRigidActor* actor = ( PxRigidActor* ) m_staticBodies[ actorIndex ].m_actor;
	if( !actor ) return Matrix::IDENTITY;
	PxTransform transform = actor->getGlobalPose();
	PxMat44 pose = transform;
	return TO_MAT( pose );
#endif
}

Uint32 CPhysicsTileWrapper::GetActorsCount() const
{
#ifndef USE_PHYSX
	return 0;
#else
	return m_staticBodies.Size();
#endif
}

void* CPhysicsTileWrapper::GetActor( Uint32 actorIndex ) const
{
#ifndef USE_PHYSX
	return 0;
#else
	if( m_staticBodies.Size() <= actorIndex ) return 0;
	return m_staticBodies[ actorIndex ].m_actor;
#endif
}

Int32 CPhysicsTileWrapper::AddTerrainBody( const Vector& position, const SPhysicalMaterial* material, void* geometry )
{
	PC_SCOPE_PHYSICS( Physics Tile Wrapper set terrain body );

#ifdef USE_PHYSX
	PxTransform pose;
	pose.p = TO_PX_VECTOR( position );
	pose.q = PxQuat( M_PI / 2.f, PxVec3( 0.f, 0.f, 1.f ) ) * PxQuat( M_PI / 2.f, PxVec3( 1.f, 0.f, 0.f ) );
	ASSERT( pose.isValid() );

	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();

	PxPhysics& physics = scene->getPhysics();

	PxRigidStatic* rigidActor = GPhysXEngine->GetPxPhysics()->createRigidStatic( pose );

	Uint16 currentSize = ( Uint16 ) m_staticBodies.Size();
	Int32 index = -1;
	for( Uint32 i = m_minIndex; i != currentSize; ++i )
		if( !m_staticBodies[ i ].m_actor )
		{
			SStaticBodyStruct& test = m_staticBodies[ i ];
			index = i;
			m_staticBodies[ i ] = SStaticBodyStruct( rigidActor );
			break;
		}

		if( index == -1  )
		{
			if( currentSize < 65535 )
			{
				index = currentSize;
				m_staticBodies.PushBack( SStaticBodyStruct( rigidActor ) );
			}

		}

	m_minIndex = index + 1;

	if( index >= 65535 ) 
	{
		ASSERT( false && "tile wrapper full");
		rigidActor->release();
		return -1;
	}

	PxShape* shape = 0;

	PxHeightFieldGeometry* phxGeometry = ( PxHeightFieldGeometry* ) geometry;

	if( material )
	{
		shape = rigidActor->createShape( *phxGeometry, *( PxMaterial* )material->m_middlewareInstance );
	}
	else
	{
		TDynArray< PxMaterial* > materials = GPhysXEngine->GetMaterialsArray();
		shape = rigidActor->createShape( *phxGeometry, &materials[ 0 ], ( Uint16 ) materials.Size() );
	}

	if ( shape )
	{
		shape->userData = 0;

		SPhysicalFilterData filterData( m_collisionType, m_collisionGroup );
		shape->setSimulationFilterData( filterData.m_data );
		shape->setQueryFilterData( filterData.m_data );

		shape->setContactOffset( SPhysicsSettings::m_contactOffset );
		shape->setRestOffset( SPhysicsSettings::m_restOffset );

		PHYSICS_STATISTICS_INC(StaticBodiesInstanced)
	}

#ifndef NO_EDITOR
//	m_staticBodies[ index ].m_debugName = UNICODE_TO_ANSI( owner->GetFriendlyName().AsChar() );
//	( ( PxRigidActor* ) m_staticBodies[ index ].m_actor )->setName( m_staticBodies[ index ].m_debugName.AsChar() );
#endif

	AddRef();
	SetFlag( PRBW_PoseIsDirty, true );

	rigidActor->userData = this;

	Int16 shapeIndex = ( Int16 ) rigidActor->getNbShapes() - 1;
	SActorShapeIndex& actorShapeIndex = ( SActorShapeIndex& ) shape->userData;
	actorShapeIndex = SActorShapeIndex( ( Uint16 ) index, ( Uint16 ) shapeIndex );

	Uint64 sectorMask = 0;
	Vector2 dimensions = m_box.Max - m_box.Min;
	Vector2 sectorSize = dimensions / 8;
	Uint8 counter = 0;
	for( Uint8 y = 0; y != 8 ; ++y )
		for( Uint8 x = 0; x != 8; ++x )
		{
			Box2 sector( m_box.Min.X + sectorSize.X * x, m_box.Min.Y + sectorSize.Y * y, m_box.Min.X + sectorSize.X * ( x + 1 ), m_box.Min.Y + sectorSize.Y * ( y + 1 ) );
			if( sector.Contains( position ) )
			{
				sectorMask |= 0x1LL << counter;
			}
			++counter;
		}

	m_staticBodies[ index ].m_sectorMask = sectorMask;

	m_world->MarkSectorAsStuffAdded();

	return index;
#else
	return -1;
#endif
}

Int32 CPhysicsTileWrapper::AddStaticBody( const Matrix& localToWorld, CompiledCollisionPtr& compiledCollision, CPhysicsEngine::CollisionMask collisionType, CPhysicsEngine::CollisionMask collisionGroup )
{
	PC_SCOPE_PHYSICS( PhysicsTileWrapperAddStatic );

#ifndef USE_PHYSX
	return -1;
#else
	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();
	PxPhysics& physics = scene->getPhysics();

	if( m_task && m_task->GetWorld() == m_world )
	{
		PxRigidStatic* rigidStatic = physics.createRigidStatic( PxTransform::createIdentity() );
		Uint16 currentSize = ( Uint16 ) m_staticBodies.Size();
		Int32 index = -1;
		{
			for( Uint32 i = m_minIndex; i != currentSize; ++i )
				if( !m_staticBodies[ i ].m_actor )
				{
					SStaticBodyStruct& test = m_staticBodies[ i ];
					index = i;
					m_staticBodies[ i ] = SStaticBodyStruct( rigidStatic );
					break;
				}
		}

		if( index == -1  )
		{
			if( currentSize < 65535 )
			{
				index = currentSize;
				m_staticBodies.PushBack( SStaticBodyStruct( rigidStatic ) );
			}

		}

		m_minIndex = index + 1;

		if( index >= 65535 ) 
		{
			ASSERT( false && "tile wrapper full");
			rigidStatic->release();
			return -1;
		}

		m_task->AddData( rigidStatic, index, collisionType, collisionGroup, compiledCollision, localToWorld, m_box );

#ifndef NO_EDITOR
//		m_staticBodies[ index ].m_debugName = UNICODE_TO_ANSI( owner->GetFriendlyName().AsChar() );
//		( ( PxRigidActor* ) m_staticBodies[ index ].m_actor )->setName( m_staticBodies[ index ].m_debugName.AsChar() );
#endif
		m_staticBodies[ index ].m_compiledCollision = compiledCollision;
		m_staticBodies[ index ].m_qued = true;

		AddRef();
		SetFlag( PRBW_PoseIsDirty, true );

		m_world->MarkSectorAsStuffAdded();
		return index;
	}

	Vector xAxis = localToWorld.V[ 0 ];
	Vector yAxis = localToWorld.V[ 1 ];
	Vector zAxis = localToWorld.V[ 2 ];
	Vector pos = localToWorld.V[ 3 ];

	Vector scale( xAxis.Normalize3(), yAxis.Normalize3(), zAxis.Normalize3() );

	PxMat44 mat( TO_PX_VECTOR( xAxis ), TO_PX_VECTOR( yAxis ), TO_PX_VECTOR( zAxis ), TO_PX_VECTOR( pos ) );
	PxTransform pose( mat );
	pose.q.normalize();

	PxRigidStatic* rigidStatic = physics.createRigidStatic( pose );

	Uint16 currentSize = ( Uint16 ) m_staticBodies.Size();
	Int32 index = -1;
	{
		PC_SCOPE_PHYSICS( PhysicsTileWrapperAddStatic Search );
		for( Uint32 i = m_minIndex; i != currentSize; ++i )
			if( !m_staticBodies[ i ].m_actor )
			{
				SStaticBodyStruct& test = m_staticBodies[ i ];
				index = i;
				m_staticBodies[ i ] = SStaticBodyStruct( rigidStatic );
				break;
			}
	}

	if( index == -1  )
	{
		if( currentSize < 65535 )
		{
			index = currentSize;
			m_staticBodies.PushBack( SStaticBodyStruct( rigidStatic ) );
		}

	}

	if( index >= 65535 || index == -1 ) 
	{
		ASSERT( false && "tile wrapper full");
		rigidStatic->release();
		return -1;
	}

	m_minIndex = index + 1;

	if( !RebuildCollision( index, compiledCollision, scale, collisionType, collisionGroup ) )
	{
		rigidStatic->release();
		m_staticBodies[ index ] = SStaticBodyStruct();
		return -1;
	}

	PHYSICS_STATISTICS_INC(StaticBodiesInstanced)

#ifndef NO_EDITOR
//	m_staticBodies[ index ].m_debugName = UNICODE_TO_ANSI( owner->GetFriendlyName().AsChar() );
//	( ( PxRigidActor* ) m_staticBodies[ index ].m_actor )->setName( m_staticBodies[ index ].m_debugName.AsChar() );
#endif
	m_staticBodies[ index ].m_compiledCollision = compiledCollision;

	AddRef();
	SetFlag( PRBW_PoseIsDirty, true );

	rigidStatic->userData = this;

	CountGetSectorMask( index );

	SetOcclusionParameters( index, compiledCollision->GetOcclusionDiagonalLimit(), compiledCollision->GetOcclusionAttenuation() );

	m_world->MarkSectorAsStuffAdded();

	return index;
#endif
}

#ifdef USE_PHYSX
void CreateFoliageShape( PxRigidStatic* rigidStatic, const PxGeometry & geometry, const PxMat44 & matShape, Uint16 actorIndex, Uint32 shapeIndex )
{
	PC_SCOPE_PHYSICS( Physics Tile Wrapper create foliage );

	PxMaterial* material = GPhysXEngine->GetMaterial( CNAME( default ) );

	PxShape* shape = rigidStatic->createShape( geometry, *material );
	if( shape )
	{
		shape->setLocalPose( PxTransform( matShape ) );
		SActorShapeIndex& actorShapeIndex = ( SActorShapeIndex& ) shape->userData;
		actorShapeIndex = SActorShapeIndex( ( Int16 )actorIndex, ( Int16 )shapeIndex );

		CPhysicsEngine::CollisionMask collisionType = GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) );
		CPhysicsEngine::CollisionMask collisionGroup = GPhysicEngine->GetCollisionGroupMask( collisionType );
		SPhysicalFilterData data( collisionType, collisionGroup );
		shape->setSimulationFilterData( data.m_data );
		shape->setQueryFilterData( data.m_data );
	}
}
#endif

Int32 CPhysicsTileWrapper::AddFoliageBody( const void* instancePtr, const TDynArray< Sphere >& shapes )
{
	const SFoliageInstance* instance = ( const SFoliageInstance* ) instancePtr;
	PC_SCOPE_PHYSICS( Physics Tile Wrapper add foliage );

#ifndef USE_PHYSX
	return 0;
#else

	const Uint32 shapesCount = shapes.Size();
	for( Uint32 i = 0; i != shapesCount; i += 2 )
	{
		Vector shapePosition1 = shapes[ i ].GetCenter()  * instance->GetScale();
		float radius = shapes[ i ].GetRadius();
		if( radius <= 0.0f )
		{
/*			const CResource* resource =  Cast< const CResource > ( owner );
			DATA_HALT( DES_Minor, resource, TXT("Physical tree collision"), TXT("Radius is zero, collision wont be made from that") );*/
			return -1;
		}
	}

	CPhysicsWorldPhysXImpl* world = nullptr;
	GetPhysicsWorld( world );
	PxScene* scene = world->GetPxScene();

	PxPhysics& physics = scene->getPhysics();

	PxQuat instanceQuaternion = TO_PX_QUAT( instance->GetQuaterion() );
	instanceQuaternion *= PxQuat( M_PI_HALF, PxVec3( 0.0f, 0.0f, 1.0f ) );

	PxTransform transform( TO_PX_VECTOR( instance->GetPosition() ), instanceQuaternion );
	
	PxRigidStatic* rigidStatic = physics.createRigidStatic( transform );


	Uint16 currentSize = ( Uint16 ) m_staticBodies.Size();
	Int32 index = -1;
	for( Uint32 i = m_minIndex; i != currentSize; ++i )
		if( !m_staticBodies[ i ].m_actor )
		{
			SStaticBodyStruct& test = m_staticBodies[ i ];
			index = i;
			m_staticBodies[ i ] = SStaticBodyStruct( rigidStatic );
			break;
		}

	if( index == -1  )
	{
		if( currentSize < 65535 )
		{
			index = currentSize;
			m_staticBodies.PushBack( SStaticBodyStruct( rigidStatic ) );
		}

	}

	m_minIndex = index + 1;

	if( index >= 65535 ) 
	{
		ASSERT( false && "tile wrapper full");
		rigidStatic->release();
		return -1;
	}


	rigidStatic->userData = this;

	for( Uint32 i = 0; i != shapesCount; i += 2 )
	{
		Vector shapePosition1 = shapes[ i ].GetCenter()  * instance->GetScale();
		Vector shapePosition2 = shapes[ i + 1 ].GetCenter() * instance->GetScale();
	
		float radius = shapes[ i ].GetRadius() * instance->GetScale();
		shapePosition1.W = 1.0f;
		shapePosition2.W = 1.0f;

		Vector direction = shapePosition2 - shapePosition1;
		float halfHeight = 0.0f;
		
		PxTransform capsuleTransform = PxTransformFromSegment(TO_PX_VECTOR( shapePosition1 ), TO_PX_VECTOR( shapePosition2 ), &halfHeight );	
		
		if( halfHeight > 0.0f )
		{
			PxCapsuleGeometry capsule( radius, halfHeight ); 
			CreateFoliageShape(rigidStatic, capsule, capsuleTransform, index, i);
		}
		else
		{
			PxSphereGeometry sphere( radius ); 
			PxMat44 matShape = PxMat44(PxIdentity);
			matShape.setPosition( PxVec3( shapePosition1.X, shapePosition1.Y, shapePosition1.Z ) );
			CreateFoliageShape(rigidStatic, sphere, matShape, index, i);
		}
	}

	SetFlag( PRBW_PoseIsDirty, true );

	m_world->MarkSectorAsStuffAdded();

	CountGetSectorMask( index );

	AddRef();

	PHYSICS_STATISTICS_INC(StaticBodiesInstanced)

	return index;
#endif
}

Bool CPhysicsTileWrapper::SetOcclusionParameters( Uint32 actorIndex, Float diagonalLimit, Float attenuation )
{
#ifndef USE_PHYSX
	return 0;
#else
	if( m_staticBodies.Size() <= actorIndex ) return false;

	PxBounds3 bounds = ( ( PxRigidStatic* )m_staticBodies[ actorIndex ].m_actor )->getWorldBounds();
	Float currentDiameter = bounds.getDimensions().magnitude();
	Bool isOccluding = diagonalLimit == -1 && attenuation == -1 ? false : diagonalLimit <= currentDiameter;

	Uint32 shapesCount = GetShapesCount( actorIndex );
	for( Uint32 j = 0; j != shapesCount; ++j )
	{
		PxShape* shape = ( PxShape* ) GetShape( j, actorIndex );
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

	return true;
#endif
}

Bool CPhysicsTileWrapper::GetOcclusionParameters( Uint32 actorIndex, Float* diagonalLimit, Float* attenuation )
{
#ifndef USE_PHYSX
	return false;
#else
	if( m_staticBodies.Size() <= actorIndex ) return false;

	SStaticBodyStruct& staticBody = m_staticBodies[ actorIndex ];
	if( staticBody.m_compiledCollision )
	{
		if( diagonalLimit )	*diagonalLimit = staticBody.m_compiledCollision->GetOcclusionDiagonalLimit();
		if( attenuation ) *attenuation = staticBody.m_compiledCollision->GetOcclusionAttenuation();
	}
	return true;
#endif
}

Bool CPhysicsTileWrapper::IsSectorReady( const Vector& position ) const
{
	PC_SCOPE_PHYSICS( Physics Tile Wrapper is sector ready );

#ifndef USE_PHYSX
	return false;
#else
	if( !m_box.Contains( position ) )
	{
		return false;
	}

	float xDiv = ( m_box.Max.X - m_box.Min.X ) / 8;
	float yDiv = ( m_box.Max.Y - m_box.Min.Y ) / 8;
	Vector local = position - m_box.Min;
	Uint8 x = ( Uint8 ) ( local.X / xDiv ); 
	Uint8 y = ( Uint8 ) ( local.Y / yDiv );
	
	Uint8 counter = x + y * 8;
	Uint64 bit = 0x1LL << counter;
	return ( m_sectorMaskProcessed & bit ) != 0;
#endif

}