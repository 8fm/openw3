#include "build.h"
#include "boidLairEntity.h"
#include "boidManager.h"

#include "gameplayStorage.h"
#include "boidInstance.h"
#include "commonGame.h"
#include "animBoidNode.h"
#include "boidNodeData.h"
#include "sequenceBoidNode.h"
#include "pointOfInterestSpeciesConfig.h"

#include "../core/feedback.h"
#include "../engine/tickManager.h"
#include "../core/mathUtils.h"

#include "../engine/swarmRenderComponent.h"

//#include "../renderer/renderSwarmAnimationManager.h"
#include "../engine/renderSwarmData.h"

RED_DISABLE_WARNING_MSC( 4355 )										// 'this' used in member initializer list

/////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( IBoidLairEntity );

/////////////////////////////////////////////////////////////////////////
// CDynamicPointsAcceptor
/////////////////////////////////////////////////////////////////////////
static IBoidLairEntity::CPlayerDynamicPointsAcceptor s_PlayerDynamicPointsAcceptor;
static IBoidLairEntity::CActorDynamicPointsAcceptor s_ActorDynamicPointsAcceptor;

Bool IBoidLairEntity::CDynamicPointsAcceptor::IsGlobal() const			{ return false; }
IBoidLairEntity::CDynamicPointsAcceptor::~CDynamicPointsAcceptor()		{}
Bool IBoidLairEntity::CGlobalDynamicPointsAcceptor::IsGlobal() const	{ return true; }

Bool IBoidLairEntity::CPlayerDynamicPointsAcceptor::AcceptEntity( CEntity* entity )
{
	return entity->IsA< CPlayer >();
}

IBoidLairEntity::CPlayerDynamicPointsAcceptor* IBoidLairEntity::CPlayerDynamicPointsAcceptor::GetInstance()
{
	return &s_PlayerDynamicPointsAcceptor;
}

Bool IBoidLairEntity::CActorDynamicPointsAcceptor::AcceptEntity( CEntity* entity )
{
	return entity->IsA< CActor >();
}

IBoidLairEntity::CActorDynamicPointsAcceptor* IBoidLairEntity::CActorDynamicPointsAcceptor::GetInstance()
{
	return &s_ActorDynamicPointsAcceptor;
}

Bool IBoidLairEntity::CTagDynamicPointsAcceptor::AcceptEntity( CEntity* entity )
{
	return entity->GetTags().HasTag( m_tag );
}

IBoidLairEntity::CComplexDynamicPointsAcceptor::~CComplexDynamicPointsAcceptor()
{
	if ( !m_a1->IsGlobal() )
		delete m_a1;

	if ( !m_a2->IsGlobal() )
		delete m_a2;
}

Bool IBoidLairEntity::CANDDynamicPointsAcceptor::AcceptEntity( CEntity* entity )
{
	return m_a1->AcceptEntity( entity ) && m_a2->AcceptEntity( entity );
}


Bool IBoidLairEntity::CORDynamicPointsAcceptor::AcceptEntity( CEntity* entity )
{
	return m_a1->AcceptEntity( entity ) || m_a2->AcceptEntity( entity );
}



/////////////////////////////////////////////////////////////////////////
// IBoidLairEntity
/////////////////////////////////////////////////////////////////////////
IBoidLairEntity::IBoidLairEntity()
	: m_boidSpeciesName( CName::NONE )
	, m_params( NULL )
	, m_time ( 0.0f )
	, m_isActivated( false )
	, m_isVisible( false )
	, m_visibilityUpdateDelay( 2.122111f )
	, m_spawnFrequency( 10.0f )
	, m_range( 10.0f )
	, m_visibilityRange( 128.f )
	, m_totalLifetimeSpawnLimit( -1 )
	, m_spawnLimit( 64 )
	, m_activeBoids( 0 )
	, m_maxActiveBoidIndex( 0 )
	, m_nextPointOfInterestId( 1 )
	, m_boundingBox( Vector( 0,0,0 ), Vector( 0,0,0 ))
	, m_soundsCollectionArray( )
	, m_swarmRenderComponent( nullptr )
	, m_swarmSoundComponent( nullptr )
	, m_fullyInitialized( false )
{
	SetForceNoLOD( true );
}

void IBoidLairEntity::SetupFromTool( CEntity *const boidArea, CName speciesName )
{ 
	m_lairBoundings.Set( boidArea ); 
	if ( speciesName != CName::NONE )
	{
		m_boidSpeciesName = speciesName; 
	}
}
IBoidLairEntity::~IBoidLairEntity()
{
	for ( Uint32 i = 0, n = m_boidInstances.Size(); i != n; ++i )
	{
		if ( m_boidInstances[ i ] )
		{
			delete m_boidInstances[ i ];
		}
	}

	for ( Uint32 i = 0, n = m_dynamicPointTypes.Size(); i != n; ++i )
	{
		if ( !m_dynamicPointTypes[ i ]->IsGlobal() )
		{
			delete m_dynamicPointTypes[ i ];
		}
	}

	for ( Uint32 i = 0; i < m_soundsCollectionArray.Size(); ++i )
	{
		delete m_soundsCollectionArray[i];
	}
}

