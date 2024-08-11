#include "build.h"
#include "behTreeNodeTemplate.h"

#include "../core/feedback.h"

#include "aiLog.h"
#include "aiParameters.h"
#include "behTreeInstance.h"
#include "behTree.h"
#include "behTreeNodeComposite.h"
#include "behTreeScriptedNode.h"
#include "behTreeNodeAtomicAction.h"


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeTemplateDefinition
////////////////////////////////////////////////////////////////////////
const String CBehTreeNodeTemplateDefinition::EMPTY_NODE_NAME( TXT("Template: EMPTY") );
const String CBehTreeNodeTemplateDefinition::CAPTION_PREFIX( TXT("Template: ") );

IBehTreeNodeInstance* CBehTreeNodeTemplateDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( !m_res )
	{
		return NULL;
	}

	IBehTreeNodeDefinition* childDef = m_res->GetRootNode();
	if ( childDef )
	{
		IAIParameters* params = m_aiParameters.Get();
		if( params )
		{
			if ( !context.Push( params ) )
			{
				return nullptr;
			}
		}
		IBehTreeNodeInstance* instance = childDef->SpawnInstance( owner, context, parent );
		if ( params )
		{
			context.Pop( params );
		}
		return instance;
	}
	
	return nullptr;
}
Bool CBehTreeNodeTemplateDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if ( !m_res )
	{
		return false;
	}

	IBehTreeNodeDefinition* childDef = m_res->GetRootNode();
	if ( childDef )
	{
		IAIParameters* aiParameters = m_aiParameters.Get();
		if( aiParameters )
		{
			if ( !context.Push( aiParameters ) )
			{
				return false;
			}
		}
		Bool b = childDef->OnSpawn( node, context );
		if ( aiParameters )
		{
			context.Pop( aiParameters );
		}
		return b;
	}

	return false;
}

String CBehTreeNodeTemplateDefinition::GetNodeCaption() const
{
	if ( m_res )
	{
		return CAPTION_PREFIX + m_res->GetFriendlyName();
	}
	return EMPTY_NODE_NAME;
}
Bool CBehTreeNodeTemplateDefinition::IsValid() const
{
	if ( !m_res || m_res->GetRootNode() == NULL )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
void CBehTreeNodeTemplateDefinition::OnPropertyPostChange( IProperty* property )
{

}
////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSubtreeDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeSubtreeDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	IAITree* treeDef = m_data;
	// Override custom params?
	treeDef = context.GetVal< IAITree* >( m_treeName, treeDef );	

	if( treeDef )
	{
		CBehTree* res = treeDef->GetTree();
		if( res )
		{
			IBehTreeNodeDefinition* def = res->GetRootNode();
			if ( def )
			{
				Int32 stack = 1;
				if ( !context.Push( treeDef ) )
				{
					return nullptr;
				}
				// Process params
				IBehTreeNodeInstance* instance = def->SpawnInstance( owner, context, parent );

				context.Pop( stack );

				return instance;
			}
		}
	}

	return nullptr;
}

Bool CBehTreeNodeSubtreeDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	IAITree* treeDef = m_data;
	// Override custom params?
	treeDef = context.GetVal< IAITree* >( m_treeName, treeDef );	

	if( treeDef )
	{
		CBehTree* res = treeDef->GetTree();
		if( res )
		{
			IBehTreeNodeDefinition* def = res->GetRootNode();
			if ( def )
			{
				Int32 stack = 1;
				context.Push( treeDef );

				Bool b = def->OnSpawn( node, context );

				context.Pop( stack );
				return b;
			}
		}
	}
	return false;
}

String CBehTreeNodeSubtreeDefinition::GetNodeCaption() const
{
	if ( !m_treeName.Empty() )
	{
		return String::Printf( TXT( "Subtree %s" ), m_treeName.AsChar() );
	}
	else if ( m_data )
	{
		return String::Printf( TXT( "Subtree %s" ), m_data->GetClass()->GetName().AsChar() );
	}
	return String( TXT( "Subtree" ) );
}

