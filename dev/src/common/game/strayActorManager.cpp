#include "build.h"

#include "strayActorManager.h"
#include "behTreeMachine.h"
#include "behTreeGuardAreaData.h"
#include "behTreeInstance.h"
#include "entityPool.h"
#include "../../common/engine/gameTimeManager.h"
#include "../../common/engine/idTagManager.h"


//////////////////////////////////////////////////////////////////////////
RED_DEFINE_STATIC_NAME( StrayActors );
RED_DEFINE_STATIC_NAME( strayActor );
RED_DEFINE_STATIC_NAME( entityHandle );
RED_DEFINE_STATIC_NAME( timePast );
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStrayActorManager );


CStrayActorManager::CStrayActorData::CStrayActorData()
	: m_registrationTime( GGame->GetTimeManager()->GetTime() )
	, m_secondsPast( 0 )
{
	m_entityHandle.Set( nullptr );
}

CStrayActorManager::CStrayActorData::CStrayActorData( CActor* const actor )
	: m_registrationTime( GGame->GetTimeManager()->GetTime() )
	, m_secondsPast( 0 )
{
	m_entityHandle.Set( actor );
}

CStrayActorManager::CStrayActorData::CStrayActorData( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME( strayActor ) );

	loader->ReadValue( CNAME( entityHandle ), m_entityHandle );
	loader->ReadValue( CNAME( timePast ), m_secondsPast );
	
	m_registrationTime = GGame->GetTimeManager()->GetTime();
}

Bool CStrayActorManager::CStrayActorData::operator==( const CStrayActorData& strayActorData ) const
{
	return strayActorData.m_entityHandle == m_entityHandle;
}

Bool CStrayActorManager::CStrayActorData::operator!=( const CStrayActorData& strayActorData ) const
{
	return !operator==( strayActorData );
}

Bool CStrayActorManager::CStrayActorData::operator==( const CEntity* entity ) const
{
	return m_entityHandle.Get() == entity;
}

void CStrayActorManager::CStrayActorData::Save( IGameSaver* saver ) const
{
	CGameSaverBlock block( saver, CNAME( strayActor ) );

	saver->WriteValue( CNAME( entityHandle ), m_entityHandle );
	
	const Int32 timePast = ( GGame->GetTimeManager()->GetTime() - m_registrationTime ).m_seconds;
	saver->WriteValue( CNAME( timePast ), timePast );
}

CStrayActorManager::CStrayActorManager()
{
}

bool CStrayActorManager::OnSaveGame( IGameSaver* saver )
{
	{
		CGameSaverBlock block( saver, CNAME( StrayActors ) );

		const Uint32 size = m_strayActorDataList.Size();
		saver->WriteValue( CNAME( count ), size );

		for ( const auto& it : m_strayActorDataList )
		{
			it.Save( saver );
		}
	}

	return true;
}

void CStrayActorManager::Load( IGameLoader* loader )
{
	m_strayActorDataList.Clear();

	{
		CGameSaverBlock block( loader, CNAME( StrayActors ) );

		Uint32 size = 0;
		loader->ReadValue( CNAME( count ), size );

		m_strayActorDataList.Reserve( size );

		for ( Uint32 i = 0; i < size; ++i )
		{
			m_strayActorDataList.PushBack( CStrayActorData( loader ) );
		}
	}
}