void IBoidLairEntity::ComputeStaticPointsOfInterest()
{
	CAreaComponent* boundingsArea = NULL;
	CEntity* lairBoundingsEntity = m_lairBoundings.Get();
	if ( lairBoundingsEntity )
	{
		boundingsArea = lairBoundingsEntity->FindComponent< CAreaComponent >();
	}

	// Points of interest
	struct Functor
	{
		IBoidLairEntity* m_lair;
		CAreaComponent* m_boundingsArea;
		TDynArray< THandle< CEntity > >* m_triggers;

		enum { SORT_OUTPUT = false };

		Functor( IBoidLairEntity* lair, CAreaComponent* boundingsArea, TDynArray< THandle< CEntity > >* triggers )
			: m_lair( lair ), m_boundingsArea( boundingsArea ), m_triggers( triggers ) {}

		RED_INLINE Bool operator()( TPointerWrapper< CGameplayEntity > member )
		{
			Bool entityOverlaps = false;
			const CGameplayEntity* entity = member.Get();
			Vector entityPosition = entity->GetWorldPositionRef();

			// Check if entity contains point of interest and if so store it
			CBoidPointOfInterestComponent* pointOfInterest = entity->FindComponent<CBoidPointOfInterestComponent>();
			if( pointOfInterest == nullptr )
				return true;

			// Check in lair area component
			if ( m_boundingsArea )
			{
				if( m_boundingsArea->TestPointOverlap( entityPosition ) )
				{
					entityOverlaps = true;
				}
			}
			
			// Check in lair triggers if POI has proper flag
			if( pointOfInterest->GetAcceptor() == ZA_BothTriggersAndLairArea )
			{
				for ( Uint32 i = 0; i < m_triggers->Size(); ++i )
				{
					const CEntity* triggerEntity = (*m_triggers)[ i ].Get();
					if ( triggerEntity )
					{
						const CAreaComponent* areaComponent = triggerEntity->FindComponent< CAreaComponent >();
						if( areaComponent != nullptr && areaComponent->TestPointOverlap( entityPosition ) )
						{
							entityOverlaps = true;
						}
					}
				}
			}

			if( entityOverlaps )
			{
				m_lair->HandleStaticPointOfInterestNoticed( pointOfInterest );
			}

			return true;
		}
	};

	// Query in area component and triggers' area components
	Functor functor( this, boundingsArea, &m_triggers );
	Box queryBox;
	queryBox.Clear();

	if( boundingsArea != nullptr )
	{
		queryBox.AddBox( boundingsArea->GetBoundingBox() );
	}

	// Add triggers' area component bounding boxes
	for ( Uint32 i = 0; i < m_triggers.Size(); ++i )
	{
		const CEntity* triggerEntity = m_triggers[ i ].Get();
		if ( triggerEntity )
		{
			const CAreaComponent* areaComponent = triggerEntity->FindComponent< CAreaComponent >();
			if( areaComponent != nullptr )
			{
				queryBox.AddBox( areaComponent->GetBoundingBox() );
			}
		}
	}

	if( queryBox.IsEmpty() == false )
	{
		GCommonGame->GetGameplayStorage()->TQuery( Vector::ZEROS, functor, queryBox, true, NULL, 0 );
	}
}

void IBoidLairEntity::OnTimer(const CName name, Uint32 id, Float timeDelta)
{
	TBaseClass::OnTimer( name, id, timeDelta );

	if( name == CNAME( Update ) )
	{
		LazyInitialize();	// Delayed initialize - due to lots of dependencies between lair, it's area and points of interest
		UpdateTime(timeDelta);
		UpdateAnimation();
		UpdateSwarmBoundingBox();
	}
}

void IBoidLairEntity::UpdateTime(Float timeDelta)
{
	m_time += timeDelta;
}

void IBoidLairEntity::UpdateAnimation()
{
	while( m_loopedAnimPriorityQueue.Empty() == false)
	{
		const CBoidNodeData animNodeData(m_loopedAnimPriorityQueue.Front());
		if (m_time < animNodeData.m_timeOut)
		{
			break;
		}
		m_loopedAnimPriorityQueue.PopHeap();
		CBoidInstance *const boidInstance = animNodeData.GetBoidInstance();
		if (animNodeData.m_stateCounter == boidInstance->GetCurrentStateCounter())
		{
			boidInstance->ActivateNextAnimNode(m_loopedAnimPriorityQueue, m_time);
		}
	}
}

void IBoidLairEntity::UpdateSwarmBoundingBox()
{
	EulerAngles iHaveNoIdeaWhatThatIs( 0.0f, -90.0f, 0.0f );	// Where does this angle offset come from?

	// update swarm render data
	// TODO: is this the best place? (might cause 1 frame behind issues?)
	if( m_swarmRenderComponent.IsValid() )
	{
		CSwarmBoidData* writeData = m_swarmRenderComponent->GetWriteData();
		CSkeleton* skeleton = m_swarmRenderComponent->GetSkeleton();
		Uint32 iIndex = 0;
		Box swarmBounds;
		swarmBounds.Clear();

		if( writeData )
		{
			for( Uint32 i=0; i<m_boidInstances.Size(); ++i )
			{
				// is boid spawned?
				CBoidInstance* boid = m_boidInstances[ i ];
				if( !boid || boid->GetBoidState() < BOID_STATE_IDLE )
				{
					continue;
				}

				if ( boid->GetAnimation().Empty() )
				{
					continue;
				}

				// exceeded max amount of boids?
				if( iIndex >= CSwarmRenderComponent::MAX_NUM_BOIDS )
				{
					// TODO: some error message?
					break;
				}

				// write swarm data
				CSwarmBoidData& data		= writeData[ iIndex++ ];
				data.m_position				= boid->GetPosition();
				data.m_rotation				= boid->GetOrientation() + iHaveNoIdeaWhatThatIs;
				data.m_scale				= boid->GetScale();
				data.m_currentAnimation		= boid->GetAnimation();

				// create swarm bounding box
				swarmBounds.AddPoint( data.m_position );
			}

			// set swarm bounds & boid count
			swarmBounds.Extrude( 1.0f );			// assuming we won't have bigger boids than 1 meter
			m_swarmRenderComponent->SetBoundingBox( swarmBounds );
			m_swarmRenderComponent->SetBoidCount( iIndex );
		}
	}
}

