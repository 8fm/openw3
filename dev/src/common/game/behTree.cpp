/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTree.h"

#include "behTreeLog.h"
#include "behTreeMachine.h"
#include "behTreeMachineListener.h"
#include "behTreeInstance.h"
#include "behTreeNodeComposite.h"
#include "behTreeNodeTemplate.h"

#include "../core/factory.h"

class CBehTreeFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CBehTreeFactory, IFactory, 0 );

public:
	CBehTreeFactory()
	{
		m_resourceClass = ClassID< CBehTree >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options )
	{
		return ::CreateObject< CBehTree >( options.m_parentObject );		
	}
};

BEGIN_CLASS_RTTI( CBehTreeFactory )
	PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CBehTreeFactory );

////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CBehTree );

void CBehTree::SetRootNode( IBehTreeNodeDefinition * rootNode )
{
	m_rootNode = rootNode;
	if ( m_rootNode )
	{
		m_rootNode->SetParent( this );
	}
}

#ifdef EDITOR_AI_DEBUG
void CBehTree::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	for( Int32 i = m_nodes.Size()-1; i >= 0; --i )
	{
		if ( !m_nodes[ i ] )
		{
			m_nodes.RemoveAtFast( i );
		}
	}

	for( Uint32 i = 0; i < m_nodes.Size(); ++i )
	{
		m_nodeMap.Insert( m_nodes[i]->GetUniqueId(), m_nodes[i] );
	}
}
#endif


#ifdef BEHREE_AUTOFIX_TREE_PARENTAGE
void CBehTree::FixTreeParentage( IBehTreeNodeDefinition* node, CObject* parentObject )
{
	// HACK! We encountered a data, that had parentage broken. With this code, we can auto-fix them, and make sure that problem won't come back.
	if ( node->GetParent() != parentObject )
	{
		node->SetParent( parentObject );
	}
	for( Int32 i = 0; i < node->GetNumChildren(); ++i )
	{
		IBehTreeNodeDefinition* child = node->GetChild( i );
		if ( child )
		{
			FixTreeParentage( child, node );
		}
	}
}

void CBehTree::FixTreeParentage()
{
	if ( m_rootNode )
	{
		FixTreeParentage( m_rootNode, this );
	}
}

#endif

void CBehTree::OnSave()
{
	if ( m_rootNode )
	{
#ifdef EDITOR_AI_DEBUG
		m_nodeMap.ClearFast();
		m_nodes.ClearFast();
#endif
#if 0
#ifdef BEHREE_AUTOFIX_TREE_PARENTAGE
		FixTreeParentage( m_rootNode, this );
#endif
#endif

		// Iterate over tree elements and add them to hash save list
		OnNodeSave( m_rootNode );
	}
}

void CBehTree::OnNodeSave( IBehTreeNodeDefinition* node )
{	
#ifdef EDITOR_AI_DEBUG
	// Always generate new hash, to avoid problems with repeated hashes from copy-pasting trees
	m_nodes.PushBack( node );
	m_nodeMap.Insert( node->GetUniqueId(), node );	
#endif

	for( Int32 i = 0; i < node->GetNumChildren(); ++i )
	{
		IBehTreeNodeDefinition* child = node->GetChild( i );
		if ( child )
		{
			OnNodeSave( child );
		}
	}
}



#ifdef EDITOR_AI_DEBUG
IBehTreeNodeDefinition* CBehTree::GetNodeByHash( Int32 hash ) const
{
	THashMap< Int32, IBehTreeNodeDefinition* >::const_iterator itr = m_nodeMap.Find( hash );
	if( itr != m_nodeMap.End() )
	{
		return itr->m_second;
	}

	return NULL;
}
#endif


/////////////////////////////////////////////////////////////////////////////////
#ifdef EDITOR_AI_DEBUG
IBehTreeDebugInterface* GBehTreeDebugInterface = NULL;
#endif	//EDITOR_AI_DEBUG

static void funcDebugBehTreeStart( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( stack );
	RED_UNUSED( result );

	GET_PARAMETER_OPT( THandle< CActor >, actor, NULL );
	FINISH_PARAMETERS;

#ifdef EDITOR_AI_DEBUG
	if( GBehTreeDebugInterface )
	{
		CActor* obj = actor.Get();
		if ( obj )
		{
			CBehTreeMachine* machine = obj->GetBehTreeMachine();
			if ( machine )
			{
				GBehTreeDebugInterface->DebugBehTreeStart( machine );
			}
		}
	}
#endif	//EDITOR_AI_DEBUG
}

static void funcDebugBehTreeStopAll( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( stack );
	RED_UNUSED( result );
	FINISH_PARAMETERS;
#ifdef EDITOR_AI_DEBUG
	if( GBehTreeDebugInterface )
	{
		GBehTreeDebugInterface->DebugBehTreeStopAll();
	}
#endif // EDITOR_AI_DEBUG
}

void RegisterBehTreeDebugGlobalFunctions()
{	
	NATIVE_GLOBAL_FUNCTION("DebugBehTreeStart", funcDebugBehTreeStart);
	NATIVE_GLOBAL_FUNCTION("DebugBehTreeStopAll", funcDebugBehTreeStopAll);
}
