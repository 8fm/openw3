/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeComposite.h"

#include "aiLog.h"
#include "behTreeLog.h"
#include "behTreeInstance.h"
#include "behTreeMachine.h"


////////////////////////////////////////////////////////////////////////
// IBehTreeNodeCompositeDefinition
////////////////////////////////////////////////////////////////////////

void IBehTreeNodeCompositeDefinition::CustomPostInstanceSpawn( IBehTreeNodeCompositeInstance* instance ) const
{

}

Bool IBehTreeNodeCompositeDefinition::IsTerminal() const
{
	return false;
}

IBehTreeNodeCompositeDefinition::eEditorNodeType IBehTreeNodeCompositeDefinition::GetEditorNodeType() const
{
	return NODETYPE_COMPOSITE;
}

Bool IBehTreeNodeCompositeDefinition::CanAddChild() const
{
	return m_children.Size() < MAX_CHILDREN_COUNT;
}
Int32 IBehTreeNodeCompositeDefinition::GetNumChildren() const
{
	return m_children.Size();
}

//! Get child
IBehTreeNodeDefinition* IBehTreeNodeCompositeDefinition::GetChild( Int32 index ) const
{
	return m_children[ index ];
}

//! Add child node
void IBehTreeNodeCompositeDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	AI_ASSERT( node->GetParent() == this );
	m_children.PushBack( node );
}

//! Remove child node
void IBehTreeNodeCompositeDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	m_children.Remove( node );
}

void IBehTreeNodeCompositeDefinition::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	RemoveNullChildren();
}

Bool IBehTreeNodeCompositeDefinition::IsValid() const
{
	if ( m_children.Empty() )
	{
		return false;
	}

	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		if ( (*it) == NULL )
		{
			return false;
		}
	}
	return TBaseClass::IsValid();
}
void IBehTreeNodeCompositeDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< IBehTreeNodeCompositeDefinition* >( this ) );
	for( Uint32 i=0; i<m_children.Size(); i++ )
	{
		m_children[i]->CollectNodes( nodes );
	}
}

Bool IBehTreeNodeCompositeDefinition::RemoveNullChildren()
{
	Bool modified = false;

	for( Int32 i = Int32(m_children.Size())-1; i >= 0; i-- )
	{
		if( !m_children[i] )
		{
			m_children.RemoveAt( i );
			modified = true;
		}
	}

	return modified;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool IBehTreeNodeCompositeDefinition::CorrectChildrenSpacing()
{
	const Int32 DELTA = 30;

	struct NodePosPred
	{
	public:
		Bool operator()( IBehTreeNodeDefinition* node1, IBehTreeNodeDefinition* node2) const
		{
			return ( node1->GetGraphPosX() < node2->GetGraphPosX() );
		}
	};

	Bool modified = false;

	TDynArray< IBehTreeNodeDefinition* > nodes( m_children );
	Sort( nodes.Begin(), nodes.End(), NodePosPred() ); 

	// For children with near X offset all next children
	for( Int32 i=0; i<Int32(nodes.Size())-1; i++ )
	{
		Int32 x = nodes[i]->GetGraphPosX();
		Int32 x2 = nodes[i+1]->GetGraphPosX();
		AI_ASSERT( x <= x2 );
		if( ( x2 - x ) < DELTA )
		{
			Int32 offset = DELTA - ( x2 - x );
			for( Uint32 j = i+1; j<m_children.Size(); j++ )
			{
				Int32 x = nodes[j]->GetGraphPosX();
				Int32 y = nodes[j]->GetGraphPosY();
				nodes[j]->SetGraphPosition( x + offset, y );
				modified = true;
			}
		}
	}

	return modified;
}

Bool IBehTreeNodeCompositeDefinition::CorrectChildrenPositions()
{
	Bool modified = RemoveNullChildren();
	modified = CorrectChildrenSpacing() || modified;

	// Correct children order
	for( Uint32 i=0; i<m_children.Size(); i++ )
	{		
		Int32 x = m_children[i]->GetGraphPosX();
		for( Uint32 j = i+1; j<m_children.Size(); j++ )
		{
			Int32 x2 = m_children[j]->GetGraphPosX();	

			// If wrong order swap positions
			if( x2 < x )
			{
				Int32 y = m_children[i]->GetGraphPosY();
				Int32 y2 = m_children[j]->GetGraphPosY();
				m_children[i]->SetGraphPosition( x2, y2 );				
				m_children[j]->SetGraphPosition( x, y );

				x = x2;
				modified = true;
			}
		}
	}

	// Correct recursively
	for( Uint32 i=0; i<m_children.Size(); i++ )
	{
		modified = m_children[i]->CorrectChildrenPositions() || modified;
	}

	return modified;
}

Bool IBehTreeNodeCompositeDefinition::CorrectChildrenOrder()
{
	Bool modified = RemoveNullChildren();
	modified = CorrectChildrenSpacing() || modified;

	// Correct children order
	for( Uint32 i=0; i<m_children.Size(); i++ )
	{		
		Int32 x = m_children[i]->GetGraphPosX();
		for( Uint32 j = i+1; j<m_children.Size(); j++ )
		{
			Int32 x2 = m_children[j]->GetGraphPosX();	

			// If wrong order swap order
			if( x2 < x )
			{
				m_children.Swap( i, j );
				x = x2;
				modified = true;
			}
		}
	}

	// Correct recursively
	for( Uint32 i=0; i<m_children.Size(); i++ )
	{
		modified = m_children[i]->CorrectChildrenOrder() || modified;
	}

	return modified;
}

void IBehTreeNodeCompositeDefinition::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	TBaseClass::OffsetNodesPosition( offsetX, offsetY );

	RemoveNullChildren();

	// Offset children
	for( Uint32 i=0; i<m_children.Size(); i++ )
	{
		m_children[i]->OffsetNodesPosition( offsetX, offsetY );
	}
}