void CBehTreeNodeSubtreeDefinition::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSubtreeListDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeSubtreeListDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return NULL;
}
void CBehTreeNodeSubtreeListDefinition::SpawnInstanceList( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent, TStaticArray< IBehTreeNodeInstance*, 128 >& instanceList ) const
{
	TBehTreeValList treeList;
	context.GetValRef< TBehTreeValList >( m_listName, treeList );

	for( Uint32 i = 0; i < treeList.Size(); ++i )
	{
		CAITree* tree = treeList[i];
		if ( !tree )
		{
			continue;
		}
		CBehTree* res = tree->GetTree();
		if( !res )
		{
			continue;
		}
		IBehTreeNodeDefinition* def = res->GetRootNode();
		if ( !def )
		{
			continue;
		}
		// Process params
		Int32 paramsCount = 1;
		if ( !context.Push( tree ) )
		{
			continue;
		}
		IBehTreeNodeInstance* instance = def->SpawnInstance( owner, context, parent );
		if( instance )
		{
			instanceList.PushBack( instance );
		}
		context.Pop( paramsCount );
	}
}

Bool CBehTreeNodeSubtreeListDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	return false;
}

Uint32 CBehTreeNodeSubtreeListDefinition::OnSpawnList( IBehTreeNodeInstance* const* nodeList, Uint32 nodeCount, CBehTreeSpawnContext& context ) const
{
	Uint32 ret = 0;

	TBehTreeValList treeList;
	context.GetValRef< TBehTreeValList >( m_listName, treeList );

	for( Uint32 i = 0; i < treeList.Size(); ++i )
	{
		CAITree* tree = treeList[i];
		if ( !tree )
		{
			continue;
		}
		CBehTree* res = tree->GetTree();
		if( !res )
		{
			continue;
		}
		IBehTreeNodeDefinition* def = res->GetRootNode();
		if ( !def )
		{
			continue;
		}
		// Process params
		Int32 paramsCount = 1;
		if ( !context.Push( tree ) )
		{
			continue;
		}
		if ( def->OnSpawn( nodeList[ 0 ], context ) )
		{
			++nodeList;
			++ret;
		}
		context.Pop( paramsCount );
	}
	return ret;
}

String CBehTreeNodeSubtreeListDefinition::GetNodeCaption() const
{
	return String::Printf( TXT( "Subtree list %s" ), m_listName.AsString().AsChar() );
}


IBehTreeNodeInstance* CBehTreeNodeBaseConditionalTreeDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( CheckCondition( owner, context ) ^ m_invert && m_child )
	{
		return m_child->SpawnInstance( owner, context, parent );
	}
	return nullptr;
}

Bool CBehTreeNodeBaseConditionalTreeDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if( node && ( CheckCondition( node->GetOwner(), context ) ^ m_invert ) && m_child )
	{
		return m_child->OnSpawn( node, context );
	}
	return false;
}

Bool CBehTreeNodeBaseConditionalTreeDefinition::IsTerminal() const
{
	return false;
}
Bool CBehTreeNodeBaseConditionalTreeDefinition::CanAddChild() const
{
	return m_child == NULL;
}
Int32 CBehTreeNodeBaseConditionalTreeDefinition::GetNumChildren() const
{
	return m_child ? 1 : 0;
}
IBehTreeNodeDefinition* CBehTreeNodeBaseConditionalTreeDefinition::GetChild( Int32 index ) const
{
	AI_ASSERT( index == 0 );
	return m_child;
}
void CBehTreeNodeBaseConditionalTreeDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	AI_ASSERT( node->GetParent() == this );
	AI_ASSERT( !m_child );
	m_child = node;
}
void CBehTreeNodeBaseConditionalTreeDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	if ( m_child == node )
	{
		m_child = NULL;
	}
}
Bool CBehTreeNodeBaseConditionalTreeDefinition::IsValid() const
{
	if ( m_child == NULL )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
void CBehTreeNodeBaseConditionalTreeDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< CBehTreeNodeBaseConditionalTreeDefinition* >( this ) );
	if ( m_child )
	{
		m_child->CollectNodes( nodes );
	}
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehTreeNodeBaseConditionalTreeDefinition::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	TBaseClass::OffsetNodesPosition( offsetX, offsetY );

	if ( m_child )
		m_child->OffsetNodesPosition( offsetX, offsetY );
}
#endif // NO_EDITOR_GRAPH_SUPPORT

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionalTreeDefinition
////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodeConditionalTreeDefinition::CheckCondition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return  m_val.GetVal( context );
}

