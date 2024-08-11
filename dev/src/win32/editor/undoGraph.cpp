/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#include "build.h"
#include "undoGraph.h"
#include "undoProperty.h"
#include "../../common/engine/graphConnection.h"
#include "../../common/engine/graphContainer.h"

CUndoGraphConnectionInfo::CUndoGraphConnectionInfo( CGraphConnection* con )
	: m_source( con->GetSource( true ) )
	, m_dest( con->GetDestination( true ) )
	, m_active( con->IsActive() )
	{}

void CUndoGraphConnectionInfo::DoStep( Bool creating ) const
{
	ASSERT( m_source.Get() && m_dest.Get() );

	if ( m_source.Get() && m_dest.Get() )
	{
		if ( creating )
		{
			m_source.Get()->ConnectTo( m_dest.Get(), m_active );
		}
		else
		{
			m_source.Get()->DisconnectFrom( m_dest.Get() );
		}
	}
}

// ----------------------------------------

CUndoGraphSocketInfo::CUndoGraphSocketInfo( CGraphSocket* socket )
	: m_socket( socket )
{
	TDynArray< CGraphConnection* > connections = socket->GetConnections();

	for ( Uint32 i=0; i<connections.Size(); ++i )
	{
		m_connections.PushBack( CUndoGraphConnectionInfo( connections[i] ) );
	}
}

void CUndoGraphSocketInfo::RestoreConnections() const
{
	for ( Uint32 j=0; j<m_connections.Size(); ++j )
	{
		m_connections[j].DoStep( true );
	}
}

//-------------------

IMPLEMENT_ENGINE_CLASS( CUndoGraphBlockExistance );

CUndoGraphBlockExistance::CUndoGraphBlockExistance( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor )
	: CUndoGraphStep( undoManager, graphEditor ) // <- here the UndoManager becomes the parent of the step
{
}

/*static*/
CUndoGraphBlockExistance* CUndoGraphBlockExistance::PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor )
{
	CUndoGraphBlockExistance* step = undoManager.SafeGetStepToAdd< CUndoGraphBlockExistance >();

	if ( !step )
	{
		step = new CUndoGraphBlockExistance( undoManager, graphEditor );
		undoManager.SetStepToAdd( step );
	}

	return step;
}

/*static*/
void CUndoGraphBlockExistance::PrepareCreationStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block )
{
	CUndoGraphBlockExistance* step = PrepareStep( undoManager, graphEditor );
	step->DoPrepareCreationStep( block );
}

/*static*/
void CUndoGraphBlockExistance::PrepareDeletionStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block )
{
	CUndoGraphBlockExistance* step = PrepareStep( undoManager, graphEditor );
	step->DoPrepareDeletionStep( block );
}

void CUndoGraphBlockExistance::DoPrepareCreationStep( CGraphBlock* block )
{
	Info info( block, block->GetParent() );
	m_createdBlocks.PushBack( info );
}

void CUndoGraphBlockExistance::DoPrepareDeletionStep( CGraphBlock* block )
{
	Info info( block, block->GetParent() );
	block->SetParent( this );
	StoreConnections( info );
	m_deletedBlocks.PushBack( info );
}

/*static*/
void CUndoGraphBlockExistance::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoGraphBlockExistance* step = undoManager.SafeGetStepToAdd< CUndoGraphBlockExistance >() )
	{
		// Store the step in the undo queue
		step->PushStep();
	}
}

void CUndoGraphBlockExistance::DoCreationOn( TDynArray< Info >& infos )
{
	for ( Uint32 i=0; i<infos.Size(); ++i )
	{
		if( infos[ i ].m_parent->IsA< CUndoGraphBlockExistance >() )
		{
			// this means that node was added to undo queue multiple times.
			// this is now possible, because we can delete one block multiple times if the block represents connection
			continue;
		}

		infos[i].m_block->SetParent( infos[i].m_parent );
		GetGraph()->GraphGetBlocks().PushBack( infos[i].m_block );
		RestoreConnections( infos[i] );
		infos[i].m_block->InvalidateLayout();
	}
}

