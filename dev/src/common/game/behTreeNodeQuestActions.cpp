#include "build.h"
#include "behTreeNodeQuestActions.h"

#include "actorLatentAction.h"
#include "questScriptedActionsAIParameters.h"

IMPLEMENT_ENGINE_CLASS( SBehTreeExternalListenerPtr )

////////////////////////////////////////////////////////////////////////
// CBehTreeExternalListener
////////////////////////////////////////////////////////////////////////
CBehTreeExternalListener::~CBehTreeExternalListener()
{
	Unregister();
}

// default implementations are empty
void CBehTreeExternalListener::OnBehaviorCompletion( Bool success )
{}
void CBehTreeExternalListener::OnBehaviorDestruction()
{}
void CBehTreeExternalListener::OnBehaviorEvent( CBehTreeEvent& e )
{}

void CBehTreeExternalListener::Unregister()
{
	if ( m_node )
	{
		m_node->m_listener = NULL;
		m_node = NULL;
	}
}
void CBehTreeExternalListener::Register( CBehTreeNodeExternalListenerInstance* node )
{
	Unregister();

	if ( node )
	{
		m_node = node;
		ASSERT( !node->m_listener );
		node->m_listener = this;
	}
}



////////////////////////////////////////////////////////////////////////
// CBehTreeNodeExternalListenerDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeExternalListenerDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeExternalListenerInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeExternalListenerInstance::CBehTreeNodeExternalListenerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_listener( NULL )
{
	SBehTreeExternalListenerPtr listener;
	if ( context.GetValRef< SBehTreeExternalListenerPtr >( CAIQuestScriptedActionsTree::ListenerParamName(), listener ) && listener.m_ptr )
	{
		listener.m_ptr->Register( this );
	}
}

Bool CBehTreeNodeExternalListenerInstance::OnEvent( CBehTreeEvent& e )
{
	if ( m_listener )
	{
		m_listener->OnBehaviorEvent( e );
	}

	return Super::OnEvent( e );
}

void CBehTreeNodeExternalListenerInstance::Complete( eTaskOutcome outcome )
{
	if ( m_listener )
	{
		m_listener->OnBehaviorCompletion( outcome == BTTO_SUCCESS );
	}

	Super::Complete( outcome );
}

void CBehTreeNodeExternalListenerInstance::OnDestruction()
{
	if ( m_listener )
	{
		m_listener->OnBehaviorDestruction();

		if ( m_listener )
		{
			m_listener->Unregister();
		}
	}
	
	Super::OnDestruction();
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeScriptedActionsListReaderDefinition
////////////////////////////////////////////////////////////////////////
void CBehTreeNodeScriptedActionsListReaderDefinition::SpawnInstanceList( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent, TStaticArray< IBehTreeNodeInstance*, 128 >& instanceList ) const
{
	TDynArray< IActorLatentAction* >* actionList = NULL;
	context.GetValRef< TDynArray< IActorLatentAction* >* >( m_actionListVar, actionList );

	if( !actionList )
	{
		return;
	}

	for ( auto it = actionList->Begin(), end = actionList->End(); it != end; ++it )
	{
		IActorLatentAction* action = *it;
		if ( action )
		{
			IBehTreeNodeInstance* instance = action->ComputeAction( owner, context, parent );
			if ( instance )
			{
				instanceList.PushBack( instance );
				// sanity check
				if ( instanceList.Size() >= instanceList.Capacity() )
				{
					return;
				}
			}
		}
	}
}
IBehTreeNodeInstance* CBehTreeNodeScriptedActionsListReaderDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return NULL;
}

Bool CBehTreeNodeScriptedActionsListReaderDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	return false;
}
//
//Bool CBehTreeNodeScriptedActionsListReaderDefinition::IsMyInstance( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
//{
//	// -> like SpawnInstance
//	return false;
//}