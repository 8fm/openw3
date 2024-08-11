/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphAnimationSwitchNode.h"
#include "behaviorGraphContainerNode.h"
#include "behaviorGraphOutputNode.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphTopLevelNode.h"
#include "../engine/graphHelperBlock.h"
#include "behaviorGraphMimicNodes.h"
#include "behaviorGraphTransitionNode.h"
#include "behaviorGraphSocket.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphContainerNode );

CBehaviorGraphContainerNode::CBehaviorGraphContainerNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphContainerNode::OnRebuildSockets()
{
	// the method is called by child classes, which already have rebuilders
	// no neeed to call it here
	//GraphConnectionRebuilder rebuilder( this );

	// Create sockets for animation inputs
	for ( Uint32 i=0; i<m_animationInputs.Size(); ++i )
	{
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( m_animationInputs[i] ) );	
	}

	// Create sockets for value inputs
	for ( Uint32 i=0; i<m_valueInputs.Size(); ++i )
	{
		CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( m_valueInputs[i] ) );		
	}

	// Create sockets for vector value inputs
	for ( Uint32 i=0; i<m_vectorValueInputs.Size(); ++i )
	{
		CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( m_vectorValueInputs[i] ) );		
	}

	// Create sockets for mimic inputs
	for ( Uint32 i=0; i<m_mimicInputs.Size(); ++i )
	{
		CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( m_mimicInputs[i] ) );	
	}
}

#endif

void CBehaviorGraphContainerNode::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	for ( Int32 i=(Int32)m_nodes.Size()-1; i>=0; i-- )
	{
		if ( !m_nodes[i] )
		{
			m_nodes.Erase( m_nodes.Begin() + i );
		}
	}
}

void CBehaviorGraphContainerNode::OnSerialize( IFile &file )
{
	TBaseClass::OnSerialize( file );

	file << m_nodes;
	file << m_animationInputs;
	file << m_valueInputs;
}

void CBehaviorGraphContainerNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	for ( Uint32 i=0; i<m_nodes.Size(); ++i )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->OnBuildDataLayout( compiler );
		}
	}
}

void CBehaviorGraphContainerNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );
	
	for ( Uint32 i=0; i<m_nodes.Size(); ++i )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->OnInitInstance( instance );
		}
	}
}

void CBehaviorGraphContainerNode::CollectNodesToRelease( TDynArray< CBehaviorGraphNode* >& nodesToRelease )
{
	for ( Uint32 i=0; i<m_nodes.Size(); ++i )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			CBehaviorGraphContainerNode* cNode = Cast< CBehaviorGraphContainerNode >( bgNode );
			if ( cNode )
			{
				RED_FATAL_ASSERT( !cNode->IsOnReleaseInstanceManuallyOverridden(), "CBehaviorGraphContainerNode::CollectNodesToRelease" );
			}

			const Bool ovM = bgNode->IsOnReleaseInstanceManuallyOverridden();
			
#ifdef USE_BEHAVIIOR_AUTO_VIRTUAL_FUNC_TESTER
			if ( !cNode )
			{
				const Bool ovA = bgNode->IsOnReleaseInstanceAutoOverridden();

				RED_FATAL_ASSERT( ovA == ovM, "Behavior graph node class '%ls' does not have the IsOnReleaseInstanceManuallyOverridden() func. Please add it.", bgNode->GetClass()->GetName().AsChar() );
			}
#endif

			RED_FATAL_ASSERT( !nodesToRelease.Exist( bgNode ), "CBehaviorGraphContainerNode::CollectNodesToRelease" );

			if ( ovM )
			{
				nodesToRelease.PushBack( bgNode );
			}
		}
	}
}

void CBehaviorGraphContainerNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	for ( Uint32 i=0; i<m_nodes.Size(); ++i )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->OnReleaseInstance( instance );
		}
	}
}

void CBehaviorGraphContainerNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	// Reset all contained nodes
	for ( Uint32 i=0; i<m_nodes.Size(); ++i )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->Reset( instance );
		}
	}
}

void CBehaviorGraphContainerNode::OnOpenInEditor( CBehaviorGraphInstance& instance ) const
{
	for ( Uint32 i=0; i<m_nodes.Size(); ++i )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->OnOpenInEditor( instance );
		}
	}
}

void CBehaviorGraphContainerNode::OnUpdateAnimationCache( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnUpdateAnimationCache( instance );
	
	for ( Uint32 i=0; i<m_nodes.Size(); i++ )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->OnUpdateAnimationCache( instance );
		}
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphContainerNode::OnChildNodeAdded( CGraphBlock *node )
{
	m_nodes.PushBack( node );
}