void IBoidLairEntity::NoticeFireInCone( const Vector& position, const Vector2& coneDir, Float coneHalfAngle, Float coneRange )
{
	// should be reimplemented in subclasses
}
void IBoidLairEntity::OnStaticPointOfInterestAdded( CBoidPointOfInterestComponent* poi )
{
	HandleStaticPointOfInterestNoticed( poi );
}

void IBoidLairEntity::OnStaticPointOfInterestRemoved( CBoidPointOfInterestComponent* poi )
{
	const Boids::PointOfInterestType &type = poi->GetParameters().m_type;
	
	auto removeSpawns = [poi]( const CPoiItem& item ) { return item.m_item.Get() == poi; };
	m_spawnPoints.Erase( RemoveIf( m_spawnPoints.Begin(), m_spawnPoints.End(), removeSpawns ), m_spawnPoints.End() );	

	auto itFind = m_staticPointsOfInterest.Find( type );
	if ( itFind != m_staticPointsOfInterest.End() )
	{
		auto& list = itFind->m_second;
		for ( auto it = list.Begin(), end = list.End(); it != end; ++it )
		{
			if ( it->m_item.Get() == poi )
			{
				RemoveStaticPointOfInterest( poi, it->m_uid );
				list.Erase( it );
				break;
			}
		}
	}
}

Bool IBoidLairEntity::OnDynamicComponentEntered( CComponent* poi )
{
	if ( !poi )
		return false;
	CEntity* entity = poi->GetEntity();
	if ( !entity )
		return false;

	LazyInitialize();
	RED_ASSERT( m_dynamicPointTypes.Size() > 0, TXT("Dynamic point types in boid lair entity should not be empty at this stage. Something with initializing or attaching swarms to world is wrong.") );

	for ( auto it = m_dynamicPointTypes.Begin(), end = m_dynamicPointTypes.End(); it != end; ++it )
	{
		if ( (*it)->AcceptEntity( entity ) )
		{
			CollectDynamicPointOfInterest( entity, (*it)->GetFilterTag() );
			return true;
		}
	}
	return false;
}
void IBoidLairEntity::OnDynamicComponentLeft( CComponent* poi )
{
	if ( !poi )
		return;
	CEntity* entity = poi->GetEntity();
	if ( !entity )
		return;

	for ( auto it = m_dynamicPointTypes.Begin(), end = m_dynamicPointTypes.End(); it != end; ++it )
	{
		if ( (*it)->AcceptEntity( entity ) )
		{
			RemoveDynamicPointOfInterest( entity );;
		}
	}
}

CBoidInstance* IBoidLairEntity::SpawnInstance( Uint32 index, const Vector& position, const EulerAngles& orientation )
{
	CWorld* world = GetLayer()->GetWorld();
	if ( !world )
	{
		return nullptr;
	}

	if ( FindComponent< CSwarmRenderComponent >() == nullptr )
	{
		return nullptr;
	}

	RED_MESSAGE( "This should probably use an equality check with a tolerance/epsilon" );
	const Float scale			= m_params->m_boidScaleMax != m_params->m_boidScaleMin ? GEngine->GetRandomNumberGenerator().Get< Float >( m_params->m_boidScaleMin , m_params->m_boidScaleMax ) : m_params->m_boidScaleMin;
	CBoidInstance* instance		= new CBoidInstance( this, position, orientation, scale );
	m_boidInstances[ index ]	= instance;
	AddActiveBoid();

	return instance;
}

Bool IBoidLairEntity::ActivateLair()
{
	if ( m_isActivated == false )
	{
		if ( m_params->IsValid() )
		{
			m_isActivated = true;
			CWorld* world = GetLayer()->GetWorld();
			if ( world )
			{
				AddTimer( CNAME( Update ), 0.0f, true, false, TICK_PrePhysicsPost );
			}

			CBoidManager::GetInstance()->OnLairActivated();
		}
		else
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("boid species parameters not found\n") );
		}
	}
	if ( m_isActivated )
	{
		if ( m_swarmRenderComponent.IsValid() )
		{	
			m_swarmRenderComponent->SetBoidTemplateHandle( m_params->m_boidTemplateHandle );
			m_swarmRenderComponent->OnLairActivated();
		}

		if ( m_swarmSoundComponent )
		{
			m_swarmSoundComponent->OnLairActivated();
		}
	}
	return m_isActivated;
}


void IBoidLairEntity::DeactivateLair()
{
	if ( m_isActivated )
	{
		m_isActivated = false;

		CWorld* world = GetLayer()->GetWorld();
		if ( world )
		{
			RemoveTimer( CNAME( Update ), TICK_PrePhysicsPost );
		}

		if ( m_swarmRenderComponent.IsValid() )
		{
			m_swarmRenderComponent->OnLairDeactivated();
		}
		if ( m_swarmSoundComponent )
		{
			m_swarmSoundComponent->OnLairDeactivated();
		}

		CBoidManager::GetInstance()->OnLairDeactivated();
	}
}

