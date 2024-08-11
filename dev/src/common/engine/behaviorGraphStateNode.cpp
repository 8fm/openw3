/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphAnimationNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutputNode.h"
#include "behaviorGraphStartingStateNode.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphTransitionNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnection.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animSyncInfo.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( SBehaviorGraphStateBehaviorGraphSyncInfo );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateNode );

///////////////////////////////////////////////////////////////////////////////

SBehaviorGraphStateBehaviorGraphSyncInfo::SBehaviorGraphStateBehaviorGraphSyncInfo()
	:	m_inSyncPriority( 0 )
	,	m_inAllSyncTagsRequired( false )
{

}

Int32 SBehaviorGraphStateBehaviorGraphSyncInfo::GetMatchingInboundTagCount( const SBehaviorSyncTags& syncTags) const
{
	Int32 matchCount = 0;
	for ( TStaticArray< CName, 32 >::const_iterator iSyncTag = syncTags.m_syncTags.Begin(); iSyncTag != syncTags.m_syncTags.End(); ++ iSyncTag )
	{
		matchCount += m_inSyncTags.Exist( *iSyncTag ) ? 1 : 0;
	}
	return ! m_inAllSyncTagsRequired || matchCount == m_inSyncTags.SizeInt() ? matchCount : 0;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
String SBehaviorGraphStateBehaviorGraphSyncInfo::GetInListAsString() const
{
	String out = TXT("");
	String separator = TXT("");
	for ( TDynArray< CName >::const_iterator iSyncTag = m_inSyncTags.Begin(); iSyncTag != m_inSyncTags.End(); ++ iSyncTag )
	{
		out += separator + iSyncTag->AsString();
		separator = TXT(", ");
	}
	return out;
}
#endif

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphStateNode::CBehaviorGraphStateNode()
	: m_rootNode( NULL )
{	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphStateNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	// Create master output node
	m_rootNode = SafeCast< CBehaviorGraphNode >( CreateChildNode( GraphBlockSpawnInfo( CBehaviorGraphOutputNode::GetStaticClass() ) ) );
}

void CBehaviorGraphStateNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphStateInSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( CBehaviorGraphStateOutSocketSpawnInfo( CNAME( Out ) ) );

	TBaseClass::OnRebuildSockets();
}

EGraphBlockShape CBehaviorGraphStateNode::GetBlockShape() const
{
	return GBS_Slanted;
}

EGraphBlockDepthGroup CBehaviorGraphStateNode::GetBlockDepthGroup() const
{
	return GBDG_Foreground;
}

String CBehaviorGraphStateNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("State - %s"), m_name.AsChar() );
	}
	else
	{
		// For state with just one animation return animation name as caption
		if( m_nodes.Size() == 2 )
		{
			const CBehaviorGraphAnimationNode* animNode = NULL;
			if( m_nodes[0]->IsA< CBehaviorGraphAnimationNode >() )
			{
				animNode = static_cast< const CBehaviorGraphAnimationNode* >( m_nodes[0] );
			}
			else if( m_nodes[1]->IsA< CBehaviorGraphAnimationNode >() )
			{
				animNode = static_cast< const CBehaviorGraphAnimationNode* >( m_nodes[1] );
			}

			if( animNode )
			{
				const CName& animName = animNode->GetAnimationName();
				if( !animName.Empty() )
				{
					return animName.AsString();
				}
			}
		}

		return String( TXT("State") );
	}
}

