#include "build.h"
#include "r4CreateEntityManager.h"

#include "../../common/core/resourceDefManager.h"
#include "../../common/core/depot.h"

#include "../../common/engine/idTagManager.h"
#include "../../common/engine/pathlibWorld.h"

#include "../../common/game/actorsManager.h"
#include "../../common/game/aiPositioning.h"
#include "../../common/game/commonGame.h"
#include "../../common/game/createEntityHelper.h"

#include "r4Player.h"
#include "w3GenericVehicle.h"

IMPLEMENT_ENGINE_CLASS( CR4CreateEntityHelper )


CR4CreateEntityHelper::SpawnInfo::SpawnInfo()
	: m_spawnPos( Vector::ZEROS )
	, m_idTag()
	, m_eventHandler( nullptr )
	, m_template( nullptr )
	, m_detachTemplate( true )
	, m_important( false )
    , m_forceNonStreamed( false )
{

}


CR4CreateEntityHelper::SpawnInfo::~SpawnInfo()
{
	if ( m_eventHandler )
	{
		delete m_eventHandler;
	}
}

///////////////////////////////////////////////////////////////////////////////
// CR4CreateEntityHelper
///////////////////////////////////////////////////////////////////////////////
CR4CreateEntityHelper::CR4CreateEntityHelper()
	: m_isQueryPending( false )
	, m_isLoadingResource( false )
	, m_isJobRunning( false )
	, m_flags( FLAGS_DEFAULT )

{

}
CR4CreateEntityHelper::~CR4CreateEntityHelper()
{
}

Bool CR4CreateEntityHelper::AsyncLoadTemplate( const String& fileName )
{
	m_entityTemplate = Move( AsyncEntityHandle( fileName ) );
	switch ( m_entityTemplate.GetAsync() )
	{
	case BaseSoftHandle::ALR_InProgress:
		// entity loading in progress
		m_isLoadingResource = true;
		return true;
	case BaseSoftHandle::ALR_Loaded:
		// entity fully loaded
		m_isLoadingResource = false;
		m_spawnInfo.m_template = m_entityTemplate.Get();
		break;
	case BaseSoftHandle::ALR_Failed:
		// failed entity loading
		return false;
	}
	return true;
}

void CR4CreateEntityHelper::RunAsyncPositioningQuery( CWorld* world, const CPositioningFilterRequest::Ptr& ptr )
{
	m_isQueryPending = true;
	m_positioningQuery = ptr;

	m_positioningQuery->Submit( *world->GetPathLibWorld() );
}

Bool CR4CreateEntityHelper::Update( CCreateEntityManager* manager )
{
	if ( m_isQueryPending )
	{
		switch( m_positioningQuery->GetQueryState() )
		{
		default:
		case CPositioningFilterRequest::STATE_ONGOING:
			return false;
		case CPositioningFilterRequest::STATE_COMPLETED_SUCCESS:
			{
				m_spawnInfo.m_spawnPos.AsVector3() = m_positioningQuery->GetComputedPosition();
				m_isQueryPending = false;
			}
			return false;
		case CPositioningFilterRequest::STATE_COMPLETED_FAILURE:
			if ( m_flags & FLAG_SPAWN_EVEN_IF_QUERY_FAILS )
			{
				m_isQueryPending = false;
				return false;
			}
			return true;
		case CPositioningFilterRequest::STATE_DISPOSED:
		case CPositioningFilterRequest::STATE_SETUP:
			ASSERT( false );
			ASSUME( false );
		}
	}
	if ( m_isLoadingResource )
	{
		switch ( m_entityTemplate.GetAsync() )
		{
		case BaseSoftHandle::ALR_InProgress:
			// entity loading in progress
			return false;
		case BaseSoftHandle::ALR_Loaded:
			// entity fully loaded
			m_isLoadingResource = false;
			m_spawnInfo.m_template = m_entityTemplate.Get();
			break;
		case BaseSoftHandle::ALR_Failed:
			// failed entity loading
			return true;
		}
	}
	if ( !m_isJobRunning )
	{
		ASSERT( m_spawnInfo.m_template, TXT("No template loaded for create entity manager!") );


		EntitySpawnInfo entitySpawnInfo;
		entitySpawnInfo.m_spawnPosition		= m_spawnInfo.m_spawnPos;
		entitySpawnInfo.m_spawnRotation		= m_spawnInfo.m_spawnRotation;
		entitySpawnInfo.m_detachTemplate	= m_spawnInfo.m_detachTemplate;
		entitySpawnInfo.m_template			= m_spawnInfo.m_template;
		entitySpawnInfo.m_importantEntity	= m_spawnInfo.m_important;
		entitySpawnInfo.m_entityFlags		= m_spawnInfo.m_entityFlags;
        entitySpawnInfo.m_forceNonStreamed  = m_spawnInfo.m_forceNonStreamed;

        if( !m_spawnInfo.m_tagList.Empty() )
            entitySpawnInfo.m_tags.AddTags( m_spawnInfo.m_tagList );

		if ( m_spawnInfo.m_idTag.IsValid() )
		{
			entitySpawnInfo.m_idTag			= m_spawnInfo.m_idTag;
			entitySpawnInfo.m_entityFlags	= EF_ManagedEntity;
		}

		if ( m_spawnInfo.m_eventHandler )
		{
			entitySpawnInfo.AddHandler( m_spawnInfo.m_eventHandler );
			m_spawnInfo.m_eventHandler = nullptr;
		}

		StartSpawnJob( Move( entitySpawnInfo ) );
		m_isJobRunning = true;
		return false;
	}
	return CCreateEntityHelper::UpdateSpawnJob();
}

