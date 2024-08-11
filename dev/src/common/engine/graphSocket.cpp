/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "graphSocket.h"
#include "graphConnection.h"
#include "graphBlock.h"


IMPLEMENT_ENGINE_CLASS( CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

IMPLEMENT_RTTI_ENUM( ELinkedSocketPlacement );
IMPLEMENT_RTTI_ENUM( ELinkedSocketDirection );
IMPLEMENT_RTTI_ENUM( ELinkedSocketDrawStyle );

CGraphSocket::CGraphSocket()
{
}

CGraphSocket::~CGraphSocket()
{
	m_connections.ClearPtr();
}

ISerializable* CGraphSocket::GetSerializationParent() const
{
	return m_block;
}

void CGraphSocket::RestoreSerializationParent( ISerializable* parent )
{
	RED_ASSERT( parent != nullptr, TXT("Trying to parent CGraphSocket to null parent") );
	RED_ASSERT( m_block == nullptr, TXT("Trying to change parent of already parented socket") );
	if ( m_block != nullptr )
	{
		RED_ASSERT( parent->IsA< CGraphBlock >(), TXT("Trying to parent socket to '%ls'"), parent->GetClass()->GetName().AsChar() );
		m_block = Cast< CGraphBlock >( parent );
	}
}

Color CGraphSocket::GetLinkColor() const
{
	return Color::BLACK;
}

Bool CGraphSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	return true;
}

CGraphConnection* CGraphSocket::ConnectTo( CGraphSocket *socket, Bool active )
{
	ASSERT( socket, TXT( "Cannot connect '%ls'->'%ls' to NULL socket" ), GetBlock()->GetBlockName().AsChar(), GetName().AsString().AsChar() );

	// Connect only if not connected
	if ( !HasConnectionsTo( socket, true ) )
	{
		// Show socket
		SetSocketVisibility( true );

		// Build local connection
		CGraphConnection* connection = new CGraphConnection( this, socket );
		connection->SetActive( active );

		// Add to connection list
		m_connections.PushBack( connection );

		// Build remote connection
		socket->ConnectTo( this, active );

		// Report connection
		OnConnectionCreated( connection );
		return connection;
	}

	return NULL;
}

void CGraphSocket::DisconnectFrom( CGraphSocket *socket )
{
	ASSERT( socket );

	// Search for link to break
	for ( Uint32 i=0; i<m_connections.Size(); i++ )
	{
		CGraphConnection* connection = m_connections[i];
		if ( connection->GetDestination( true ) == socket )
		{
			// Remove from list of connections on this side
			m_connections.Remove( connection );

			// Info
			OnConnectionBroken( connection );

			// Disconnect other side
			socket->DisconnectFrom( this );

			// Cleanup connection
			delete connection;
		}
	}
}

void CGraphSocket::BreakAllLinks()
{
	for ( Int32 i = (Int32)m_connections.Size()-1; i>=0; --i )
	{
		CGraphConnection* connection = m_connections[i];
		DisconnectFrom( connection->GetDestination( true ) );
	}
}

void CGraphSocket::BreakAllLinksTo( CGraphBlock *block )
{
	ASSERT( block );

	// Search for link to break
	for ( Int32 i = (Int32)m_connections.Size()-1; i>=0; --i )
	{
		CGraphConnection* connection = m_connections[i];
		if ( connection->GetDestination( true ) && connection->GetDestination( true )->GetBlock() == block )
		{
			DisconnectFrom( connection->GetDestination( true ) );
		}
	}
}

#endif

Bool CGraphSocket::HasConnections( Bool includeHidden ) const
{
	if ( includeHidden )
	{
		return m_connections.Size() > 0;
	}
	else
	{
		for (Uint32 i=0; i<m_connections.Size(); i++)
		{
			if (m_connections[i]->IsActive())
			{
				return true;
			}
		}

		return false;
	}
}

Bool CGraphSocket::HasConnectionsTo( CGraphSocket *socket, Bool includeHidden ) const
{
	if ( includeHidden )
	{
		for ( Uint32 i=0; i<m_connections.Size(); i++ )
		{
			CGraphConnection* connection = m_connections[i];
			if ( connection->GetDestination( true ) == socket )
			{
				return true;
			}
		}

		return false;
	}
	else
	{
		for ( Uint32 i=0; i<m_connections.Size(); i++ )
		{
			CGraphConnection* connection = m_connections[i];
			if ( connection->GetDestination() == socket && connection->IsActive())
			{
				return true;
			}
		}

		return false;
	}
}

