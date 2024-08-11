/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "graphBlock.h"
#include "graphSocket.h"
#include "graphConnection.h"

IMPLEMENT_ENGINE_CLASS( CGraphBlock );

#ifndef NO_EDITOR_GRAPH_SUPPORT

GraphSocketSpawnInfo::GraphSocketSpawnInfo( CClass* socketClass )
	: m_class( socketClass )
	, m_name( CNAME( Socket ) )
	, m_color( Color::BLACK )
	, m_isVisible( true )
	, m_isVisibleByDefault( true )
	, m_isMultiLink( false )
	, m_isNoDraw( false )
	, m_canHide( true )
	, m_canStartLink( true )
	, m_canEndLink( true )
	, m_captionHidden( false )
	, m_forceDrawConnections( false )
	, m_placement( LSP_Left )
	, m_direction( LSD_Input )
	, m_drawStyle( LSDS_Default )
{};

#endif

CGraphBlock::CGraphBlock()
{
}

CGraphBlock::~CGraphBlock()
{
	m_sockets.ClearPtr();
}

String CGraphBlock::GetBlockName() const
{
	return GetClass()->GetName().AsString();
}

String CGraphBlock::GetCaption() const
{
	return GetBlockName();
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CGraphBlock::GetBlockCategory() const
{
	return TXT("Misc");
}

EGraphBlockShape CGraphBlock::GetBlockShape() const
{
	return GBS_Default;
}

EGraphBlockDepthGroup CGraphBlock::GetBlockDepthGroup() const
{
	return GBDG_Background;
}

String CGraphBlock::GetDisplayValue() const
{
	return String::EMPTY;
}

Color CGraphBlock::GetBorderColor() const
{
	return Color::BLACK;
}

Color CGraphBlock::GetClientColor() const
{
	return HasFlag( OF_Highlighted ) 
		? Color::YELLOW : Color( 142, 142, 142 );
}

Color CGraphBlock::GetTitleColor() const
{
	return HasFlag( OF_Highlighted ) 
		? Color::YELLOW : Color( 255, 128, 64 );
}

Bool CGraphBlock::IsActivated() const
{
	return false;
}

Float CGraphBlock::GetActivationAlpha() const
{
	return 0.0f;
}

void CGraphBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Update editor layout
	m_needsLayoutUpdate = true;
}

void CGraphBlock::OnSpawned( const GraphBlockSpawnInfo& info )
{
	// Set initial position
	m_position = info.m_position;

	// Set layer
	m_layerFlags = info.m_layers;

	// Request layout update
	InvalidateLayout();
}

void CGraphBlock::OnPasted( Bool wasCopied )
{

}

void CGraphBlock::OnDestroyed()
{
	// Graph block was destroyed
}

void CGraphBlock::OnRebuildSockets()
{
}

void CGraphBlock::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// rebind sockets (we changed the socket from CObject to ISerializable so it's lost it's parent link)
	for ( Uint32 i=0; i<m_sockets.Size(); ++i )
	{
		CGraphSocket* socket = m_sockets[i];
		if ( NULL != socket )
		{
			socket->BindToBlock( this );
		}
	}
}

void CGraphBlock::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	if ( m_layerFlags <= 0 )
	{
		// Set first layer as default
		m_layerFlags = 1<<0;
	}
}

Uint32 CGraphBlock::GetClassVersion() const
{
	CGraphBlock* defaultBlock = GetClass()->GetDefaultObject< CGraphBlock >();
	Uint32 version = defaultBlock->GetVersion();
	return version;
}

Bool CGraphBlock::IsObsolete() const
{
	return GetClassVersion() > m_version;
}

Bool CGraphBlock::UpdateVersion()
{
	const Uint32 classVersion = GetClassVersion();
	if ( 1 )//m_version != classVersion )
	{
		// Rebuild sockets
		OnRebuildSockets();

		// Bump version
		m_version = classVersion;
	}

	// Updated
	return true;
}

void CGraphBlock::ShowSocket( CGraphSocket *socket )
{
	// Hide sockets that can be hidden and have no connections
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		if ( socket == m_sockets[ i ] )
		{
			socket->SetSocketVisibility( true );
			return;
		}
	}
}

