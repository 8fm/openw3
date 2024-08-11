#include "build.h"

#include "../../common/game/questThread.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questGraphSocket.h"
#include "../../common/core/instanceDataLayoutCompiler.h"

#include "questBehaviorEventBlock.h"
#include "questBehaviorNotificationBlock.h"
#include "questBehaviorCtrlBlock.h"
#include "questControlledNPC.h"
#include "r4QuestSystem.h"
#include "questBehaviorSocket.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( SQuestBehaviorEvent );
RED_DEFINE_STATIC_NAME( Sync )

CBehaviorGraph* SQuestBehaviorEvent::GetParentGraph()
{
	if ( m_npcTag == CName::NONE )
	{
		// tag for which we need to look up the graph is not specified
		return NULL;
	}

	// get parent block
	CQuestBehaviorEventBlock* parent = Cast< CQuestBehaviorEventBlock >( GetParent() );
	CQuestBehaviorCtrlBlock* hostBehCtrlBlock = parent->GetParentBehaviorBlock();
	ASSERT( hostBehCtrlBlock );
	if ( !hostBehCtrlBlock )
	{
		return NULL;
	}

	// get the behavior assigned to the selected tag
	TSoftHandle< CBehaviorGraph > behavior = hostBehCtrlBlock->GetBehaviorFor( m_npcTag );

	return behavior.Get();
}

CQuestBehaviorCtrlBlock* SQuestBehaviorEvent::GetParentBehaviorBlock()
{
	CQuestBehaviorEventBlock* parent = Cast< CQuestBehaviorEventBlock >( GetParent() );
	return parent->GetParentBehaviorBlock();
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestBehaviorEventBlock );

CQuestBehaviorEventBlock::CQuestBehaviorEventBlock()
	: m_timeout( 20.f )
{
	m_name = TXT( "Behavior Event Emitter" );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestBehaviorEventBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Sync ), LSD_Output, LSP_Center, ClassID<CQuestBehaviorSyncGraphSocket>() ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

Bool CQuestBehaviorEventBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const 
{ 
	return graph && graph->GetParent()->IsExactlyA< CQuestBehaviorCtrlBlock >();
}

void CQuestBehaviorEventBlock::CacheConnections()
{
	TBaseClass::CacheConnections();

	
}

#endif

void CQuestBehaviorEventBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_startTime;
}

void CQuestBehaviorEventBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_startTime ] = 0.f;
}

void CQuestBehaviorEventBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	String errorMsg;
	// at this point all connected notification blocks should be active
	if ( VerifyConnectedNotifications( data, parentThread, errorMsg ) == false )
	{
		ThrowError( data, errorMsg.AsChar() );
		return;
	}

	if ( RaiseEvent( data, errorMsg ) == false )
	{
		ThrowError( data, errorMsg.AsChar() );
		return;
	}

	data[ i_startTime ] = ( Float )GGame->GetEngineTime();
}

void CQuestBehaviorEventBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data  );

	if ( VerifyEventsProcessed( data ) )
	{
		ActivateOutput( data, CNAME( Out ) );
	}

	// Check timeout
	Float startTime = data[ i_startTime ];

	if ( ( Float )GGame->GetEngineTime() - startTime > m_timeout )
	{
		ThrowErrorNonBlocking( data, CNAME( Out ), TXT("CQuestBehaviorEventBlock timeout") );
	}
}

void CQuestBehaviorEventBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );

	CQuestControlledNPCsManager* mgr = GCommonGame->GetSystem< CR4QuestSystem >()->GetNPCsManager();
	if ( !mgr )
	{
		return;
	}

	// cleanup after the event
	for ( TDynArray< SQuestBehaviorEvent* >::const_iterator it = m_events.Begin();
		it != m_events.End(); ++it )
	{
		const SQuestBehaviorEvent* eventDesc = *it;
		if ( !eventDesc )
		{
			continue;
		}

		mgr->CleanupAfterBehaviorEvent( eventDesc->m_npcTag, eventDesc->m_behaviorEvent );
	}
}

