/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNode.h"

#include "aiLog.h"
#include "behTreeLog.h"
#include "behTreeInstance.h"
#include "behTreeMachine.h"
#include "behTreeMachineListener.h"
#include "behTreeScriptedNode.h"
#include "behTreeNodeAtomicAction.h"
#include "aiLog.h"
#include "behTreeVarsUtils.h"

IMPLEMENT_RTTI_ENUM( EBTNodeStatus );
IMPLEMENT_ENGINE_CLASS( SBTNodeResult );

Red::Threads::CAtomic< Uint32 > IBehTreeNodeDefinition::s_globalId( 0 );

////////////////////////////////////////////////////////////////////////
String IBehTreeNodeDefinition::GetNodeCaption() const
{
	return GetNodeName().AsString();
}
Bool IBehTreeNodeDefinition::IsTerminal() const
{
	return true;
}
Bool IBehTreeNodeDefinition::CanAddChild() const
{
	return false;
}

void IBehTreeNodeDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
	AI_ASSERT( false );
}
Int32 IBehTreeNodeDefinition::GetNumChildren() const
{
	return 0;
}

IBehTreeNodeDefinition* IBehTreeNodeDefinition::GetChild( Int32 index ) const
{
	AI_ASSERT( false );
	return NULL;
}

void IBehTreeNodeDefinition::AddChild( IBehTreeNodeDefinition* node )
{
	AI_ASSERT( false );
}
Bool IBehTreeNodeDefinition::IsValid() const
{
	for ( Int32 i = 0, n = GetNumChildren(); i < n; ++i )
	{
		IBehTreeNodeDefinition* child = GetChild( i );
		if ( child )
		{
			if ( !child->IsValid() )
			{
				return false;
			}
			if ( child->GetParent() != this )
			{
				HALT( "AI resource node parentage is broken. Internal resource integrity error. Plz inform programming team." );
				return false;
			}
		}
	}
	return true;
}
IBehTreeNodeDefinition::eEditorNodeType IBehTreeNodeDefinition::GetEditorNodeType() const
{
	return NODETYPE_DEFAULT;
}
IBehTreeTaskDefinition* IBehTreeNodeDefinition::GetTask() const
{
	return NULL;
}
Bool IBehTreeNodeDefinition::SetTask( IBehTreeTaskDefinition* task )
{
	return false;
}
Bool IBehTreeNodeDefinition::IsSupportingTasks() const
{
	return false;
}
Bool IBehTreeNodeDefinition::IsSupportingTaskClass( const CClass* classId ) const
{
	return true;
}
void IBehTreeNodeDefinition::CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const
{
	nodes.PushBack( const_cast< IBehTreeNodeDefinition* >( this ) );
	for ( Uint32 i = 0, n = GetNumChildren(); i != n; ++i )
	{
		IBehTreeNodeDefinition* child = GetChild( i );
		if ( child )
		{
			child->CollectNodes( nodes );
		}
	}
}

Bool IBehTreeNodeDefinition::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	{
		static CName priorityName( TXT("priority") );
		if ( priorityName == propertyName )
		{
			IRTTIType* prevType = readValue.GetRTTIType();
			if ( prevType->GetType() == RT_Fundamental )
			{
				CName prevTypeName = prevType->GetName();
				if ( prevTypeName == CNAME( Int8 ) )
				{
					m_priority.m_value = *reinterpret_cast< const Int8* >( readValue.GetData() );
				}
			}
			return true;
		}
	}

	if ( BehTreeVarsUtils::OnPropertyTypeMismatch( this, propertyName, existingProperty, readValue )  )
	{
		return true;
	}
	
	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}


#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool IBehTreeNodeDefinition::CorrectChildrenPositions()
{
	Bool modified = false;
	for ( Uint32 i = 0, n = GetNumChildren(); i != n; ++i )
	{
		modified = GetChild( i )->CorrectChildrenPositions() || modified;
	}
	return modified;
}
Bool IBehTreeNodeDefinition::CorrectChildrenOrder()
{
	Bool modified = false;
	for ( Uint32 i = 0, n = GetNumChildren(); i != n; ++i )
	{
		modified = GetChild( i )->CorrectChildrenOrder() || modified;
	}
	return modified;
}

void IBehTreeNodeDefinition::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	m_graphPosX += offsetX;
	m_graphPosY += offsetY;
}

#endif	// NO_EDITOR_GRAPH_SUPPORT


RED_DEFINE_STATIC_NAMED_NAME( BTNODE_STATUS_ENUM, "EBTNodeStatus" );

