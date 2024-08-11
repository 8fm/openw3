/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/communitySystem.h"

struct SAgentStub;

struct SCommunityDebuggerMessages
{
	String m_spawnedBeg;
	String m_spawnedEnd;
	String m_stubBeg;
	String m_stubEnd;
	String m_currentBeg;
	String m_currentEnd;

	// whole words
	String m_spawned; // agent is spawned
	String m_stub;    // agent stub
};

class CCommunityDebugger
{
public:
	CCommunityDebugger();
	
	Bool Init();
	void DeInit();

	void Update();

	// Agents debug informations
	const TDynArray< String >& GetAgentsShortInfo();
	Uint32 GetAgentsNum();
	Bool SetCurrentAgent( Uint32 agentIndex );
	const String& GetCurrentAgentInfo();
	Bool GetAgentInfo( Uint32 agentIndex, String &agentInfo ); // detailed description for agent

	// Action points debug informations
	const TDynArray< String >& GetActionPointsShortInfo();
	Uint32 GetActionPointsNum();
	Bool SetCurrentActionPoint( Uint32 apIndex );
	const String& GetCurrentActionPointInfo();
	Bool GetActionPointInfo( Uint32 apIndex, String &apInfo );

	// Despawn places
	const TDynArray< String >& GetDespawnPlacesShortInfo();
	Uint32 GetDespawnPlacesNum();
	Bool SetCurrentDespawnPlace( Uint32 despawnPlaceIndex );

	// Story phases
	const TDynArray< String >& GetStoryPhasesShortInfo();
	Bool SetCurrentStoryPhase( Uint32 storyPhaseIndex );

private:
	String GetInfoForAgentStub( const SAgentStub *agentStub );
	String GetInfoForActionPoint( const CActionPointComponent *actionPoint );
	TActionPointID GetActionPointIDByIndex( Uint32 index );
	CActionPointComponent* GetActionPointByIndex( Uint32 index );
	String GetStringFromArray( const TDynArray< CName > &names );

	// general data
	CCommunitySystem             *m_communitySystem;
	CCommunitySystem::SDebugData  m_csDebugData;
	Bool                          m_isInitialized; // is community debugger initialized?
	SCommunityDebuggerMessages    m_msg;

	// agents data
	Int32                 m_currentAgentIndex;
	TDynArray< String > m_agentsShortInfo;
	String              m_currentAgentInfo;

	// action points data
	Int32                 m_currentActionPointIndex;
	TDynArray< String > m_actionPointsShortInfo;
	String              m_currentActionPointInfo;

	// despawn places
	Int32                 m_currentDespawnPlaceIndex;
	TDynArray< String > m_despawnPlacesShortInfo;

	// story phases
	Int32                 m_currentStoryPhaseIndex;
	TDynArray< String > m_storyPhasesShortInfo;
};
