#include "build.h"
#include "questResetScriptedActionsBlock.h"

#include "../../common/engine/graphConnectionRebuilder.h"
#include "../../common/game/questGraphSocket.h"

///////////////////////////////////////////////////////////////////////////////
// CQuestResetScriptedActionsBlock
IMPLEMENT_ENGINE_CLASS( CQuestResetScriptedActionsBlock );

CQuestResetScriptedActionsBlock::CQuestResetScriptedActionsBlock()
	: m_onlyOneActor( true )
{
	m_name = TXT("Reset scripted actions");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestResetScriptedActionsBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CQuestGraphBlock::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

EGraphBlockShape CQuestResetScriptedActionsBlock::GetBlockShape() const
{
	return GBS_Default;
}
Color CQuestResetScriptedActionsBlock::GetClientColor() const
{
	return Color( 255, 0, 0 );
}
String CQuestResetScriptedActionsBlock::GetBlockCategory() const
{
	static const String STR( TXT( "Scripting" ) );
	return STR;
}
Bool CQuestResetScriptedActionsBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const
{
	return true;
}

#endif


Bool CQuestResetScriptedActionsBlock::OnProcessActivation( InstanceBuffer& data ) const
{
	if ( !CQuestGraphBlock::OnProcessActivation( data ) )
	{
		return false;
	}

	TDynArray< CEntity* > entities;
	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	tagMgr->CollectTaggedEntities( m_npcTag, entities );		

	Bool result = true;
	Uint32 actorsCount = 0;
	for ( auto entity : entities )
	{
		CActor* actor = Cast< CActor >( entity );
		if ( actor )
		{
			if ( actorsCount++ && m_onlyOneActor )
			{
				RED_LOG_ERROR( CNAME( CBaseQuestScriptedActionsBlock ), TXT( "CQuestResetScriptedActionsBlock, Too many NPCs with a tag '%ls' were found - can't decide which one to pick" ), m_npcTag.AsString().AsChar() );
				break;
			}

			result &= actor->CancelAIBehavior( SForcedBehaviorEventData::IgnoreActionId, CNAME( AI_Rider_Forced_Cancel ) );
			result &= actor->CancelAIBehavior( SForcedBehaviorEventData::IgnoreActionId );
		}
	}

	if ( actorsCount && result )
	{
		ActivateOutput( data, CNAME( Out ), true );
		return true;
	}

	return false;
}
