/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphAnimationNode.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphTransitionNode.h"
#include "../engine/graphConnection.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationInputSocket );

Bool CBehaviorGraphAnimationInputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphAnimationOutputSocket >();	
}

CBehaviorGraphAnimationOutputSocket* CBehaviorGraphAnimationInputSocket::GetConnectedSocket() const
{	
	if ( HasConnections() )
	{
		CGraphConnection* connection = m_connections[0];
		return SafeCast< CBehaviorGraphAnimationOutputSocket >( connection->GetDestination() );
	}

	return NULL;
}

CBehaviorGraphNode* CBehaviorGraphAnimationInputSocket::GetConnectedNode() const
{
	CBehaviorGraphAnimationOutputSocket *connectedSocket = GetConnectedSocket();
	if ( connectedSocket )
	{
		return SafeCast< CBehaviorGraphNode >( connectedSocket->GetBlock() );
	}

	return NULL;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphAnimationInputSocketSpawnInfo::CBehaviorGraphAnimationInputSocketSpawnInfo( const CName& name )
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphAnimationInputSocket >() )
{	
	m_color = Color( 255, 128, 128 );
	m_name = name;
	
	m_direction = LSD_Input;
	m_placement = LSP_Right;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationOutputSocket );

Bool CBehaviorGraphAnimationOutputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphAnimationInputSocket >();	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphAnimationOutputSocketSpawnInfo::CBehaviorGraphAnimationOutputSocketSpawnInfo( const CName& name )
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphAnimationOutputSocket >() )
{
	m_color = Color( 255, 128, 128 );
	m_name = name;

	m_direction = LSD_Output;
	m_placement = LSP_Left;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphExactlyAnimationInputSocket );

Bool CBehaviorGraphExactlyAnimationInputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	Bool ret = otherSocket->IsA< CBehaviorGraphAnimationOutputSocket >();

	if ( ret )
	{
		return Cast< CBehaviorGraphAnimationNode >( otherSocket->GetBlock() ) ? true : false;
	}

	return ret;	
}

CBehaviorGraphAnimationOutputSocket* CBehaviorGraphExactlyAnimationInputSocket::GetConnectedSocket() const
{	
	if ( HasConnections() )
	{
		CGraphConnection* connection = m_connections[0];
		return SafeCast< CBehaviorGraphAnimationOutputSocket >( connection->GetDestination() );
	}

	return NULL;
}

CBehaviorGraphNode* CBehaviorGraphExactlyAnimationInputSocket::GetConnectedNode() const
{
	CBehaviorGraphAnimationOutputSocket *connectedSocket = GetConnectedSocket();
	if ( connectedSocket )
	{
		return SafeCast< CBehaviorGraphAnimationNode >( connectedSocket->GetBlock() );
	}

	return NULL;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphExactlyAnimationInputSocketSpawnInfo::CBehaviorGraphExactlyAnimationInputSocketSpawnInfo( const CName& name )
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphExactlyAnimationInputSocket >() )
{	
	m_color = Color( 255, 0, 0 );
	m_name = name;

	m_direction = LSD_Input;
	m_placement = LSP_Right;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVariableInputSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CBehaviorGraphVariableInputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphVariableOutputSocket >();	
}

#endif

CBehaviorGraphVariableOutputSocket* CBehaviorGraphVariableInputSocket::GetConnectedSocket() const
{	
	if ( HasConnections() )
	{
		CGraphConnection* connection = m_connections[0];
		if( connection->GetDestination() )
		{
			return SafeCast< CBehaviorGraphVariableOutputSocket >( connection->GetDestination() );
		}		
	}

	return NULL;
}

CBehaviorGraphValueNode* CBehaviorGraphVariableInputSocket::GetConnectedNode() const
{
	CBehaviorGraphVariableOutputSocket *connectedSocket = GetConnectedSocket();
	if ( connectedSocket )
	{
		return SafeCast< CBehaviorGraphValueNode >( connectedSocket->GetBlock() );
	}

	return NULL;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphVariableInputSocketSpawnInfo::CBehaviorGraphVariableInputSocketSpawnInfo( const CName& name, Bool visible /* = true  */ )
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphVariableInputSocket >() )
{
	m_color = Color( 128, 128, 255 );
	m_name = name;

	m_direction = LSD_Input;
	m_placement = LSP_Right;

	m_isVisible = visible;
	m_isVisibleByDefault = visible;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVariableOutputSocket );

Bool CBehaviorGraphVariableOutputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphVariableInputSocket >();	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphVariableOutputSocketSpawnInfo::CBehaviorGraphVariableOutputSocketSpawnInfo(const CName& name, bool visible /* = true  */)
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphVariableOutputSocket >() )
{
	m_color = Color( 128, 128, 255 );
	m_name = name;

	m_direction = LSD_Output;
	m_placement = LSP_Left;

	m_isVisible = visible;
	m_isVisibleByDefault = visible;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVectorVariableInputSocket );

Bool CBehaviorGraphVectorVariableInputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphVectorVariableOutputSocket >();	
}

CBehaviorGraphVectorVariableOutputSocket* CBehaviorGraphVectorVariableInputSocket::GetConnectedSocket() const
{	
	if ( HasConnections() )
	{
		CGraphConnection* connection = m_connections[0];
		return SafeCast< CBehaviorGraphVectorVariableOutputSocket >( connection->GetDestination() );
	}

	return NULL;
}

CBehaviorGraphVectorValueNode* CBehaviorGraphVectorVariableInputSocket::GetConnectedNode() const
{
	CBehaviorGraphVectorVariableOutputSocket *connectedSocket = GetConnectedSocket();
	if ( connectedSocket )
	{
		return SafeCast< CBehaviorGraphVectorValueNode >( connectedSocket->GetBlock() );
	}

	return NULL;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphVectorVariableInputSocketSpawnInfo::CBehaviorGraphVectorVariableInputSocketSpawnInfo( const CName& name, Bool visible /* = true  */ )
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphVectorVariableInputSocket >() )
{
	m_color = Color( 128, 255, 255 );
	m_name = name;

	m_direction = LSD_Input;
	m_placement = LSP_Right;

	m_isVisible = visible;
	m_isVisibleByDefault = visible;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphVectorVariableOutputSocket );

Bool CBehaviorGraphVectorVariableOutputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphVectorVariableInputSocket >();	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphVectorVariableOutputSocketSpawnInfo::CBehaviorGraphVectorVariableOutputSocketSpawnInfo(const CName& name, bool visible /* = true  */)
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphVectorVariableOutputSocket >() )
{
	m_color = Color( 128, 255, 255 );
	m_name = name;

	m_direction = LSD_Output;
	m_placement = LSP_Left;

	m_isVisible = visible;
	m_isVisibleByDefault = visible;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateOutSocket );

Bool CBehaviorGraphStateOutSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphStateInSocket >() ||
		   otherSocket->IsA< CBehaviorGraphTransitionSocket>();
}

Int32 CBehaviorGraphStateOutSocket::GetNumConnectedTransitions() const
{
	return m_connections.Size();
}

CBehaviorGraphStateTransitionNode* CBehaviorGraphStateOutSocket::GetConnectedTransition( Uint32 index ) const
{
	CBehaviorGraphTransitionSocket *socket = SafeCast< CBehaviorGraphTransitionSocket >( m_connections[ index ]->GetDestination() );
	RED_ASSERT( socket );
	if( !socket )
	{
		return nullptr;
	}

	return Cast< CBehaviorGraphStateTransitionNode >( socket->GetBlock() );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphStateOutSocketSpawnInfo::CBehaviorGraphStateOutSocketSpawnInfo( const CName& name, Bool visible )
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphStateOutSocket >() )
{
	m_color = Color( 255, 128, 64 );
	m_name = name;

	m_direction = LSD_Output;
	m_placement = LSP_Center;
	m_isNoDraw = true;
	m_forceDrawConnections = false;
	m_isMultiLink = true;
	m_isVisible = visible;
	m_isVisibleByDefault = visible;		
	m_captionHidden = true;
}