void CUndoGraphBlockExistance::DoDeletionOn( TDynArray< Info >& infos )
{
	for ( Uint32 i=0; i<infos.Size(); ++i )
	{
		infos[i].m_block->SetParent( this );
		StoreConnections( infos[i] );
		GetGraph()->GraphRemoveBlock( infos[i].m_block );
	}
}

void CUndoGraphBlockExistance::StoreConnections( Info & info )
{
	ASSERT( info.m_socketConnections.Empty( ) );

	// Copy the connections aside to restore them later

	const TDynArray< CGraphSocket* > & sockets = info.m_block->GetSockets( );
	for ( Uint32 i=0; i<sockets.Size(); ++i )
	{
		CGraphSocket* mySocket = sockets[ i ];
		RED_ASSERT( mySocket );

		info.m_socketConnections.PushBack( TDynArray< CUndoGraphConnectionInfo >() );
		const TDynArray< CGraphConnection* > & connections = mySocket->GetConnections();
		for ( Uint32 i=0; i<connections.Size(); ++i )
		{
			CGraphConnection* connection = connections[ i ];
			RED_ASSERT( connection );

			CGraphSocket* otherSocket = nullptr;
			if( mySocket == connection->GetSource( true ) )
			{
				otherSocket = connection->GetDestination( true );
			}
			else
			{
				RED_ASSERT( mySocket == connection->GetDestination() );
				otherSocket = connection->GetSource( true );
			}

			CGraphBlock* otherBlock = otherSocket->GetBlock();
			Bool anotherDeletion = false;

			if( otherBlock->IsA< CBehaviorGraphStateTransitionNode >() )
			{
				if( ! otherBlock->GetParent()->IsA< CUndoGraphBlockExistance >() )
				{
					// if other block parent is undo graph block existance, it means that the block is already added.
					anotherDeletion = true;
				}
			}

			if( anotherDeletion )
			{
				DoPrepareDeletionStep( otherBlock );
			}
			else
			{
				info.m_socketConnections.Back().PushBack( CUndoGraphConnectionInfo( connections[i] ) );
			}
		}
	}
}

void CUndoGraphBlockExistance::RestoreConnections( Info & info )
{
	const TDynArray< CGraphSocket* > & sockets = info.m_block->GetSockets( );
	ASSERT( sockets.Size() == info.m_socketConnections.Size() );

	for ( Uint32 i=0; i<info.m_socketConnections.Size(); ++i )
	{
		const TDynArray< CUndoGraphConnectionInfo > & connections = info.m_socketConnections[i];
		for ( Uint32 i=0; i<connections.Size(); ++i )
		{
			connections[i].DoStep( true );
		}
	}

	// Connections are restored, the stored data is no longer needed
	info.m_socketConnections.Clear();
}


/*virtual*/
void CUndoGraphBlockExistance::DoUndo()
{
	m_graphEditor->DeselectAllBlocks();
	DoDeletionOn( m_createdBlocks );
	DoCreationOn( m_deletedBlocks );
	GraphStructureModified();
}

/*virtual*/
void CUndoGraphBlockExistance::DoRedo()
{
	m_graphEditor->DeselectAllBlocks();
	DoCreationOn( m_createdBlocks );
	DoDeletionOn( m_deletedBlocks );
	GraphStructureModified();
}

/*virtual*/
void CUndoGraphBlockExistance::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	for ( Uint32 i=0; i<m_createdBlocks.Size(); ++i )
	{
		file << m_createdBlocks[i].m_block;
		file << m_createdBlocks[i].m_parent;
	}

	for ( Uint32 i=0; i<m_deletedBlocks.Size(); ++i )
	{
		file << m_deletedBlocks[i].m_block;
		file << m_deletedBlocks[i].m_parent;
	}
}

/*virtual*/
String CUndoGraphBlockExistance::GetName()
{
	if ( !m_createdBlocks.Empty() )
	{
		if ( !m_deletedBlocks.Empty() )
		{
			return TXT("creating and removing blocks");
		}
		else
		{
			return TXT("creating blocks");
		}
	}
	else
	{
		return TXT("removing blocks");
	}
}

