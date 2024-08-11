#include "build.h"

#include "../../common/engine/behaviorGraph.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/tagManager.h"

#include "questControlledNPC.h"

RED_DEFINE_STATIC_NAME( QuestControlledBehavior ) 

///////////////////////////////////////////////////////////////////////////////

CQuestControlledNPCsManager::CQuestControlledNPCsManager()
{
}

CQuestControlledNPCsManager::~CQuestControlledNPCsManager()
{
	Reset();
	m_listeners.Clear();
}

void CQuestControlledNPCsManager::AttachListener( IQuestNPCsManagerListener& listener )
{
	m_listeners.PushBackUnique( &listener );
}

void CQuestControlledNPCsManager::DetachListener( IQuestNPCsManagerListener& listener )
{
	m_listeners.Remove( &listener );
}

Bool CQuestControlledNPCsManager::Activate( TDynArray< CQuestControlledNPC* >& npcs )
{
	Uint32 activatedCount = 0;
	for ( TDynArray< CQuestControlledNPC* >::iterator npcIt = npcs.Begin(); 
		npcIt != npcs.End(); ++npcIt )
	{
		const CName& npcTag = (*npcIt)->GetTag();

		// check if the NPC isn't already controlled by the quests system
		if ( IsInMap( *npcIt ) )
		{
			NotifyError( String::Printf( TXT( "NPC '%ls' is already managed by the quests system" ), npcTag.AsString().AsChar() ) );
			break;
		}

		// activate the NPC
		String errMsg;
		if ( (*npcIt)->Activate( errMsg ) == false )
		{
			NotifyError( errMsg );
			break;
		}

		++activatedCount;
	}

	if ( activatedCount < npcs.Size() )
	{
		// not all NPCs were activated - deactivate those that were and exit
		for ( Uint32 i = 0; i < activatedCount; ++i )
		{
			String errMsg;
			if ( npcs[i]->Deactivate( errMsg ) == false )
			{
				NotifyError( errMsg );
			}
		}
		return false;
	}

	// assuming all npcs were correctly activated, add them to the tracking list
	for ( TDynArray< CQuestControlledNPC* >::iterator npcIt = npcs.Begin(); 
		npcIt != npcs.End(); ++npcIt )
	{
		const CName& npcTag = (*npcIt)->GetTag();

		AddToMap( *npcIt );
		NotifySuccess( String::Printf( TXT( "Quests system has taken control over NPC '%ls' " ), npcTag.AsString().AsChar() ) );
	}

	return true;
}

void CQuestControlledNPCsManager::Deactivate( TDynArray< CQuestControlledNPC* >& npcsArr )
{
	// deactivate all NPCs, despite possible errors
	for ( TDynArray< CQuestControlledNPC* >::iterator npcIt = npcsArr.Begin(); 
		npcIt != npcsArr.End(); ++npcIt )
	{
		if ( !IsInMap( *npcIt ) )
		{
			continue;
		}

		String errMsg;
		if ( (*npcIt)->Deactivate( errMsg ) == false )
		{
			NotifyError( errMsg );
		}
		else
		{
			NotifySuccess( String::Printf( TXT( "Quests system has relinquished control over NPC '%ls' " ), (*npcIt)->GetTag().AsString().AsChar() ) );
		}

		// remove the NPC from the tracked NPCs map
		RemoveFromMap( *npcIt );
	}
}

void CQuestControlledNPCsManager::Reset()
{
	for ( NPCsMap::iterator it = m_npcs.Begin(); it != m_npcs.End(); ++it )
	{
		NPCsList* npcsList = it->m_second;
		ASSERT( npcsList );

		for ( NPCsList::const_iterator listIt = npcsList->Begin(); listIt != npcsList->End(); ++listIt )
		{
			CQuestControlledNPC* npc = *listIt;
			ASSERT( npc );

			String errMsg;
			if ( npc->Deactivate( errMsg ) == false )
			{
				NotifyError( errMsg );
			}
			else
			{
				NotifySuccess( String::Printf( TXT( "Quests system has relinquished control over NPC '%ls' " ), npc->GetTag().AsString().AsChar() ) );
			}
		}
		delete npcsList;
	}
	m_npcs.Clear();
}

