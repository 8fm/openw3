#include "build.h"

#include "../../common/game/questGraphSocket.h"
#include "../../common/game/questGraph.h"
#include "../../common/core/instanceDataLayoutCompiler.h"

#include "questBehaviorNotificationBlock.h"
#include "questControlledNPC.h"
#include "r4QuestSystem.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestBehaviorNotificationBlock );
IMPLEMENT_ENGINE_CLASS( SQuestBehaviorNotification );


///////////////////////////////////////////////////////////////////////////////

void SQuestBehaviorNotification::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_expectedNotificationsCount;
	compiler << i_receivedNotificationsCount;
}

void SQuestBehaviorNotification::OnInitInstance( InstanceBuffer& instanceData ) const
{
	instanceData[ i_expectedNotificationsCount ] = 0;
	instanceData[ i_receivedNotificationsCount ] = 0;
}

void SQuestBehaviorNotification::SetExpectedNotificationsCount( InstanceBuffer& data, CQuestControlledNPCsManager& mgr ) const
{
	if ( m_all )
	{
		data[ i_expectedNotificationsCount ] = mgr.GetControlledEntitiesCount( m_npcTag );
	}
	else
	{
		data[ i_expectedNotificationsCount ] = 1;
	}
	data[ i_receivedNotificationsCount ] = 0;
}

void SQuestBehaviorNotification::UpdateReceivedNotificationsCount( InstanceBuffer& data, CQuestControlledNPCsManager& mgr ) const
{
	data[ i_receivedNotificationsCount ] += mgr.WasNotificationReceived( m_npcTag, m_notification );
}

Bool SQuestBehaviorNotification::IsFulfilled( InstanceBuffer& data ) const
{
	return ( data[ i_receivedNotificationsCount ] >= data[ i_expectedNotificationsCount ] );
}

CBehaviorGraph* SQuestBehaviorNotification::GetParentGraph()
{
	if ( m_npcTag == CName::NONE )
	{
		// tag for which we need to look up the graph is not specified
		return NULL;
	}

	// get parent block
	CQuestBehaviorNotificationBlock* parent = Cast< CQuestBehaviorNotificationBlock >( GetParent() );
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

CQuestBehaviorCtrlBlock* SQuestBehaviorNotification::GetParentBehaviorBlock()
{
	CQuestBehaviorNotificationBlock* parent = Cast< CQuestBehaviorNotificationBlock >( GetParent() );
	return parent->GetParentBehaviorBlock();
}

///////////////////////////////////////////////////////////////////////////////

CQuestBehaviorNotificationBlock::CQuestBehaviorNotificationBlock()
{
	m_name = TXT( "Behavior Notification Pause" );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestBehaviorNotificationBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("npcTag") )
	{
		OnRebuildSockets();
	}
}

void CQuestBehaviorNotificationBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

Bool CQuestBehaviorNotificationBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const 
{ 
	return graph && graph->GetParent()->IsExactlyA< CQuestBehaviorCtrlBlock >();
}

#endif

TagList CQuestBehaviorNotificationBlock::GetNPCTags() const
{
	TagList list;

	for ( TDynArray< SQuestBehaviorNotification* >::const_iterator it = m_notifications.Begin();
		it != m_notifications.End(); ++it )
	{
		const SQuestBehaviorNotification* notifDesc = *it;
		if ( !notifDesc )
		{
			continue;
		}
		list.AddTag( notifDesc->m_npcTag );
	}

	return list;
}

void CQuestBehaviorNotificationBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	for ( TDynArray< SQuestBehaviorNotification* >::iterator it = m_notifications.Begin();
		it != m_notifications.End(); ++it )
	{
		SQuestBehaviorNotification* notifDesc = *it;
		if ( !notifDesc )
		{
			continue;
		}
		notifDesc->OnBuildDataLayout( compiler );
	}
}

void CQuestBehaviorNotificationBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	for ( TDynArray< SQuestBehaviorNotification* >::const_iterator it = m_notifications.Begin();
		it != m_notifications.End(); ++it )
	{
		const SQuestBehaviorNotification* notifDesc = *it;
		if ( !notifDesc )
		{
			continue;
		}
		notifDesc->OnInitInstance( instanceData );
	}
}

void CQuestBehaviorNotificationBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CQuestControlledNPCsManager* mgr = GCommonGame->GetSystem< CR4QuestSystem >()->GetNPCsManager();
	if ( !mgr )
	{
		ASSERT( mgr, TXT( "CQuestControlledNPCsManager instance doesn't exist" ) );
		ERR_R4( TXT( "CQuestControlledNPCsManager instance doesn't exist" ) );
		ThrowError( data, TXT( "CQuestControlledNPCsManager instance doesn't exist" ) );
		return;
	}

	// set expected values
	for ( TDynArray< SQuestBehaviorNotification* >::const_iterator it = m_notifications.Begin();
		it != m_notifications.End(); ++it )
	{
		const SQuestBehaviorNotification* notifDesc = *it;
		if ( !notifDesc )
		{
			continue;
		}
		notifDesc->SetExpectedNotificationsCount( data, *mgr );
	}

	if ( IsFulfilled( data ) )
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}

void CQuestBehaviorNotificationBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	if ( IsFulfilled( data )  )
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}

Bool CQuestBehaviorNotificationBlock::IsFulfilled( InstanceBuffer& data ) const
{
	CQuestControlledNPCsManager* mgr = GCommonGame->GetSystem< CR4QuestSystem >()->GetNPCsManager();
	if ( !mgr )
	{
		ASSERT( mgr, TXT( "CQuestControlledNPCsManager instance doesn't exist" ) );
		ERR_R4( TXT( "CQuestControlledNPCsManager instance doesn't exist" ) );
		return false;
	}

	Bool result = true;
	for ( TDynArray< SQuestBehaviorNotification* >::const_iterator it = m_notifications.Begin();
		it != m_notifications.End(); ++it )
	{
		const SQuestBehaviorNotification* notifDesc = *it;
		if ( !notifDesc )
		{
			continue;
		}
		notifDesc->UpdateReceivedNotificationsCount( data, *mgr );
		result &= notifDesc->IsFulfilled( data );
	}

	return result;
}

CQuestBehaviorCtrlBlock* CQuestBehaviorNotificationBlock::GetParentBehaviorBlock()
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