Bool CGraphSocket::HasConnectionsTo( CGraphBlock *block, Bool includeHidden ) const
{
	if ( includeHidden )
	{
		for ( Uint32 i=0; i<m_connections.Size(); i++ )
		{
			CGraphConnection* connection = m_connections[i];
			if ( connection->GetDestination( true ) && connection->GetDestination( true )->GetBlock() == block )
			{
				return true;
			}
		}

		return false;
	}
	else
	{
		for ( Uint32 i=0; i<m_connections.Size(); i++ )
		{
			CGraphConnection* connection = m_connections[i];
			if ( connection->GetDestination() && connection->GetDestination()->GetBlock() == block )
			{
				return true;
			}
		}

		return false;
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CGraphSocket::ShouldDraw() const
{
	// Draw socket if visible
	return IsVisible();
}

void CGraphSocket::SetCaption( const String &caption )
{
	m_caption = caption;
}

void CGraphSocket::GetCaption( String& caption ) const
{
	// Use existing string
	if ( !m_caption.Empty() )
	{
		caption = m_caption;
		return;
	}

	// Use name
	caption = m_name.AsString();
}

void CGraphSocket::SetSocketVisibility( Bool visible )
{
	// Make sure we can hide this socket
	if ( !visible && !CanHide() )
	{
		WARN_ENGINE( TXT("Trying to hide unhidable socket '%ls' in '%ls'"), m_name.AsString().AsChar(), GetBlock()->GetCaption().AsChar() );
		return;
	}

	// We are hidding this socket, disconnect all links
	if ( !visible )
	{
		BreakAllLinks();
	}

	// Change visibility status
	if ( visible )
	{
		m_flags |= LSF_Visible;
		OnShow();
	}
	else
	{
		m_flags &= ~LSF_Visible;
		OnHide();
	}

	// Invalidate block layout
	GetBlock()->InvalidateLayout();
}

#endif

void CGraphSocket::BindToBlock( CGraphBlock* block )
{
	ASSERT( block != NULL );
	m_block = block;
}

void CGraphSocket::OnPostLoad()
{
	ISerializable::OnPostLoad();

	// remove any connections pointing to invalid blocks
	CleanupConnections();
}

void CGraphSocket::CleanupConnections()
{
	TDynArray< CGraphConnection* > currentConnections;
	Swap( currentConnections, m_connections );

	// Remove connections that points to nothing
	for ( Uint32 i=0; i<currentConnections.Size(); i++ )
	{
		CGraphConnection* connection = currentConnections[i];
		if ( connection && connection->GetSource( true ) == this && connection->GetDestination( true ) && connection->GetDestination( true )->GetBlock() )
		{
			m_connections.PushBack( connection );
		}
		else
		{
			WARN_ENGINE( TXT("Graph connection in socket '%ls' in '%ls' removed because it was pointing to NULL"),
				GetName().AsChar(), GetBlock() ? GetBlock()->GetCaption().AsChar() : TXT("Unknown") );
		}
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CGraphSocket::OnSpawned( class CGraphBlock* block, const class GraphSocketSpawnInfo& info )
{
	ASSERT( GetClass() == info.GetClass() );

	// Setup graph block link
	ASSERT( block != NULL );
	ASSERT( m_block == NULL );
	m_block = block;

	// Setup socket params
	m_name = info.m_name;
	m_color = info.m_color;
	m_direction = info.m_direction;
	m_placement = info.m_placement;
	m_drawStyle = info.m_drawStyle;

	// Assemble flags
	m_flags = info.m_canHide ? LSF_CanHide : 0;
	m_flags |= info.m_isVisible ? LSF_Visible : 0;
	m_flags |= info.m_isVisibleByDefault ? LSF_VisibleByDefault : 0;
	m_flags |= info.m_isMultiLink ? LSF_MultiLinks : 0;
	m_flags |= info.m_isNoDraw ? LSF_NoDraw : 0;
	m_flags |= info.m_canStartLink ? LSF_CanStartLink : 0;
	m_flags |= info.m_canEndLink ? LSF_CanEndLink : 0;
	m_flags |= info.m_captionHidden ? LSF_CaptionHidden : 0;
	m_flags |= info.m_forceDrawConnections ? LSF_ForceDrawConnections : 0;
}

void CGraphSocket::OnDestroyed()
{
}

void CGraphSocket::OnHide()
{
}

void CGraphSocket::OnShow()
{
}

void CGraphSocket::OnConnectionCreated( CGraphConnection* connection )
{
}

void CGraphSocket::OnConnectionBroken( CGraphConnection* connection )
{
}

#endif