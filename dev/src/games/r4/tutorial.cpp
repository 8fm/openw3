/**
* Copyright c 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "tutorial.h"
#include "../../common/core/gameSave.h"

IMPLEMENT_ENGINE_CLASS( CR4TutorialSystem );


CR4TutorialSystem::CR4TutorialSystem()
	: m_needsTickEvent( false )
{
}

CR4TutorialSystem::~CR4TutorialSystem()
{
}

void CR4TutorialSystem::Initialize()
{
	SGameSessionManager::GetInstance().RegisterGameSaveSection( this );
}

void CR4TutorialSystem::Shutdown()
{
	SGameSessionManager::GetInstance().UnregisterGameSaveSection( this );
}

void CR4TutorialSystem::OnGameStart( const CGameInfo& gameInfo )
{
	const Bool restored = nullptr != gameInfo.m_gameLoadStream;

	if ( restored )
	{
		CClass* theClass = GetClass();
		IGameLoader* loader = gameInfo.m_gameLoadStream;
		{
			CGameSaverBlock block( loader, theClass->GetName() );

			const Uint32 numProperties = loader->ReadValue< Uint32 >( CNAME( nP ) );		
			for ( Uint32 i = 0; i < numProperties; ++i )
			{
				loader->ReadProperty( this, theClass, this );
			}
		}	
	}

	CallEvent( CNAME( OnGameStart ), restored );
}

void CR4TutorialSystem::OnGameEnd( const CGameInfo& gameInfo )
{
	CallEvent( CNAME( OnGameEnd ) );
}

void CR4TutorialSystem::OnWorldStart( const CGameInfo& gameInfo )
{
	CallEvent( CNAME( OnWorldStart ) );
}

void CR4TutorialSystem::OnWorldEnd( const CGameInfo& gameInfo )
{
	const Bool restored = nullptr != gameInfo.m_gameLoadStream;
	CallEvent( CNAME( OnWorldEnd ) );
}

void CR4TutorialSystem::Tick( Float timeDelta )
{
	if ( m_needsTickEvent )
	{
		CallEvent( CNAME( OnTick ), timeDelta );
	}
}

void CR4TutorialSystem::OnGenerateDebugFragments( CRenderFrame* frame )
{
}

Bool CR4TutorialSystem::OnSaveGame( IGameSaver* saver )
{
	// Get properties to save
	const Uint32 maxNumProperties( 1024 );	
	CProperty* propertiesToSave[ maxNumProperties ];

	Uint32 numPropertiesToSave  = saver->GetSavableProperties( GetClass(), propertiesToSave, maxNumProperties );
	if ( numPropertiesToSave >= maxNumProperties )
	{
		HALT( "IGameSaver::OnSaveGameplayState() reached maxNumProperties (=%ld) limit! Please increase this limit or reduce number of saveable properties in class %ls.", maxNumProperties, GetClass()->GetName().AsChar() ); 
		return false;
	}

	{
		CGameSaverBlock block( saver, GetClass()->GetName() );

		// Save count
		saver->WriteValue( CNAME( nP ), numPropertiesToSave );

		// Save properties
		for ( Uint32 i = 0; i < numPropertiesToSave; ++i )
		{
			saver->WriteProperty( this, propertiesToSave[ i ] );
		}
	}
	return true;
}
