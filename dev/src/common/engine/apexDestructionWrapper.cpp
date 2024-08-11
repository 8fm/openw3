#include "build.h"
#include "apexDestructionWrapper.h"
#include "apexDestructionResource.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "collisionColoring.h"
#include "PxGeometryQuery.h"
#include "../core/dataError.h"
#include "collisionCache.h"

#ifdef USE_APEX

#include "NxParamUtils.h"
#include "NxResourceProvider.h"
#include "NxDestructibleAsset.h"
#include "NxDestructibleActor.h"
#include "NxApexScene.h"
#include "NxDestructibleActor.h"
#include "NxDestructibleRenderable.h"


#endif
#include "../physics/physicsSettings.h"
#include "renderer.h"
#include "destructionSystemComponent.h"
#include "game.h"
#include "renderVertices.h"
#include "entity.h"
#include "layer.h"
#include "utils.h"

#ifdef USE_APEX
using namespace physx;

DECLARE_PHYSICS_WRAPPER(CApexDestructionWrapper,EPW_ApexDestruction,false,false)

//////////////////////////////////////////////////////////////////////////
//
// NOTE FOR UPGRADING APEX :
//
// See if workaround in CApexDestructionWrapper::SetPose can be removed. Just
// look for "NVIDIA STATIC CHUNK WORKAROUND".
//
// Basic repro:
//   - Destroy something that has static chunks (marked as "do not fracture").
//   - The static chunks should stay where they are (without the workaround, they would
//     render at 0,0,0 after a while).
//   - If that works properly, and you can move destructibles in the editor without
//     the call to setInitialGlobalPose(), it probably isn't needed anymore :)
//
//////////////////////////////////////////////////////////////////////////

class CApexPhysX3Interface : public NxApexPhysX3Interface
{
public:

	virtual ~CApexPhysX3Interface(){}

private:

	void setContactReportFlags(PxShape* shape, PxPairFlags flags, NxDestructibleActor* actor, PxU16 actorChunkIndex, PxU16 depth )
	{
		PC_SCOPE_PHYSICS( Apex scene fetch destruction shapes init );

		PxRigidActor* pxActor = shape->getActor();

		SActorShapeIndex& actorShapeIndex = ( SActorShapeIndex& ) shape->userData;
		actorShapeIndex = SActorShapeIndex( actorChunkIndex, ( Int16 ) pxActor->getNbShapes() - 1 );

		CApexDestructionWrapper* wrapper = ( CApexDestructionWrapper* ) pxActor->userData;
		const SPhysicalMaterial* material = wrapper->GetPhysicalMaterial( depth > 0 ); 
		PxMaterial* phxMaterial = ( PxMaterial* )material->m_middlewareInstance;
		shape->setMaterials( &phxMaterial, 1 );

		PxRigidDynamic* dynamic = pxActor->isRigidDynamic();

		CPhysicsEngine::CollisionMask collisionType;
		CPhysicsEngine::CollisionMask collisionGroup;
		if( dynamic && dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC )
		{
			//for those which will be static collision type shouldnt be taken from fractured level
			wrapper->GetPhysicalCollisionType( 0 ).RetrieveCollisionMasks( collisionType, collisionGroup );
		}
		else
		{
			wrapper->GetPhysicalCollisionType( depth ).RetrieveCollisionMasks( collisionType, collisionGroup );
		}
		SPhysicalFilterData data( collisionType, collisionGroup, SPhysicalFilterData::EPFDF_CountactSoundable );

		data.m_data.word3 = flags;
		shape->setSimulationFilterData( data.m_data );
		shape->setQueryFilterData( data.m_data );
		shape->setContactOffset( SPhysicsSettings::m_contactOffset );
		shape->setRestOffset( SPhysicsSettings::m_restOffset );

		if( dynamic )
		{
			Float mass = dynamic->getMass();
			dynamic->setContactReportThreshold( SPhysicsSettings::m_contactReportsThreshold * mass );
			dynamic->setSolverIterationCounts( SPhysicsSettings::m_rigidbodyPositionIters, SPhysicsSettings::m_rigidbodyVelocityIters );
			if( depth > 0 )
			{
				dynamic->setSleepThreshold( SPhysicsSettings::m_actorSleepThreshold );
			}
			else
			{
				dynamic->setSleepThreshold( SPhysicsSettings::m_destructibleUnfracturedSleepThreshold );
			}
		}

		if( depth > 0 )
		{
			wrapper->m_amountOfBaseFractures.Increment();
		}
	}

	PxPairFlags getContactReportFlags(const PxShape* shape) const
	{
		return PxPairFlags();
	}
};

void CreateApexInterface( physx::apex::NxApexPhysX3Interface** apexInterface )
{
	*apexInterface = ( physx::apex::NxApexPhysX3Interface* ) RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_SmallObjects, MC_PhysX, sizeof( CApexPhysX3Interface ), 16 );
	*apexInterface = ::new(*apexInterface) CApexPhysX3Interface();
}

RED_FORCE_INLINE static Uint32 GetChunkIndexFromShape( const PxShape* shape )
{
	// It appears that the shape's userData holds the chunk index in its lower 16 bits.
	size_t data = ( size_t ) shape->userData;
	
	return ( ( Uint32 ) data ) & 0x0000FFFF;
}

CApexDestructionWrapper::CApexDestructionWrapper( struct SDestructionParameters* parameters, CPhysicsWorld* world, Uint32 visibiltyId )
	: CApexWrapper()
	, m_actor( 0 )
	, m_forceFracture( false )
#ifndef NO_EDITOR
	, m_debugFracturePointsWhereTaken( false )
#endif
{
	if( world )
	{
		m_world = static_cast< CPhysicsWorldPhysXImpl* >( world );
	}

	SWrapperContext* position = m_world->GetWrappersPool< CApexDestructionWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	if( !position )
	{
		int a = 0;
	}

	m_componentOffsetInverse = Matrix::IDENTITY;

	GetParentProvider()->GetTransform().CalcLocalToWorld(m_componentOffsetInverse);
	m_componentOffsetInverse.Invert();

	Matrix transform = m_componentOffsetInverse * parameters->m_pose;

	const Vector& pos = transform.GetTranslationRef();

	position->m_x = parameters->m_pose.GetTranslationRef().X;
	position->m_y = parameters->m_pose.GetTranslationRef().Y;
	position->m_resultDistanceSquared = FLT_MAX;
	position->m_visibilityQueryId = visibiltyId;
}

CApexDestructionWrapper::~CApexDestructionWrapper()
{
	Destroy();
}