void CR4CreateEntityHelper::Discard( CCreateEntityManager* manager )
{
	if( m_positioningQuery )
	{
		static_cast< CR4CreateEntityManager* >( manager )->ReleasePositioningQuery( m_positioningQuery );
		m_positioningQuery.Clear();
	}
	CCreateEntityHelper::Discard( manager );
}


///////////////////////////////////////////////////////////////////////////////
// CR4CreateEntityManager
///////////////////////////////////////////////////////////////////////////////
CR4CreateEntityManager::CR4CreateEntityManager()
{
}

CR4CreateEntityManager::~CR4CreateEntityManager()
{

}

CPositioningFilterRequest::Ptr CR4CreateEntityManager::RequestPositioningQuery()
{
	if ( m_requestsPool.Empty() )
	{
		return new CPositioningFilterRequest();
	}
	else
	{
		return m_requestsPool.PopBackFast();
	}
}
void CR4CreateEntityManager::DefaultPositioningFilter( SPositioningFilter& outFilter )
{
	outFilter.m_awayFromCamera = true;
	outFilter.m_onlyReachable = true;
	outFilter.m_noRoughTerrain = true;
	outFilter.m_noInteriors = false;
	outFilter.m_limitToBaseArea = false;
}
CPositioningFilterRequest::Ptr CR4CreateEntityManager::SetupPositioningQuery( const Vector& pos, const SPositioningFilter& posFilter )
{
	CWorld* world = GGame->GetActiveWorld();
	Vector3 testPos;
	Float minZ = pos.Z - 2.f;
	Float maxZ = pos.Z + 2.f;
	Float personnalSpace = posFilter.m_personalSpace;
	Float radius = 10.0f;
	if ( !world->GetPathLibWorld()->FindSafeSpot( PathLib::INVALID_AREA_ID, pos.AsVector3(), radius, personnalSpace, testPos, &minZ, &maxZ ) )
	{
		return nullptr;
	}
	CPositioningFilterRequest::Ptr query = RequestPositioningQuery();
	if ( !query->Setup( posFilter, world, testPos, personnalSpace ) )
	{
		ReleasePositioningQuery( query );
		return nullptr;
	}
	// lazy creation of global callback object
	if ( !m_poolMeBackCallback)
	{
		m_poolMeBackCallback = new CPoolMeBackCallback();
	}
	// callback that pools back request into manager request pool
	query->AddCallback( m_poolMeBackCallback );
	return query;
}

void CR4CreateEntityManager::ReleasePositioningQuery( CPositioningFilterRequest::Ptr query )
{
	m_requestsPool.PushBack( query );
}

Bool CR4CreateEntityManager::SpawnAliasEntityToPosition( CR4CreateEntityHelper *const createEntityHelper, const Matrix &summonMatrix, const String &alias, const IdTag &idTag, TDynArray<CName> tags, ISpawnEventHandler *const spawnEventHandler, Bool important, Bool forceNoStreaming )
{
	const String aliasFilename( CResourceDefManager::RESDEF_PROTOCOL + alias );

	if ( !createEntityHelper->AsyncLoadTemplate( aliasFilename ) )
	{
		createEntityHelper->Discard( this );
		return false;
	}

	CR4CreateEntityHelper::SpawnInfo& spawnInfo = createEntityHelper->GetSpawnInfo();
	spawnInfo.m_spawnPos		= summonMatrix.GetTranslation();
	spawnInfo.m_spawnRotation	= summonMatrix.ToEulerAngles();
	spawnInfo.m_detachTemplate	= false;
	spawnInfo.m_tagList			= tags;
	spawnInfo.m_idTag			= idTag;
	spawnInfo.m_eventHandler	= spawnEventHandler;
	spawnInfo.m_important		= important;
    spawnInfo.m_forceNonStreamed= forceNoStreaming;

	AddProcessingItem( createEntityHelper );
	return true;
}