#endif

IBehTreeNodeInstance* IBehTreeNodeCompositeDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	// spawn children
	Int32 currDynamicChild = 0;
	TStaticArray< IBehTreeNodeInstance*, MAX_CHILDREN_COUNT > children;
	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		(*it)->SpawnInstanceList( owner, context, NULL, children );
	}
	if ( children.Empty() )
	{
		return NULL;
	}

	Instance* instance = SpawnCompositeInstanceInternal( owner, context, parent );

	instance->m_children.Resize( children.Size() );

	for ( Uint32 i = 0, n = children.Size(); i < n; ++i )
	{
		instance->m_children[ i ] = children[ i ];

		children[ i ]->SetParent( instance );
	}

	CustomPostInstanceSpawn( instance );

	return instance;
}
Bool IBehTreeNodeCompositeDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if ( !IsMyInstance( node ) )
	{
		return false;
	}

	IBehTreeNodeCompositeInstance* composite = static_cast< IBehTreeNodeCompositeInstance* >( node );

	node->OnSpawn( *this, context );

	const auto& instanceArray = composite->GetChildren();

	Uint32 defChildCount = m_children.Size();
	Uint32 insChildCount = instanceArray.Size();
	Uint32 defChild = 0;
	Uint32 insChild = 0;
	while ( defChild < defChildCount && insChild < insChildCount )
	{
		IBehTreeNodeInstance* const* childIns = &instanceArray[ insChild ];
		IBehTreeNodeDefinition* childDef = m_children[ defChild ];

		++defChild;

		insChild += childDef->OnSpawnList( childIns, insChildCount-insChild , context );
	}

	return true;
}

////////////////////////////////////////////////////////////////////////
// IBehTreeNodeCompositeInstance
////////////////////////////////////////////////////////////////////////
IBehTreeNodeCompositeInstance::IBehTreeNodeCompositeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeInstance( def, owner, context, parent )
	, m_children()
	, m_activeChild( INVALID_CHILD )
{
	m_children.Reserve( def.m_children.Size() );
}
IBehTreeNodeCompositeInstance::~IBehTreeNodeCompositeInstance()
{
	for ( auto it = m_children.Begin(), end = m_children.End(); it != end; ++it )
	{
		delete *it;
	}
}

void IBehTreeNodeCompositeInstance::Update()
{
	if ( m_activeChild != INVALID_CHILD )
	{
		m_children[ m_activeChild ]->Update();
	}
}
void IBehTreeNodeCompositeInstance::Deactivate()
{
	if ( m_activeChild != INVALID_CHILD )
	{
		m_children[ m_activeChild ]->Deactivate();
		m_activeChild = INVALID_CHILD;
	}
	Super::Deactivate();
}
void IBehTreeNodeCompositeInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	m_activeChild = INVALID_CHILD;
	Super::OnSubgoalCompleted( outcome );
}
Bool IBehTreeNodeCompositeInstance::OnEvent( CBehTreeEvent& e )
{
	if ( m_activeChild != INVALID_CHILD )
	{
		return m_children[ m_activeChild ]->OnEvent( e );
	}
	return false;
}
Bool IBehTreeNodeCompositeInstance::Interrupt()
{
	if ( m_activeChild != INVALID_CHILD )
	{
		if ( !m_children[ m_activeChild ]->Interrupt() )
		{
			return false;
		}
		m_activeChild = INVALID_CHILD;
	}
	Deactivate();
	return true;
}

Int32 IBehTreeNodeCompositeInstance::GetNumChildren() const 
{ 
	return m_children.Size(); 
}

IBehTreeNodeInstance* IBehTreeNodeCompositeInstance::GetChild( Int32 index ) const
{ 
	AI_ASSERT( index >= 0 && index < GetNumChildren() )
	return m_children[ index ]; 
}
IBehTreeNodeInstance* IBehTreeNodeCompositeInstance::GetActiveChild() const
{
	return
		( m_activeChild != INVALID_CHILD ) ?
		m_children[ m_activeChild ] :
		NULL;
}
void IBehTreeNodeCompositeInstance::OnDestruction()
{
	for( Uint32 i = 0; i < m_children.Size(); ++i )
	{
		m_children[i]->OnDestruction();
	}

	Super::OnDestruction();
}
Bool IBehTreeNodeCompositeInstance::IsMoreImportantNodeActive( IBehTreeNodeInstance* askingChild )
{
	if( m_isActive )
	{
		for( Uint32 i=0; i<=m_activeChild; ++i )
		{
			if( m_children[ i ] == askingChild )
			{
				return false;
			}
		}
		return true;
	}
	else
	{
		return Super::IsMoreImportantNodeActive( askingChild );
	}	
}


#ifdef EDITOR_AI_DEBUG
Bool IBehTreeNodeCompositeInstance::DebugUpdate(Bool cascade)
{
	if( cascade )
	{
		for( Uint32 i = 0; i < m_children.Size(); ++i )
		{
			m_children[i]->DebugUpdate( cascade );
		}
	}

	return Super::DebugUpdate(false);
}
#endif