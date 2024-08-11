#include "build.h"
#include "questGraphSocket.h"
#include "questConditionBlock.h"
#include "questCondition.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestConditionBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestConditionBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( True ), LSD_Output, LSP_Right ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( False ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestConditionBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	Bool isConditionFulfilled = true;

	if ( m_questCondition )
	{
		IQuestCondition* condition = Cast< IQuestCondition >( m_questCondition->Clone( const_cast< CQuestConditionBlock* >( this ) ) );
		condition->Activate();
		isConditionFulfilled = condition->IsFulfilled();
		condition->Deactivate();
		condition->Discard();
	}
	else
	{
		WARN_GAME( TXT("Quest conditions was not set for '%ls'. Defaulting to TRUE."), GetFriendlyName().AsChar() );
		isConditionFulfilled = true;
	}

	ActivateOutput( data, isConditionFulfilled ? CNAME( True ) : CNAME( False ) );
}