void IBoidLairEntity::OnInitialized()
{
	TBaseClass::OnInitialized();

	// Find swarm render component
	m_swarmRenderComponent = FindComponent< CSwarmRenderComponent >();
	#ifndef NO_EDITOR
	if( m_swarmRenderComponent.IsValid() == false )
	{
		WARN_ENGINE( TXT("[Swarm] Lair %ls has no CSwarmRenderComponent in template. This means corrupted entity in world. Missing components will be created at runtime so everything works."), this->GetName().AsChar() );
	}
	#endif

	// [HACK] double check that streamed flag is set to false
	if ( m_swarmRenderComponent.IsValid() && m_swarmRenderComponent->IsStreamed() )
	{
		RemoveComponent( m_swarmRenderComponent );
		m_swarmRenderComponent = nullptr;
	}
	
	// [HACK] if the entity doesn't have a component from template
	if( !m_swarmRenderComponent )
	{
		m_swarmRenderComponent = static_cast< CSwarmRenderComponent * >( CreateComponent( ClassID< CSwarmRenderComponent >(), SComponentSpawnInfo() ) );
	}

	// Find swarm sound component
	m_swarmSoundComponent = FindComponent< CSwarmSoundEmitterComponent >();
	#ifndef NO_EDITOR
	if( m_swarmSoundComponent == nullptr )
	{
		WARN_ENGINE( TXT("[Swarm] Lair %ls has no CSwarmSoundEmitterComponent in template. This means corrupted entity in world. Missing components will be created at runtime so everything works."), this->GetName().AsChar() );
	}
	#endif

	// [HACK]
	if( m_swarmSoundComponent == nullptr )
	{
		m_swarmSoundComponent = static_cast< CSwarmSoundEmitterComponent * >( CreateComponent( ClassID< CSwarmSoundEmitterComponent >(), SComponentSpawnInfo() ) );
	}

	// Setup boid instances array
	#ifndef NO_EDITOR
	RED_ASSERT( IsInGame() == false || m_spawnLimit > 0, TXT("[Swarm] Lair %ls has m_spawnLimit == 0, it's useless."), this->GetName().AsChar() );
	#endif
	m_boidInstances.Resize( m_spawnLimit );
}

void IBoidLairEntity::OnUninitialized()
{
	TBaseClass::OnUninitialized();

	// Clear runtime data
	m_loopedAnimPriorityQueue.Clear();
	m_triggers.Clear();
	m_spawnPoints.Clear();
}

//! Entity was attached to world
void IBoidLairEntity::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// Register boid lair entity
	CBoidManager::GetInstance()->AddLair( this );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Swarms );
}


void IBoidLairEntity::OnDetached( CWorld* world )
{
	for ( Uint32 i = 0; i < m_soundsCollectionArray.Size(); ++i )
	{
		m_soundsCollectionArray[ i ]->FadeOutAll();
	}
	if ( m_isActivated )
	{
		DeactivateLair();
		RemoveTimer( CNAME( Update ), TICK_PrePhysicsPost );
	}

	CBoidManager::GetInstance()->RemoveLair( this );

	for ( Uint32 i = 0, n = m_boidInstances.Size(); i != n; ++i )
	{
		if ( m_boidInstances[ i ] )
		{
			delete m_boidInstances[ i ];
			m_boidInstances[ i ] = 0;
		}
	}
	m_boidInstances.ClearFast();
	m_activeBoids			= 0;
	m_maxActiveBoidIndex	= 0;

	TBaseClass::OnDetached( world );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Swarms );
}

void IBoidLairEntity::DelayedInitialize()
{
	// Initialize lair boundings
	m_boundingBox = Box( GetWorldPositionRef(), m_range );
	CEntity* lairBoundingsEntity	= m_lairBoundings.Get();
	#ifndef NO_EDITOR
	RED_ASSERT( IsInGame() == false || lairBoundingsEntity != nullptr, TXT("[Swarm] Lair %ls has no bounding area setup."), this->GetName().AsChar() );
	#endif

	if ( lairBoundingsEntity )
	{
		CAreaComponent* boundingsArea = lairBoundingsEntity->FindComponent< CAreaComponent >();
		#ifndef NO_EDITOR
		RED_ASSERT( IsInGame() == false || boundingsArea != nullptr, TXT("[Swarm] Lair %ls has invalid bounding area setup. Check if bounding entity %ls has CAreaComponent"), this->m_name.AsChar(), lairBoundingsEntity->GetName().AsChar() );
		#endif

		if ( boundingsArea )
		{
			m_boundingBox = boundingsArea->GetBoundingBox();
		}
	}

	if ( IsInGame() )
	{
		DeterminePointsOfInterestTypes();
		// In case Poi were added to the world before the BoidLair Entity : 
		ComputeStaticPointsOfInterest();
	}
}

void IBoidLairEntity::OnActivationTriggerAdded( CEntity *const activationtrigger )
{ 
	m_triggers.PushBack( activationtrigger ); 
}
void IBoidLairEntity::OnActivationTriggerRemoved( CEntity *const activationtrigger )
{ 
	m_triggers.Remove( activationtrigger ); 
}


Bool IBoidLairEntity::ContainsPoint( const Vector& location, Bool testBothArea )
{
	CEntity* lairBoundingsEntity = m_lairBoundings.Get();
	if ( lairBoundingsEntity )
	{
		CAreaComponent* boundingsArea = lairBoundingsEntity->FindComponent< CAreaComponent >();
		if ( boundingsArea )
		{
			if ( boundingsArea->TestPointOverlap( location ) )
			{
				return true;
			}
		}
	}
	if ( testBothArea == false )
	{
		return false;
	}
	for ( Uint32 i = 0; i < m_triggers.Size(); ++i )
	{
		const CEntity *const entity									= m_triggers[ i ].Get();
		if ( entity )
		{
			CAreaComponent *const	areaComponent	= static_cast< CAreaComponent* >( entity->FindComponent< CAreaComponent >() );
			if ( areaComponent )
			{
				//if ( MathUtils::GeometryUtils::IsPointInPolygon2D( areaComponent->GetWorldPoints(), location.AsVector2() ) )
				if ( areaComponent->TestPointOverlap( location ) )
				{
					return true;
				}
			}
		}
	}
	return false;
}