Bool CQuestControlledNPCsManager::RaiseBehaviorEvent( const CName& npcTag, const CName& eventName )
{
	NPCsList* npcs = NULL;
	m_npcs.Find( npcTag, npcs );

	if ( !npcs )
	{
		NotifyError( String::Printf( TXT( "No NPC with tag '%ls' are managed by quests, so we can't have event '%ls' sent to them" ), npcTag.AsString().AsChar(), eventName.AsString().AsChar() ) );
		return false;
	}

	String errMsg;
	for ( NPCsList::iterator it = npcs->Begin(); it != npcs->End(); ++it )
	{
		if ( (*it)->RaiseBehaviorEvent( eventName, errMsg ) == false )
		{
			NotifyError( errMsg );
			return false;
		}
	}

	NotifySuccess( String::Printf( TXT( "Behavior event '%ls' sent to NPC '%ls'" ), eventName.AsString().AsChar(), npcTag.AsString().AsChar() ) );

	return true;
}

Bool CQuestControlledNPCsManager::HasBehaviorBeenProcessed( const CName& npcTag, const CName& eventName )
{
	NPCsList* npcs = NULL;
	m_npcs.Find( npcTag, npcs );

	if ( !npcs )
	{
		NotifyError( String::Printf( TXT( "No NPC with tag '%ls' are managed by quests, so we can't have event '%ls' sent to them" ), npcTag.AsString().AsChar(), eventName.AsString().AsChar() ) );
		return false;
	}

	Bool processed = true;
	String errMsg;
	for ( NPCsList::iterator it = npcs->Begin(); it != npcs->End(); ++it )
	{
		processed &= (*it)->HasBehaviorBeenProcessed( eventName, errMsg );

		if ( !errMsg.Empty() )
		{
			NotifyError( errMsg );
			return false;
		}

		if ( !processed )
		{
			return false;
		}
	}

	return true;
}

void CQuestControlledNPCsManager::CleanupAfterBehaviorEvent( const CName& npcTag, const CName& eventName )
{
	NPCsList* npcs = NULL;
	m_npcs.Find( npcTag, npcs );

	if ( !npcs )
	{
		NotifyError( String::Printf( TXT( "No NPC with tag '%ls' are managed by quests, so we can't have event '%ls' sent to them" ), npcTag.AsString().AsChar(), eventName.AsString().AsChar() ) );
		return;
	}

	for ( NPCsList::iterator it = npcs->Begin(); it != npcs->End(); ++it )
	{
		(*it)->CleanupAfterBehaviorEvent( eventName );
	}
}

Uint32 CQuestControlledNPCsManager::WasNotificationReceived( const CName& npcTag, const CName& notificationName )
{
	NPCsList* npcs = NULL;
	m_npcs.Find( npcTag, npcs );

	if ( !npcs )
	{
		NotifyError( String::Printf( TXT( "No NPC with tag '%ls' are managed by quests, so we can't check if it received notification '%ls'" ), npcTag.AsString().AsChar(), notificationName.AsString().AsChar() ) );
		return false;
	}

	Uint32 count = 0;
	String errMsg;
	for ( NPCsList::iterator it = npcs->Begin(); it != npcs->End(); ++it )
	{
		Bool result = (*it)->WasNotificationReceived( notificationName, errMsg );
		if ( !errMsg.Empty() )
		{
			NotifyError( errMsg );
			return 0;
		}

		if ( result )
		{
			++count;
		}
	}

	return count;
}

Uint32 CQuestControlledNPCsManager::GetControlledEntitiesCount( const CName& npcTag ) const
{
	NPCsList* npcs = NULL;
	m_npcs.Find( npcTag, npcs );

	if ( !npcs )
	{
		return 0;
	}
	else
	{
		return npcs->Size();
	}
}

void CQuestControlledNPCsManager::NotifyError( const String& errMsg ) const
{
	for ( Listeners::const_iterator it = m_listeners.Begin(); it != m_listeners.End(); ++it )
	{
		(*it)->NotifyError( errMsg );
	}
}

void CQuestControlledNPCsManager::NotifySuccess( const String& msg ) const
{
	for ( Listeners::const_iterator it = m_listeners.Begin(); it != m_listeners.End(); ++it )
	{
		(*it)->NotifySuccess( msg );
	}
}