Bool CStrayActorManager::ConvertToStrayActor( CActor* const actor )
{
	TDynArray< IActorTerminationListener* >& terminationListenerArray = actor->GetTerminationListeners();
	for ( Uint32 i = 0, n = terminationListenerArray.Size(); i < n; ++i )
	{
		IActorTerminationListener* const terminationListener = terminationListenerArray[i];
		if ( !terminationListener->CanBeConvertedToStrayActor() )
		{
			return false;
		}
	}
	for ( Uint32 i = 0, n = terminationListenerArray.Size(); i < n; ++i )
	{
		IActorTerminationListener* const terminationListener = terminationListenerArray[i];
		terminationListener->OnConvertToStrayActor( actor );
	}

	if ( !actor->IsManaged() )
	{
		actor->ConvertToManagedEntity( GGame->GetIdTagManager()->Allocate() );
	}

	// [Step] Adding the actor to the list if it is not there already
	if  ( m_strayActorDataList.PushBackUnique( CStrayActorData( actor ) ) )
	{
		actor->RegisterTerminationListener( true, this );
	}

	// [Step] Checking out if the actor implements guard area behavior
	if ( CBehTreeMachine* const behTreeMachine = actor->GetBehTreeMachine() )
	{
		if ( CBehTreeInstance* const behTreeInstance = behTreeMachine->GetBehTreeInstance() )
		{
			if ( CBehTreeGuardAreaData* data = CBehTreeGuardAreaData::Find( behTreeInstance ) )
			{
				data->SetupBaseState( nullptr, nullptr );
			}
		}
	}

	return true;
}
void CStrayActorManager::Tick( Float timeDelta )
{
	const Float& despawnDistance	= GGame->GetGameplayConfig().m_strayActorDespawnDistance;
	const Int32& despawnMaxTime		= GGame->GetGameplayConfig().m_strayActorMaxHoursToKeep;
	const GameTime currentGameTime	= GGame->GetTimeManager()->GetTime();
	const Float despawnDistanceSq	= despawnDistance * despawnDistance;

	TAllocArrayCreate( CEntity*, notInCameraViewActors, m_strayActorDataList.Size() );

	auto funAddToPoolOrDestroy = []( CEntity* entity ) 
	{
		if ( entity->CheckEntityFlag( EF_Poolable ) )
		{
			GCommonGame->GetEntityPool()->AddEntity( entity );
		}
		else
		{
			entity->Destroy();
		}
	};

	Uint32 inCameraViewActorsCount	= 0;

	for ( Int32 i = m_strayActorDataList.Size() - 1; i >= 0; --i )
	{
		CStrayActorData& actorData = m_strayActorDataList[i];
		if ( CEntity* const entity = actorData.m_entityHandle.Get() )
		{
			const Bool isInCameraView = GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetCameraDirector()->IsPointInView( entity->GetWorldPositionRef(), 1.5f ) : false;

			if ( isInCameraView )
			{
				++inCameraViewActorsCount;
				continue;
			}

			if ( IsTooFarAway( entity, despawnDistanceSq ) && IsTooOld( actorData, currentGameTime, despawnMaxTime ) )
			{
				funAddToPoolOrDestroy( entity );
			}
			else
			{
				notInCameraViewActors.PushBack( entity );
			}
		}
	}

	const Uint32 maxActorsToKeep = GGame->GetGameplayConfig().m_strayActorMaxActorsToKeep;

	if ( inCameraViewActorsCount + notInCameraViewActors.Size() > maxActorsToKeep )
	{
		Int32 actorsToRemove = Min( notInCameraViewActors.Size(), inCameraViewActorsCount + notInCameraViewActors.Size() - maxActorsToKeep );
		if ( actorsToRemove > 0 )
		{
			Sort( &notInCameraViewActors[0], &notInCameraViewActors[ notInCameraViewActors.Size() - 1 ], 
				[]( const CEntity* l, const CEntity* r )
			{
				const Vector& playerPosition = GCommonGame->GetPlayer()->GetWorldPositionRef();
				return playerPosition.DistanceSquaredTo( l->GetWorldPositionRef() ) < playerPosition.DistanceSquaredTo( r->GetWorldPositionRef() );
			} );

			for ( Int32 i = notInCameraViewActors.Size() - 1; actorsToRemove > 0; --i, --actorsToRemove )
			{
				funAddToPoolOrDestroy( notInCameraViewActors[i] );
			}
		}
	}
}

// On actor removed from world
void CStrayActorManager::OnDetach( CActor* actor )
{
	m_strayActorDataList.Remove( actor );
}

Bool CStrayActorManager::IsTooFarAway( const CEntity* const entity, Float maxDistanceSq ) const
{
	const Vector3& actorPosition		= entity->GetWorldPositionRef();
	const Vector3& playerPosition		= GCommonGame->GetPlayer()->GetWorldPosition();
	const Float playerActorSquareDist	= ( actorPosition.AsVector2() - playerPosition.AsVector2() ).SquareMag();

	return playerActorSquareDist > maxDistanceSq;
}

Bool CStrayActorManager::IsTooOld( const CStrayActorData& strayActorData, const GameTime& currentTime, Int32 maxTimeHoursToKeep ) const
{
	return ( ( currentTime - strayActorData.m_registrationTime ).m_seconds + strayActorData.m_secondsPast ) / GameTime::HOUR.m_seconds > maxTimeHoursToKeep;
}

void CStrayActorManager::OnGameStart( const CGameInfo& gameInfo )
{
	if ( gameInfo.m_gameLoadStream )
	{
		Load( gameInfo.m_gameLoadStream );
	}
}

void CStrayActorManager::Initialize()
{
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CStrayActorManager::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

void CStrayActorManager::OnGameEnd( const CGameInfo& gameInfo )
{
	m_strayActorDataList.ClearFast();
}

void CStrayActorManager::OnWorldEnd( const CGameInfo& gameInfo )
{
	m_strayActorDataList.ClearFast();
}