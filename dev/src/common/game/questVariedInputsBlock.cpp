#include "build.h"
#include "questGraphSocket.h"
#include "questVariedInputsBlock.h"
#include "../engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CQuestVariedInputsBlock )

CQuestVariedInputsBlock::CQuestVariedInputsBlock()
	: m_inputsCount( 2 )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestVariedInputsBlock::AddInput()
{
	++m_inputsCount;
	OnRebuildSockets();
}

void CQuestVariedInputsBlock::RemoveInput()
{
	--m_inputsCount;
	if ( m_inputsCount < 2 )
	{
		m_inputsCount = 2;
	}
	OnRebuildSockets();
}

Bool CQuestVariedInputsBlock::CanRemoveInput() const
{
	return m_inputsCount > 2;
}

void CQuestVariedInputsBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	for ( Uint32 i = 0; i < m_inputsCount; ++i )
	{
		CreateSocket( CQuestGraphSocketSpawnInfo( CName( String::Printf( TXT( "In %d" ), i ) ), LSD_Input, LSP_Left ) );
	}

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );
}

#endif