// ---------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGraphSocketSnaphot );

CUndoGraphSocketSnaphot::CUndoGraphSocketSnaphot( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block )
	: CUndoGraphStep( undoManager, graphEditor )
	, m_block( block )
{
	StoreSocketsFromBlock();
}

/*static*/ 
void CUndoGraphSocketSnaphot::CreateStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block )
{
	CUndoGraphSocketSnaphot* step = new CUndoGraphSocketSnaphot( undoManager, graphEditor, block );
	step->PushStep();
}

void CUndoGraphSocketSnaphot::StoreSocketsFromBlock()
{
	m_sockets.Clear();

	const TDynArray< CGraphSocket* >& sockets = m_block->GetSockets();
	for ( Uint32 i=0; i < sockets.Size(); ++i )
	{
		CUndoGraphSocketInfo info( sockets[i] );
		m_sockets.PushBack( info );
	}
}

void CUndoGraphSocketSnaphot::DoStep()
{
	TDynArray< CUndoGraphSocketInfo > socketsToRestore = m_sockets;
	StoreSocketsFromBlock();

	m_block->RemoveAllSockets();

	for ( Uint32 i=0; i < socketsToRestore.Size(); ++i )
	{
		CGraphSocket* socket = socketsToRestore[i].GetSocket();

		if ( NULL != socket )
		{
			socket->BindToBlock( m_block );
			m_block->m_sockets.PushBack( socket );

			socketsToRestore[i].RestoreConnections();
		}
	}

	GraphStructureModified();
}

/*virtual*/ 
void CUndoGraphSocketSnaphot::DoUndo()
{
	DoStep();
}

/*virtual*/ 
void CUndoGraphSocketSnaphot::DoRedo()
{
	DoStep();
}

/*virtual*/ 
String CUndoGraphSocketSnaphot::GetName()
{
	return TXT("modifying sockets");
}

/*virtual*/
void CUndoGraphSocketSnaphot::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize(file);

	//file << m_sockets;
	file << m_block;
}

// ---------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGraphConnectionExistance );

CUndoGraphConnectionExistance::CUndoGraphConnectionExistance( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor )
	: CUndoGraphStep( undoManager, graphEditor ) // <- here the UndoManager becomes the parent of the step
{
}

/*static*/ 
CUndoGraphConnectionExistance* CUndoGraphConnectionExistance::PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor )
{
	CUndoGraphConnectionExistance* step = undoManager.SafeGetStepToAdd< CUndoGraphConnectionExistance >();

	if ( !step )
	{
		step = new CUndoGraphConnectionExistance( undoManager, graphEditor );
		undoManager.SetStepToAdd( step );
	}

	return step;
}

/*static*/ 
void CUndoGraphConnectionExistance::PrepareCreationStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphConnection* connection )
{
	CUndoGraphConnectionExistance* step = PrepareStep( undoManager, graphEditor );
	step->m_created.PushBack( CUndoGraphConnectionInfo( connection ) );
}

/*static*/ 
void CUndoGraphConnectionExistance::PrepareDeletionStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphConnection* connection )
{
	CUndoGraphConnectionExistance* step = PrepareStep( undoManager, graphEditor );
	step->m_deleted.PushBack( CUndoGraphConnectionInfo( connection ) );
}

/*static*/ 
void CUndoGraphConnectionExistance::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoGraphConnectionExistance* step = undoManager.SafeGetStepToAdd< CUndoGraphConnectionExistance >() )
	{
		// Store the step in the undo queue
		step->PushStep();
	}
}

void CUndoGraphConnectionExistance::DoCreationOn( TDynArray< CUndoGraphConnectionInfo > & infos )
{
	for ( Uint32 i=0; i<infos.Size(); ++i )
	{
		infos[i].DoStep( true );
	}
}

void CUndoGraphConnectionExistance::DoDeletionOn( TDynArray< CUndoGraphConnectionInfo > & infos )
{
	for ( Uint32 i=0; i<infos.Size(); ++i )
	{
		infos[i].DoStep( false );
	}
}