void CBehaviorGraphStateNode::OnCopyAllTransitionsFrom( CBehaviorGraphStateNode *const srcStateNode )
{

	const TDynArray< CGraphSocket* > & sockets	= this->GetSockets();
	CGraphSocket *inSocket						= sockets[ 0 ];
	CGraphSocket *outSocket						= sockets[ 1 ];

	const TDynArray< CGraphSocket* > & srcSockets	= srcStateNode->GetSockets();
	const CGraphSocket *srcInSocket					= srcSockets[ 0 ];
	const CGraphSocket *srcOutSocket				= srcSockets[ 1 ];
	if ( srcInSocket )
	{
		const TDynArray< CGraphConnection* > srcInConnections = srcInSocket->GetConnections();
		for ( Uint32 j = 0; j < srcInConnections.Size(); ++j )
		{
			const CGraphConnection *const srcInConnection	= srcInConnections[ j ];
			const CGraphSocket *const srcDestinationSocket	= srcInConnection->GetDestination();

			if ( srcDestinationSocket->GetBlock()->IsA<CBehaviorGraphStateTransitionNode>() )
			{
				CBehaviorGraphStateTransitionNode *const srcTransition	= Cast< CBehaviorGraphStateTransitionNode >( srcDestinationSocket->GetBlock() );
				CBehaviorGraphStateTransitionNode *const newTransition	= SafeCast< CBehaviorGraphStateTransitionNode >( srcTransition->Clone( GetParent() ) );
				SafeCast< CBehaviorGraphContainerNode >( GetParent() )->OnChildNodeAdded( newTransition );
				newTransition->SetPosition( srcTransition->GetPosition() + Vector2( 10,10 )  );

				CGraphSocket *const newTransitionInSocket	= newTransition->GetSockets()[0];
				CGraphSocket *const newTransitionOutSocket	= newTransition->GetSockets()[1];

				if ( newTransitionOutSocket->CanConnectTo( inSocket ) && inSocket->CanConnectTo( newTransitionOutSocket ) )
				{
					newTransitionOutSocket->ConnectTo( inSocket );
					const CBehaviorGraphStateNode *const otherState = srcTransition->GetSourceState();
					if ( otherState )
					{
						CGraphSocket *const otherStateOutSocket = otherState->GetSockets()[ 1 ];
						if ( otherStateOutSocket->CanConnectTo( newTransitionInSocket ) && newTransitionInSocket->CanConnectTo( otherStateOutSocket ) )
						{
							otherStateOutSocket->ConnectTo( newTransitionInSocket );
							newTransition->InvalidateLayout();
						}
					}
				}

				//copying all other connection of the transition
				const TDynArray< CGraphSocket* > & srcTransitionSockets				=  srcTransition->GetSockets();
				for ( Uint32 j = 2; j < srcTransitionSockets.Size(); ++j )
				{
					const TDynArray< CGraphConnection* > srcTransitionConnections	= srcTransition->GetSockets()[ j ]->GetConnections();
					if ( srcTransitionConnections.Size() != 0 )
					{
						const CGraphConnection *const srcTransitionConnection		= srcTransitionConnections[ 0 ]; // there should be only one
						const CGraphSocket *const srcTransitionDestinationSocket	= srcTransitionConnection->GetDestination();
						CGraphSocket *const newTransitionOutSocket					= newTransition->GetSockets()[ j ];
						CGraphSocket *const blockInSocket							= srcTransitionDestinationSocket->GetBlock()->GetSockets()[ 0 ];
						if ( newTransitionOutSocket->CanConnectTo( blockInSocket ) && blockInSocket->CanConnectTo( newTransitionOutSocket ) )
						{
							newTransitionOutSocket->ConnectTo( blockInSocket );
						}
					}
				}
			}
		}
	}
	if ( srcOutSocket )
	{
		const TDynArray< CGraphConnection* > srcOutConnections = srcOutSocket->GetConnections();
		for ( Uint32 j = 0; j < srcOutConnections.Size(); ++j )
		{
			const CGraphConnection *const srcOutConnection	= srcOutConnections[ j ];
			const CGraphSocket *const srcDestinationSocket	= srcOutConnection->GetDestination();

			if ( srcDestinationSocket->GetBlock()->IsA<CBehaviorGraphStateTransitionNode>() )
			{
				CBehaviorGraphStateTransitionNode *const srcTransition	= Cast< CBehaviorGraphStateTransitionNode >( srcDestinationSocket->GetBlock() );
				CBehaviorGraphStateTransitionNode *const newTransition	= SafeCast< CBehaviorGraphStateTransitionNode >( srcTransition->Clone( GetParent() ) );
				SafeCast< CBehaviorGraphContainerNode >( GetParent() )->OnChildNodeAdded( newTransition );
				newTransition->SetPosition( srcTransition->GetPosition() + Vector2( 10,10 )  );

				CGraphSocket *const newTransitionInSocket	= newTransition->GetSockets()[0];
				CGraphSocket *const newTransitionOutSocket	= newTransition->GetSockets()[1];

				if ( outSocket->CanConnectTo( newTransitionInSocket ) && newTransitionInSocket->CanConnectTo( outSocket ) )
				{
					outSocket->ConnectTo( newTransitionInSocket );
					const CBehaviorGraphStateNode *const otherState = srcTransition->GetDestState();
					CGraphSocket *const otherStateSocket			= otherState->GetSockets()[ 0 ];
					if ( newTransitionOutSocket->CanConnectTo( otherStateSocket ) && otherStateSocket->CanConnectTo( newTransitionOutSocket ) )
					{
						newTransitionOutSocket->ConnectTo( otherStateSocket );
						newTransition->InvalidateLayout();
					}
				}

				//copying all other connection of the transition
				const TDynArray< CGraphSocket* > & srcTransitionSockets				=  srcTransition->GetSockets();
				for ( Uint32 j = 2; j < srcTransitionSockets.Size(); ++j )
				{
					const TDynArray< CGraphConnection* > srcTransitionConnections	= srcTransition->GetSockets()[ j ]->GetConnections();
					if ( srcTransitionConnections.Size() != 0 )
					{
						const CGraphConnection *const srcTransitionConnection	= srcTransitionConnections[ 0 ]; // there should be only one
						const CGraphSocket *const srcTransitionDestinationSocket	= srcTransitionConnection->GetDestination();
						CGraphSocket *const newTransitionOutSocket					= newTransition->GetSockets()[ j ];
						CGraphSocket *const blockInSocket							= srcTransitionDestinationSocket->GetBlock()->GetSockets()[ 0 ];
						if ( newTransitionOutSocket->CanConnectTo( blockInSocket ) && blockInSocket->CanConnectTo( newTransitionOutSocket ) )
						{
							newTransitionOutSocket->ConnectTo( blockInSocket );
						}
					}
				}
			}
		}
	}
	InvalidateLayout();
}

