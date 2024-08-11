#include "build.h"
#include "questDeletionMarkerBlock.h"
#include "questGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/graphSocket.h"
#include "../engine/graphConnection.h"


IMPLEMENT_ENGINE_CLASS( CQuestDeletionMarkerBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestDeletionMarkerBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

void CQuestDeletionMarkerBlock::OnPasted( Bool wasCopied )
{
	TBaseClass::OnPasted( wasCopied );
	// remove all guids
	m_guids.Clear();
}

void CQuestDeletionMarkerBlock::AddBlock( const CQuestGraphBlock* block )
{
	block->GetGUIDs( m_guids );
	SetPosition( block->GetPosition() );
}

void CQuestDeletionMarkerBlock::AddConnection( CGraphSocket* socket, CGraphConnection* connection )
{
	if ( !socket || !connection )
	{
		ASSERT( false && "Invalid quest connection specified" );
		return;
	}

	CGraphSocket* otherSocket = connection->GetSource();
	if ( otherSocket == socket )
	{
		otherSocket = connection->GetDestination();
	}

	// reroute the connection
	if ( socket->GetDirection() == LSD_Output )
	{
		// this is an outgoing connection
		CGraphSocket* output = FindSocket( CNAME( Out ) );
		output->ConnectTo( otherSocket );
	}
	else if ( socket->GetDirection() == LSD_Input )
	{
		// this is an incoming connection
		CGraphSocket* input = FindSocket( CNAME( In ) );
		input->ConnectTo( otherSocket );
	}
	else
	{
		ASSERT( false && "The connection doesn't connect the specified socket" );
	}
}

#endif

Bool CQuestDeletionMarkerBlock::MatchesGUID( const CGUID& guid ) const
{
	if ( TBaseClass::MatchesGUID( guid ) )
	{
		return true;
	}

	for ( TDynArray< CGUID >::const_iterator it = m_guids.Begin(); it != m_guids.End(); ++it )
	{
		if ( *it == guid )
		{
			return true;
		}
	}

	return false;
}

void CQuestDeletionMarkerBlock::GetGUIDs( TDynArray< CGUID >& outGUIDs ) const 
{
	TBaseClass::GetGUIDs( outGUIDs );
	for ( TDynArray< CGUID >::const_iterator it = m_guids.Begin(); it != m_guids.End(); ++it )
	{
		outGUIDs.PushBack( *it );
	}
}

void CQuestDeletionMarkerBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
	ActivateOutput( data, CNAME( Out ) );
}
