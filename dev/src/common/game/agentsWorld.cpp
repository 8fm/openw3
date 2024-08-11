#include "build.h"
#include "agentsWorld.h"
#include "agent.h"
#include "spawnStrategy.h"
#include "communityAgentStub.h"
#include "communitySystem.h"
#include "../engine/renderFrame.h"

///////////////////////////////////////////////////////////////////////////////

CAgentsWorld::CAgentsWorld()
	: m_spawnedCount( 0 )
	, m_updateIndex( 0 )
    , m_restoringState( false )
	, m_forceLodUpdate( false )
{
}

///////////////////////////////////////////////////////////////////////////////

CAgentsWorld::~CAgentsWorld()
{
	for ( TDynArray< IAgent* >::iterator it = m_allAgents.Begin(); it != m_allAgents.End(); ++it )
	{
		delete *it;
	}
	m_allAgents.Clear();
}

///////////////////////////////////////////////////////////////////////////////

void CAgentsWorld::ForceFullLodUpdate()
{
	m_forceLodUpdate = true;
}


void CAgentsWorld::Update( Float deltaTime )
{
	// We don't process NPCs when there is no player or active world
	if ( GCommonGame->GetPlayer() == nullptr || GCommonGame->GetActiveWorld() == nullptr )
	{
		return;
	}

	// Update agents LODs
	if ( !m_allAgents.Empty() )
	{
#ifdef PROFILE_COMMUNITY
		PC_SCOPE_PIX( AgentsWorld_CalculateLOD );
#endif
		if( m_forceLodUpdate )
		{
			for ( IAgent* agent : m_allAgents )
			{
				agent->CalculateLOD( *this );				
			}
			m_forceLodUpdate = false;
		}
		else
		{
			const Float LOD_UPDATE_INTERVAL = 1.0f;
			Float updateRatio = deltaTime / LOD_UPDATE_INTERVAL;
			Uint32 toUpdate = Uint32( updateRatio * Float( m_allAgents.Size() ) );

			toUpdate = Max( toUpdate, 4U );
			toUpdate = Min( toUpdate, m_allAgents.Size() );

			m_updateIndex = ( m_updateIndex >= m_allAgents.Size() ) ? 0 : m_updateIndex;
			for ( Uint32 i = 0; i < toUpdate; i++ )
			{
				m_allAgents[ m_updateIndex ]->CalculateLOD( *this );
				if ( ++m_updateIndex >= m_allAgents.Size() )
				{
					m_updateIndex = 0;
				}
			}
		}		
	}

	// Despawn agents
	{
#ifdef PROFILE_COMMUNITY
		PC_SCOPE_PIX( AgentsWorld_DespawnAgents );
#endif

		if ( !m_restoringState )
		{
			for ( IAgent* agent : m_allAgents )
			{
				if ( agent->GetDesiredLOD() == ALOD_Invisible && agent->GetLOD() != agent->GetDesiredLOD() )
				{
					if ( !agent->ProcessLODChange( deltaTime ) )
					{
						LOG_GAME( TXT( "ProcessLODChange for agent failed." ) );
					}
				}
			}
		}
	}


	// Spawn agents
	{
#ifdef PROFILE_COMMUNITY
		PC_SCOPE_PIX( AgentsWorld_SpawnAgents );
#endif

		const Uint32 allowedToSpawn = m_restoringState ? 256 : MAX_NPC_SPAWNED;
		m_spawnedCount = 0;
		for ( IAgent* agent : m_allAgents )
		{
			if ( agent->GetDesiredLOD() > ALOD_Invisible && agent->GetLOD() != agent->GetDesiredLOD() )
			{
				if ( agent->ProcessLODChange( deltaTime ) )
				{
					if ( ++m_spawnedCount >= allowedToSpawn )
					{
						break;
					}
				}
				else
				{
					LOG_GAME( TXT( "ProcessLODChange for agent failed." ) );
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void CAgentsWorld::AddAgent( IAgent* agent )
{
	if ( agent )
	{
		m_allAgents.PushBackUnique( agent );
	}
}

///////////////////////////////////////////////////////////////////////////////

void CAgentsWorld::RemoveAgent( IAgent* agent )
{
	m_allAgents.Remove( agent );
}

///////////////////////////////////////////////////////////////////////////////

void CAgentsWorld::Reset()
{
	m_allAgents.ClearPtr();
	m_spawnedCount = 0;
}

void CAgentsWorld::GetAlwaysSpawnedAgents( TDynArray< StubDesc >& alwaysSpawned )
{
	// Collect 'always spawn' NPCs

	const Vector playerPosition = GCommonGame->GetPlayer()->GetWorldPosition();

	for ( IAgent* _agent : m_allAgents )
	{
		SAgentStub* agent = static_cast< SAgentStub* >( _agent );
		if ( agent->IsAlwaysSpawned() )
		{
			const Vector agentWorldPos = Vector( agent->m_communityAgent.GetPosition() );

			StubDesc stub;
			stub.m_agent = agent;
			stub.m_distance = playerPosition.DistanceTo2D( agentWorldPos );
			stub.m_communityName = agent->GetCommunity()->GetFile()->GetDepotPath();

			alwaysSpawned.PushBack( stub );
		}
	}

	// Sort 'always spawned' NPCs by distance (and other attributes)

	struct CompareAlwaysSpawned
	{
		Bool operator () ( const StubDesc& a, const StubDesc& b ) const
		{
			const Float distanceCmp = b.m_distance - a.m_distance;
			if ( distanceCmp != 0.0f )
			{
				return distanceCmp < 0.0f;
			}

			if ( const Int32 cmp = Red::System::StringCompare( a.m_communityName.AsChar(), b.m_communityName.AsChar() ) )
			{
				return cmp < 0;
			}

			if ( const Int32 cmp = Red::System::StringCompare( a.m_agent->GetActivePhaseName().AsChar(), b.m_agent->GetActivePhaseName().AsChar() ) )
			{
				return cmp < 0;
			}

			if ( const Int32 cmp = Red::System::StringCompare( a.m_agent->GetAppearanceName().AsChar(), b.m_agent->GetAppearanceName().AsChar() ) )
			{
				return cmp < 0;
			}

			return false;
		}
	};

	Sort( alwaysSpawned.Begin(), alwaysSpawned.End(), CompareAlwaysSpawned() );
}

void CAgentsWorld::OnGenerateDebugFragments( CRenderFrame* frame )
{
	if ( !( frame->GetFrameInfo().IsShowFlagOn( SHOW_CommunityAgents ) ) ) return;

	TDynArray< StubDesc > alwaysSpawned;
	GetAlwaysSpawnedAgents( alwaysSpawned );

	String message;
	GetDumpMessage( alwaysSpawned, message );

	frame->AddDebugScreenText( 200, 200, message.AsChar(), Color::RED );
}

Bool CAgentsWorld::Dump()
{
	TDynArray< StubDesc > alwaysSpawned;
	GetAlwaysSpawnedAgents( alwaysSpawned );

	// Dump to logs

	String message;
	GetDumpMessage( alwaysSpawned, message );

	LOG_GAME( message.AsChar() );

	// Dump to CSV

	const AnsiChar* csvFileName = "community_agents_always_active.csv";

	FILE* csv = fopen( csvFileName, "w" );
	if ( !csv )
	{
		LOG_GAME( TXT( "Failed to open CSV file to write always spawned community agents stats" ) );
		return false;
	}

	StringAnsi csvContent;
	csvContent += "index;distance;community;active_phase;appearance\n";
	Uint32 index = 0;
	for ( const StubDesc& stub : alwaysSpawned )
	{
		csvContent += StringAnsi::Printf( "%u;%5.2f;%ls;%ls;%ls\n",
			index,
			stub.m_distance,
			stub.m_communityName.AsChar(),
			stub.m_agent->GetActivePhaseName().AsChar(),
			stub.m_agent->GetAppearanceName().AsChar() );

		++index;
	}

	fprintf( csv, "%s", csvContent.AsChar() );

	fclose( csv );

	LOG_GAME( TXT( "Always spawned community agents stats dumped to '%s' file" ), ANSI_TO_UNICODE( csvFileName ) );
	return true;
}

void CAgentsWorld::GetDumpMessage( const TDynArray< StubDesc >& alwaysSpawned, String& message )
{
	message.ClearFast();
	message += String::Printf( TXT("Always spawned community agents [%u]:\n"), alwaysSpawned.Size() );

	Uint32 index = 0;
	for ( const StubDesc& stub : alwaysSpawned )
	{
		message += String::Printf( TXT("  [%u] Distance: %5.2f m %ls Active phase: %ls Appearance: %ls\n"),
			index,
			stub.m_distance,
			stub.m_communityName.AsChar(),
			stub.m_agent->GetActivePhaseName().AsChar(),
			stub.m_agent->GetAppearanceName().AsChar() );

		++index;
	}
}

void funcDumpCommunityAgentsCPP( IScriptable* context, CScriptStackFrame& stack, void* result  )
{
	FINISH_PARAMETERS;

	if ( !GCommonGame->IsActive() )
	{
		return;
	}

	if ( CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >() )
	{
		communitySystem->GetAgentsWorld().Dump();
	}
}

void RegisterAgentsWorldScriptFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "DumpCommunityAgentsCPP", funcDumpCommunityAgentsCPP );
}

///////////////////////////////////////////////////////////////////////////////

#if 0
RED_MESSAGE("FIXME>>>>> Don't make noncopyable and then copy")
AllAgentsIterator CAgentsWorld::GetAllAgents() const
{
	return AllAgentsIterator( m_allAgents );
}

///////////////////////////////////////////////////////////////////////////////

SpawnedAgentsIterator CAgentsWorld::GetSpawnedAgents() const
{
	return SpawnedAgentsIterator( m_allAgents );
}
#endif

///////////////////////////////////////////////////////////////////////////////

NearbyAgentsIterator CAgentsWorld::GetNearbyAgents( const Vector& pos, Float maxDist ) const
{
	TDynArray< TPointerWrapper< IAgent > > agents;

	return NearbyAgentsIterator( agents );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SpawnedAgentsIterator::operator++ ()
{
	do
	{
		m_currAgent++;
	} while( m_currAgent < m_agents.Size() && ( m_agents[ m_currAgent ]->GetLOD() <= ALOD_Invisible ) );
}

///////////////////////////////////////////////////////////////////////////////