void CGraphBlock::HideSocket( CGraphSocket *socket )
{
	// Hide sockets that can be hidden and have no connections
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		if ( socket == m_sockets[ i ] )
		{
			socket->SetSocketVisibility( false );
			return;
		}
	}
}

TDynArray< CGraphSocket* > CGraphBlock::HideUnusedSockets()
{
	TDynArray< CGraphSocket* > hiddenSockets;

	// Hide sockets that can be hidden and have no connections
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		CGraphSocket *socket = m_sockets[ i ];
		if ( socket->CanHide() && !socket->HasConnections() )
		{
			if ( socket->IsVisible() )
			{
				hiddenSockets.PushBack( socket );
				socket->SetSocketVisibility( false );
			}
		}
	}

	return hiddenSockets;
}

TDynArray< CGraphSocket* > CGraphBlock::ShowAllSockets()
{
	TDynArray< CGraphSocket* > shownSockets;

	// Restore default visibility settings for all sockets
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		CGraphSocket *socket = m_sockets[ i ];
		if ( socket->IsVisibleByDefault() )
		{
			if ( !socket->IsVisible() )
			{
				shownSockets.PushBack( socket );
				socket->SetSocketVisibility( true );
			}
		}
	}

	return shownSockets;
}

void CGraphBlock::InvalidateLayout()
{
	m_needsLayoutUpdate = true;
}

CGraphSocket* CGraphBlock::CreateSocket( const GraphSocketSpawnInfo& info )
{
	ASSERT( info.GetClass() );

	// Create socket
	CGraphSocket* socket = info.GetClass()->CreateObject< CGraphSocket>();
	m_sockets.PushBack( socket );

	// Initialize
	socket->OnSpawned( this, info );
	return socket;
}

void CGraphBlock::RemoveSocket( CGraphSocket* socket )
{
	ASSERT( socket );

	// Find and remove socket
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{		
		if ( m_sockets[i] == socket )
		{
			socket->OnDestroyed();
			socket->BreakAllLinks();
			m_sockets.Remove( socket );
			delete socket;
			break;
		}
	}
}

void CGraphBlock::RemoveAllSockets()
{
	// Break all links
	BreakAllLinks();

	// Clear array
	m_sockets.ClearPtr();

	// Update layout
	InvalidateLayout();
}

void CGraphBlock::BreakAllLinks()
{
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		CGraphSocket *socket = m_sockets[i];
		socket->BreakAllLinks();
	}
}

#endif

CGraphSocket* CGraphBlock::FindSocket( const String& name ) const
{
	return FindSocket( CName( name ) );
}

CGraphSocket* CGraphBlock::FindSocket( const CName& name ) const
{
	// Linear search :P
	for ( Uint32 i=0; i<m_sockets.Size(); i++ )
	{
		CGraphSocket *socket = m_sockets[ i ];
		if ( socket->GetName() == name )
		{
			return socket;
		}
	}

	// Not found
	return NULL;	
}
#ifndef NO_EDITOR_GRAPH_SUPPORT
void CGraphBlock::GetChildNodesRecursively(TDynArray<CGraphBlock* > &children)
{
	// Collect sockets
	TDynArray< CGraphSocket* > socks = GetSockets();
	for ( Uint32 i = 0; i < socks.Size(); ++i )
	{
		if ( socks[i]->GetDirection() == LSD_Output )
		{
			TDynArray< CGraphConnection* > conns = socks[i]->GetConnections();
			if ( !conns.Empty() )
			{
				for ( Uint32 j =0; j< conns.Size(); ++j )
				{
					CGraphBlock *block;

					if ( this == conns[j]->GetDestination( true )->GetBlock() )
					{
						block = conns[j]->GetSource( true )->GetBlock();
					}
					else 
					{
						block = conns[j]->GetDestination( true )->GetBlock();
					}

					// This will also prevent this check to go into a loop
					if ( children.PushBackUnique( block ) )
					{
						block->GetChildNodesRecursively( children );
					}
				}
			}
		}
	}
}
#endif
