
#include "build.h"
#include "explorationScriptSupport.h"
#include "expPlayer.h"

IMPLEMENT_ENGINE_CLASS( SExplorationQueryContext );
IMPLEMENT_ENGINE_CLASS( SExplorationQueryToken );
IMPLEMENT_ENGINE_CLASS( CScriptedExplorationTraverser );

CScriptedExplorationTraverser::CScriptedExplorationTraverser()
	: m_player( NULL )
{
}

Bool CScriptedExplorationTraverser::Init( const SExplorationQueryToken& token, ExpManager* dir, CEntity* ent, const THandle< IScriptable >& listener )
{
	ASSERT( !m_player );

	m_player = new ExpPlayer( token, dir, ent, listener );

	return m_player->Init();
}
void CScriptedExplorationTraverser::OnActionStoped()
{
	if ( m_player )
	{
		m_player->OnActionStoped();
	}
}
void CScriptedExplorationTraverser::Release()
{
	ASSERT( m_player );

	delete m_player;
	m_player = NULL;
}

void CScriptedExplorationTraverser::Update( Float dt )
{
	if ( m_player )
	{
		m_player->Update( dt );
	}
}

Bool CScriptedExplorationTraverser::IsRunning() const
{
	return m_player && m_player->IsRunning();
}

Bool CScriptedExplorationTraverser::GetExplorationType(  EExplorationType& explorationType ) 
{ 
	if( m_player ) 
	{ 
		explorationType	=  m_player->GetExplorationType(); 

		return true;
	} 

	return false;
}

void CScriptedExplorationTraverser::GenerateDebugFragments( CRenderFrame* frame )
{
	if ( m_player )
	{
		m_player->GenerateDebugFragments( frame );
	}
}

void CScriptedExplorationTraverser::funcUpdate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, dt, 0.f );
	FINISH_PARAMETERS;

	Update( dt );
}

void CScriptedExplorationTraverser::funcConnectListener( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< IScriptable >, objectH, NULL );
	FINISH_PARAMETERS;

	if ( m_player && objectH )
	{
		m_player->ConnectListener( objectH );
	}
}

void CScriptedExplorationTraverser::funcDisconnectListener( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( m_player )
	{
		m_player->DisconnectListener();
	}
}

void CScriptedExplorationTraverser::funcGetExplorationType( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( EExplorationType, expType, ET_Jump );
	FINISH_PARAMETERS;

	Bool	found;

	found	= GetExplorationType( expType );

	RETURN_BOOL( found );
}