Bool CR4CreateEntityManager::SpawnAliasEntityAroundOwner( CR4CreateEntityHelper *const createEntityHelper, const String &alias, CActor *const horseOwner, const IdTag &horseIdTag, TDynArray<CName> tags, ISpawnEventHandler *const spawnEventHandler, Bool important, Bool noInteriors )
{
	if ( horseOwner == nullptr )
	{
		createEntityHelper->Discard( this );
		return false;
	}
	SPositioningFilter posFilter;
	DefaultPositioningFilter( posFilter );
	posFilter.m_noInteriors = noInteriors;
	posFilter.m_personalSpace = 2.0f;
	const Vector& ownerPosition = horseOwner->GetWorldPositionRef();
	CPositioningFilterRequest::Ptr query = SetupPositioningQuery( ownerPosition, posFilter );
	if ( !query )
	{
		return false;
	}
	CWorld* world = GGame->GetActiveWorld();
	createEntityHelper->RunAsyncPositioningQuery( world, query );

	EulerAngles rotation( 0.0f, 0.0f, GEngine->GetRandomNumberGenerator().Get< Float >( 0.0f , 360.0f ) );
	Matrix spawnMatrix = rotation.ToMatrix();
	spawnMatrix.SetTranslation( ownerPosition );
	return SpawnAliasEntityToPosition( createEntityHelper, spawnMatrix, alias, horseIdTag, tags, spawnEventHandler, important );
}
void CR4CreateEntityManager::SpawnUniqueAliasVehicle( CR4CreateEntityHelper *const createEntityHelper, Bool teleportIfSpawned, const String &alias, const IdTag &idTag, TDynArray<CName> tags, const Matrix *inSpawnMatrix, Bool noInteriors )
{
	// Right now a unique alias entity is always flagged as important !

	CEntity* entity			= CPeristentEntity::FindByIdTag( idTag );
	CR4Player *const player = static_cast< CR4Player * >( GGame->GetPlayerEntity() );
	if( entity )
	{
		// If actor is not alive then destroy actor
		if ( entity->IsA<CActor>() )
		{
			CActor *const actor = static_cast<CActor *>( entity );
			if ( actor->IsAlive() == false )
			{
				actor->Destroy();
				// This doesn't work yet Marek needs to do some work
				//GCommonGame->GetEntityPool()->PoolEntity( entity );
				entity = nullptr;
			}
		}
	}

	if( entity )
	{
		if ( teleportIfSpawned )
		{
			if ( inSpawnMatrix )
			{
				entity->Teleport( inSpawnMatrix->GetTranslationRef(), inSpawnMatrix->ToEulerAngles() );

				// Need this in order to call call back after creation of the entity
				CCreateEntityHelper::CScriptSpawnEventHandler *const scriptSpawnEventHandler = createEntityHelper->GetScriptSpawnEventHandler();
				if ( scriptSpawnEventHandler )
				{
					scriptSpawnEventHandler->CallOnPostSpawnCallback( entity );
				}
			}
			else
			{
				CWorld* world = GGame->GetActiveWorld();

				// setup filter
				SPositioningFilter posFilter;
				DefaultPositioningFilter( posFilter );
				posFilter.m_noInteriors = noInteriors;

				// run fire-and-forget positioning query
				CPositioningFilterRequest::Ptr query = SetupPositioningQuery( player->GetWorldPositionRef(), posFilter );
				if ( query )
				{
					// callback that will teleport entity to given position
					query->AddCallback( new CPositionFilterTeleportEntityCallback( entity ) );
					// callback that will run post-spawn code: I personally find running this code risky, but I don't want any regresion
					CCreateEntityHelper::CScriptSpawnEventHandler *const scriptSpawnEventHandler = createEntityHelper->StealScriptSpawnEventHandler();
					if ( scriptSpawnEventHandler )
					{
						query->AddCallback( new CRunOnPostSpawnCallback( entity, scriptSpawnEventHandler ) );
					}

					query->Submit( *world->GetPathLibWorld() );
				}
			}
			CPlayerVehicleSpawnEventHandler::LinkVehicleToPlayer( entity );
		}
	}
	else
	{
		if ( inSpawnMatrix )
		{
			SpawnAliasEntityToPosition( createEntityHelper, *inSpawnMatrix, alias, idTag, tags, new CPlayerVehicleSpawnEventHandler(), true );
		}
		else
		{
			SpawnAliasEntityAroundOwner( createEntityHelper, alias, player, idTag, tags, new CPlayerVehicleSpawnEventHandler(), true, true );
		}
	}
}