/*virtual*/ 
void CUndoGraphConnectionExistance::DoUndo()
{
	DoDeletionOn( m_created );
	DoCreationOn( m_deleted );
	GraphStructureModified();
}

/*virtual*/ 
void CUndoGraphConnectionExistance::DoRedo()
{
	DoDeletionOn( m_deleted );
	DoCreationOn( m_created );
	GraphStructureModified();
}

/*virtual*/ 
String CUndoGraphConnectionExistance::GetName()
{
	if ( !m_created.Empty() )
	{
		if ( !m_deleted.Empty() )
		{
			return TXT("creating and removing connection");
		}
		else
		{
			return TXT("creating connections");
		}
	}
	else
	{
		return TXT("removing connections");
	}
}

/*virtual*/
void CUndoGraphConnectionExistance::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

//	file << m_created;
//	file << m_deleted;
}

// ---------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGraphBlockMove );

CUndoGraphBlockMove::CUndoGraphBlockMove( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor )
	: CUndoGraphStep( undoManager, graphEditor )
{
}

/*static*/
void CUndoGraphBlockMove::PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block  )
{
	CUndoGraphBlockMove* step = undoManager.SafeGetStepToAdd< CUndoGraphBlockMove >();
	if ( !step )
	{
		step = new CUndoGraphBlockMove( undoManager, graphEditor );
		undoManager.SetStepToAdd( step );
	}

	Vector oldValue = block->GetPosition();
	step->m_positions.Insert( block, oldValue );
}

/*static*/
void CUndoGraphBlockMove::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoGraphBlockMove* step = undoManager.SafeGetStepToAdd< CUndoGraphBlockMove >() )
	{
		step->PushStep();
	}
}

void CUndoGraphBlockMove::DoStep()
{
	for ( auto it = m_positions.Begin(); it != m_positions.End(); ++it )
	{
		Vector prevValue = it->m_first->GetPosition();
		it->m_first->SetPosition( it->m_second );
		it->m_second = prevValue;
	}

	GraphStructureModified();
}

/*virtual*/
void CUndoGraphBlockMove::DoUndo()
{
	DoStep();
}

/*virtual*/
void CUndoGraphBlockMove::DoRedo()
{
	DoStep();
}

String CUndoGraphBlockMove::GetName()
{
	return TXT("move block");
}

// -------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGraphBlockLayout );

CUndoGraphBlockLayout::CUndoGraphBlockLayout( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor )
	: CUndoGraphStep( undoManager, graphEditor )
{
}

/*static*/
void CUndoGraphBlockLayout::PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphBlock* block, CEdGraphEditor::BlockLayoutInfo* layout )
{
	CUndoGraphBlockLayout* step = undoManager.SafeGetStepToAdd< CUndoGraphBlockLayout >();
	if ( !step )
	{
		step = new CUndoGraphBlockLayout( undoManager, graphEditor );
		undoManager.SetStepToAdd( step );
	}

	Vector oldValue = block->GetSize();
	Info info = { layout, oldValue };
	step->m_layoutInfos.Insert( block, info );
}

/*static*/
void CUndoGraphBlockLayout::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoGraphBlockLayout* step = undoManager.SafeGetStepToAdd< CUndoGraphBlockLayout >() )
	{
		step->PushStep();
	}
}

void CUndoGraphBlockLayout::DoStep()
{
	for ( auto it = m_layoutInfos.Begin(); it != m_layoutInfos.End(); ++it )
	{
		Vector prevValue;

		prevValue = it->m_first->GetSize(); 

		CGraphBlock* block = it->m_first;
		Vector& sizeToSet = it->m_second.m_size;
		CEdGraphEditor::BlockLayoutInfo* layout = it->m_second.m_layout;

		block->SetSize( sizeToSet );
		layout->m_windowSize.x = sizeToSet.X;
		layout->m_windowSize.y = sizeToSet.Y;

		sizeToSet = prevValue;
	}

	GraphStructureModified();
}

