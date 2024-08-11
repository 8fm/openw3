#include "build.h"
#include "behTreeDynamicNode.h"

#include "../engine/gameSaveManager.h"
#include "../engine/gameDataStorage.h"

#include "aiLog.h"
#include "behTree.h"
#include "behTreeDynamicNodeEvent.h"
#include "behTreeInstance.h"
#include "behTreeStateSerializer.h"


IMPLEMENT_ENGINE_CLASS( SBehTreeDynamicNodeEventData )
IMPLEMENT_ENGINE_CLASS( SBehTreeDynamicNodeCancelEventData )
IMPLEMENT_ENGINE_CLASS( SDynamicNodeSaveStateRequestEventData )


///////////////////////////////////////////////////////////////////////////////
// IBehTreeDynamicNodeBaseDefinition
///////////////////////////////////////////////////////////////////////////////
IAITree* IBehTreeDynamicNodeBaseDefinition::GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	// node starting tree as default implementation
	return NULL;
}
IBehTreeNodeInstance* IBehTreeDynamicNodeBaseDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}
Bool IBehTreeDynamicNodeBaseDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	AI_ASSERT( node->GetDefinitionId() == m_uniqueId );

	if ( !IsMyInstance( node ) )
	{
		return false;
	}

	CBehTreeDynamicNodeInstance* dynamicNode = static_cast< CBehTreeDynamicNodeInstance* >( node );
	node->OnSpawn( *this, context );

	IBehTreeNodeInstance* childNode = dynamicNode->GetDynamicChildNode();
	if ( childNode )
	{
		IAITree* treeDef = GetBaseDefinition( node->GetOwner(), context );
		if ( treeDef )
		{
			CBehTree* res = treeDef->GetTree();
			AI_ASSERT( res );

			IBehTreeNodeDefinition* rootNode = res->GetRootNode();
			AI_ASSERT( rootNode );
			// spawn new child node
			Uint32 params = 1;
			if ( !context.Push( treeDef ) )
			{
				return false;
			}
			Bool wasDynamic = context.MarkAsDynamicBranch( true );
			Bool b = rootNode->OnSpawn( childNode, context );
			context.MarkAsDynamicBranch( wasDynamic );
			context.Pop( params );
			AI_ASSERT( b, TXT("OnSpawn consistency problem") );
		}
	}
		

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// IBehTreeDynamicNodeInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeDynamicNodeInstance::CBehTreeDynamicNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeInstance( def, owner, context, parent )
	, m_eventName( def.GetEventName( owner, context ) )
{
	if ( !m_eventName.Empty() )
	{
		SBehTreeEvenListeningData eventData;
		eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		eventData.m_eventName = m_eventName;
		context.AddEventListener( eventData, this );
	}
	{
		SBehTreeEvenListeningData eventData;
		eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		eventData.m_eventName = CNAME( OnPoolRequest );
		context.AddEventListener( eventData, this );
	}
	{
		SBehTreeEvenListeningData eventData;
		eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		eventData.m_eventName = CNAME( OnReattachFromPoolAsync );
		context.AddEventListener( eventData, this );
	}
	{
		SBehTreeEvenListeningData eventData;
		eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		eventData.m_eventName = CNAME( OnReattachFromPool );
		context.AddEventListener( eventData, this );
	}

	m_baseDefinition = &const_cast< Definition& >( def );

	IAITree* tree = def.GetBaseDefinition( owner, context );
	if ( tree )
	{
		SpawnChildNodeWithContext( tree, context, this, m_owner );
	}
}
CBehTreeDynamicNodeInstance::~CBehTreeDynamicNodeInstance()
{

}

void CBehTreeDynamicNodeInstance::OnDestruction()
{
	if ( !m_eventName.Empty() )
	{
		SBehTreeEvenListeningData eventData;
		eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		eventData.m_eventName = m_eventName;
		m_owner->RemoveEventListener( eventData, this );
	}
	{
		SBehTreeEvenListeningData eventData;
		eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		eventData.m_eventName = CNAME( OnPoolRequest );
		m_owner->RemoveEventListener( eventData, this );
	}
	{
		SBehTreeEvenListeningData eventData;
		eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		eventData.m_eventName = CNAME( OnReattachFromPoolAsync );
		m_owner->RemoveEventListener( eventData, this );
	}
	{
		SBehTreeEvenListeningData eventData;
		eventData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		eventData.m_eventName = CNAME( OnReattachFromPool );
		m_owner->RemoveEventListener( eventData, this );
	}
	

	if ( m_childNode )
	{
		m_childNode->OnDestruction();
	}

	Super::OnDestruction();
}
void CBehTreeDynamicNodeInstance::Update()
{
	m_childNode->Update();
}
Bool CBehTreeDynamicNodeInstance::Activate()
{
	if ( m_childNode )
	{
		if ( m_childNode->Activate() )
		{
			return Super::Activate();
		}
	}

	DebugNotifyActivationFail();
	return false;
}
void CBehTreeDynamicNodeInstance::Deactivate()
{
	if ( m_childNode && m_childNode->IsActive() )
	{
		m_childNode->Deactivate();
	}
	Super::Deactivate();
}
void CBehTreeDynamicNodeInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	Complete( outcome );
}