void CR4CreateEntityManager::SummonPlayerHorse( CR4CreateEntityHelper *const createEntityHelper, Bool teleportIfSpawned, const Matrix *inSpawnMatrix )
{
	static IdTag horseIdTag = GGame->GetIdTagManager()->GetReservedId( RESERVED_TAG_ID_INDEX_GERALT_HORSE );

    TDynArray<CName> names;
    names.PushBack(PLAYER_HORSE_TAG);
	SpawnUniqueAliasVehicle( createEntityHelper, teleportIfSpawned, TXT("playerHorse"), horseIdTag, names, inSpawnMatrix, true );
}

Bool CR4CreateEntityManager::CreateEntityAsync( CCreateEntityHelper *const createEntityHelper, EntitySpawnInfo && entitySpawnInfo )
{
	if ( createEntityHelper->IsA<CR4CreateEntityHelper>() )
	{
		CR4CreateEntityHelper::SpawnInfo & spawnInfo = (static_cast< CR4CreateEntityHelper * >( createEntityHelper ))->GetSpawnInfo();
		spawnInfo.m_template		= entitySpawnInfo.m_template;
		spawnInfo.m_spawnPos		= entitySpawnInfo.m_spawnPosition;
		spawnInfo.m_spawnRotation	= entitySpawnInfo.m_spawnRotation;
		spawnInfo.m_detachTemplate	= entitySpawnInfo.m_detachTemplate;
		spawnInfo.m_important		= entitySpawnInfo.m_importantEntity;
		spawnInfo.m_entityFlags		|= EF_ManagedEntity;
	}
	return CCreateEntityManager::CreateEntityAsync( createEntityHelper, Move( entitySpawnInfo ) );
}

////////////////////////////////////////////////////////////////////////////////////////
// CR4CreateEntityManager::CPlayerVehicleSpawnEventHandler
////////////////////////////////////////////////////////////////////////////////////////
RED_DEFINE_STATIC_NAME( OnPlayerHorseSpawned );

void CR4CreateEntityManager::CPlayerVehicleSpawnEventHandler::OnPostAttach( CEntity* entity )
{
	LinkVehicleToPlayer( entity );
	CR4Player *const player = static_cast< CR4Player * >( GGame->GetPlayerEntity() );
	player->CallEvent( CNAME( OnPlayerHorseSpawned ), THandle<CActor>( static_cast<CActor*>( entity ) ) );
}
void CR4CreateEntityManager::CPlayerVehicleSpawnEventHandler::LinkVehicleToPlayer( CEntity* entity )
{
	CR4Player *const player = static_cast< CR4Player * >( GGame->GetPlayerEntity() );

	CActor* vehicleActor = static_cast<CActor*>( entity );
	if ( vehicleActor == nullptr )
	{
		return;
	}

	// if vehicle is a horse 
	ComponentIterator< W3HorseComponent > it( vehicleActor );
	if ( it )
	{ 
		W3HorseComponent* horseComponent	= *it;

		player->SetHorseWithInventory( vehicleActor );

		CAIStorageRiderData *const riderData = player->GetScriptAiStorageData< CAIStorageRiderData >( CNAME( RiderData ) );
		if ( riderData == nullptr )
		{
			return;
		}
		CHorseRiderSharedParams *const sharedParams = riderData->m_sharedParams.Get();
		if ( sharedParams == nullptr )
		{
			return;
		}

	
		if ( horseComponent->PairWithRider( vehicleActor, sharedParams ) == false )
		{
			ASSERT( false, TXT( "PairWithRider failed after summon something is wrong here" ) );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// CR4CreateEntityManager::CPoolMeBackCallback
////////////////////////////////////////////////////////////////////////////////////////
void CR4CreateEntityManager::CPoolMeBackCallback::Callback( PathLib::CWalkableSpotQueryRequest* request )
{
	CR4CreateEntityManager *const createEntityManager	= GR4Game->GetR4CreateEntityManager();
	if ( createEntityManager )
	{
		CPositioningFilterRequest* positioningRequest = static_cast< CPositioningFilterRequest* >( request );
		createEntityManager->ReleasePositioningQuery( positioningRequest );
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// CR4CreateEntityManager::CRunOnPostSpawnCallback
////////////////////////////////////////////////////////////////////////////////////////
void CR4CreateEntityManager::CRunOnPostSpawnCallback::Callback( PathLib::CWalkableSpotQueryRequest* request )
{
	CEntity* entity = m_entity.Get();
	if ( entity )
	{
		m_spawnEventHandler->CallOnPostSpawnCallback( entity );
	}
}
CR4CreateEntityManager::CRunOnPostSpawnCallback::CRunOnPostSpawnCallback( CEntity* entity, CCreateEntityHelper::CScriptSpawnEventHandler* eventHandler )
	: m_entity( entity )
	, m_spawnEventHandler( eventHandler )
{}
CR4CreateEntityManager::CRunOnPostSpawnCallback::~CRunOnPostSpawnCallback()
{
	delete m_spawnEventHandler;
}