String CBehTreeNodeConditionalTreeDefinition::GetNodeCaption() const
{
	if ( m_val.m_varName.Empty() )
		return TXT("if ( UNSET )");

	return String::Printf( TXT("if ( %s%s )"), (m_invert ? TXT("^") : TXT("")),m_val.m_varName.AsString().AsChar() );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionalFlagTreeDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeConditionalFlagTreeDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_val.GetVal( context ) & (1 << m_flag) )
	{
		return m_child->SpawnInstance( owner, context, parent );
	}
	return nullptr;
}
Bool CBehTreeNodeConditionalFlagTreeDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if ( m_val.GetVal( context ) & (1 << m_flag) )
	{
		return m_child->OnSpawn( node, context );
	}
	return false;
}

String CBehTreeNodeConditionalFlagTreeDefinition::GetNodeCaption() const
{
	if ( m_val.m_varName.Empty() )
	{
		return TXT("if ( UNSET )");
	}

	return String::Printf( TXT("if ( %s has flag %d )"), m_val.m_varName.AsString().AsChar(), m_flag );
}
Bool CBehTreeNodeConditionalFlagTreeDefinition::IsTerminal() const
{
	return false;
}
Bool CBehTreeNodeConditionalFlagTreeDefinition::CanAddChild() const
{
	return m_child == NULL;
}
Int32 CBehTreeNodeConditionalFlagTreeDefinition::GetNumChildren() const
{
	return m_child ? 1 : 0;
}
IBehTreeNodeDefinition* CBehTreeNodeConditionalFlagTreeDefinition::GetChild( Int32 index ) const
{
	AI_ASSERT( index == 0 );
	return m_child;
}
void CBehTreeNodeConditionalFlagTreeDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	AI_ASSERT( node->GetParent() == this );
	AI_ASSERT( !m_child );
	m_child = node;
}
void CBehTreeNodeConditionalFlagTreeDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	if ( m_child == node )
	{
		m_child = NULL;
	}
}
Bool CBehTreeNodeConditionalFlagTreeDefinition::IsValid() const
{
	if ( m_child == NULL )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
void CBehTreeNodeConditionalFlagTreeDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< CBehTreeNodeConditionalFlagTreeDefinition* >( this ) );
	if ( m_child )
	{
		m_child->CollectNodes( nodes );
	}
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehTreeNodeConditionalFlagTreeDefinition::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	TBaseClass::OffsetNodesPosition( offsetX, offsetY );

	if ( m_child )
		m_child->OffsetNodesPosition( offsetX, offsetY );
}
#endif // NO_EDITOR_GRAPH_SUPPORT



////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionalChooseBranchTreeDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeConditionalChooseBranchDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_val.GetVal( context ) )
	{
		if ( m_child1 )
		{
			return m_child1->SpawnInstance( owner, context, parent );
		}
	}
	else
	{
		if ( m_child2 )
		{
			return m_child2->SpawnInstance( owner, context, parent );
		}
	}
	return NULL;
}