////////////////////////////////////////////////////////////////////
//! Event handling

void CBehTreeDynamicNodeInstance::RequestChildDespawn()
{

}
Bool CBehTreeDynamicNodeInstance::OnEvent( CBehTreeEvent& e )
{
	AI_ASSERT( m_childNode, TXT("Dynamic node should never be active when child node is NULL") );
	if ( !m_childNode )
	{
		return false;
	}
	return m_childNode->OnEvent( e );
}

Bool CBehTreeDynamicNodeInstance::HandleSpawnEvent( SBehTreeDynamicNodeEventData& eventData )
{
	// try to deactivate child node
	if ( m_isActive )
	{
		if ( !eventData.m_interrupt || !m_childNode->Interrupt() )
		{
			RequestChildDespawn();
			eventData.m_isHandled = SBehTreeDynamicNodeEventData::OUTPUT_DELAYED;
			return false;
		}
	}

	// delete previous child node
	if ( m_childNode )
	{
		if ( !eventData.m_interrupt )
		{
			eventData.m_isHandled = SBehTreeDynamicNodeEventData::OUTPUT_DELAYED;
			return false;
		}
		m_childNode->RegisterForDeletion();
		m_childNode = nullptr;
	}

	if ( m_isActive )
	{
		Complete( BTTO_FAILED );
	}


	IAITree* tree = eventData.m_treeDefinition;
	if ( tree )
	{
		if ( SpawnChildNode( tree, eventData.m_parameters, this, m_owner ) )
		{
			eventData.m_isHandled = SBehTreeDynamicNodeEventData::OUTPUT_HANDLED;
			if ( eventData.m_savedState )
			{
				// try to load previously saved state
				CBehTreeStateSerializer serializer( m_childNode );
				IGameLoader* loader = SGameSaveManager::GetInstance().CreateLoader( eventData.m_savedState, nullptr, nullptr );
				if ( loader )
				{
					{
						CGameSaverBlock block( loader, CNAME( ai ) );
						serializer.Load( loader );
					}

					delete loader;
				}
			}
		}
	}
	return true;
}

Bool CBehTreeDynamicNodeInstance::HandleEvent( SBehTreeDynamicNodeCancelEventData& eventData )
{
	// try to deactivate child node
	if ( m_isActive )
	{
		if ( !eventData.m_interrupt || !m_childNode->Interrupt() )
		{
			RequestChildDespawn();
			eventData.m_isHandled = SBehTreeDynamicNodeCancelEventData::OUTPUT_DELAYED;
			return false;
		}
	}

	// delete previous child node
	if ( m_childNode )
	{
		if ( !eventData.m_interrupt )
		{
			eventData.m_isHandled = SBehTreeDynamicNodeCancelEventData::OUTPUT_DELAYED;
			return false;
		}
		m_childNode->RegisterForDeletion();
		m_childNode = nullptr;
	}

	// complete
	if ( m_isActive )
	{
		Complete( BTTO_FAILED );
	}

	return false;
}

void CBehTreeDynamicNodeInstance::HandlePoolRequest()
{
	if ( m_childNode )
	{
		if ( m_isActive )
		{
			m_childNode->Deactivate();
		}
		m_childNode->RegisterForDeletion();
		m_childNode = nullptr;
	}
	if ( m_isActive )
	{
		Super::Complete( BTTO_FAILED );
	}
}

void CBehTreeDynamicNodeInstance::HandleSaveEvent( SDynamicNodeSaveStateRequestEventData& saveEventData )
{
	if ( !saveEventData.m_processed )
	{
		saveEventData.m_processed = true;

		CBehTreeStateSerializer serializer( m_childNode );
		if ( serializer.IsSaving() )
		{
			IGameDataStorage* dataStorage = CGameDataStorage< MC_Gameplay, MemoryPool_GameSave >::Create( 1024, 0 );
			IGameSaver* saver = SGameSaveManager::GetInstance().CreateSaver( dataStorage, nullptr );
			{
				CGameSaverBlock block( saver, CNAME( ai ) );
				serializer.Save( saver );
			}
			delete saver;
			saveEventData.m_dataBuffer = dataStorage;
		}
		
	}
	else
	{
		saveEventData.m_invalidated = true;
	}
}

Bool CBehTreeDynamicNodeInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == m_eventName )
	{
		{
			SBehTreeDynamicNodeEventData* eventData = e.m_gameplayEventData.Get< SBehTreeDynamicNodeEventData >();
			if ( eventData )
			{
				return HandleSpawnEvent( *eventData );
			}
		}
		{
			SBehTreeDynamicNodeCancelEventData* eventData = e.m_gameplayEventData.Get< SBehTreeDynamicNodeCancelEventData >();
			if ( eventData )
			{
				return HandleEvent( *eventData );
			}
		}
		{
			SDynamicNodeSaveStateRequestEventData* saveEventData = e.m_gameplayEventData.Get< SDynamicNodeSaveStateRequestEventData >();
			if ( saveEventData )
			{
				HandleSaveEvent( *saveEventData );
				return false;
			}
		}
		
	}
	else if ( e.m_eventName == CNAME( OnPoolRequest ) )
	{
		HandlePoolRequest();
	}
	else if ( e.m_eventName == CNAME( OnReattachFromPool ) )
	{
		if ( m_childNode )
		{
			Definition* def = m_baseDefinition.Get();
			if ( def )
			{
				CBehTreeSpawnContext* context = e.m_gameplayEventData.Get< CBehTreeSpawnContext >();
				ASSERT( context );

				IAITree* treeDef = def->GetBaseDefinition( m_owner, *context );
				if ( treeDef )
				{
					CBehTree* res = treeDef->GetTree();
					AI_ASSERT( res );

					IBehTreeNodeDefinition* rootNode = res->GetRootNode();
					AI_ASSERT( rootNode );
					// spawn new child node
					Uint32 params = 1;
					if ( !context->Push( treeDef ) )
					{
						return false;
					}
					Bool wasDynamic = context->MarkAsDynamicBranch( true );
					Bool b = rootNode->OnSpawn( m_childNode, *context );
					context->MarkAsDynamicBranch( wasDynamic );
					context->Pop( params );
					AI_ASSERT( b, TXT("OnSpawn consistency problem") );
				}
			}
		}
		
	}
	else if ( e.m_eventName == CNAME( OnReattachFromPoolAsync ) )
	{
		Definition* def = m_baseDefinition.Get();
		if ( def )
		{
			CBehTreeSpawnContext* context = e.m_gameplayEventData.Get< CBehTreeSpawnContext >();
			ASSERT( context );
			IAITree* treeDef = def->GetBaseDefinition( m_owner, *context );
			if ( treeDef )
			{
				SpawnChildNodeWithContext( treeDef, *context, this, m_owner );
			}
		}
	}
	return false;
}

Bool CBehTreeDynamicNodeInstance::IsAvailable()
{
	if ( m_childNode )
	{
		return m_childNode->IsAvailable();
	}

	DebugNotifyAvailableFail();
	return false;
}

Int32 CBehTreeDynamicNodeInstance::Evaluate()
{
	if ( m_childNode )
	{
		return m_childNode->Evaluate();
	}

	DebugNotifyAvailableFail();
	return -1;
}

Bool CBehTreeDynamicNodeInstance::Interrupt()
{
	if ( m_childNode && !m_childNode->Interrupt() )
	{
		return false;
	}
	Super::Deactivate();
	return true;
}

Int32 CBehTreeDynamicNodeInstance::GetNumChildren() const
{
	return m_childNode ? 1 : 0;
}
Int32 CBehTreeDynamicNodeInstance::GetNumPersistantChildren() const
{
	return 0;
}
IBehTreeNodeInstance* CBehTreeDynamicNodeInstance::GetChild( Int32 index ) const
{
	AI_ASSERT( index == 0 );
	return m_childNode;
}
IBehTreeNodeInstance* CBehTreeDynamicNodeInstance::GetActiveChild() const
{
	if( m_childNode )
	{
		AI_ASSERT( m_childNode->IsActive() );
	}

	return m_childNode;
}
///////////////////////////////////////////////////////////////////////////////
// CBehTreeDynamicNodeDefinition
///////////////////////////////////////////////////////////////////////////////
CName CBehTreeDynamicNodeDefinition::GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return m_dynamicEventName;
}
IAITree* CBehTreeDynamicNodeDefinition::GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	IAITree* treeDef = m_baseTree.Get();
	// Override custom params?
	return context.GetVal< IAITree* >( m_baseTreeVar, treeDef );	
}