CGraphBlock* CBehaviorGraphContainerNode::CreateChildNode( const GraphBlockSpawnInfo& info )
{
	// Make sure this is a valid class
	//ASSERT( info.GetClass()->IsBasedOn( CBehaviorGraphNode::GetStaticClass() ) );

	// Create node
	CGraphBlock* newNode = CreateObject<CGraphBlock>( info.GetClass(), this );
	OnChildNodeAdded( newNode );	

	// Inform node that it has been spawned
	newNode->OnSpawned( info );

	// Rebuild sockets of new node
	newNode->OnRebuildSockets();

	// Done
	return newNode;
}

void CBehaviorGraphContainerNode::RemoveChildNode( CGraphBlock* node )
{
	// Make sure node exist before deleting it
	ASSERT( m_nodes.Exist( node ) );
	m_nodes.Remove( node );
}

Bool CBehaviorGraphContainerNode::ChildNodeClassSupported( CClass *nodeClass )
{
	return !nodeClass->IsAbstract() && 
		( nodeClass->IsBasedOn( ClassID< CBehaviorGraphNode >() ) || nodeClass->IsBasedOn( ClassID< CGraphHelperBlock >() ) ) &&
		nodeClass != CBehaviorGraphOutputNode::GetStaticClass() &&
		nodeClass != CBehaviorGraphMimicOutputNode::GetStaticClass() &&
		nodeClass != CBehaviorGraphTopLevelNode::GetStaticClass() &&
		!nodeClass->IsBasedOn( CBehaviorGraphStateNode::GetStaticClass() ) &&
		nodeClass != CBehaviorGraphAnimationSwitchNode::GetStaticClass() &&
		!nodeClass->IsBasedOn( CBehaviorGraphStateTransitionNode::GetStaticClass() );
}

void CBehaviorGraphContainerNode::CreateAnimationInput( const CName& name )
{
	m_animationInputs.PushBack( name );
	OnRebuildSockets();
}

void CBehaviorGraphContainerNode::CreateValueInput( const CName& name )
{
	m_valueInputs.PushBack( name );
	OnRebuildSockets();
}

void CBehaviorGraphContainerNode::CreateVectorValueInput( const CName& name )
{
	m_vectorValueInputs.PushBack( name );
	OnRebuildSockets();
}

void CBehaviorGraphContainerNode::CreateMimicInput( const CName& name )
{
	m_mimicInputs.PushBack( name );
	OnRebuildSockets();
}

void CBehaviorGraphContainerNode::RemoveAnimationInput( const CName& name )
{
	m_animationInputs.Remove( name );
	OnRebuildSockets();
}

void CBehaviorGraphContainerNode::RemoveValueInput( const CName& name )
{
	m_valueInputs.Remove( name );
	OnRebuildSockets();
}

void CBehaviorGraphContainerNode::RemoveVectorValueInput( const CName& name )
{
	m_vectorValueInputs.Remove( name );
	OnRebuildSockets();
}

void CBehaviorGraphContainerNode::RemoveMimicInput( const CName& name )
{
	m_mimicInputs.Remove( name );
	OnRebuildSockets();
}

Bool CBehaviorGraphContainerNode::CanBeExpanded() const
{
	return true;
}

#endif

const TDynArray< CName >& CBehaviorGraphContainerNode::GetAnimationInputs() const
{
	return m_animationInputs;
}

const TDynArray< CName >& CBehaviorGraphContainerNode::GetValueInputs() const
{
	return m_valueInputs;
}

const TDynArray< CName >& CBehaviorGraphContainerNode::GetVectorValueInputs() const
{
	return m_vectorValueInputs;
}

const TDynArray< CName >& CBehaviorGraphContainerNode::GetMimicInputs() const
{
	return m_mimicInputs;
}

void CBehaviorGraphContainerNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	// Generate debug info from all contained nodes
	for (Uint32 i=0; i<m_nodes.Size(); i++)
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->OnGenerateFragments( instance, frame );
		}
	}
}

void CBehaviorGraphContainerNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	// Cache connections in all contained nodes
	for ( Uint32 i=0; i<m_nodes.Size(); i++ )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->CacheConnections();
		}
	}
}

void CBehaviorGraphContainerNode::RemoveConnections()
{
	TBaseClass::RemoveConnections();

	// Remove connections in all contained nodes
	for ( Uint32 i=0; i<m_nodes.Size(); i++ )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->RemoveConnections();
		}
	}
}

Bool CBehaviorGraphContainerNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	TBaseClass::ProcessForceEvent( instance, event );

	Bool ret = false;

	for ( Uint32 i=0; i<m_nodes.Size(); i++ )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			ret |= bgNode->ProcessForceEvent( instance, event );
		}
	}

	return ret;
}

void CBehaviorGraphContainerNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	for ( Uint32 i=0; i<m_nodes.Size(); i++ )
	{
		if ( CBehaviorGraphNode* bgNode = Cast< CBehaviorGraphNode >( m_nodes[i] ) )
		{
			bgNode->GetUsedVariablesAndEvents( var, vecVar, events, intVar, intVecVar );
		}
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
