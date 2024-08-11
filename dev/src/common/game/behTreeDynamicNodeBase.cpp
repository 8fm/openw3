/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeDynamicNodeBase.h"

#include "aiLog.h"
#include "behTree.h"
#include "behTreeInstance.h"
#include "../core/feedback.h"

IBehTreeDynamicNodeBase::~IBehTreeDynamicNodeBase()
{
	if ( m_childNode )
	{
		delete m_childNode;
	}
}

Bool IBehTreeDynamicNodeBase::SpawnChildNodeWithContext( IAITree* treeDef, CBehTreeSpawnContext& context, IBehTreeNodeInstance* node, CBehTreeInstance* owner )
{
	CBehTree* res = treeDef->GetTree();
	if ( res == NULL )
	{
		GFeedback->ShowWarn( TXT("AI problem! Tree definition of class '%s' has no beh tree resource specified. aiTreeName='%s'"), treeDef->GetClass()->GetName().AsString().AsChar(), treeDef->GetAITreeName().AsChar() );
		return false;
	}

	IBehTreeNodeDefinition* rootNode = res->GetRootNode();
	if ( rootNode == NULL )
	{
		return false;
	}

	// spawn new child node
	Uint32 params = 1;
	if ( !context.Push( treeDef ) )
	{
		return false;
	}
	Bool wasDynamic = context.MarkAsDynamicBranch( true );
	m_childNode = rootNode->SpawnInstance( owner, context, node );
	context.MarkAsDynamicBranch( wasDynamic );
	context.Pop( params );

	return m_childNode != NULL;
}

Bool IBehTreeDynamicNodeBase::SpawnChildNode( IAITree* treeDef, const SBehTreeDynamicNodeEventData::Parameters& params, IBehTreeNodeInstance* node, CBehTreeInstance* owner )
{
	CBehTree* res = treeDef->GetTree();
	AI_ASSERT( res, TXT(" You forgot to specify a tree !! ") );
	if ( res == NULL )
	{
		return false;
	}

	IBehTreeNodeDefinition* rootNode = res->GetRootNode();
	if ( rootNode == NULL )
	{
		return false;
	}
	// spawn new child node
	CBehTreeSpawnContext context( true );
	context.Push( treeDef );

	for( auto it = params.Begin(); it != params.End(); ++it )
	{
		if ( !context.Push( *it ) )
		{
			return false;
		}
	}

	m_childNode = rootNode->SpawnInstance( owner, context, node );
	if ( !m_childNode )
	{
		return false;
	}

	rootNode->OnSpawn( m_childNode, context );
	owner->BindEventListeners( context );
	return true;
}

void IBehTreeDynamicNodeBase::DespawnChildNode()
{
	if ( m_childNode )
	{
		if ( m_childNode->IsActive() )
		{
			m_childNode->Deactivate();
		}
		m_childNode->OnDestruction();
		delete m_childNode;
		m_childNode = NULL;
	}
}


void IBehTreeDynamicNodeBase::Update()
{
	if ( m_childNode )
	{
		m_childNode->Update();
	}
}
Bool IBehTreeDynamicNodeBase::Activate()
{
	if( m_childNode )
	{
		return m_childNode->Activate();
	}

	return false;
}
void IBehTreeDynamicNodeBase::Deactivate()
{
	if ( m_childNode && m_childNode->IsActive() )
	{
		m_childNode->Deactivate();
	}
}
void IBehTreeDynamicNodeBase::OnSubgoalCompleted( IBehTreeNodeInstance* node, IBehTreeNodeInstance::eTaskOutcome outcome )
{
	node->Complete( outcome );
}

Bool IBehTreeDynamicNodeBase::OnEvent( CBehTreeEvent& e )
{
	if ( m_childNode )
	{
		return m_childNode->OnEvent( e );
	}
	return false;
}

Bool IBehTreeDynamicNodeBase::IsAvailable()
{
	if ( m_childNode )
	{
		return m_childNode->IsAvailable();
	}
	return false;
}
Int32 IBehTreeDynamicNodeBase::Evaluate()
{
	if ( m_childNode )
	{
		return m_childNode->Evaluate();
	}
	return -1;
}

Bool IBehTreeDynamicNodeBase::Interrupt() const
{
	if ( m_childNode )
	{
		return m_childNode->Interrupt();
	}
	return true;
}

Int32 IBehTreeDynamicNodeBase::GetNumChildren() const
{
	return m_childNode ? 1 : 0;
}
IBehTreeNodeInstance* IBehTreeDynamicNodeBase::GetChild( Int32 index ) const
{
	return m_childNode;
}
IBehTreeNodeInstance* IBehTreeDynamicNodeBase::GetActiveChild() const
{
	if ( m_childNode && m_childNode->IsActive() )
	{
		return m_childNode;
	}
	else
	{
		return NULL;
	}
}