Bool CQuestBehaviorEventBlock::VerifyConnectedNotifications( InstanceBuffer& data, CQuestThread* parentThread, String& errorMsg ) const
{
	TDynArray< SBlockDesc > blocks;
	GetConnectedBlocks( CNAME( Sync ), blocks );

	for( TDynArray< SBlockDesc >::const_iterator it = blocks.Begin(); it != blocks.End(); ++it )
	{
		CQuestBehaviorNotificationBlock* block = SafeCast< CQuestBehaviorNotificationBlock >( it->block );

		if ( parentThread->IsBlockActive( block ) == false )
		{
#ifndef NO_EDITOR_GRAPH_SUPPORT
			errorMsg = String::Printf( TXT("Synchronized block '%ls' is not active"), block->GetCaption().AsChar() );
#endif
			return false;
		}
	}

	return true;
}

Bool CQuestBehaviorEventBlock::RaiseEvent( InstanceBuffer& data, String& errorMsg ) const
{
	CQuestControlledNPCsManager* mgr = GCommonGame->GetSystem< CR4QuestSystem >()->GetNPCsManager();
	if ( !mgr )
	{
		errorMsg = TXT( "CQuestControlledNPCsManager instance doesn't exist" );
		ASSERT( mgr, errorMsg.AsChar() );
		ERR_R4( errorMsg.AsChar() );
		return false;
	}

	// send an event to the specified NPC
	for ( TDynArray< SQuestBehaviorEvent* >::const_iterator it = m_events.Begin();
		it != m_events.End(); ++it )
	{
		const SQuestBehaviorEvent* eventDesc = *it;
		if ( !eventDesc )
		{
			continue;
		}

		if ( !mgr->RaiseBehaviorEvent( eventDesc->m_npcTag, eventDesc->m_behaviorEvent ) )
		{
			errorMsg = String::Printf( TXT( "Could not raise behavior event '%ls' for actor '%ls'" ), eventDesc->m_behaviorEvent.AsString().AsChar(), eventDesc->m_npcTag.AsString().AsChar() );
			return false ;
		}
	}
	return true;
}

Bool CQuestBehaviorEventBlock::VerifyEventsProcessed( InstanceBuffer& data ) const
{
	CQuestControlledNPCsManager* mgr = GCommonGame->GetSystem< CR4QuestSystem >()->GetNPCsManager();
	if ( !mgr )
	{
		String errorMsg = TXT( "CQuestControlledNPCsManager instance doesn't exist" );
		ASSERT( mgr, errorMsg.AsChar() );
		ERR_R4( errorMsg.AsChar() );
		return false;
	}

	// send an event to the specified NPC
	for ( TDynArray< SQuestBehaviorEvent* >::const_iterator it = m_events.Begin();
		it != m_events.End(); ++it )
	{
		const SQuestBehaviorEvent* eventDesc = *it;
		if ( !eventDesc )
		{
			continue;
		}

		if ( !mgr->HasBehaviorBeenProcessed( eventDesc->m_npcTag, eventDesc->m_behaviorEvent ) )
		{
			return false;
		}
	}
	return true;
}

CQuestBehaviorCtrlBlock* CQuestBehaviorEventBlock::GetParentBehaviorBlock()
{
	if ( CQuestGraph* graph = Cast< CQuestGraph >( GetParent() ) )
	{
		if ( CQuestBehaviorCtrlBlock *scopeBlock = Cast< CQuestBehaviorCtrlBlock >( graph->GetParent() ) )
		{
			return scopeBlock;
		}
	}

	return NULL;
}

Bool CQuestBehaviorEventBlock::DoesContainTags( const TagList& tags ) const
{
	for ( TDynArray< SQuestBehaviorEvent* >::const_iterator it = m_events.Begin();
		it != m_events.End(); ++it )
	{
		const SQuestBehaviorEvent* eventDesc = *it;
		if ( !eventDesc )
		{
			continue;
		}

		if ( tags.HasTag( eventDesc->m_npcTag ) )
		{
			return true;
		}
	}

	return false;
}