Bool CApexDestructionWrapper::MakeReadyToDestroy( TDynArray< void* >* toRemove )
{
	Bool result = true;
	Uint32 count = GetActorsCount();

	{
		PC_SCOPE_PHYSICS(CApexDestructionWrapper MakeReadyToDestroy )

		for( Uint32 i = 0; i != count; ++i )
		{
			PxActor* actor = m_actor->getChunkPhysXActor( i );
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

void CApexDestructionWrapper::RequestReCreate( const SDestructionParameters& parameters )
{
//	Destroy();
}

Bool CApexDestructionWrapper::IsReady() const
{
	if( !m_actor ) return false;
	Uint32 actorCount = GetActorsCount();
	for( Uint32 i = 0; i != actorCount; ++i )
	{
		PxActor* actor = ( PxActor* ) GetActor( 0 );
		if( !actor ) continue;
		return actor->getScene() != 0;
	}
	return false;
}

NxDestructibleAsset* CApexDestructionWrapper::GetResourceAsset( const SDestructionParameters& parameters )
{
	CApexResource* resource = parameters.m_resource.Get();

#ifdef NO_EDITOR
	return reinterpret_cast<physx::apex::NxDestructibleAsset*>( resource->GetAsset() );
#else
	if ( parameters.m_usePreviewAsset )
		return reinterpret_cast<physx::apex::NxDestructibleAsset*>( resource->GetPreviewAsset() );
	else
		return reinterpret_cast<physx::apex::NxDestructibleAsset*>( resource->GetAsset() );
#endif
}

Bool CApexDestructionWrapper::Create( const SDestructionParameters& parameters )
{
	PC_SCOPE_PHYSICS(CApexDestructionWrapper Create )

	if( m_actor )
	{
		Destroy();
	};

	m_basePhysicalCollisionType = parameters.m_physicalCollisionType;
	m_fracturedPhysicalCollisionType = parameters.m_fracturedPhysicalCollisionType;
	NxApexSDK* apexSDK = NxGetApexSDK();

	CApexDestructionResource* resource = Cast< CApexDestructionResource >( parameters.m_resource.Get() );
	if( !resource ) return false;

	CompiledCollisionPtr compiledCollision;
#ifdef NO_EDITOR
	if( !resource->GetAsset() )
	{
		ICollisionCache::EResult result = GCollisionCache->FindCompiled( compiledCollision, resource->GetDepotPath(), resource->GetFileTime() );
		if( ICollisionCache::eResult_Valid != result )
		{
			if( ICollisionCache::eResult_NotReady == result )
			{
				m_world->MarkSectorAsStuffAdded();
			}
			return false;
		}
	}
#endif

	if( !resource->TryPreload( compiledCollision ) ) return false;
	resource->AddRef();

	NxDestructibleAsset* asset = GetResourceAsset( parameters );

	if( !asset ) return false;

	resource->AddAssetRef( asset );

	NxParameterized::Interface* descParams = asset->getDefaultActorDesc();

#ifndef NO_EDITOR
	if( !GGame->IsActive() )
	{
		NxParameterized::setParamBool( *descParams, "dynamic", false );
	}
	else
#endif
	{
		NxParameterized::setParamBool( *descParams, "dynamic", parameters.m_dynamic );
	}

	// dont change this crapp
	NxParameterized::setParamBool(*descParams, "formExtendedStructures", false );
	// dont change this crapp

	const physx::PxCookingParams& cookingParams = apexSDK->getCookingInterface()->getParams();

	NxParameterized::setParamF32(*descParams, "p3ShapeDescTemplate.restOffset", -cookingParams.skinWidth);
	const SPhysicalMaterial* physicalMaterial = GPhysicEngine->GetMaterial( resource->GetMaterialForChunkDepth( 0 ) );
	if ( !physicalMaterial )
	{
		physicalMaterial = GPhysicEngine->GetMaterial();
	}
	m_physicalMaterial = physicalMaterial;
	Float densityUnfractured = 1.0f;
	Float densityFractured = 1.0f;
	resource->GetDensities( densityUnfractured, densityFractured );
	densityUnfractured = physicalMaterial->m_density * densityUnfractured;
	densityFractured = physicalMaterial->m_density * densityFractured;
	NxParameterized::setParamF32( *descParams, "p3BodyDescTemplate.density", densityUnfractured );

	m_fracturedPhysicalMaterial = GPhysicEngine->GetMaterial( resource->GetMaterialForChunkDepth( 0 ) );
	if( physicalMaterial->m_name == CNAME( default ) )
	{
		m_fracturedPhysicalMaterial = m_physicalMaterial;
	}

	Vector xAxis = parameters.m_pose.V[ 0 ];
	Vector yAxis = parameters.m_pose.V[ 1 ];
	Vector zAxis = parameters.m_pose.V[ 2 ];
	Vector pos = parameters.m_pose.V[ 3 ];

	xAxis.Normalize3();
	yAxis.Normalize3();
	zAxis.Normalize3();

	PxMat44 myPose( TO_PX_VECTOR( xAxis ), TO_PX_VECTOR( yAxis ), TO_PX_VECTOR( zAxis ), TO_PX_VECTOR( pos ) ); 

	NxParameterized::setParamMat44( *descParams, "globalPose", myPose);
	NxParameterized::setParamString(*descParams, "crumbleEmitterName", nullptr );
	NxParameterized::setParamBool( *descParams, "renderStaticChunksSeparately", true );
	NxParameterized::setParamVec3( *descParams, "scale", PxVec3( 1.f, 1.f, 1.f ) );
	NxParameterized::setParamF32( *descParams, "destructibleParameters.damageCap", parameters.m_damageCap );
	NxParameterized::setParamF32( *descParams, "defaultBehaviorGroup.damageThreshold", parameters.m_damageThreshold );
	NxParameterized::setParamF32( *descParams, "defaultBehaviorGroup.damageToRadius", parameters.m_damageToRadius );
	NxParameterized::setParamI32( *descParams, "destructibleParameters.debrisDepth", parameters.m_debrisDepth );
	NxParameterized::setParamF32( *descParams, "destructibleParameters.debrisDestructionProbability", parameters.m_debrisDestructionProbability );
	NxParameterized::setParamF32( *descParams, "destructibleParameters.debrisLifetimeMin", parameters.m_debrisLifetimeMin );
	NxParameterized::setParamF32( *descParams, "destructibleParameters.debrisLifetimeMax", parameters.m_debrisLifetimeMax );
	NxParameterized::setParamF32( *descParams, "destructibleParameters.debrisMaxSeparationMin", parameters.m_debrisMaxSeparationMin );
	NxParameterized::setParamF32( *descParams, "destructibleParameters.debrisMaxSeparationMax", parameters.m_debrisMaxSeparationMax );
	NxParameterized::setParamF32( *descParams, "defaultBehaviorGroup.fadeOut", parameters.m_fadeOutTime );
	NxParameterized::setParamU32( *descParams, "destructibleParameters.essentialDepth", parameters.m_essentialDepth );
	NxParameterized::setParamF32( *descParams, "destructibleParameters.forceToDamage", parameters.m_forceToDamage );
	NxParameterized::setParamF32( *descParams, "destructibleParameters.fractureImpulseScale", parameters.m_fractureImpulseScale );

	Int32 impactDamageDepth = -1;
	CComponent* parent = nullptr;
	if ( GetParentProvider()->GetParent( parent ) )
	{
		CEntity* entity = parent->GetEntity();
		Uint32 count = ForEachSumResult< Uint32 >( entity->GetComponents(), []( CComponent* component )-> int { return Cast< CDestructionSystemComponent >( component ) ? 1 : 0; } );
		if( count > 1 )
		{
			impactDamageDepth = parameters.m_impactDamageDefaultDepth > -1 ? parameters.m_impactDamageDefaultDepth : 0;
		}
	}
	

	NxParameterized::setParamI32( *descParams, "destructibleParameters.impactDamageDefaultDepth", impactDamageDepth );
	NxParameterized::setParamF32( *descParams, "destructibleParameters.impactVelocityThreshold", parameters.m_impactVelocityThreshold );
	NxParameterized::setParamF32( *descParams, "defaultBehaviorGroup.materialStrength", parameters.m_materialStrength );
	NxParameterized::setParamF32( *descParams, "destructibleParameters.maxChunkSpeed", parameters.m_maxChunkSpeed );
	NxParameterized::setParamU32( *descParams, "destructibleParameters.minimumFractureDepth", parameters.m_minimumFractureDepth );
	NxParameterized::setParamBool( *descParams, "structureSettings.useStressSolver", parameters.m_useStressSolver );
	NxParameterized::setParamF32( *descParams, "structureSettings.stressSolverTimeDelay", parameters.m_stressSolverTimeDelay );
	NxParameterized::setParamF32( *descParams, "structureSettings.stressSolverMassThreshold", parameters.m_stressSolverMassThreshold );

	int dpCount = 0;
	NxParameterized::getParamArraySize(*descParams, "depthParameters", dpCount);

	Uint32 supportDepth = parameters.m_supportDepth;
	if( supportDepth >= (Uint32 )dpCount - 1 )
	{
		supportDepth = dpCount - 1;
	}
	while( supportDepth )
	{
		char tmpStr[128];
		sprintf_s(tmpStr, 128, "firstChunkAtDepth[%d]", supportDepth + 1);
		Uint32 firstChunkAtDepth = 0;
		NxParameterized::getParamU32( *asset->getAssetNxParameterized(), tmpStr, firstChunkAtDepth );
		if( firstChunkAtDepth ) break;
		--supportDepth;
	}
	NxParameterized::setParamU32( *descParams, "supportDepth", supportDepth );
	NxParameterized::setParamBool( *descParams, "useAssetDefinedSupport", parameters.m_useAssetDefinedSupport );
	NxParameterized::setParamBool( *descParams, "useWorldSupport", parameters.m_useWorldSupport );
	NxParameterized::setParamF32( *descParams, "sleepVelocityFrameDecayConstant", parameters.m_sleepVelocityFrameDecayConstant );
	NxParameterized::setParamBool( *descParams, "useHardSleeping", parameters.m_useHardSleeping );
	NxParameterized::setParamBool( *descParams, "destructibleParameters.flags.ACCUMULATE_DAMAGE", parameters.m_accumulateDamage );
	NxParameterized::setParamBool( *descParams, "destructibleParameters.flags.DEBRIS_TIMEOUT", parameters.m_debrisTimeout );
	NxParameterized::setParamBool( *descParams, "destructibleParameters.flags.DEBRIS_MAX_SEPARATION", parameters.m_debrisMaxSeparation );
	NxParameterized::setParamBool( *descParams, "destructibleParameters.flags.CRUMBLE_SMALLEST_CHUNKS", parameters.m_crumbleSmallestChunks );

	CPhysicalCollision physicalCollision = parameters.m_physicalCollisionType;
	CPhysicsEngine::CollisionMask terrainCollisionTypeMask;
	CPhysicsEngine::CollisionMask terrainCollisionGroupMask;
	if (physicalCollision.HasCollisionTypeName())
	{
		physicalCollision.RetrieveCollisionMasks( terrainCollisionTypeMask, terrainCollisionGroupMask );
	}
	else
	{
		terrainCollisionTypeMask = CPhysicalCollision::COLLIDES_ALL;
		terrainCollisionGroupMask = CPhysicalCollision::COLLIDES_ALL;
	}

	SPhysicalFilterData filterData( terrainCollisionTypeMask, terrainCollisionGroupMask );
	NxParameterized::setParamU32(*descParams, "p3ShapeDescTemplate.simulationFilterData.word0", filterData.m_data.word0 );
	NxParameterized::setParamU32(*descParams, "p3ShapeDescTemplate.simulationFilterData.word1", filterData.m_data.word1 );
	NxParameterized::setParamU32(*descParams, "p3ShapeDescTemplate.simulationFilterData.word2", filterData.m_data.word2 );
	NxParameterized::setParamU32(*descParams, "p3ShapeDescTemplate.simulationFilterData.word3", filterData.m_data.word3 );

	NxParameterized::setParamU32(*descParams, "p3ShapeDescTemplate.queryFilterData.word0", filterData.m_data.word0 );
	NxParameterized::setParamU32(*descParams, "p3ShapeDescTemplate.queryFilterData.word1", filterData.m_data.word1 );
	NxParameterized::setParamU32(*descParams, "p3ShapeDescTemplate.queryFilterData.word2", filterData.m_data.word2 );
	NxParameterized::setParamU32(*descParams, "p3ShapeDescTemplate.queryFilterData.word3", filterData.m_data.word3 );

	NxParameterized::setParamU64( *descParams, "p3ActorDescTemplate.userData", (Uint64)this );

	for (int i = 0; i < dpCount; i++)
	{
		char tmpStr[128];
		sprintf_s(tmpStr, 128, "depthParameters[%d].OVERRIDE_IMPACT_DAMAGE", i);
		NxParameterized::setParamBool(*descParams, tmpStr, false);
	}

	m_world->AddRef();

#ifndef RED_FINAL_BUILD
	CPhysXLogger::ClearLastError();
#endif
	{
		PC_SCOPE_PHYSICS( CApexDestructionWrapper Create createApexActor )	
		PxScene* scene = ( ( CPhysicsWorldPhysXImpl* )m_world )->GetPxScene();
		NxApexScene* apexScene = ( NxApexScene* ) scene->userData;
		m_actor = static_cast< NxDestructibleActor* >( asset->createApexActor( *descParams, *apexScene ) );
	}
#ifndef RED_FINAL_BUILD
	if( CPhysXLogger::GetLastErrorString().Size() && CPhysXLogger::IsLastErrorFromSameThread() )
	{
		CComponent* component = nullptr;
		if( GetParentProvider()->GetParent( component ) )
		{
			DATA_HALT( DES_Major, CResourceObtainer::GetResource( component ), TXT("Apex Destruction"), CPhysXLogger::GetLastErrorString().AsChar() );
		}
		CPhysXLogger::ClearLastError();
	}
#endif

	Uint32 actorCount = asset->getChunkCount();
	m_lowestKinematicsCount = ( float ) actorCount;
	AddActorRef( m_actor );

	if( !GGame->IsActive() )
	{
		m_actor->enableHardSleeping();
	}

	PxBounds3 pxBounds = m_actor->getBounds();
	m_bounds.Min = TO_VECTOR( pxBounds.minimum );
	m_bounds.Max = TO_VECTOR( pxBounds.maximum );

	::NxParameterized::Interface* runtimeParameters = const_cast< ::NxParameterized::Interface* >( m_actor->getNxParameterized( NxDestructibleParameterizedType::Params ) );
	NxParameterized::setParamF32( *runtimeParameters, "p3BodyDescTemplate.density", densityFractured );

	m_simulationType = SM_DYNAMIC;

	PHYSICS_STATISTICS_INC(DestructionsInstanced)

	return true;
}

Bool CApexDestructionWrapper::Destroy()
{
	if( !m_actor ) return false;

	PHYSICS_STATISTICS_DEC(DestructionsInstanced)

#ifndef NO_EDITOR
	for ( Uint32 i = 0; i < m_chunkDebugMeshes.Size(); ++i )
	{
		SAFE_RELEASE( m_chunkDebugMeshes[ i ] );
	}
	m_chunkDebugMeshes.Clear();
#endif

	NxDestructibleAsset* asset = reinterpret_cast<physx::apex::NxDestructibleAsset*>( m_actor->getOwner() );

	ReleaseActorRef( m_actor );
	// Don't clear out userdata, since someone else might still be holding a reference (e.g. a render command or something). userdata will
	// be NULL'd when the last reference is released.
	m_actor = NULL;

	CApexResource* resource = CApexResource::FromAsset( asset );
	resource->ReleaseAssetRef( asset );
	resource->ReleaseRef();
	m_world->ReleaseRef();

	return true;
}

Uint32 CApexDestructionWrapper::GetActorsCount() const
{
	if( !m_actor ) return 0;

	NxDestructibleAsset* asset = reinterpret_cast<physx::apex::NxDestructibleAsset*>( m_actor->getOwner() );
	if( !asset ) return 0;
	
	return asset->getChunkCount();
}

void* CApexDestructionWrapper::GetActor( Uint32 actorIndex ) const
{
	if( !m_actor ) return 0;

	PxRigidActor* actor = m_actor->getChunkPhysXActor( actorIndex );
	return actor;
}

void CApexDestructionWrapper::Release( Uint32 actorIndex )
{
	RED_ASSERT( m_ref.GetValue() > 0 )
	if( !m_ref.Decrement() )
	{
		m_world->GetWrappersPool< CApexDestructionWrapper, SWrapperContext >()->PushWrapperToRemove( this );
	}
}

void CApexDestructionWrapper::SetPose( const Matrix& localToWorld, Uint32 actorIndex )
{
	SetFlag( PRBW_PoseIsDirty, false );

	if( !m_actor ) return;

	Vector xAxis = localToWorld.V[ 0 ];
	Vector yAxis = localToWorld.V[ 1 ];
	Vector zAxis = localToWorld.V[ 2 ];
	const Vector& pos = localToWorld.V[ 3 ];
	xAxis.Normalize3();
	yAxis.Normalize3();
	zAxis.Normalize3();
	PxMat44 mat( TO_PX_VECTOR( xAxis ), TO_PX_VECTOR( yAxis ), TO_PX_VECTOR( zAxis ), TO_PX_VECTOR( pos ) );
	PxTransform pose( mat );

	m_actor->setGlobalPose( pose );

	//////////////////////////////////////////////////////////////////////////
	// NVIDIA STATIC CHUNK WORKAROUND
	//
	// There was a problem where static chunks could be rendered with the wrong
	// transform (identity plus global 0.01 scale). nVidia has provided a workaround,
	// with a small change to the Apex code, and this. The change in Apex fixes the
	// incorrect transform on static chunks. This allows for moving destructibles
	// in the editor (without this, the destructible stays in the same place until
	// you save or otherwise recreate it).
	//
	// The problem should be fixed in later Apex versions, so if we upgrade,
	// this call should be removed.
	m_actor->setInitialGlobalPose( pose );
	//////////////////////////////////////////////////////////////////////////

	SWrapperContext* position = m_world->GetWrappersPool< CApexDestructionWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	position->m_x = pos.X;
	position->m_y = pos.Y;
}

Matrix CApexDestructionWrapper::GetPose( Uint32 actorIndex ) const
{
	if( !m_actor ) return Matrix::IDENTITY;

	PxActor* actor = ( PxActor* ) GetActor( actorIndex );
	if( !actor ) return Matrix::IDENTITY;

	PxRigidDynamic* dynamicActor = actor->isRigidDynamic();
	if( !dynamicActor ) return Matrix::IDENTITY;
	if( dynamicActor->getNbShapes() == 0 ) return Matrix::IDENTITY;

	PxMat44 mat = dynamicActor->getGlobalPose();
	return TO_MAT( mat );
}

Vector CApexDestructionWrapper::GetPosition( Uint32 actorIndex ) const
{
	if( !m_actor ) return Vector::ZEROS;

	PxActor* actor = ( PxActor* ) GetActor( actorIndex );
	if( !actor ) return Vector::ZEROS;

	PxRigidDynamic* dynamicActor = actor->isRigidDynamic();
	if( !dynamicActor ) return Vector::ZEROS;
	if( dynamicActor->getNbShapes() == 0 ) return Vector::ZEROS;

	PxMat44 mat = dynamicActor->getGlobalPose();

	return Vector( mat.column3.x, mat.column3.y, mat.column3.z, 1.0f );
}

Vector CApexDestructionWrapper::GetCenterOfMassPosition( Uint32 actorIndex ) const
{
	if( !m_actor ) return Vector::ZEROS;

	PxActor* actor = ( PxActor* ) GetActor( actorIndex );
	if( !actor ) return Vector::ZEROS;

	PxRigidDynamic* dynamicActor = actor->isRigidDynamic();
	if( !dynamicActor ) return Vector::ZEROS;
	if( dynamicActor->getNbShapes() == 0 ) return Vector::ZEROS;

	PxMat44 mat = dynamicActor->getGlobalPose();

	PxRigidDynamic* dynamic = actor->isRigidDynamic();
	if( dynamic )
	{
		mat = mat * dynamic->getCMassLocalPose();
	}
	return Vector( mat.column3.x, mat.column3.y, mat.column3.z, 1.0f );
}

Bool CApexDestructionWrapper::FillResourceParameters( SDestructionParameters& parameters )
{
	CApexResource* resource = parameters.m_resource.Get();
	if( !resource ) return false; 

	NxDestructibleAsset* asset = GetResourceAsset( parameters );
	if( !asset ) return false;

	NxParameterized::Interface* descParams = asset->getDefaultActorDesc();
	if( !descParams ) return false;

	NxParameterized::getParamF32( *descParams, "destructibleParameters.damageCap", parameters.m_damageCap );
	NxParameterized::getParamF32( *descParams, "defaultBehaviorGroup.damageThreshold", parameters.m_damageThreshold );
	NxParameterized::getParamF32( *descParams, "defaultBehaviorGroup.damageToRadius", parameters.m_damageToRadius );
	NxParameterized::getParamI32( *descParams, "destructibleParameters.debrisDepth", parameters.m_debrisDepth );
	NxParameterized::getParamF32( *descParams, "destructibleParameters.debrisDestructionProbability", parameters.m_debrisDestructionProbability );
	NxParameterized::getParamF32( *descParams, "destructibleParameters.debrisLifetimeMin", parameters.m_debrisLifetimeMin );
	NxParameterized::getParamF32( *descParams, "destructibleParameters.debrisLifetimeMax", parameters.m_debrisLifetimeMax );
	NxParameterized::getParamF32( *descParams, "destructibleParameters.debrisMaxSeparationMin", parameters.m_debrisMaxSeparationMin );
	NxParameterized::getParamF32( *descParams, "destructibleParameters.debrisMaxSeparationMax", parameters.m_debrisMaxSeparationMax );
	NxParameterized::getParamU32( *descParams, "destructibleParameters.essentialDepth", parameters.m_essentialDepth );
	NxParameterized::getParamF32( *descParams, "destructibleParameters.debrisDestructionProbability", parameters.m_debrisDestructionProbability );
	NxParameterized::getParamF32( *descParams, "destructibleParameters.forceToDamage", parameters.m_forceToDamage );
	NxParameterized::getParamF32( *descParams, "destructibleParameters.fractureImpulseScale", parameters.m_fractureImpulseScale );
	NxParameterized::getParamI32( *descParams, "destructibleParameters.impactDamageDefaultDepth", parameters.m_impactDamageDefaultDepth );
	NxParameterized::getParamF32( *descParams, "destructibleParameters.impactVelocityThreshold", parameters.m_impactVelocityThreshold );
	NxParameterized::getParamF32( *descParams, "defaultBehaviorGroup.materialStrength", parameters.m_materialStrength );
	NxParameterized::getParamF32( *descParams, "destructibleParameters.maxChunkSpeed", parameters.m_maxChunkSpeed );
	NxParameterized::getParamU32( *descParams, "destructibleParameters.minimumFractureDepth", parameters.m_minimumFractureDepth );
	NxParameterized::getParamBool( *descParams, "structureSettings.useStressSolver", parameters.m_useStressSolver );
	NxParameterized::getParamF32( *descParams, "structureSettings.stressSolverTimeDelay", parameters.m_stressSolverTimeDelay );
	NxParameterized::getParamF32( *descParams, "structureSettings.stressSolverMassThreshold", parameters.m_stressSolverMassThreshold );
	NxParameterized::getParamU32( *descParams, "supportDepth", parameters.m_supportDepth );
	NxParameterized::getParamBool( *descParams, "useAssetDefinedSupport", parameters.m_useAssetDefinedSupport );
	NxParameterized::getParamBool( *descParams, "useWorldSupport", parameters.m_useWorldSupport );
	NxParameterized::getParamF32( *descParams, "sleepVelocityFrameDecayConstant", parameters.m_sleepVelocityFrameDecayConstant );
	NxParameterized::getParamBool( *descParams, "useHardSleeping", parameters.m_useHardSleeping );
	NxParameterized::getParamBool( *descParams, "destructibleParameters.flags.ACCUMULATE_DAMAGE", parameters.m_accumulateDamage );
	NxParameterized::getParamBool( *descParams, "destructibleParameters.flags.DEBRIS_TIMEOUT", parameters.m_debrisTimeout );
	NxParameterized::getParamBool( *descParams, "destructibleParameters.flags.DEBRIS_MAX_SEPARATION", parameters.m_debrisMaxSeparation );
	NxParameterized::getParamBool( *descParams, "destructibleParameters.flags.CRUMBLE_SMALLEST_CHUNKS", parameters.m_crumbleSmallestChunks );

	return true;
}

void CApexDestructionWrapper::PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd )
{
	PC_SCOPE_PHYSICS(CApexDest PreSimulation )

	PxBounds3 pxBounds;

	static Uint64 currentTickMarker = 0;
	if ( !m_actor )
	{
		const float distanceFromViewportSquared = simulationContext->m_resultDistanceSquared;
		if ( simulationContext->m_desiredDistanceSquared > distanceFromViewportSquared )
		{
			if( allowAdd && ( currentTickMarker < tickMarker || !tickMarker ) ) 
			{
				PC_SCOPE_PHYSICS(CApexDest Create2 )
				CDestructionSystemComponent* destructionComponent = nullptr;
				if ( !GetParent( destructionComponent ) ) return;
				Create( destructionComponent->GetParameters() );
				if( m_actor )
				{
					currentTickMarker = tickMarker;

					pxBounds = m_actor->getBounds();
					m_bounds.Min = TO_VECTOR( pxBounds.minimum );
					m_bounds.Max = TO_VECTOR( pxBounds.maximum );
					destructionComponent->RefreshRenderProxies();

					Uint32 count = ForEachSumResult< Uint32 >( destructionComponent->GetEntity()->GetComponents(), []( CComponent* component )-> int { CPhysicsWrapperInterface* wrapper = component->GetPhysicsRigidBodyWrapper(); return ( wrapper && wrapper->UpdatesEntity() ) ? 1 : 0; } );
					SetFlag( PRBW_UpdateEntityPose, count == 1 );

				}
				else return;
			}
			else
			{
				simulationContext->m_requestProcessingFlag = true;
			}
		}
	}

	if( m_actor )
	{
		pxBounds = m_actor->getBounds();
		m_bounds.Max = TO_VECTOR( pxBounds.maximum );
		m_bounds.Min = TO_VECTOR( pxBounds.minimum );

		const ERenderVisibilityResult visibilityQueryResult = ( const ERenderVisibilityResult ) simulationContext->m_visibilityQueryResult;

		m_actor->setVisible( visibilityQueryResult == ERenderVisibilityResult::RVR_Visible || visibilityQueryResult == ERenderVisibilityResult::RVR_NotTested );

	}
	else
	{
		m_bounds.Clear();
		pxBounds.setEmpty();
		return;
	}

	PxScene* scene = m_world->GetPxScene();

	const float distanceFromViewportSquared = simulationContext->m_resultDistanceSquared;
	const float desiredDistanceSquared = simulationContext->m_desiredDistanceSquared;
	Bool shouldBeSimulated = m_simulationType == SM_DYNAMIC && distanceFromViewportSquared < desiredDistanceSquared;
	Bool isHardSleepingEnabled = m_actor->isHardSleepingEnabled();
	Uint32 actorCount = GetActorsCount();
	if( !shouldBeSimulated && !isHardSleepingEnabled )
	{
		m_actor->enableHardSleeping();
		for( Uint32 i = 0; i != actorCount; ++i )
		{
			if( PxActor* actor = ( PxActor* ) GetActor( i ) )
			{
				if( PxRigidDynamic* rigidDynamic = actor->isRigidDynamic() )
				{
					if( !rigidDynamic->getScene() ) continue;
					if( rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) continue;
					rigidDynamic->putToSleep();
				}
			}
		}
	}
	else if( shouldBeSimulated && isHardSleepingEnabled && m_ref.GetValue() > 0 )
	{
		m_actor->disableHardSleeping( true );
	}

	if( GetFlag( PRBW_PoseIsDirty ) )
	{
		CComponent* component = nullptr;
		if( GetParentProvider()->GetParent( component ) )
		{
			component->GetTransform().CalcLocalToWorld(m_componentOffsetInverse);
			m_componentOffsetInverse.Invert();
			SetPose( component->GetLocalToWorld() );
		}
	}

	if( m_forceFracture && IsReady() )
	{
		PxActor* actor = ( PxActor* ) GetActor( 0 );
		if( actor )
		{
			if( PxRigidActor* rigidActor = actor->isRigidActor() )
			{
				PxVec3 position = rigidActor->getGlobalPose().p;
				::NxParameterized::Interface* runtimeParameters = const_cast< ::NxParameterized::Interface* >( m_actor->getNxParameterized( NxDestructibleParameterizedType::Params ) );
				NxParameterized::setParamF32( *runtimeParameters, "defaultBehaviorGroup.damageThreshold", 0.1f );
				m_actor->applyRadiusDamage( 1.0f, 3.0f, position, 0.5f, true );
				m_forceFracture = false;
			}
		}
	}
	Uint32 count = m_toProcessFracture.Size();
	if( count )
	{
		PC_SCOPE_PHYSICS(CApexDest fracturing )

		for( Uint32 i = 0; i != count; i++ )
		{
			SFractureRequest& fractureRequest = m_toProcessFracture[ i ];

			CComponent* fracturingVolumeComponent = Cast< CComponent >( fractureRequest.m_fracturingVolumeComponent.Get() );
			if( !fracturingVolumeComponent ) continue;
			PxActor* actor = ( PxActor* ) fracturingVolumeComponent->GetPhysicsRigidBodyWrapper()->GetActor();
			if( !actor ) continue;
			PxRigidActor* fracturingActor = actor->isRigidActor();

			if( !fracturingActor || fracturingActor->getNbShapes() == 0 ) continue;

			if( actorCount <= fractureRequest.m_actorIndex ) continue;

			PxActor* fractureActor = ( PxActor* ) GetActor( fractureRequest.m_actorIndex );
			if( !fractureActor ) continue;

			PxRigidActor* desctructionActor = fractureActor->isRigidActor();
			if( !desctructionActor ) continue;

			PxShape* fractureShape = 0;
			fracturingActor->getShapes( &fractureShape, 1, 0 );

			PxGeometryHolder fracturingActorGeometryHandler = fractureShape->getGeometry();
			PxGeometry& fracturingActorGeometry = fracturingActorGeometryHandler.any();

			PxTransform fractureTransform = fracturingActor->getGlobalPose() * fractureShape->getLocalPose();

#ifndef NO_EDITOR
			if( m_debugFracturePointsWhereTaken )
			{
				m_debugFracturePoints.PushBackUnique( TO_VECTOR( fractureTransform.p ) );
			}
#endif

			PxBounds3 fracturingActorGlobalBounds = fracturingActor->getWorldBounds();
			Uint32 shapesCount = desctructionActor->getNbShapes();
			Float fractureAmount = fractureRequest.m_amount * timeDelta;
			for( Uint32 j = 0; j != shapesCount; ++j )
			{
				PxShape* destructionShape = 0;
				desctructionActor->getShapes( &destructionShape, 1, j );
				PxGeometryHolder destructionHolder = destructionShape->getGeometry();
				PxConvexMeshGeometry& destructionGeometry = destructionHolder.convexMesh();

				PxTransform destuctionTransform = desctructionActor->getGlobalPose() * destructionShape->getLocalPose();
				PxBounds3 destructionLocalBounds = destructionGeometry.convexMesh->getLocalBounds();

				PxBounds3 destructionGlobalBounds = PxBounds3::transformFast( destuctionTransform, destructionLocalBounds );

				if( !fracturingActorGlobalBounds.intersects( destructionGlobalBounds ) && !destructionGlobalBounds.isInside( fracturingActorGlobalBounds ) ) continue;

				bool result = PxGeometryQuery::overlap( fracturingActorGeometry, fractureTransform, destructionGeometry, destuctionTransform );

				if( result )
				{
					PxVec3 center = destructionLocalBounds.getCenter();
					PxVec3 point = destuctionTransform.transform( center );
					m_actor->applyDamage( fractureAmount, 0.0f, point, PxVec3( 0.0f, 0.0f, 0.0f ), fractureRequest.m_actorIndex );
#ifndef NO_EDITOR
					if( m_debugFracturePointsWhereTaken )
					{
						m_debugFracturePoints.PushBackUnique( TO_VECTOR( point ) );
					}
#endif
				}
			}
		}
		m_toProcessFracture.ClearFast();
	}

	if( Uint32 amount = m_amountOfBaseFractures.GetValue() )
	{
		CDestructionSystemComponent* component;
		if( GetParent( component ) )
		{
			component->ScheduleTick( amount );
		}
		m_amountOfBaseFractures.SetValue( 0 );
	}

#ifndef NO_EDITOR
	if( !m_debugFracturePointsWhereTaken && m_debugFracturePoints.Size() ) 
	{
		m_debugFracturePoints.Clear();
	}
	m_debugFracturePointsWhereTaken = false;
#endif


/*	if( GetFlag( PRBW_DisableBuoyancy ) ) return;
	for ( Uint32 i = 0; i < actorCount; i++ )
	{
		PxActor* actor = m_actor->getChunkPhysXActor( i );

		if( !actor ) continue;
		PxRigidDynamic* dynamic = actor->isRigidDynamic();
		if( !dynamic ) continue;

		if( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) continue;

		ApplyBuoyancyForce( dynamic, SPhysicsSettings::m_destructionLinearDamper, SPhysicsSettings::m_destructionAngularDamper );
	}*/
}

void CApexDestructionWrapper::PostSimulationUpdateTransform( const Matrix& transformOrg, void* actorPtr )
{
	if( m_actor && m_actor->isHardSleepingEnabled() ) return;

	CComponent* component = nullptr;
	if( !GetParentProvider()->GetParent( component ) ) return;

	component->ScheduleUpdateTransformNode();

	physx::PxRigidActor* actor = ( physx::PxRigidActor* ) actorPtr;
	if( actor->getNbShapes() )
	{
		PxShape* shape = 0;
		actor->getShapes( &shape, 1, 0 );
		SActorShapeIndex& index = ( SActorShapeIndex& ) shape->userData;
		if( index.m_actorIndex > 0 )
		{
			if( m_actor )
			{
				Float distanceSquared = m_actor->getActorDistanceSquaredFromViewport( index.m_actorIndex );
				if( distanceSquared >= 0.0f )
				{
					//remove those dynamic fractured actors which are far from simualtion distance
					SWrapperContext* position = static_cast< CPhysicsWorldPhysXImpl* >( m_world )->GetWrappersPool< CApexDestructionWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
					if( position->m_desiredDistanceSquared < distanceSquared )
					{
						// HACK - apparently, when inventory is opened, player is recreated or teleported. This can cause getActorDistanceSquaredFromViewport to return a weirdly huge distance > 1 000 000.
						// Since this is already a deprecated destruction system, fixing it with a stupid sanity check hack 
						Vector refPos = m_world->GetReferencePosition();
						PxVec3 actorPos = actor->getGlobalPose().p ;
						Float refDist = (refPos.X - actorPos.x)*(refPos.X - actorPos.x) + (refPos.Y - actorPos.y)*(refPos.Y - actorPos.y);

						if( position->m_desiredDistanceSquared < refDist )
						{
							m_actor->setActorChunksToBeDestroyed( *actor );
						}
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

	if( !GetFlag( PRBW_UpdateEntityPose ) ) return;

	CEntity* entity = component->GetEntity();
	if( !entity ) return;

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

	SWrapperContext* position = m_world->GetWrappersPool< CApexDestructionWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	position->m_x = pos.X;
	position->m_y = pos.Y;
}

void CApexDestructionWrapper::ForceDynamicState()
{
	if( !m_actor ) return;

	m_actor->setDynamic();
}

Bool CApexDestructionWrapper::ApplyFracture()
{
	m_forceFracture = true;
	return true;
}

Bool CApexDestructionWrapper::ApplyFractureByVolume( Uint32 actorIndex, Float damageAmount, IScriptable* fracturingVolume )
{
	if( !m_actor ) return false;

	NxDestructibleAsset* asset = reinterpret_cast<physx::apex::NxDestructibleAsset*>( m_actor->getOwner() );
	if( !asset ) return false;

	Uint32 maxDepth = asset->getDepthCount();
	Uint32 currentDepth = asset->getChunkDepth( actorIndex );

	PxActor* actor = ( PxActor* ) GetActor( actorIndex );
	if( !actor ) return true;
	PxRigidActor* rigidActor = actor->isRigidActor();
	if( !rigidActor ) return true;
	if( currentDepth == maxDepth - 1 )
	{
		Uint32 shapesCount = rigidActor->getNbShapes();
		if( shapesCount <= 1 ) return true;
	}

	m_toProcessFracture.PushBackUnique( SFractureRequest( fracturingVolume, actorIndex, damageAmount, actor ) );
	return true;
}

Bool CApexDestructionWrapper::ApplyDamageAtPoint( Float amount, Float momentum, const Vector& position, const Vector& direction )
{
	return false;
}

Bool CApexDestructionWrapper::ApplyRadiusDamage( Float amount, Float momentum, const Vector& position, Float radius, Bool falloff )
{
	return false;
}

Float CApexDestructionWrapper::GetMass( Uint32 actorIndex ) const
{
	if( !m_actor ) return 0;

	PxActor* actor = ( PxActor* ) GetActor( actorIndex );
	if( !actor ) return 0;

	PxRigidDynamic* dynamicActor = actor->isRigidDynamic();
	if( !dynamicActor ) return 0;
	if( dynamicActor->getNbShapes() == 0 ) return 0;

	return dynamicActor->getMass();
}

physx::apex::NxApexActor* CApexDestructionWrapper::GetApexActor()
{
	return m_actor;
}

physx::apex::NxApexRenderable* CApexDestructionWrapper::AcquireRenderable()
{
	if ( !m_actor ) return NULL;
	return m_actor->acquireRenderableReference();
}

void CApexDestructionWrapper::ReleaseRenderable( physx::apex::NxApexRenderable* renderable )
{
	if ( !renderable ) return;

	NxDestructibleRenderable* rend = static_cast< NxDestructibleRenderable* >( renderable );
	rend->release();
}

void* CApexDestructionWrapper::GetChunkShape( Uint32 chunkIndex ) const
{
	PxActor* chunkActor = m_actor->getChunkPhysXActor( chunkIndex );
	if ( !chunkActor ) return NULL;

	PxRigidActor* rigidActor = chunkActor->isRigidActor();
	if ( !rigidActor ) return NULL;

	Uint32 numShapes = rigidActor->getNbShapes();
	TDynArray< PxShape* > shapeBuffer( numShapes );
	Uint32 numRead = rigidActor->getShapes( shapeBuffer.TypedData(), numShapes, 0 );
	ASSERT( numRead == numShapes, TXT("Didn't get all shapes from destructible actor. Guess we need to handle this.") );

	for ( Uint32 i = 0; i < numRead; ++i )
	{
		PxShape* shape = shapeBuffer[i];

		Uint32 shapeChunk = GetChunkIndexFromShape( shape );
		if ( shapeChunk == chunkIndex )
		{
			return shape;
		}
	}

	return NULL;
}


Matrix CApexDestructionWrapper::GetChunkLocalToWorld( Uint32 chunkIndex ) const
{
	PxActor* chunkActor = m_actor->getChunkPhysXActor( chunkIndex );
	if ( !chunkActor ) return Matrix::IDENTITY;

	PxRigidActor* rigidActor = chunkActor->isRigidActor();
	if ( !rigidActor ) return Matrix::IDENTITY;

	PxShape* chunkShape = ( PxShape* ) GetChunkShape( chunkIndex );
	if ( !chunkShape ) return Matrix::IDENTITY;
	
	return TO_MAT( PxMat44( rigidActor->getGlobalPose() * chunkShape->getLocalPose() ) );
}

const SPhysicalMaterial* CApexDestructionWrapper::GetPhysicalMaterial( bool fractured )
{
	return fractured ? m_fracturedPhysicalMaterial : m_physicalMaterial;
}

CPhysicalCollision& CApexDestructionWrapper::GetPhysicalCollisionType( Uint16 depth )
{
	if( depth == 0 )
	{
		return m_basePhysicalCollisionType;
	}
	return m_fracturedPhysicalCollisionType;
}

Float CApexDestructionWrapper::GetFractureRatio()
{
	if ( !m_actor ) return 0.0f;

	Uint32 actorCount = GetActorsCount();
	Float kinematicsCount = 0.0f;
	for( Uint32 i = 0; i != actorCount; ++i )
	{
		PxActor* actor = ( PxActor* ) GetActor( i );
		if( !actor ) continue;
		PxRigidDynamic* dynamic = actor->isRigidDynamic();
		if( !dynamic ) continue;

		if( !( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC ) ) continue;

		kinematicsCount += 1.0f;
	}

	if( m_lowestKinematicsCount > kinematicsCount )
	{
		m_lowestKinematicsCount = kinematicsCount;
	}

	return m_lowestKinematicsCount / actorCount;
}


float CApexDestructionWrapper::GetThresholdLeft( Uint32 actorIndex )
{
	if( !m_actor ) return -1.0f;
	const ::NxParameterized::Interface* runtimeParameters = m_actor->getNxParameterized( NxDestructibleParameterizedType::State );

	char tmpStr[128];
	sprintf_s(tmpStr, 128, "DestructibleActorChunks.data[%i].damage", actorIndex);

	float damage = -1.0f;
	NxParameterized::getParamF32( *runtimeParameters, tmpStr, damage );
	return damage;
}

#ifndef NO_EDITOR
IRenderResource* CApexDestructionWrapper::GetChunkDebugMesh( Uint32 chunkIndex )
{
	// Few simple checks for validity.
	if ( !m_actor ) return NULL;

	const physx::PxActor* chunkActor = m_actor->getChunkPhysXActor( chunkIndex );
	if ( !chunkActor ) return NULL;

	const physx::PxRigidActor* rigidActor = chunkActor->isRigidActor();
	if ( !rigidActor ) return NULL;

	const PxShape* chunkShape = ( const PxShape* ) GetChunkShape( chunkIndex );
	if ( !chunkShape ) return NULL;


	// Make room in the mesh list for this new one.
	m_chunkDebugMeshes.Reserve( chunkIndex + 1 );
	while ( m_chunkDebugMeshes.Size() <= chunkIndex )
	{
		m_chunkDebugMeshes.PushBack( NULL );
	}

	if ( m_chunkDebugMeshes[ chunkIndex ] )
		return m_chunkDebugMeshes[ chunkIndex ];


	CollisionFalseColoring coloring;
	TDynArray< DebugVertex > vertices;
	TDynArray< Uint32 > indices;

	if ( chunkShape->getGeometryType() == physx::PxGeometryType::eCONVEXMESH )
	{
		physx::PxConvexMeshGeometry& convexGeom = chunkShape->getGeometry().convexMesh();

		const physx::PxVec3* pxVertices = convexGeom.convexMesh->getVertices();
		const physx::PxU8* pxIndices = convexGeom.convexMesh->getIndexBuffer();

		physx::PxMaterial* pxMaterial = chunkShape->getMaterialFromInternalFaceIndex( 0 );
		const SPhysicalMaterial* mtl = (SPhysicalMaterial*)pxMaterial->userData;
		Uint32 color = coloring.MaterialColor( mtl->m_name );

		Uint32 numVertices = convexGeom.convexMesh->getNbVertices();
		vertices.Reserve( numVertices );

		PxTransform transform = PxTransform( PxMat44( convexGeom.scale.toMat33(), PxVec3( 0.0f ) ) );
		for ( Uint32 v = 0; v < numVertices; ++v )
		{
			physx::PxVec3 transformedPt = transform.transform( pxVertices[v] );
			Vector pos = TO_VECTOR( transformedPt );
			new ( vertices ) DebugVertex( pos, color );
		}


		Uint32 numPolys = convexGeom.convexMesh->getNbPolygons();
		// Make a rough guess at how much index space we'll need. We'll basically assume that each polygon will have on average
		// 3 triangles. Doesn't have to be exact -- just something to get our indices close to the right size, and hopefully
		// avoid a few re-allocations.
		const Uint32 guessTrianglesPerPolygon = 3;
		indices.Reserve( indices.Size() + numPolys * guessTrianglesPerPolygon*3 );

		for ( Uint32 p = 0; p < numPolys; ++p )
		{
			physx::PxHullPolygon poly;
			convexGeom.convexMesh->getPolygonData( p, poly );

			Uint32 numVerts = poly.mNbVerts;

			for ( Uint32 v = 1; v < numVerts - 1; ++v )
			{
				// Swap index ordering
				indices.PushBack( pxIndices[ (Uint32)poly.mIndexBase ] );
				indices.PushBack( pxIndices[ (Uint32)poly.mIndexBase + v + 1 ] );
				indices.PushBack( pxIndices[ (Uint32)poly.mIndexBase + v ] );
			}
		}
	}

	// If we have no vertices or indices, we have no mesh to draw.
	if ( vertices.Size() == 0 || indices.Size() == 0 )
		return NULL;

		
	IRenderResource* newMesh = GRender->UploadDebugMesh( vertices, indices );

	// Even if we couldn't create the mesh, this is fine. We'll just end up re-writing and returning a NULL.
	m_chunkDebugMeshes[ chunkIndex ] = newMesh;
	return newMesh;
}

const TDynArray< Vector >& CApexDestructionWrapper::GetDebugFracturePoints()
{
	m_debugFracturePointsWhereTaken = true;
	return m_debugFracturePoints;
}

#endif

#endif
