/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "r6behTreeInstance.h"


enum EAITickPriorityGroup
{
	TICK_EveryFrame = 0,
	TICK_Important,
	TICK_Normal,
	TICK_Idle,

	TICK_MAX
};

BEGIN_ENUM_RTTI( EAITickPriorityGroup )
	ENUM_OPTION( TICK_EveryFrame )
	ENUM_OPTION( TICK_Important )
	ENUM_OPTION( TICK_Normal )
	ENUM_OPTION( TICK_Idle )
END_ENUM_RTTI()

class CR6AISystem : public IGameSystem
{
	DECLARE_ENGINE_CLASS( CR6AISystem, IGameSystem, 0 );

protected:
	static Uint32 TICK_GROUP_BUDGET[ TICK_MAX ]; 

	struct STickEntry
	{
		STickEntry()
			: m_component( nullptr )
			, m_timeAccum( 0.f )
		{
		}

		STickEntry( CAITreeComponent* component )
			: m_component( component )
			, m_timeAccum( 0.f )
		{
		}

		CAITreeComponent*			m_component;
		Float						m_timeAccum;
	};

	TDynArray< STickEntry >			m_tickGroups[ TICK_MAX ];
	Uint32							m_tickIndices[ TICK_MAX ];

	TDynArray< CAIAction* >			m_runningActions;
	TDynArray< CAIAction* >			m_tickableActions;

public:
	CR6AISystem();

	// IGameSystem interface
	virtual void OnGameStart	( const CGameInfo& gameInfo )		{};
	virtual void OnGameEnd		( const CGameInfo& gameInfo )		{};
	virtual void OnWorldStart	( const CGameInfo& gameInfo );
	virtual void OnWorldEnd		( const CGameInfo& gameInfo );
	virtual void Tick			( Float timeDelta );
	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) override;

public:
	// own interface
	void RegisterComponent( CAITreeComponent* component );
	void UnregisterComponent( CAITreeComponent* component );

	void OnPerformAIAction( CAIAction* runningAction );
	void OnStopAIAction( CAIAction* stoppedAction );

protected:
	TDynArray< STickEntry >::iterator FindTickEntryByComponent( EAITickPriorityGroup priorityGroup, const CAITreeComponent* component	);

	ASSIGN_R6_GAME_SYSTEM_ID( GSR6_AI )
};

BEGIN_CLASS_RTTI( CR6AISystem )
	PARENT_CLASS( IGameSystem )
END_CLASS_RTTI()