Bool CBehTreeNodeConditionalChooseBranchDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if ( m_val.GetVal( context ) )
	{
		if ( m_child1 )
		{
			return m_child1->OnSpawn( node, context );
		}
	}
	else
	{
		if ( m_child2 )
		{
			return m_child2->OnSpawn( node, context );
		}
	}
	return false;
}

Bool CBehTreeNodeConditionalChooseBranchDefinition::IsTerminal() const
{
	return false;
}
Bool CBehTreeNodeConditionalChooseBranchDefinition::CanAddChild() const
{
	return !m_child2 || !m_child1;
}
Int32 CBehTreeNodeConditionalChooseBranchDefinition::GetNumChildren() const
{
	if ( m_child2 )
		return 2;
	if ( m_child1 )
		return 1;
	return 0;
}
IBehTreeNodeDefinition* CBehTreeNodeConditionalChooseBranchDefinition::GetChild( Int32 index ) const
{
	if ( index == 1 )
		return m_child2;
	else if ( index == 0 )
		return m_child1;
	return NULL;
}
void CBehTreeNodeConditionalChooseBranchDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	AI_ASSERT( node->GetParent() == this );
	AI_ASSERT( m_child2 == NULL );

	if ( !m_child1 )
	{
		m_child1 = node;
	}
	else
	{
		m_child2 = node;
	}
}
void CBehTreeNodeConditionalChooseBranchDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	if ( node == m_child2 )
	{
		m_child2 = NULL;
	}
	else if ( node == m_child1 )
	{
		m_child1 = m_child2;
	}
}
Bool CBehTreeNodeConditionalChooseBranchDefinition::IsValid() const
{
	if ( !m_child1 || !m_child2 )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
void CBehTreeNodeConditionalChooseBranchDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< CBehTreeNodeConditionalChooseBranchDefinition* >( this ) );

	if ( m_child1 )
		m_child1->CollectNodes( nodes );

	if ( m_child2 )
		m_child2->CollectNodes( nodes );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehTreeNodeConditionalChooseBranchDefinition::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	TBaseClass::OffsetNodesPosition( offsetX, offsetY );

	if ( m_child1 )
		m_child1->OffsetNodesPosition( offsetX, offsetY );

	if ( m_child2 )
		m_child2->OffsetNodesPosition( offsetX, offsetY );
}
Bool CBehTreeNodeConditionalChooseBranchDefinition::CorrectChildrenOrder()
{
	Bool b = false;
	if ( m_child1 && m_child2 && m_child2->GetGraphPosX() < m_child1->GetGraphPosX() )
	{
		Swap( m_child1, m_child2 );
		b = true;
	}
	return TBaseClass::CorrectChildrenOrder() || b;
}
#endif // NO_EDITOR_GRAPH_SUPPORT

String CBehTreeNodeConditionalChooseBranchDefinition::GetNodeCaption() const
{
	if ( m_val.m_varName.Empty() )
		return TXT("if ( UNSET )");

	return String::Printf( TXT("if ( %s ) LEFT else RIGHT"), m_val.m_varName.AsString().AsChar() );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionalBaseNodeDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* IBehTreeNodeConditionalBaseNodeDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_child == nullptr )
	{
		return nullptr;
	}
	if ( CheckCondition( context ) ^ m_invertCondition )
	{
		return m_child->SpawnInstance( owner, context, parent );
	}
	else
	{
		IBehTreeNodeDefinition * child = m_child;
		for ( Uint32 i = 0; i < m_childNodeToDisableCount; ++i )
		{
			if ( child->GetNumChildren() != 1  )
			{
				GFeedback->ShowError(TXT("CBehTreeNodeConditionalNodeDefinition disabled: the node you want to disable has more than one child! Node number: %i name : %s"), i + 1, child->GetClass()->GetName().AsString().AsChar() );
				return nullptr;
			}
			child = child->GetChild(0);
			if ( !child )
			{
				return nullptr;
			}
		}
		return child->SpawnInstance( owner, context, parent );
	}
}

