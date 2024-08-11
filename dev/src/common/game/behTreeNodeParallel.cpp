#include "build.h"
#include "behTreeNodeParallel.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeParallelDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeParallelDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( !m_child1 || !m_child2 )
	{
		return nullptr;
	}
	IBehTreeNodeInstance* child1 = m_child1->SpawnInstance( owner, context );
	IBehTreeNodeInstance* child2 = m_child2->SpawnInstance( owner, context );
	if ( !child1 && !child2 )
	{
		return nullptr;
	}
	else if ( !child1 || !child2 )
	{
		if ( !child1 )
		{
			child2->SetParent( parent );
			return child2;
		}
		else
		{
			child1->SetParent( parent );
			return child1;
		}
	}
	Instance* me = new Instance( *this, owner, context, parent );
	me->m_child1 = child1;
	me->m_child2 = child2;
	child1->SetParent( me );
	child2->SetParent( me );
	return me;
}

Bool CBehTreeNodeParallelDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if ( !IsMyInstance( node ) )
	{
		// handle case if we didn't spawn ourself because we had just one kid
		if ( m_child1 && m_child1->OnSpawn( node, context ) )
		{
			return true;
		}
		if ( m_child2 && m_child2->OnSpawn( node, context ) )
		{
			return true;
		}
		return false;
	}

	Instance* parallel = static_cast< Instance* >( node );

	m_child1->OnSpawn( parallel->GetParallelChild( 0 ), context );
	m_child2->OnSpawn( parallel->GetParallelChild( 1 ), context );

	return true;
}

Bool CBehTreeNodeParallelDefinition::IsTerminal() const
{
	return false;
}
Bool CBehTreeNodeParallelDefinition::IsValid() const
{
	if ( !m_child1 || !m_child2 )
	{
		return false;
	}
	return TBaseClass::IsValid();
}

Bool CBehTreeNodeParallelDefinition::CanAddChild() const
{
	return !m_child1 || !m_child2;
}
void CBehTreeNodeParallelDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	if ( node == m_child2 )
	{
		m_child2 = NULL;
	}
	else if ( node == m_child1 )
	{
		m_child1 = m_child2;
		m_child2 = NULL;
	}
}
Int32 CBehTreeNodeParallelDefinition::GetNumChildren() const
{
	return m_child2 ? 2 : m_child1 ? 1 : 0;
}
IBehTreeNodeDefinition* CBehTreeNodeParallelDefinition::GetChild( Int32 index ) const
{
	ASSERT( index < 2 );
	return index == 1 ? m_child2 : m_child1;
}
void CBehTreeNodeParallelDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	ASSERT( node->GetParent() == this );
	ASSERT( m_child2 == NULL );
	if ( m_child1 )
	{
		m_child2 = node;
	}
	else
	{
		m_child1 = node;
	}
}

void CBehTreeNodeParallelDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< CBehTreeNodeParallelDefinition* >( this ) );
	if ( m_child1 )
	{
		m_child1->CollectNodes( nodes );
		if ( m_child2 )
		{
			m_child2->CollectNodes( nodes );
		}
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
Bool CBehTreeNodeParallelDefinition::CorrectChildrenOrder()
{
	Bool b = false;
	if ( m_child1 && m_child2 && m_child2->GetGraphPosX() < m_child1->GetGraphPosX() )
	{
		Swap( m_child1, m_child2 );
		b = true;
	}

	return TBaseClass::CorrectChildrenOrder() || b;
}
#endif


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeParallelInstance
////////////////////////////////////////////////////////////////////////

CBehTreeNodeParallelInstance::~CBehTreeNodeParallelInstance()
{
	if ( m_child1 )
	{
		delete m_child1;
	}
	if ( m_child2 )
	{
		delete m_child2;
	}
}

void CBehTreeNodeParallelInstance::Update()
{
	AI_ASSERT( m_isActive );
	m_child1->Update();
	if ( m_isActive )
	{
		m_child2->Update();
	}
}
Bool CBehTreeNodeParallelInstance::Activate()
{
	if ( !m_child1->Activate() )
	{
		DebugNotifyActivationFail();
		return false;
	}
	if ( !m_child2->Activate() )
	{
		DebugNotifyActivationFail();
		m_child1->Deactivate();
		return false;
	}
	return Super::Activate();
}
void CBehTreeNodeParallelInstance::Deactivate()
{
	if ( m_child1->IsActive() )
	{
		m_child1->Deactivate();
	}
	if ( m_child2->IsActive() )
	{
		m_child2->Deactivate();
	}
	Super::Deactivate();
}
void CBehTreeNodeParallelInstance::Complete( eTaskOutcome outcome )
{
	Super::Complete( outcome );
}
void CBehTreeNodeParallelInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	Complete( outcome );
}

Bool CBehTreeNodeParallelInstance::OnEvent( CBehTreeEvent& e )
{
	Bool b1 = m_child1->OnEvent( e );
	Bool b2 = m_child2->OnEvent( e );
	return b1 || b2;
}

Bool CBehTreeNodeParallelInstance::Interrupt()
{
	if ( !m_child1->Interrupt() )
	{
		return false;
	}
	m_child2->Deactivate();
	Deactivate();
	return true;
}
void CBehTreeNodeParallelInstance::OnDestruction()
{
	m_child1->OnDestruction();
	m_child2->OnDestruction();

	Super::OnDestruction();
}


Int32 CBehTreeNodeParallelInstance::GetNumChildren() const
{
	return 2;
}
IBehTreeNodeInstance* CBehTreeNodeParallelInstance::GetChild( Int32 index ) const
{
	return index == 0 ? m_child1 : m_child2;
}

Uint32 CBehTreeNodeParallelInstance::GetActiveChildCount() const
{
	return 2;
}
IBehTreeNodeInstance* CBehTreeNodeParallelInstance::GetActiveChild( Uint32 activeChild ) const
{
	return activeChild == 0 ? m_child1 : m_child2;
}