void IBehTreeNodeDefinition::LogResult( CBehTreeInstance& instance ) const
{
	CEnum* e = SRTTI::GetInstance().FindEnum( CNAME( BTNODE_STATUS_ENUM ) );
	const SBTNodeResult res; // TODO: FIX ME = GetResult( instance );
	SBTNodeResult modRes; // TODO: FIX ME = GetModifiedResult( instance );
	CName resName, modResName;
	Bool b = e->FindName( (Int32)res.m_status, resName );
	b = e->FindName( (Int32)modRes.m_status, modResName ) || b;	
	String text;
	if( b )
	{
		text = String::Printf( TXT("Result: %s, Modified: %s"), resName.AsString().AsChar(), modResName.AsString().AsChar() );
	}
	else
	{
		text = TXT("FindName error");
	}
	BT_LOG( instance, this, text.AsChar() );
}

void IBehTreeNodeDefinition::SpawnInstanceList( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent, TStaticArray< IBehTreeNodeInstance*, 128 >& instanceList ) const
{
	IBehTreeNodeInstance* instance = SpawnInstance( owner, context, parent );
	if ( instance )
	{
		instanceList.PushBack( instance );
	}
}

Bool IBehTreeNodeDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if ( !IsMyInstance( node ) )
	{
		return false;
	}

	node->OnSpawn( *this, context );

	Uint32 defChildCount = GetNumChildren();
	Uint32 insChildCount = node->GetNumChildren();
	Uint32 defChild = 0;
	Uint32 insChild = 0;
	while ( defChild < defChildCount && insChild < insChildCount )
	{
		IBehTreeNodeInstance* childIns = node->GetChild( insChild );
		IBehTreeNodeDefinition* childDef = GetChild( defChild );

		++defChild;

		if ( !childDef->OnSpawn( childIns, context ) )
		{
			continue;
		}
		++insChild;
	}
	AI_ASSERT( insChild == insChildCount );

	return true;
}

Uint32 IBehTreeNodeDefinition::OnSpawnList( IBehTreeNodeInstance* const* nodeList, Uint32 nodeCount, CBehTreeSpawnContext& context ) const
{
	return OnSpawn( nodeList[ 0 ], context ) ? 1 : 0;
}

Bool IBehTreeNodeDefinition::IsMyInstance( IBehTreeNodeInstance* node ) const
{
	return node->GetDefinitionId() == m_uniqueId;
}


////////////////////////////////////////////////////////////////////
// IBehTreeNodeInstance
////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance::~IBehTreeNodeInstance()
{
#ifdef DEBUG_BEHTREENODE_DESTRUCTION
	RED_FATAL_ASSERT( m_isDestroyed, "No 'OnDestruction' called" );
#endif
}

void IBehTreeNodeInstance::Update()
{
}
Bool IBehTreeNodeInstance::Activate()
{
	m_isActive = true;
#ifdef EDITOR_AI_DEBUG
	if ( m_owner->GetIsDebugged() )
	{
		if ( nullptr != m_owner->GetMachine() )
		{
			DebugUpdate();
		}
	}
	
#endif
	return true;
}

void IBehTreeNodeInstance::Deactivate()
{
	m_isActive = false;
#ifdef EDITOR_AI_DEBUG
	if ( m_owner->GetIsDebugged() )
	{
		if ( nullptr != m_owner->GetMachine() )
		{
			DebugUpdate();
		}
	}
#endif
}

#ifdef EDITOR_AI_DEBUG
void IBehTreeNodeInstance::DebugReport()
{
	// Report to the editor
	for( Uint32 i = 0; i < this->m_owner->GetMachine()->GetListenerCount(); ++i )
	{
		IBehTreeMachineListener* listener = this->m_owner->GetMachine()->GetListener( i );
		if( listener )
		{
			listener->OnNodeReport( this );
		}
	}

	// Prompt children to report
	for( Int32 i = 0; i < GetNumChildren(); ++i )
	{
		GetChild( i )->DebugReport();
	}
}

Bool IBehTreeNodeInstance::DebugUpdate(Bool cascade)
{
	Bool res = false;
	for( Uint32 i = 0; i < this->m_owner->GetMachine()->GetListenerCount(); ++i )
	{
		IBehTreeMachineListener* listener = this->m_owner->GetMachine()->GetListener( i );
		if( listener )
		{
			// Breakpoints
			listener->OnBreakpoint( this );
			
			// State change
			res |= listener->OnNodeResultChanged( this, m_isActive );
		}
	}

	return res;
}

void IBehTreeNodeInstance::ForceSetDebugColor( Uint8 R, Uint8 G, Uint8 B )
{
	if( !( m_owner->GetIsDebugged() ) )
	{
		return;
	}

	AI_ASSERT( m_owner->GetMachine() != nullptr );

	for( Uint32 i = 0; i < this->m_owner->GetMachine()->GetListenerCount(); ++i )
	{
		IBehTreeMachineListener* listener = this->m_owner->GetMachine()->GetListener( i );
		if( listener )
		{
			listener->OnForceDebugColorSet( this, R, G, B );
		}
	}
}

