#pragma once

#include "build.h"

class CTeam;

class CTeamManager : public CObject
{
	DECLARE_ENGINE_CLASS( CTeamManager, CObject, 0 );

private:
	TDynArray< CName >	m_teamsNames;
	TDynArray< CTeam* >	m_teams;
	CTeam*				m_playerTeam;

private:
	CTeam* AddNewTeam( CName teamName );

public:	
	CTeam* GetTeamByName( CName teamName );
	void Initialize();
	void Tick( Float timeDelta );

private:
	void funcGetTeamByName( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CTeamManager );
	PARENT_CLASS( CObject );	
	PROPERTY( m_teams		);
	PROPERTY( m_playerTeam	);
	PROPERTY( m_teamsNames	);

	NATIVE_FUNCTION( "I_GetTeamByName"	, funcGetTeamByName		);
END_CLASS_RTTI();