Bool IBehTreeNodeConditionalBaseNodeDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if ( m_child == nullptr )
	{
		return false;
	}
	if ( CheckCondition( context ) ^ m_invertCondition )
	{
		return m_child->OnSpawn( node, context );
	}
	else
	{
		IBehTreeNodeDefinition * child = m_child;
		for ( Uint32 i = 0; i < m_childNodeToDisableCount; ++i )
		{
			if ( child->GetNumChildren() != 1  )
			{
				return false;
			}
			child = child->GetChild(0);
			if ( !child )
			{
				return false;
			}
		}
		return child->OnSpawn( node, context );
	}
}
String IBehTreeNodeConditionalBaseNodeDefinition::GetNodeCaption() const
{
	return String::Printf( TXT("Decorate with %d node%s"), m_childNodeToDisableCount, m_childNodeToDisableCount != 1 ? TXT("s") : TXT("") );
}
Bool IBehTreeNodeConditionalBaseNodeDefinition::IsTerminal() const
{
	return false;
}
Bool IBehTreeNodeConditionalBaseNodeDefinition::CanAddChild() const
{
	return m_child == NULL;
}
Int32 IBehTreeNodeConditionalBaseNodeDefinition::GetNumChildren() const
{
	return m_child ? 1 : 0;
}
IBehTreeNodeDefinition* IBehTreeNodeConditionalBaseNodeDefinition::GetChild( Int32 index ) const
{
	AI_ASSERT( index == 0 );
	return m_child;
}
void IBehTreeNodeConditionalBaseNodeDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	AI_ASSERT( node->GetParent() == this );
	AI_ASSERT( !m_child );
	m_child = node;
}
void IBehTreeNodeConditionalBaseNodeDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	if ( m_child == node )
	{
		m_child = NULL;
	}
}
Bool IBehTreeNodeConditionalBaseNodeDefinition::IsValid() const
{
	if ( m_child == NULL )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
void IBehTreeNodeConditionalBaseNodeDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< IBehTreeNodeConditionalBaseNodeDefinition* >( this ) );
	if ( m_child )
	{
		m_child->CollectNodes( nodes );
	}
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
void IBehTreeNodeConditionalBaseNodeDefinition::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	TBaseClass::OffsetNodesPosition( offsetX, offsetY );

	if ( m_child )
		m_child->OffsetNodesPosition( offsetX, offsetY );
}
#endif // NO_EDITOR_GRAPH_SUPPORT

Bool CBehTreeNodeConditionalNodeDefinition::CheckCondition( CBehTreeSpawnContext& context ) const
{
	return m_val.GetVal( context );
}

String CBehTreeNodeConditionalNodeDefinition::GetNodeCaption() const
{
	return String::Printf( TXT("Decorate with %d node%s if ( %s%s )")
		, m_childNodeToDisableCount
		, m_childNodeToDisableCount != 1 ? TXT("s") : TXT("")
		, m_invertCondition ? TXT( "!" ) : TXT( "" )
		, m_val.m_varName.Empty() ? TXT( "UNSET" ) : m_val.m_varName.AsChar() );
}

Bool CBehTreeNodeConditionalNameNodeDefinition::CheckCondition( CBehTreeSpawnContext& context ) const
{
	CName value;
	m_val.GetValRef( context, value );

	CName nameToCompare;
	m_nameToCompare.GetValRef( context, nameToCompare );
	
	return value == nameToCompare;
}

String CBehTreeNodeConditionalNameNodeDefinition::GetNodeCaption() const
{
	return String::Printf( TXT("Decorate with %d node%s if ( %s %s %s )")
		, m_childNodeToDisableCount
		, m_childNodeToDisableCount != 1 ? TXT("s") : TXT("")
		, m_invertCondition ? TXT("!=") : TXT("==")
		, m_val.m_varName.Empty() ? TXT( "UNSET" ) : m_val.m_varName.AsChar(), m_nameToCompare.m_value.AsChar() );
}