#endif

void CBehaviorGraphStateNode::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
	file << m_rootNode;
}

void CBehaviorGraphStateNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( State );

	// Pass to state root node
	if ( m_rootNode )
	{
		m_rootNode->Update( context, instance, timeDelta );
	}

	// Update transitions
	TransitionsStartBlockUpdate( context, instance, timeDelta );
}

void CBehaviorGraphStateNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	// Pass to state root node
	if ( m_rootNode )
	{
		m_rootNode->Sample( context, instance, output );
	}
}

Bool CBehaviorGraphStateNode::PreloadAnimations( CBehaviorGraphInstance& instance ) const
{
	if ( m_rootNode )
	{
		return m_rootNode->PreloadAnimations( instance );
	}
	return true;
}

void CBehaviorGraphStateNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	// Pass to state root node
	if ( m_rootNode )
	{
		m_rootNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphStateNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	// Pass to state root node
	if ( m_rootNode )
	{
		m_rootNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphStateNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	// Pass to state root node
	if ( m_rootNode )
	{	
		return m_rootNode->ProcessEvent( instance, event );
	}
	return false;
}

void CBehaviorGraphStateNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	// Pass to root node
	if ( m_rootNode )
	{
		m_rootNode->CacheConnections();
	}

	// Reset
	m_cachedStateTransitions.Clear();

	// Get the state transition connection socket
	CBehaviorGraphStateOutSocket* socket = Cast< CBehaviorGraphStateOutSocket >( CGraphBlock::FindSocket( TXT("Out") ) );
	if ( socket )
	{
		// Cache connected transitions
		const Uint32 numTransitions = socket->GetNumConnectedTransitions();
		for ( Uint32 i=0; i<numTransitions; i++ )
		{
			CBehaviorGraphStateTransitionNode* node = socket->GetConnectedTransition( i );
			if ( node )
			{
				m_cachedStateTransitions.PushBack( node );
			}
		}
	}
}