/*virtual*/
void CUndoGraphBlockLayout::DoUndo()
{
	DoStep();
}

/*virtual*/
void CUndoGraphBlockLayout::DoRedo()
{
	DoStep();
}

String CUndoGraphBlockLayout::GetName()
{
	return TXT("resizing block");
}

// -------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGraphConnectionActivity );

CUndoGraphConnectionActivity::CUndoGraphConnectionActivity( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, Bool state )
	: CUndoGraphStep( undoManager, graphEditor )
	, m_state( state )
{
}

/*static*/ 
void CUndoGraphConnectionActivity::PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphConnection* connection, Bool state )
{
	CUndoGraphConnectionActivity* step = undoManager.SafeGetStepToAdd< CUndoGraphConnectionActivity >();
	if ( !step )
	{
		step = new CUndoGraphConnectionActivity( undoManager, graphEditor, state );
		undoManager.SetStepToAdd( step );
	}
	else
	{
		ASSERT( step->m_state == state, TXT("Previous CUndoGraphConnectionActivity step should be finalized first") );
	}

	step->m_connections.PushBack( connection );
}

/*static*/ 
void CUndoGraphConnectionActivity::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoGraphConnectionActivity* step = undoManager.SafeGetStepToAdd< CUndoGraphConnectionActivity >() )
	{
		step->PushStep();
	}
}

void CUndoGraphConnectionActivity::DoStep( Bool active )
{
	for ( Uint32 i=0; i<m_connections.Size(); ++i )
	{
		if ( m_connections[i].IsValid() )
		{
			m_connections[i].Get()->SetActive( active );
		}
	}

	GraphStructureModified();
}

/*virtual*/ 
void CUndoGraphConnectionActivity::DoUndo()
{
	DoStep( !m_state );
}

/*virtual*/ 
void CUndoGraphConnectionActivity::DoRedo()
{
	DoStep( m_state );
}

/*virtual*/
String CUndoGraphConnectionActivity::GetName()
{
	return TXT("disabling block");
}

// ------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoGraphSocketVisibility );

CUndoGraphSocketVisibility::CUndoGraphSocketVisibility( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, Bool state )
	: CUndoGraphStep( undoManager, graphEditor )
	, m_state( state )
{
}

/*static*/ 
void CUndoGraphSocketVisibility::PrepareStep( CEdUndoManager& undoManager, CEdGraphEditor* graphEditor, CGraphSocket* socket, Bool state )
{
	CUndoGraphSocketVisibility* step = undoManager.SafeGetStepToAdd< CUndoGraphSocketVisibility >();

	if ( !step )
	{
		step = new CUndoGraphSocketVisibility( undoManager, graphEditor, state );
		undoManager.SetStepToAdd( step );
	}
	else
	{
		ASSERT( step->m_state == state, TXT("Previous CUndoGraphSocketVisibility step should be finalized first") );
	}

	step->m_sockets.PushBack( CUndoGraphSocketInfo( socket ) );
}

/*static*/ 
void CUndoGraphSocketVisibility::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoGraphSocketVisibility* step = undoManager.SafeGetStepToAdd< CUndoGraphSocketVisibility >() )
	{
		step->PushStep();
	}
}

void CUndoGraphSocketVisibility::DoStep( Bool show )
{
	for ( Uint32 i=0; i<m_sockets.Size(); ++i )
	{
		if ( m_sockets[i].GetSocket() )
		{
			m_sockets[i].GetSocket()->SetSocketVisibility( show );

			if ( show )
			{
				m_sockets[i].RestoreConnections();
			}
		}
	}

	GraphStructureModified();
}

/*virtual*/ 
void CUndoGraphSocketVisibility::DoUndo()
{
	DoStep( !m_state );
}

/*virtual*/ 
void CUndoGraphSocketVisibility::DoRedo()
{
	DoStep( m_state );
}

/*virtual*/ 
String CUndoGraphSocketVisibility::GetName()
{
	return m_state ? TXT("showing sockets") : TXT("hiding sockets");
}