CAreaComponent* IBoidLairEntity::GetLairBoundings() const
{
	CAreaComponent* boundingsArea = NULL;
	const CEntity* lairBoundingsEntity = m_lairBoundings.Get();
	if ( lairBoundingsEntity )
	{
		boundingsArea = lairBoundingsEntity->FindComponent< CAreaComponent >();
	}
	return boundingsArea;
}

void IBoidLairEntity::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		for ( Int32 i = 0; i < m_maxActiveBoidIndex; ++i )
		{
			if ( m_boidInstances[ i ] )
			{
				// TODO: store boid position, orientation & scale?

				/*CEntity* entity = m_boidInstances[ i ]->GetEntity();
				if ( entity )
				{
					file << entity;
				}*/
			}
		}
	}
}

void IBoidLairEntity::RemoveActiveBoid()
{
	--m_activeBoids;
	ASSERT( m_activeBoids >= 0 );
	while ( m_maxActiveBoidIndex > 0 && (m_boidInstances[ m_maxActiveBoidIndex-1 ] == NULL|| m_boidInstances[ m_maxActiveBoidIndex-1 ]->GetBoidState() == BOID_STATE_NOT_SPAWNED) )
	{
		--m_maxActiveBoidIndex;
	};
}

Bool IBoidLairEntity::GetSubObjectWorldMatrix( Uint32 index, Matrix& matrix ) const
{
	const Uint32 soundCollectionId		= index / 100;
	const Uint32 soundIndex				= index - 100 * soundCollectionId;
	const Uint32 soundCollectionIndex	= GetSoundCollectionIndexFromId( soundCollectionId );
	if ( soundCollectionIndex == (Uint32)-1 )
	{
		matrix = Matrix::IDENTITY;
		return false;
	}
	CBoidSound* sound = m_soundsCollectionArray[ soundCollectionIndex ]->GetSound( soundIndex );
	if ( sound == NULL )
	{
		matrix = Matrix::IDENTITY;
		return false;
	}
	sound->GetLocalToWorld( matrix );
	return true;
}

void IBoidLairEntity::HandleStaticPointOfInterestNoticed( CBoidPointOfInterestComponent* pointOfInterest )
{
	if ( m_params == nullptr )
	{
		return;
	}
	
	LazyInitialize();

	Bool collected = false;
	const Boids::PointOfInterestType &type = pointOfInterest->GetParameters().m_type;
	// check if point is spawn point
	// NOTICE: no 'else'

	auto spawnPointIt = Find( m_params->m_spawnPointArray.Begin(), m_params->m_spawnPointArray.End(), type );
	if ( spawnPointIt != m_params->m_spawnPointArray.End() )
	{
		CPoiItem item( pointOfInterest, m_nextPointOfInterestId++ );
		m_spawnPoints.PushBack( item );
		collected = true;
	}
	// check if point is point of interest
	// NOTICE: no 'else' - component can be spawn point AND point of interest
	TPointsMap::iterator itr = m_staticPointsOfInterest.Find( type );
	if ( itr != m_staticPointsOfInterest.End() )
	{
		CPoiItem_Array& points = itr->m_second;

		CPoiItem item( pointOfInterest, m_nextPointOfInterestId++ );
		points.PushBack( item );
		CollectStaticPointOfInterest( pointOfInterest, item.m_uid );
		collected = true;
	}
	if ( collected )
	{
		pointOfInterest->AddToLair( this );
	}
}

void IBoidLairEntity::AddStaticPointOfInterestType( Boids::PointOfInterestType poiType )
{
	m_staticPointsOfInterest.Insert( poiType, CPoiItem_Array() );
}
void IBoidLairEntity::AddDynamicPointOfInterestAcceptor( CDynamicPointsAcceptor* poiAcceptor )
{
	m_dynamicPointTypes.PushBack( poiAcceptor );
}
void IBoidLairEntity::DeterminePointsOfInterestTypes()
{
	// nothing
}


void IBoidLairEntity::CollectStaticPointOfInterest( CBoidPointOfInterestComponent* poi, Boids::PointOfInterestId id )
{
	
}
void IBoidLairEntity::RemoveStaticPointOfInterest( CBoidPointOfInterestComponent* poi, Boids::PointOfInterestId id )
{
	
}

void IBoidLairEntity::CollectDynamicPointOfInterest( CEntity* entity, const CName& filterTag )
{

}

void IBoidLairEntity::RemoveDynamicPointOfInterest( CEntity* entity )
{

}

void IBoidLairEntity::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	CGameplayEntity::OnGenerateEditorFragments( frame, flag );

	// Displaying sound targets
	for ( Uint32 j = 0; j < m_soundsCollectionArray.Size(); ++j )
	{
		const TDynArray< CBoidSound* > & runningSoundArray =	m_soundsCollectionArray[ j ]->GetRunningSoundArray();
		for ( Uint32 i = 0; i < runningSoundArray.Size(); ++i )
		{
			CBoidSound* sound = runningSoundArray[ i ];
			sound->OnGenerateEditorFragments( frame, flag );
		}
	}
}

