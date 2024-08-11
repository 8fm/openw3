/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "graphConnectionRebuilder.h"
#include "graphConnection.h"
#include "graphBlock.h"
#include "graphSocket.h"

#ifndef NO_EDITOR_GRAPH_SUPPORT

GraphConnectionRebuilder::GraphConnectionRebuilder( CGraphBlock* block )
	: m_block( block )
{
	ASSERT( block );

	// Grab connections
	for ( SocketIterator< CGraphSocket > i( block ); i; ++i )
	{
		StoreSocket( *i, i->GetName() );
	}

	// Prepare block for recreating sockets
	block->RemoveAllSockets();
}

GraphConnectionRebuilder::GraphConnectionRebuilder( CGraphBlock* block, TRenameMap* map )
	: m_block( block )
{
	ASSERT( block );

	// Grab connections
	for ( SocketIterator< CGraphSocket > i( block ); i; ++i )
	{
		CName name = i->GetName();
		if ( map )
		{
			TRenameMap::iterator mapiter = map->Find(name);
			if ( mapiter != map->End() )
			{
				name = mapiter->m_second;
			}

			StoreSocket( *i, name );
		}
	}

	// Prepare block for recreating sockets
	block->RemoveAllSockets();
}

void GraphConnectionRebuilder::StoreSocket( CGraphSocket* socket, const CName& socketName )
{
	// Create socket visibility info if socket current one defaults from the default
	if ( socket->IsVisible() != socket->IsVisibleByDefault() )
	{
		m_sockets.PushBack( Socket( socket->GetName(), socket->IsVisible() ) );
	}

	// Grab connections
	const TDynArray< CGraphConnection* >& connections = socket->GetConnections();
	
	for ( Uint32 i = 0; i < connections.Size(); ++i )
	{
		CGraphConnection* connection = connections[ i ];

		CGraphSocket* destination = connection->GetDestination( true );

		if ( destination )
		{
			m_connections.PushBack( Connection( socketName, destination->GetName(), destination->GetBlock(), connection->IsActive() ) );
		}
	}
}

void GraphConnectionRebuilder::RestoreConnections( CGraphBlock* block ) const
{
	// Restore connections
	for ( Uint32 i=0; i<m_connections.Size(); i++ )
	{
		const Connection& con = m_connections[i];		

		// Get source socket
		CGraphSocket* srcSocket = block->FindSocket( con.m_source );
		if ( srcSocket )
		{
			// Get destination socket
			CGraphSocket* destSocket = con.m_block->FindSocket( con.m_destination );
			if ( destSocket )
			{
				// If not already connected
				if ( !srcSocket->HasConnectionsTo( destSocket ) )
				{
					// Try to connect
					if ( destSocket->CanConnectTo( srcSocket ) && srcSocket->CanConnectTo( destSocket ) )
					{
						// Make sure socket with connections is visible
						if ( !srcSocket->IsVisible() )
						{
							srcSocket->SetSocketVisibility( true );
						}

						// Make sure socket with connections is visible
						if ( !destSocket->IsVisible() )
						{
							destSocket->SetSocketVisibility( true );
						}

						// Connect !
						srcSocket->ConnectTo( destSocket, con.m_active );
					}
				}
			}
		}
	}

	// Restore socket visibility
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		const Socket& socket = m_sockets[i];
		CGraphSocket* srcSocket = block->FindSocket( socket.m_name );
		if ( srcSocket )
		{
			ASSERT( !( !socket.m_isVisible && srcSocket->HasConnections() ) );

			// Override socket visibility			
			srcSocket->SetSocketVisibility( socket.m_isVisible );
		}
	}

	block->InvalidateLayout();
}

GraphConnectionRebuilder::~GraphConnectionRebuilder()
{
	if ( m_block )
	{
		RestoreConnections( m_block );
	}
}

//////////////////////////////////////////////////////////////////////////

GraphConnectionRelinker::GraphConnectionRelinker( CGraphBlock* from, CGraphBlock* to )
	: GraphConnectionRebuilder( from )
	, m_destBlock( to )
{
	m_block = NULL;
}

GraphConnectionRelinker::~GraphConnectionRelinker()
{
	RestoreConnections( m_destBlock );
}

#endif
