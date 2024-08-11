#include "build.h"

#include "aiLog.h"
#include "behTreeDecorator.h"


////////////////////////////////////////////////////////////////////////
// IBehTreeNodeDecoratorDefinition
////////////////////////////////////////////////////////////////////////


Bool IBehTreeNodeDecoratorDefinition::IsTerminal() const
{
	return false;
}
Bool IBehTreeNodeDecoratorDefinition::CanAddChild() const
{
	return m_child == NULL;
}

Int32 IBehTreeNodeDecoratorDefinition::GetNumChildren() const
{
	return m_child ? 1 : 0;
}

//! Get child
IBehTreeNodeDefinition* IBehTreeNodeDecoratorDefinition::GetChild( Int32 index ) const
{
	AI_ASSERT( index == 0 );
	return m_child;
}

//! Add child node
void IBehTreeNodeDecoratorDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	AI_ASSERT( node->GetParent() == this );
	AI_ASSERT( m_child == NULL );
	m_child = node;
}

//! Remove child node
void IBehTreeNodeDecoratorDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	if ( m_child == node )
	{
		m_child = NULL;
	}
}
Bool IBehTreeNodeDecoratorDefinition::IsValid() const
{
	if ( !m_child )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
IBehTreeNodeDecoratorDefinition::eEditorNodeType IBehTreeNodeDecoratorDefinition::GetEditorNodeType() const
{
	return NODETYPE_DECORATOR;
}
void IBehTreeNodeDecoratorDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< IBehTreeNodeDecoratorDefinition* >( this ) );
	if ( m_child )
	{
		m_child->CollectNodes( nodes );
	}
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
void IBehTreeNodeDecoratorDefinition::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	TBaseClass::OffsetNodesPosition( offsetX, offsetY );

	if ( m_child )
		m_child->OffsetNodesPosition( offsetX, offsetY );
}
#endif
IBehTreeNodeInstance* IBehTreeNodeDecoratorDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( !m_child )
		return NULL;

	IBehTreeNodeInstance* child = m_child->SpawnInstance( owner, context );
	if ( !child )
		return NULL;

	Instance* instance = SpawnDecoratorInternal( owner, context, parent );
	child->SetParent( instance );
	instance->SetDecoratorChild( child );
	return instance;
}
Bool IBehTreeNodeDecoratorDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if ( !IsMyInstance( node ) )
	{
		return false;
	}

	IBehTreeNodeDecoratorInstance* decorator = static_cast< IBehTreeNodeDecoratorInstance* >( node );

	decorator->OnSpawn( *this, context );

	if ( !m_child )
	{
		return false;
	}

	Bool b = m_child->OnSpawn( decorator->GetDecoratorChild(), context );
	AI_ASSERT( b, TXT("AI consistency in OnSpawn() failed!") );

	return true;
}

////////////////////////////////////////////////////////////////////////
// IBehTreeNodeDecoratorInstance
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance::IBehTreeNodeDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_child( NULL )
{}

IBehTreeNodeDecoratorInstance::~IBehTreeNodeDecoratorInstance()
{
	if ( m_child )
	{
		delete m_child;
	}
}

void IBehTreeNodeDecoratorInstance::Update()
{
	AI_ASSERT( m_child->IsActive() );
	m_child->Update();
}
Bool IBehTreeNodeDecoratorInstance::Activate()
{
	if ( m_child->Activate() )
	{
		return Super::Activate();
	}

	DebugNotifyActivationFail();
	return false;
}
void IBehTreeNodeDecoratorInstance::Deactivate()
{
	if( m_child->IsActive() )
	{
		m_child->Deactivate();
	}
	Super::Deactivate();
}
Bool IBehTreeNodeDecoratorInstance::OnEvent( CBehTreeEvent& e )
{
	return m_child->OnEvent( e );
}
Bool IBehTreeNodeDecoratorInstance::IsAvailable()
{
#ifdef EDITOR_AI_DEBUG
	ForceSetDebugColor( 50, 100, 0 );
#endif

	return m_child->IsAvailable();
}
Int32 IBehTreeNodeDecoratorInstance::Evaluate()
{
#ifdef EDITOR_AI_DEBUG
	ForceSetDebugColor( 50, 100, 0 );
#endif

	return m_child->Evaluate();
}

Bool IBehTreeNodeDecoratorInstance::Interrupt()
{
	if ( !m_child->Interrupt() )
	{
		return false;
	}

	Deactivate();
	return true;
}
Int32 IBehTreeNodeDecoratorInstance::GetNumChildren() const
{
	return m_child ? 1 : 0;
}
IBehTreeNodeInstance* IBehTreeNodeDecoratorInstance::GetChild( Int32 index ) const
{
	AI_ASSERT( index == 0 );
	return m_child;
}
IBehTreeNodeInstance* IBehTreeNodeDecoratorInstance::GetActiveChild() const
{
	if( m_child )
	{
		AI_ASSERT( m_child->IsActive() );
	}
	return m_child;
}
void IBehTreeNodeDecoratorInstance::OnDestruction()
{
	m_child->OnDestruction();

	Super::OnDestruction();
}
#ifdef EDITOR_AI_DEBUG
Bool IBehTreeNodeDecoratorInstance::DebugUpdate(Bool cascade)
{
	if( cascade && m_child )
	{
		m_child->DebugUpdate( cascade );
	}

	return Super::DebugUpdate(false);
}
#endif