Uint32 IBoidLairEntity::GetSoundCollectionIndexFromId( Uint32 id )const
{
	for ( Uint32 i = 0; i < m_soundsCollectionArray.Size(); ++i )
	{
		CBoidSoundsCollection *const  collection =  m_soundsCollectionArray[ i ];
		if ( collection->GetId() == id )
		{
			return i;
		}
	}
	return (Uint32)-1;
}

CSkeleton* IBoidLairEntity::GetSkeleton() const
{ 
	return m_swarmRenderComponent.IsValid() ? m_swarmRenderComponent->GetSkeleton() : nullptr; 
}

void IBoidLairEntity::LazyInitialize()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Lazy swarm initialization can be done only on main thread" );
	if( m_fullyInitialized == false )
	{
		m_fullyInitialized = true;
		DelayedInitialize();
	}
}

void funcIsLocationInSwarmLair( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	FINISH_PARAMETERS;
	Bool isInSwarmLair = false;
	CBoidManager* boidManager = CBoidManager::GetInstance();
	if ( boidManager->GetActiveLairs() > 0 )
	{
		isInSwarmLair = boidManager->FindLair( point ) != NULL;
	}
	RETURN_BOOL( isInSwarmLair );
}

void funcCastFireInCone( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Float, coneDir, 0.f );
	GET_PARAMETER( Float, coneAngle, 90.f );
	GET_PARAMETER( Float, range, 15.f );
	FINISH_PARAMETERS;

	Box testBBox( Box::RESET_STATE );

	Vector2 dir = EulerAngles::YawToVector2( coneDir );
	Vector2 reach = dir * range;
	Float halfAngle = coneAngle / 2.f;

	testBBox.AddPoint( point );
	testBBox.AddPoint( point + reach );
	testBBox.AddPoint( point + MathUtils::GeometryUtils::Rotate2D( reach, DEG2RAD(halfAngle) ) );
	testBBox.AddPoint( point + MathUtils::GeometryUtils::Rotate2D( reach, DEG2RAD(-halfAngle) ) );

	CBoidManager* boidManager = CBoidManager::GetInstance();

	auto functor =
		[ &testBBox, &point, dir, halfAngle, range ] ( IBoidLairEntity* lair )
		{
			if ( lair->GetBoundingBox().Touches( testBBox ) )
			{
				lair->NoticeFireInCone( point, dir, halfAngle, range );
			}
		};
	boidManager->IterateLairs( functor );
}



void RegisterBoidsRelatedScriptFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "Boids_IsLocationInSwarmLair", funcIsLocationInSwarmLair );
	NATIVE_GLOBAL_FUNCTION( "Boids_CastFireInCone", funcCastFireInCone );
}





//////////////////////////////////////////////////////////////////////////////////////
//				CBoidLairParams														//
//////////////////////////////////////////////////////////////////////////////////////
CBoidLairParams::CBoidLairParams( Uint32 type, CName typeName, Bool isValid )
	: m_name( CName::NONE )
	, m_typeName( typeName )
	, m_type( type )
	, m_spawnPointArray()
	, m_boidScaleMin( 1.f )
	, m_boidScaleMax( 1.f )
	, m_boidTemplateHandle( )
	, m_allowAnimationInteruption( true )
	, m_isValid( isValid )
	, m_soundBank()
{

}

CBoidLairParams::~CBoidLairParams()
{
	
}



Bool CBoidLairParams::CopyTo( CBoidLairParams* const params )const
{
	const Uint32 savedType		= params->m_type;
	const CName savedTypeName	= params->m_typeName;
	const Bool savedIsValid		= params->m_isValid; 
	if ( VirtualCopyTo(params) == false )
	{
		delete params;
		return false;
	}
	params->m_type		= savedType;
	params->m_typeName	= savedTypeName;
	params->m_isValid	= savedIsValid;
	return true;
}

Bool CBoidLairParams::VirtualCopyTo( CBoidLairParams* const params )const
{
	*params = *this;
	return true;
}

const CAtomicBoidNode *const CBoidLairParams::GetFirstAtomicNodeFromState( Boids::BoidState state )const
{
	const CBaseBoidNode *const rootBoidNode = (state >= 0 && state < (Int32)m_boidStateArray.Size()) ? m_boidStateArray[ state ].m_rootBoidNode : NULL;
	if ( rootBoidNode == NULL )
	{
		return NULL;
	}
	const CAtomicBoidNode *const pAtomicNode = rootBoidNode->As<CAtomicBoidNode>();
	if (pAtomicNode)
	{
		return pAtomicNode;
	}
	return rootBoidNode->GetNextAtomicAnimNode();
}

const CPointOfInterestSpeciesConfig *const CBoidLairParams::GetPOISpeciesConfigFromType(CName poiType)const
{
	CPointOfInterestSpeciesConfig_Map::const_iterator itFind = m_poiSpeciesConfigMap.Find( poiType );
	if ( itFind != m_poiSpeciesConfigMap.End() )
	{
		return itFind->m_second;
	}
	return NULL;
}


Bool CBoidLairParams::ParseXML( const SCustomNode & paramsNode, CBoidSpecies *const boidSpecies )
{
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= paramsNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for (attIt = paramsNode.m_attributes.Begin();  attIt !=  attEnd; ++ attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if (false == ParseXmlAttribute( att ))
		{
			GFeedback->ShowError(TXT("Boid XML Error: problem with xml attribute %s"), att.m_attributeName.AsString().AsChar());
			return false;
		}
	}
	const TDynArray< SCustomNode >::const_iterator end	= paramsNode.m_subNodes.End();
	TDynArray< SCustomNode >::const_iterator  it;
	for(  it = paramsNode.m_subNodes.Begin();  it !=  end; ++ it )
	{
		const SCustomNode & node = *it;	
		if ( ParseXmlNode( node, boidSpecies ) == false )
		{
			return false;
		}
	}
	return true;
}

