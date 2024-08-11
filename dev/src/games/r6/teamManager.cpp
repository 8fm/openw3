#include "build.h"

#include "teamManager.h"
#include "team.h"

IMPLEMENT_ENGINE_CLASS( CTeamManager );

void CTeamManager::Initialize()
{
	m_playerTeam	= CreateObject< CTeam >( this );	
	m_playerTeam->Initialize();
	m_playerTeam->SetTeamName( CNAME( PlayerTeam ) );

	m_teamsNames.Clear();
	m_teams		.Clear();
}

CTeam* CTeamManager::GetTeamByName( CName teamName )
{
	if( teamName == CNAME( PlayerTeam ) )
	{
		return m_playerTeam;
	}
	else
	{
		Uint32 teamIndex = 0;
		for( ; teamIndex<m_teamsNames.Size(); ++teamIndex )
		{
			if( m_teamsNames[ teamIndex ] == teamName )
			{
				break;
			}
		}
		if( teamIndex<m_teamsNames.Size() )
		{
			return m_teams[ teamIndex ];
		}
		else
		{
			return AddNewTeam( teamName );
		}
	} 
}

CTeam* CTeamManager::AddNewTeam( CName teamName )
{
	CTeam* team = CreateObject< CTeam >( this );
	team->Initialize();
	team->SetTeamName( teamName );

	m_teams		.PushBack( team );
	m_teamsNames.PushBack( teamName );	

	return team;
}

void CTeamManager::Tick( Float timeDelta )
{
	if( m_playerTeam )
		m_playerTeam->Tick( timeDelta );

	for( Uint32 i=0; i<m_teams.Size(); ++i )
	{
		if( m_teams[i] )
		{
			m_teams[i]->Tick( timeDelta );
		}
	}
}

void CTeamManager::funcGetTeamByName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, teamName, CName::NONE );
	FINISH_PARAMETERS;

	CTeam* team = GetTeamByName( teamName );
	THandle< CTeam > teamHandle = team;

	RETURN_HANDLE( CTeam, teamHandle );
}