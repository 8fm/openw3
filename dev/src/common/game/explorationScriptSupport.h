
#pragma once

#include "expIntarface.h"
#include "expOracle.h"

class ExpManager;

//////////////////////////////////////////////////////////////////////////

/// Scripted wrapper for exploration player
class CScriptedExplorationTraverser : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CScriptedExplorationTraverser );

	ExpPlayer*	m_player;

public:
	CScriptedExplorationTraverser();

	// Engine side
	Bool Init( const SExplorationQueryToken& token, ExpManager* dir, CEntity* ent, const THandle< IScriptable >& listener );

	void OnActionStoped();

	void Release();

	void Update( Float dt );
	Bool IsRunning() const;

	void GenerateDebugFragments( CRenderFrame* frame );

	Bool GetExplorationType( EExplorationType& explorationTtype );


protected:
	void funcUpdate( CScriptStackFrame& stack, void* result );
	void funcConnectListener( CScriptStackFrame& stack, void* result );
	void funcDisconnectListener( CScriptStackFrame& stack, void* result );
	void funcGetExplorationType( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CScriptedExplorationTraverser );
	PARENT_CLASS( IScriptable );
	NATIVE_FUNCTION( "Update", funcUpdate );
	NATIVE_FUNCTION( "ConnectListener", funcConnectListener );
	NATIVE_FUNCTION( "DisconnectListener", funcDisconnectListener );
	NATIVE_FUNCTION( "GetExplorationType", funcGetExplorationType );
END_CLASS_RTTI();