#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateInSocket );

Bool CBehaviorGraphStateInSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphStateOutSocket >() ||
		   otherSocket->IsA< CBehaviorGraphTransitionSocket>();

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphStateInSocketSpawnInfo::CBehaviorGraphStateInSocketSpawnInfo( const CName& name, Bool visible )
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphStateInSocket >() )
{
	m_color = Color( 255, 128, 64 );
	m_name = name;

	m_direction = LSD_Input;
	m_placement = LSP_Center;	
	m_isNoDraw = true;
	m_isMultiLink = true;
	m_isVisible = visible;
	m_isVisibleByDefault = visible;		
	m_captionHidden = true;
	m_canStartLink = false;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphTransitionSocket );

Bool CBehaviorGraphTransitionSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphStateInSocket >() ||
		   otherSocket->IsA< CBehaviorGraphStateOutSocket>();	
}

CBehaviorGraphStateNode* CBehaviorGraphTransitionSocket::GetConnectedNode() const
{
	CGraphSocket *connectedSocket = NULL;
	if ( HasConnections() )
	{
		CGraphConnection* connection = m_connections[0];
		connectedSocket = SafeCast< CGraphSocket >( connection->GetDestination() );
	}

	if ( connectedSocket )
	{
		return SafeCast< CBehaviorGraphStateNode >( connectedSocket->GetBlock() );
	}

	return NULL;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphTransitionSocketSpawnInfo::CBehaviorGraphTransitionSocketSpawnInfo( const CName& name, ELinkedSocketDirection direction )
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphTransitionSocket >() )
{
	m_color = Color( 255, 128, 64 );
	m_name = name;

	m_direction = direction;
	//m_placement = direction == LSD_Input ? LSP_Left : LSP_Right;	//< temp!
	m_placement = LSP_Center;
	m_isNoDraw = true;
	m_forceDrawConnections = false;
	m_canStartLink = false;
	m_captionHidden = true;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicAnimationInputSocket );

Bool CBehaviorGraphMimicAnimationInputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphMimicAnimationOutputSocket >();	
}

CBehaviorGraphMimicAnimationOutputSocket* CBehaviorGraphMimicAnimationInputSocket::GetConnectedSocket() const
{	
	if ( HasConnections() )
	{
		CGraphConnection* connection = m_connections[0];
		return SafeCast< CBehaviorGraphMimicAnimationOutputSocket >( connection->GetDestination() );
	}

	return NULL;
}

CBehaviorGraphNode* CBehaviorGraphMimicAnimationInputSocket::GetConnectedNode() const
{
	CBehaviorGraphMimicAnimationOutputSocket *connectedSocket = GetConnectedSocket();
	if ( connectedSocket )
	{
		return SafeCast< CBehaviorGraphNode >( connectedSocket->GetBlock() );
	}

	return NULL;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphMimicAnimationInputSocketSpawnInfo::CBehaviorGraphMimicAnimationInputSocketSpawnInfo( const CName& name, Bool visible /* = true  */ )
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphMimicAnimationInputSocket >() )
{
	m_color = Color( 128, 0, 128 );
	m_name = name;

	m_direction = LSD_Input;
	m_placement = LSP_Right;

	m_isVisible = visible;
	m_isVisibleByDefault = visible;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicAnimationOutputSocket );

Bool CBehaviorGraphMimicAnimationOutputSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return otherSocket->IsA< CBehaviorGraphMimicAnimationInputSocket >();	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CBehaviorGraphMimicAnimationOutputSocketSpawnInfo::CBehaviorGraphMimicAnimationOutputSocketSpawnInfo(const CName& name, bool visible /* = true  */)
	: GraphSocketSpawnInfo( ClassID< CBehaviorGraphMimicAnimationOutputSocket >() )
{
	m_color = Color( 128, 0, 128 );
	m_name = name;

	m_direction = LSD_Output;
	m_placement = LSP_Left;

	m_isVisible = visible;
	m_isVisibleByDefault = visible;
}

#endif