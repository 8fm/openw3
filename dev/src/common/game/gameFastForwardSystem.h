/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "gameSystem.h"
#include "encounter.h"


struct SFastForwardExecutionParameters
{
	SFastForwardExecutionParameters( const Vector& referencePos = Vector::ZEROS );

	Bool									m_isExternallyTicked;
	Bool									m_dontSpawnHostilesClose;
	Bool									m_despawnExistingGuyz;
	Bool									m_handleBlackScreen;
	Bool									m_fastForwardSpawnTrees;
	Vector									m_referencePosition;
};

struct SFastForwardExecutionContext
{
	Bool									m_allowSelfCompletion;
	Bool									m_isShutdownRequested;
	Bool									m_isCompleted;
	Matrix									m_referenceTransform;
	SFastForwardExecutionParameters			m_parameters;
};


/// Base class for all gameplay related systems
class CGameFastForwardSystem : public IGameSystem
{
	DECLARE_ENGINE_CLASS( CGameFastForwardSystem, IGameSystem,0 );

public:
	struct SimulatedEncounter
	{
		THandle< CEncounter >					m_encounter;
		CEncounter::FastForwardUpdateContext	m_context;
	};
protected:
	Bool									m_isOn;
	Bool									m_isWaiting;
	Bool									m_beginRequestIsPending;
	
	Float									m_waitingTimer;
	SFastForwardExecutionContext			m_executionContext;
	TDynArray< SimulatedEncounter >			m_simulatedEncounters;
	SFastForwardExecutionParameters			m_requestedExecutionParameters;

	static const String						BLACKSCREEN_REASON;

	void ProcessPendingBeginRequest();
	void TickInternal( Float timeDelta );
public:
	CGameFastForwardSystem();
	~CGameFastForwardSystem();

	Bool ExternalTick( Float timeDelta = 0.f );

	void BeginFastForward( SFastForwardExecutionParameters&& parameters );					// begin fast forward
	void EndFastForward();																	// force-end fast forward
	Bool AllowFastForwardSelfCompletion();													// Make fast forward self-terminate
	Bool RequestFastForwardShutdown();

	void CoverWithBlackscreen();															// dynamically (and immediatelly) turn on blackscreen cover

	Bool IsFastForwardCompleted() const														{ return !m_isOn || m_executionContext.m_isCompleted; }

	virtual void OnWorldStart( const CGameInfo& gameInfo ) override;
	virtual void OnWorldEnd( const CGameInfo& gameInfo ) override;

	virtual void Tick( Float timeDelta ) override;
	virtual void PausedTick() override;

	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) override;

	ASSIGN_GAME_SYSTEM_ID( GS_FastForward );

private:
	void funcBeginFastForward( CScriptStackFrame& stack, void* result );
	void funcEndFastForward( CScriptStackFrame& stack, void* result );
	void funcAllowFastForwardSelfCompletion( CScriptStackFrame& stack, void* result );
	void funcRequestFastForwardShutdown( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CGameFastForwardSystem )
	PARENT_CLASS( IGameSystem )
	NATIVE_FUNCTION( "BeginFastForward", funcBeginFastForward )
	NATIVE_FUNCTION( "EndFastForward", funcEndFastForward )
	NATIVE_FUNCTION( "AllowFastForwardSelfCompletion", funcAllowFastForwardSelfCompletion )
	NATIVE_FUNCTION( "RequestFastForwardShutdown", funcRequestFastForwardShutdown )
END_CLASS_RTTI()