Bool CQuestControlledNPCsManager::IsInMap( CQuestControlledNPC* npc ) const
{
	ASSERT( npc );

	NPCsList* npcsList = NULL;
	const CName& tag = npc->GetTag();
	m_npcs.Find( tag, npcsList );

	if ( !npcsList )
	{
		return false;
	}
	else
	{
		for ( NPCsList::const_iterator it = npcsList->Begin(); it != npcsList->End(); ++it )
		{
			if ( *it == npc )
			{
				return true;
			}
		}

		return false;
	}
}

void CQuestControlledNPCsManager::AddToMap( CQuestControlledNPC* npc )
{
	ASSERT( npc );

	NPCsList* npcsList = NULL;
	const CName& tag = npc->GetTag();
	m_npcs.Find( tag, npcsList );

	if ( !npcsList )
	{
		npcsList = new NPCsList();
		m_npcs.Insert( tag, npcsList );
	}

	npcsList->PushBackUnique( npc );
}

void CQuestControlledNPCsManager::RemoveFromMap( CQuestControlledNPC* npc )
{
	ASSERT( npc );

	NPCsList* npcsList = NULL;
	const CName& tag = npc->GetTag();
	m_npcs.Find( tag, npcsList );

	if ( !npcsList )
	{
		return;
	}
	
	npcsList->Remove( npc );
	if ( npcsList->Empty() )
	{
		delete npcsList;
		m_npcs.Erase( tag );
	}


}