Bool CBoidLairParams::ParseXmlNode( const SCustomNode & node, CBoidSpecies *const boidSpecies )
{
	if (node.m_nodeName == CNAME( spawnPointType ) )
	{
		if ( node.m_attributes.Size() != 0  )
		{
			const SCustomNodeAttribute & att = node.m_attributes[ 0 ];
			if ( att.m_attributeName == CNAME( spawnPointType_name ) )
			{
				m_spawnPointArray.PushBackUnique( att.m_attributeValueAsCName );
			}
		}
	}
	else if (node.m_nodeName == CNAME( boidStates ))
	{
		if (ParseBoidStateXML( node, boidSpecies ) == false)
		{
			WARN_GAME( TXT("problem with file boid_species.xml, boid nodes are specified incorrectly") );
			return false;
		}
	}
	else if (node.m_nodeName == CNAME( POIConfig ))
	{
		if ( OnParsePoiConfig( node, boidSpecies ) == false )
		{
			return false;
		}
	}
	else if ( node.m_nodeName == CNAME( soundConfig ) )
	{
		const TDynArray< SCustomNode >::const_iterator soundConfigEnd	= node.m_subNodes.End();
		TDynArray< SCustomNode >::const_iterator  soundConfigIt;
		for(  soundConfigIt = node.m_subNodes.Begin();  soundConfigIt !=  soundConfigEnd; ++ soundConfigIt )
		{
			const SCustomNode & soundConfigNode = *soundConfigIt;
			if ( soundConfigNode.m_nodeName == CNAME( sound ) )
			{
				CSwarmSoundConfig *const soundConfig = CreateSoundConfig();
				if ( soundConfig->ParseXml( soundConfigNode, this, boidSpecies ) == false )
				{
					delete soundConfig;
					return false;
				}
				boidSpecies->AddSoundConfig( soundConfig );

				// First make sure this sound config was not defined in the parent class
				Bool declaredInParent = false;
				for ( Uint32 i = 0; i < m_soundConfigArray.Size(); ++i )
				{
					if ( *m_soundConfigArray[ i ] == *soundConfig ) // if yes override it :
					{
						m_soundConfigArray[ i ] = soundConfig;
						declaredInParent = true;
					}
				}
				if ( declaredInParent == false )
				{
					m_soundConfigArray.PushBack( soundConfig );
				}
			}
			else
			{
				GFeedback->ShowError(TXT("Boid XML Error: problem with xml node %s"), soundConfigNode.m_nodeName.AsString().AsChar() );
				return false;
			}
		}
	}
	else if ( node.m_nodeName == CNAME( subSpecies ) )
	{
		// Do nothing this will dealt with later
	}
	else
	{
		GFeedback->ShowError(TXT("Boid XML Error: unknow node  %s"), node.m_nodeName.AsString().AsChar() );
		return false;
	}
	return true;
}

Bool CBoidLairParams::OnParsePoiConfig( const SCustomNode & node, CBoidSpecies *const boidSpecies )
{
	CPointOfInterestSpeciesConfig *const pointOfInterestConfig = new CPointOfInterestSpeciesConfig();

	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= node.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	CName poiType = CName::NONE;
	for ( attIt = node.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;

		if ( att.m_attributeName == CNAME( type_name ) )
		{
			poiType = att.m_attributeValueAsCName;
		}
	}

	if ( poiType == CName::NONE )
	{
		delete pointOfInterestConfig;
		GFeedback->ShowError( TXT("Boid XML Error: poiConfig does have a type defined") );
		return false;
	}

	CPointOfInterestSpeciesConfig_Map::iterator poiConfigIt = m_poiSpeciesConfigMap.Find( poiType );
	if ( poiConfigIt != m_poiSpeciesConfigMap.End() )
	{
		*pointOfInterestConfig = *poiConfigIt->m_second;
	}

	if ( pointOfInterestConfig->ParseXML( node, this ) == false )
	{
		WARN_GAME( TXT("problem with file boid_species.xml, boid POI are specified incorrectly") );
		delete pointOfInterestConfig;
		return false;
	}

	// adding for deletion
	boidSpecies->AddPointOfInterestConfig(pointOfInterestConfig);

	//CPointOfInterestSpeciesConfig_Map::const_iterator itFind = m_poiSpeciesConfigMap.Find( pointOfInterestConfig->m_type );
	if ( poiConfigIt != m_poiSpeciesConfigMap.End() )
	{
		// Was defined by parent :
		poiConfigIt->m_second = pointOfInterestConfig;
	}
	else
	{
		m_poiSpeciesConfigMap.Insert( pointOfInterestConfig->m_type,  pointOfInterestConfig);
	}
	
	return true;
}

CSwarmSoundConfig *const CBoidLairParams::CreateSoundConfig()
{
	return new CSwarmSoundConfig();
}


