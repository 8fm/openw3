/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idBasicBlocks.h"
#include "idGraphBlockBranch.h"
#include "idGraphBlockChoice.h"
#include "../../common/engine/graphConnection.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphSocket )
IMPLEMENT_ENGINE_CLASS( CIDGraph )

CIDGraph::CIDGraph(void)
{
}

CObject *CIDGraph::GraphGetOwner()
{
	return GetParent();
}

Vector CIDGraph::GraphGetBackgroundOffset() const
{
	return m_backgroundOffset;
}

void CIDGraph::GraphSetBackgroundOffset( const Vector& offset )
{
	m_backgroundOffset = offset;
}

void CIDGraph::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const */
{
	for ( Uint32 i = 0; i < m_graphBlocks.Size(); ++i )
	{
		CIDGraphBlock *block = Cast< CIDGraphBlock > ( m_graphBlocks[ i ] );
		R6_ASSERT( block );

		block->GetLocalizedStrings( localizedStrings );
	}
}

CIDGraphBlock* CIDGraph::GetLastBlockByXPosition() const
{
	Float maxx = -FLT_MAX;
	CGraphBlock* pick( nullptr );
	for ( Uint32 i = 0; i < m_graphBlocks.Size(); ++i )
	{
		const Float pos = m_graphBlocks[ i ]->GetPosition().X;
		if ( pos > maxx )
		{
			maxx = pos;
			pick = m_graphBlocks[ i ];
		}
	}
	return Cast< CIDGraphBlock > ( pick );
}

Bool CIDGraphSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	if ( CGraphSocket::CanConnectTo( otherSocket ) && otherSocket->IsA< CIDGraphSocket >() && otherSocket->GetBlock() != GetBlock() )
	{
		if ( GetDirection() == LSD_Input )
		{
			// reverse direction and re-check
			return otherSocket->CanConnectTo( this );
		}
		else if ( GetDirection() == LSD_Output )
		{
			// we can connect outputs only to inputs
			if ( otherSocket->GetDirection() != LSD_Input )
			{
				return false;
			}

			// ok, this socket is LSD_Output, and the other one is LSD_Input
			// from one output you can go to one block of any kind + infinite number of branch/choice blocks
			
			if ( !( Cast< CIDGraphBlock > ( otherSocket->GetBlock() )->IsRegular() ) )
			{
				return true;
			}
			else
			{
				Bool nonBranchFound( false );
				CGraphSocket* socket;

				// it's not a branch we're connecting into, so let's check if this output is already connected to some non-branch block
				for ( Uint32 i = 0; i < m_connections.Size(); ++i )
				{
					socket = m_connections[ i ]->GetDestination();
					R6_ASSERT( socket && socket->GetDirection() == LSD_Input );

					nonBranchFound |= ( Cast< CIDGraphBlock > ( socket->GetBlock() )->IsRegular() );
				}

				return !nonBranchFound;
			}
		}
	}

	return false;
}

void CIDGraphSocket::OnConnectionCreated( CGraphConnection* connection )
{
	R6_ASSERT( connection );
	R6_ASSERT( connection->GetSource() == this );

	// Disconnect previous connection
	if ( connection->GetSource() == this && IsMultiLink() == false )
	{
		if ( GetConnections().Size() > 1 )
		{
			DisconnectFrom( GetConnections()[ 0 ]->GetDestination( true ) );
		}
	}
}