void IBehTreeNodeInstance::ClearColorRecursive()
{
	if( !( m_owner->GetIsDebugged() ) )
	{
		return;
	}

	ForceSetDebugColor( 142, 142, 142 );

	if ( m_isActive )
	{
		Uint32 activeChildrenCount = GetActiveChildCount();

		for( Uint32 i = 0; i< activeChildrenCount; ++ i )
		{
			IBehTreeNodeInstance* child = GetActiveChild( i );
			if( child )
			{		
				child->ClearColorRecursive();
			}
		}
	}
}

#endif

void IBehTreeNodeInstance::Complete( eTaskOutcome outcome )
{
	Deactivate();

	if ( m_parent )
		m_parent->OnSubgoalCompleted( outcome );
}
void IBehTreeNodeInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	Complete( outcome );
}
Bool IBehTreeNodeInstance::IsAvailable()
{
	return true;
}
Int32 IBehTreeNodeInstance::Evaluate()
{
	if( IsAvailable() )
	{
		if( m_priority <= 0 )
		{
			DebugNotifyAvailableFail();
		}

		return m_priority;
	}

	DebugNotifyAvailableFail();
	return -1;
}
Bool IBehTreeNodeInstance::OnEvent( CBehTreeEvent& e )
{
	return false;
}
Bool IBehTreeNodeInstance::OnListenedEvent( CBehTreeEvent& e )
{
	return false;
}
void IBehTreeNodeInstance::MarkDirty()
{
}
void IBehTreeNodeInstance::MarkParentSelectorDirty()
{
	if ( !m_parent )
	{
		return;
	}
	// aim for tail recursion
	m_parent->MarkParentSelectorDirty();
}

void IBehTreeNodeInstance::MarkActiveBranchDirty()
{
	MarkDirty();

	IBehTreeNodeInstance* child = GetActiveChild();
	while ( child )
	{
		child->MarkDirty();
		child = child->GetActiveChild();
	}
}
Bool IBehTreeNodeInstance::Interrupt()
{
	Deactivate();
	return true;
}

void IBehTreeNodeInstance::PropagateDebugFragmentsGeneration( CRenderFrame* frame )
{
	Uint32 activeChildrenCount = GetActiveChildCount();

	for( Uint32 i = 0; i< activeChildrenCount; ++ i )
	{
		IBehTreeNodeInstance* child = GetActiveChild( i );
		if( child )
		{		
			child->OnGenerateDebugFragments( frame );
			child->PropagateDebugFragmentsGeneration( frame );
		}
	}
}

void IBehTreeNodeInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{

}

void IBehTreeNodeInstance::OnSpawn( const IBehTreeNodeDefinition& def, CBehTreeSpawnContext& context )
{
}

void IBehTreeNodeInstance::OnDestruction()
{
#ifdef DEBUG_BEHTREENODE_DESTRUCTION
	RED_FATAL_ASSERT( !m_isDestroyed, "AI node destroyed without deactivation!" );
	m_isDestroyed = true;
#endif
	ASSERT( !m_isActive, TXT("AI node destroyed without deactivation!") );
}

void IBehTreeNodeInstance::RegisterForDeletion()
{
	m_parent = nullptr;

	m_owner->RegisterNodeForDeletion( this );
}

Bool IBehTreeNodeInstance::IsMoreImportantNodeActive( IBehTreeNodeInstance* askingChild )
{
	if( m_parent )
	{
		return m_parent->IsMoreImportantNodeActive( this );
	}
	
	return false;
}

Int32 IBehTreeNodeInstance::GetNumPersistantChildren() const
{
	return GetNumChildren();
}
IBehTreeNodeInstance* IBehTreeNodeInstance::GetPersistantChild( Int32 index ) const
{
	return GetChild( index );
}

Bool IBehTreeNodeInstance::IsSavingState() const
{
	return false;
}
void IBehTreeNodeInstance::SaveState( IGameSaver* writer )
{

}
Bool IBehTreeNodeInstance::LoadState( IGameLoader* reader )
{
	return true;
}

Int32 IBehTreeNodeInstance::GetNumChildren() const
{
	return 0;
}
IBehTreeNodeInstance* IBehTreeNodeInstance::GetChild( Int32 index ) const
{
	return NULL;
}
IBehTreeNodeInstance* IBehTreeNodeInstance::GetActiveChild() const
{
	return NULL;
}
IBehTreeNodeInstance* IBehTreeNodeInstance::GetActiveChild( Uint32 activeChild ) const
{
	return GetActiveChild();
}
Uint32 IBehTreeNodeInstance::GetActiveChildCount() const
{
	return GetActiveChild() ? 1 : 0;
}