Bool CBoidLairParams::ParseXmlAttribute(const SCustomNodeAttribute & att)
{
	if ( att.m_attributeName == CNAME( species_name ) )
	{
		m_name		= att.m_attributeValueAsCName;	
	}
	else if ( att.m_attributeName == CNAME( type_name ) )
	{
		// Already taken care of earlier
	}
	else if ( att.m_attributeName == CNAME( boidScaleMin ) )
	{
		if ( att.GetValueAsFloat(m_boidScaleMin) == false )
		{
			GFeedback->ShowError(TXT("Boid XML Error: boidScaleMin is not defined as a float"));
			return false;
		}
	}
	else if ( att.m_attributeName == CNAME( boidScaleMax ) )
	{
		if ( att.GetValueAsFloat(m_boidScaleMax) == false )
		{
			GFeedback->ShowError(TXT("Boid XML Error: boidScaleMax is not defined as a float"));
			return false;
		}
	}
	else if ( att.m_attributeName == CNAME( soundBank_name ) )
	{
		m_soundBank		= att.m_attributeValueAsCName;	
	}
	else if ( att.m_attributeName == CNAME( boidTemplate ) )
	{
		m_boidTemplateHandle =  TSoftHandle<CEntityTemplate>( att.GetValueAsString() );
	}
	else if ( att.m_attributeName == CNAME( allowAnimInterruption ) )
	{
		if ( att.GetValueAsBool( m_allowAnimationInteruption ) == false )
		{
			GFeedback->ShowError(TXT("Boid XML Error: allowAnimInterruption is not defined as a bool"));
			return false;
		}
	}
	else
	{
		// This parameter doesn't exists
		return false;
	}
	return true;
}

Uint32 CBoidLairParams::GetBoidStateIndexFromName( CName name )const
{
	for ( Uint32 i=0; i<m_boidStateArray.Size(); ++i )
	{
		if ( m_boidStateArray[i].m_name == name )
		{
			return i;
		}
	}
	return (Uint32)-1;
}

Bool CBoidLairParams::ParseBoidStateXML(const SCustomNode & boidStateNode, CBoidSpecies *const boidSpecies )
{
	const TDynArray< SCustomNode >::const_iterator stateEnd	= boidStateNode.m_subNodes.End();
	TDynArray< SCustomNode >::const_iterator  stateIt;
	for(  stateIt = boidStateNode.m_subNodes.Begin();  stateIt != stateEnd; ++stateIt )
	{
		const SCustomNode & stateNode = *stateIt;
		if ( stateNode.m_nodeName != CNAME( state ) )
		{
			continue;
		}

		CName boidStateName( CName::NONE );
		const TDynArray< SCustomNodeAttribute >::const_iterator stateAttEnd	= stateNode.m_attributes.End();
		TDynArray< SCustomNodeAttribute >::const_iterator		stateAttIt;
		Bool doubleSpeed = false;
		Bool startRandom = false;
		for ( stateAttIt = stateNode.m_attributes.Begin();  stateAttIt !=  stateAttEnd; ++ stateAttIt )
		{
			const SCustomNodeAttribute & stateAtt	= *stateAttIt;
			if ( stateAtt.m_attributeName == CNAME( state_name ) )
			{
				boidStateName = stateAtt.m_attributeValueAsCName;
			}
			else if ( stateAtt.m_attributeName == CNAME( doubleSpeed ) )
			{
				if ( stateAtt.GetValueAsBool( doubleSpeed ) == false )
				{
					GFeedback->ShowError( TXT("Boid XML Error: doubleSpeed not spefcified as bool"));
				}
			}
			else if ( stateAtt.m_attributeName == CNAME( startRandom ) )
			{
				if ( stateAtt.GetValueAsBool( startRandom ) == false )
				{
					GFeedback->ShowError( TXT("Boid XML Error: startRandom not spefcified as bool"));
				}
			}
		}

		if ( boidStateName == CName::NONE )
		{
			GFeedback->ShowError( TXT("Boid XML Error: boidState has no name specified"));
			return false;
		}
		const Uint32 parentBoidStateID	= GetBoidStateIndexFromName( boidStateName );
		Uint32 boidStateID				= parentBoidStateID;
		if ( parentBoidStateID == (Uint32)-1 )
		{
			// Need to generate a new state ID :
			boidStateID = m_boidStateArray.Size();
		}

		CBaseBoidNode * baseBoidNode = NULL;
		if ( stateNode.m_subNodes.Size() != 0 )
		{
			if ( stateNode.m_subNodes.Size() > 1 )
			{
				GFeedback->ShowError( TXT("Boid XML Error: boid state has more than one child the others will be ignored") );
			}
			baseBoidNode = BaseAnimParseXML(stateNode.m_subNodes[0], NULL, boidSpecies );
			if ( baseBoidNode == NULL )
			{
				delete baseBoidNode;
				return false;
			}
		}

		while ( m_boidStateArray.Size() <= (Uint32)boidStateID )
		{
			m_boidStateArray.PushBack( CBoidState() );
		}
		m_boidStateArray[boidStateID] = CBoidState( baseBoidNode, boidStateName, doubleSpeed, startRandom );
		
	}
	return true;
}


//////////////////////////////////////////////////////////
// CSwarmSoundEmitterComponent
IMPLEMENT_ENGINE_CLASS( CSwarmSoundEmitterComponent );

CSwarmSoundEmitterComponent::CSwarmSoundEmitterComponent()
{
	SetStreamed( false );
}

CSwarmSoundEmitterComponent::~CSwarmSoundEmitterComponent()
{
}

void CSwarmSoundEmitterComponent::OnLairActivated()
{
	IBoidLairEntity *const swarmLairEntity		= static_cast< IBoidLairEntity* >( GetEntity() );
	const CBoidLairParams *const boidLairParams = swarmLairEntity->GetBoidLairParams();
	if ( boidLairParams )
	{
		m_banksDependency.PushBackUnique( boidLairParams->m_soundBank );
		m_soundBank = boidLairParams->m_soundBank;
	}
}
void CSwarmSoundEmitterComponent::OnLairDeactivated()
{
	SoundStopAll();
	UnAcquire();
	m_banksDependency.Clear();
}