Bool CQuestControlledNPCsManager::IsNPCInQuestScene( const CNewNPC* npc ) const
{
	const TagList& tagList = npc->GetTags();
	const TDynArray< CName >& tags = tagList.GetTags();

	for ( Uint32 i=0; i<tags.Size(); ++i )
	{
		const CName& npcTag = tags[ i ];

		NPCsList* npcsList = NULL;
		m_npcs.Find( npcTag, npcsList );

		if ( npcsList )
		{
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

CQuestControlledNPC::CQuestControlledNPC( const CName& tag, const TSoftHandle< CBehaviorGraph>& behavior, CEntity* entity, CBehaviorGraphStack* stack )
	: m_tag( tag )
	, m_behavior( behavior )
	, m_entity( entity )
	, m_stack( stack )
{
	ASSERT( behavior.GetAsync() == BaseSoftHandle::ALR_Loaded );
}

CQuestControlledNPC::~CQuestControlledNPC()
{
}

Bool CQuestControlledNPC::Activate( String& errMsg )
{
	if ( !IsUpToDate() )
	{
		errMsg = String::Printf( TXT("Quest controlled NPC '%ls' doesn't map onto game entity any more - some were probably deleted"), m_tag.AsString().AsChar() );
		return false;
	}

	// activate behaviors
	CBehaviorGraph *behavior = m_behavior.Get();
	if ( m_stack->AttachBehaviorInstance( behavior, CNAME( QuestControlledBehavior ) ) == false )
	{
		errMsg = String::Printf( TXT("Failed to activate behavior '%ls' on actor '%ls'"), behavior->GetFriendlyName().AsChar(), m_tag.AsString().AsChar() );
		return false;
	}
	m_stack->Lock( true );

	// if it's an actor we're controlling, take control over its AI
	CActor* actor = Cast< CActor >( m_entity );
	if ( actor )
	{
		actor->SetExternalControl( true );
	}

	return true;
}

Bool CQuestControlledNPC::Deactivate( String& errMsg )
{
	if ( !IsUpToDate() )
	{
		errMsg = String::Printf( TXT("Quest controlled NPC '%ls' doesn't map onto game entity any more - some were probably deleted"), m_tag.AsString().AsChar() );
		return false;
	}

	// Release handle
	m_behavior.Release();

	// dectivate behaviors
	m_stack->Lock( false );
	if ( m_stack->DetachBehaviorInstance( CNAME( QuestControlledBehavior ) ) == false )
	{
		errMsg = String::Printf( TXT("Could not deactivate behavior '%ls' on actor '%ls'"), m_behavior.Get()->GetFriendlyName().AsChar(), m_tag.AsString().AsChar() );
		return false;
	}

	// if it's an actor we're controlling, release control over its AI
	CActor* actor = Cast< CActor >( m_entity );
	if ( actor )
	{
		actor->SetExternalControl( false );
	}

	return true;
}

Bool CQuestControlledNPC::RaiseBehaviorEvent( const CName& eventName, String& outErrorMsg )
{
	if ( !IsUpToDate() )
	{
		outErrorMsg = String::Printf( TXT("Quest controlled NPC '%ls' doesn't map onto game entity any more - some were probably deleted"), m_tag.AsString().AsChar() );
		return false;
	}

	// verify that we haven't already received a notification about such an event being processed,
	// meaning that we're in the process of waiting on another event generation block for such an event
	if ( m_processedEvents.Exist( eventName ) )
	{
		String activeNodesList = GetActiveNodesList( m_stack );
		outErrorMsg = String::Printf( TXT("There's another event block already processing event '%ls' for entity '%s. Currently active nodes are [%s]"), eventName.AsString().AsChar(), m_tag.AsString().AsChar(), activeNodesList.AsChar() );
		return false;
	}

	Bool result = m_stack->GenerateBehaviorEvent( CNAME( QuestControlledBehavior ), eventName );
	if ( result == false )
	{
		String activeNodesList = GetActiveNodesList( m_stack );
		outErrorMsg = String::Printf( TXT("BehaviorStack failed to process event '%ls' for entity '%s. Currently active nodes are [%s]"), eventName.AsString().AsChar(), m_tag.AsString().AsChar(), activeNodesList.AsChar() );
	}
	return result;
}

Bool CQuestControlledNPC::HasBehaviorBeenProcessed( const CName& eventName, String& outErrorMsg )
{
	if ( !IsUpToDate() )
	{
		outErrorMsg = String::Printf( TXT("Quest controlled NPC '%ls' doesn't map onto game entity any more - some were probably deleted"), m_tag.AsString().AsChar() );
		return false;
	}

	if ( m_processedEvents.Exist( eventName ) )
	{
		// the event was already processed a while ago
		return true;
	}
	else
	{
		Bool result = m_stack->IsEventProcessed( eventName );
		if ( result == true )
		{
			// memorize that the event was processed
			m_processedEvents.PushBackUnique( eventName );
		}
		return result;
	}
}

void CQuestControlledNPC::CleanupAfterBehaviorEvent( const CName& eventName )
{
	m_processedEvents.Remove( eventName );
}

Bool CQuestControlledNPC::WasNotificationReceived( const CName& notificationName, String& outErrorMsg )
{
	if ( !IsUpToDate() )
	{
		outErrorMsg = String::Printf( TXT("Quest controlled NPC '%ls' doesn't map onto game entity any more - some were probably deleted"), m_tag.AsString().AsChar() );
		return false;
	}

	Bool result = m_stack->ActivationNotificationReceived( notificationName );
	result |= m_stack->DeactivationNotificationReceived( notificationName );

	return result;
}

Bool CQuestControlledNPC::IsUpToDate() const
{
	TDynArray< CNode* > nodes;
	GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( m_tag, nodes );

	for ( Uint32 i = 0; i < nodes.Size(); ++i )
	{
		CEntity* entity = Cast< CEntity >( nodes[ i ] );
		if ( !entity || entity != m_entity )
		{
			continue;
		}

		CAnimatedComponent* animComp = entity->GetRootAnimatedComponent();
		if ( !animComp || animComp->GetBehaviorStack() != m_stack  )
		{
			continue;
		}
	
		// we found a match
		return true;
	}

	return false;
}

String CQuestControlledNPC::GetActiveNodesList( CBehaviorGraphStack* stack ) const
{
	CBehaviorGraphInstance* instance = stack->GetBehaviorGraphInstance( CNAME( QuestControlledBehavior ) );
	ASSERT( instance );

	TDynArray< CBehaviorGraphNode* > nodes;
	m_behavior.Get()->GetAllNodes( nodes );

	String nodesList;
	for ( TDynArray< CBehaviorGraphNode* >::const_iterator it = nodes.Begin();
		it != nodes.End(); ++it )
	{
		if ( (*it)->IsActive( *instance ) )
		{
#ifndef NO_EDITOR_GRAPH_SUPPORT
			nodesList += (*it)->GetCaption() + TXT( ", " );
#endif
		}
	}

	return nodesList;
}