Uint32 CBehaviorGraphStateNode::GetNumConnectedTransitions() const
{
	return m_cachedStateTransitions.Size();
}

CBehaviorGraphStateTransitionNode* CBehaviorGraphStateNode::GetConnectedTransition( Uint32 index ) const
{
	ASSERT( index < m_cachedStateTransitions.Size() );
	return m_cachedStateTransitions[ index ];
}

void CBehaviorGraphStateNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	// Activate state root node
	if ( m_rootNode )
	{
		m_rootNode->Activate( instance );
	}

	// Pass to transitions
	TransitionsStartBlockActivated( instance );
}

void CBehaviorGraphStateNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	// Deactivate state root node
	if ( m_rootNode )
	{
		m_rootNode->Deactivate( instance );
	}

	// Pass to transitions
	TransitionsStartBlockDeactivated( instance );
}

Bool CBehaviorGraphStateNode::GetSyncInfoForInstance( CBehaviorGraphInstance& instance, CBehaviorSyncInfo& info ) const
{
	Uint32 transNum = m_cachedStateTransitions.Size();

	CSyncInfo timeInfo;
	m_rootNode->GetSyncInfo( instance, timeInfo );

	info.m_time = timeInfo.m_currTime;

	for ( Uint32 i=0; i<transNum; ++i )
	{
		CBehaviorGraphFlowTransitionNode* trans = Cast< CBehaviorGraphFlowTransitionNode >( m_cachedStateTransitions[ i ] );
		if ( trans && trans->GetSyncInfoForInstance( instance, info ) )
		{
			return true;
		}
	}

	return false;
}

void CBehaviorGraphStateNode::TransitionsStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	// Pass to all transitions
	const Uint32 numTransitions = m_cachedStateTransitions.Size();
	for( Uint32 i=0; i<numTransitions; ++i )
	{
		CBehaviorGraphStateTransitionNode* transition = m_cachedStateTransitions[i];
		transition->OnStartBlockActivated( instance );
	}
}

void CBehaviorGraphStateNode::TransitionsStartBlockDeactivated( CBehaviorGraphInstance& instance ) const
{
	// Pass to all transitions
	const Uint32 numTransitions = m_cachedStateTransitions.Size();
	for( Uint32 i=0; i<numTransitions; ++i )
	{
		CBehaviorGraphStateTransitionNode* transition = m_cachedStateTransitions[i];
		transition->OnStartBlockDeactivated( instance );
	}	
}

void CBehaviorGraphStateNode::TransitionsStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	// Pass to all transitions
	const Uint32 numTransitions = m_cachedStateTransitions.Size();
	for( Uint32 i=0; i<numTransitions; ++i )
	{
		CBehaviorGraphStateTransitionNode* transition = m_cachedStateTransitions[i];
		transition->OnStartBlockUpdate( context, instance, timeDelta );
	}
}

void CBehaviorGraphStateNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	// Process activation alpha from root node
	if ( m_rootNode )
	{
		m_rootNode->ProcessActivationAlpha( instance, alpha );
	}

	// Process activation alpha for transitions
	const Uint32 numTransitions = m_cachedStateTransitions.Size();
	for( Uint32 i=0; i<numTransitions; ++i )
	{
		CBehaviorGraphStateTransitionNode* transition = m_cachedStateTransitions[i];
		transition->StartBlockProcessActivationAlpha( instance, alpha );
	}	
}

const TagList& CBehaviorGraphStateNode::GetGroupList() const
{
	return m_groups;
}

Bool CBehaviorGraphStateNode::GetOutboundSyncTags( SBehaviorSyncTags& tags ) const
{
	for ( TDynArray< CName >::const_iterator iTag = m_behaviorGraphSyncInfo.m_outSyncTags.Begin(); iTag != m_behaviorGraphSyncInfo.m_outSyncTags.End(); ++ iTag )
	{
		tags.Add( *iTag );
	}

	// return true if added anything
	return ! m_behaviorGraphSyncInfo.m_outSyncTags.Empty();
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
