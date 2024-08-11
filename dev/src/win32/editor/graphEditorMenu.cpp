/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "undoGraph.h"
#include "../../common/engine/graphContainer.h"
#include "../../common/engine/graphConnection.h"

#define ID_CREATE_BLOCK_CLASS_FIRST		1000
#define ID_BREAK_ALL_CONNECTIONS		2000
#define ID_HIDE_SOCKET					2001
#define ID_HIDE_UNUSED_SOCKETS			2002
#define ID_SHOW_ALL_SOCKETS				2003
#define ID_COPY_CONNECTIONS				2004
#define ID_CUT_CONNECTIONS				2005
#define ID_PASTE_CONNECTIONS			2006
#define ID_BREAK_CONNECTION_FIRST		2100
#define ID_EXPOSE_SOCKET_FIRST			2200
#define ID_UPDATE_VERSION				2300
#define ID_ALIGN_HORZ					2400
#define ID_ALIGN_VERT					2401
#define ID_DISTRIBUTE_HORZ				2402
#define ID_DISTRIBUTE_VERT				2403
#define ID_ALIGN_CONNECTED_NODE			2404

void CEdGraphEditor::OpenContextMenu()
{
	if ( m_graph )
	{
		// Assemble menu
		wxMenu menu;

		// Save active item
		m_contextMenuItem = m_activeItem;

		// Open menu for active item	
		if ( m_activeItem.IsValid() )
		{
			// Socket menu
			if ( m_activeItem.Get()->IsA<CGraphSocket>() )
			{
				InitLinkedSocketContextMenu( static_cast< CGraphSocket* >( m_activeItem.Get() ), menu );
			}
			else if ( m_activeItem.Get()->IsA< CGraphBlock >() )
			{
				// When nothing is selected assume we select block on which we open context menu
				if ( m_selected.Empty() == true )
				{
					SelectBlock( static_cast< CGraphBlock* >( m_activeItem.Get() ), true );
				}

				InitLinkedBlockContextMenu( static_cast< CGraphBlock* >( m_activeItem.Get() ), menu );
			}
		}
		else
		{	
			// Init default menu
			InitLinkedDefaultContextMenu( menu );
		}

		// Show menu
		wxMenuUtils::CleanUpSeparators( &menu );
		PopupMenu( &menu );

		// Reset
		m_contextMenuItem = NULL;
	}
}

void CEdGraphEditor::InitLinkedDefaultContextMenu( wxMenu& menu )
{
	struct sortclasses
	{
		sortclasses( IGraphContainer* graph )
		{
		}

		RED_INLINE String GetStrippedPath( const String& path ) const
		{
			return path;
		}

		RED_INLINE Bool operator()( CClass* key1, CClass* key2 ) const
		{
			// Get default block
			CGraphBlock* block1 = key1->GetDefaultObject< CGraphBlock >();
			CGraphBlock* block2 = key2->GetDefaultObject< CGraphBlock >();
			String s1 = GetStrippedPath( block1->GetBlockCategory() ) + TXT(".") + block1->GetBlockName();
			String s2 = GetStrippedPath( block2->GetBlockCategory() ) + TXT(".") + block2->GetBlockName();
			return s1 < s2;
		}	
	} sortclasses( m_graph );

	// Collect all block classes
	TDynArray< CClass* > possibleClasses;	
	SRTTI::GetInstance().EnumClasses( ClassID< CGraphBlock >(), possibleClasses );

	Sort( possibleClasses.Begin(), possibleClasses.End(), sortclasses );

	// Adding blocks with category name
	// Add classes to menu
	for ( Uint32 i = 0; i<possibleClasses.Size(); i++ )
	{
		// Only supported classes
		CClass* blockClass = possibleClasses[ i ];
		if ( m_graph->GraphSupportsBlockClass( blockClass ) )
		{
			// Get default block
			CGraphBlock* defaultBlock = blockClass->GetDefaultObject< CGraphBlock >();

			// Create submenu to place block in
			String menuPath = defaultBlock->GetBlockCategory();
			wxMenu* subMenu = NULL;
			if ( !menuPath.Empty() )
			{
				// Create menu
				subMenu = GetSubMenuPath( &menu, sortclasses.GetStrippedPath( menuPath ), true );
			}
			// If we don't have submenu, let's add it directly to context menu
			if ( !subMenu )
			{
				subMenu = &menu;
			}
			// Create menu
			String caption = defaultBlock->GetBlockName();
			subMenu->Append( ID_CREATE_BLOCK_CLASS_FIRST + i, caption.AsChar() );

			// Connect event
			menu.Connect( ID_CREATE_BLOCK_CLASS_FIRST + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnSpawnBlock ), new PopupClassWrapper( blockClass ), this );
		}
	}
}

Bool CEdGraphEditor::InitLinkedSockedShowExposableMenu( const TDynArray< CGraphSocket* > &sockets, wxMenu &menu, wxMenu &baseMenu )
{
	// List exposable inputs
	Bool added = false;
	for ( Uint32 i=0; i<sockets.Size(); i++ )
	{
		CGraphSocket *socket = sockets[i];

		// Is this exposable socket that is hidden ?
		if ( socket->CanHide() && !socket->IsVisible() )
		{
			// Remember what we have added and where
			Uint32 exposeIndex = m_exposableSockets.Size();
			m_exposableSockets.PushBack( socket );

			// Get socket caption
			String socketCaption;
			socket->GetCaption( socketCaption );

			// Format caption
			String caption = String::Printf( TXT("%s"), socketCaption.AsChar() );

			// Initialize menu
			menu.Append( ID_EXPOSE_SOCKET_FIRST + exposeIndex, caption.AsChar() );
			baseMenu.Connect( ID_EXPOSE_SOCKET_FIRST + exposeIndex, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnShowSocket ), NULL, this );

			// Remember that we have added an item
			added = true;
		}
	}

	// Return true if at least one item was added to menu
	return added;
}

void CEdGraphEditor::InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu )
{
	// Clear list of exposable sockets so we don't have dangling pointers
	m_exposableSockets.Clear();

	bool shouldShowSocketRelatedOptions = false;

	// Collect input sockets
	const ELinkedSocketDirection directions[] = { LSD_Input, LSD_Output, LSD_Variable };
	const String names[] = { TXT("Expose Input"), TXT("Expose Output"), TXT("Expose Variable") };
	for ( Uint32 i=0; i<ARRAY_COUNT( directions ); i++ )
	{
		// Get sockets of that direction
		TDynArray< CGraphSocket* > sockets;
		for ( SocketIterator< CGraphSocket > j( block ); j; ++j )
		{
			if ( j->GetDirection() == directions[i] )
			{
				sockets.PushBack( *j );
			}
		}

		// Add menu
		if ( sockets.Size() )
		{
			shouldShowSocketRelatedOptions = true;

			wxMenu* subMenu = new wxMenu;

			// Add sockets
			if ( InitLinkedSockedShowExposableMenu( sockets, *subMenu, menu ) )
			{
				menu.Append( wxID_ANY, names[i].AsChar(), subMenu );
			}
		}
	}

	// We've added some Expose Socket options, add menu separator
	if ( m_exposableSockets.Size() )
	{
		menu.AppendSeparator();
	}

	// Show socked-related options only if there are any sockets to operate on
	if ( shouldShowSocketRelatedOptions )
	{
		// Break all
		menu.Append( ID_BREAK_ALL_CONNECTIONS, TXT("Break All Links"), wxEmptyString );
		menu.Connect( ID_BREAK_ALL_CONNECTIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnBreakAllLinks ), NULL, this );

		// Connection management
		menu.AppendSeparator();
		menu.Append( ID_HIDE_UNUSED_SOCKETS, TXT("Hide Unused Sockets"), wxEmptyString );
		menu.Connect( ID_HIDE_UNUSED_SOCKETS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnHideSockets ), NULL, this );
		menu.Append( ID_SHOW_ALL_SOCKETS, TXT("Show All Sockets"), wxEmptyString );
		menu.Connect( ID_SHOW_ALL_SOCKETS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnShowSockets ), NULL, this );
	}

	// Obsolete
	if ( 1 )//block->IsObsolete() )
	{
		menu.AppendSeparator();
		menu.Append( ID_UPDATE_VERSION, TXT("Update to new version"), wxEmptyString );
		menu.Connect( ID_UPDATE_VERSION, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnUpdateVersion ), NULL, this );		
	}

	TDynArray<CGraphBlock*> selection;
	GetSelectedBlocks( selection );

	if ( selection.Size() > 1 )
	{
		// Align
		menu.AppendSeparator();
		wxMenuUtils::AppendMenuItemWithBitmap( &menu, ID_ALIGN_HORZ, TXT("Align horizontally"), TXT("IMG_HORZ_ALIGN") );
		menu.Connect( ID_ALIGN_HORZ, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnAlignHorz ), NULL, this );
		wxMenuUtils::AppendMenuItemWithBitmap( &menu, ID_ALIGN_VERT, TXT("Align vertically"), TXT("IMG_VERT_ALIGN") );
		menu.Connect( ID_ALIGN_VERT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnAlignVert ), NULL, this );

		menu.Append( ID_ALIGN_CONNECTED_NODE, TXT("Align by connections") );
		menu.Connect( ID_ALIGN_CONNECTED_NODE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnAlignConnectedNode ), NULL, this );
	}

	if ( selection.Size() > 2 )
	{
		// Distribute
		menu.AppendSeparator();
		wxMenuUtils::AppendMenuItemWithBitmap( &menu, ID_DISTRIBUTE_HORZ, TXT("Distribute horizontally"), TXT("IMG_HORZ_DISTRIBUTE") );
		menu.Connect( ID_DISTRIBUTE_HORZ, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnDistributeHorz ), NULL, this );
		wxMenuUtils::AppendMenuItemWithBitmap( &menu, ID_DISTRIBUTE_VERT, TXT("Distribute vertically"), TXT("IMG_VERT_DISTRIBUTE") );
		menu.Connect( ID_DISTRIBUTE_VERT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnDistributeVert ), NULL, this );
	}
}

void CEdGraphEditor::InitLinkedSocketContextMenu( CGraphSocket* socket, wxMenu &menu )
{
	// Link list
	const TDynArray< CGraphConnection* >& cons = socket->GetConnections(); 
	if ( !cons.Empty() )
	{
		menu.Append( ID_BREAK_ALL_CONNECTIONS, TXT("Break All Links"), wxEmptyString );
		menu.Connect( ID_BREAK_ALL_CONNECTIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnBreakAllLinks ), NULL, this );

		// Sub menu
		wxMenu* subMenu = new wxMenu;
		for ( Uint32 i=0; i<cons.Size(); i++ )
		{
			CGraphConnection* con = cons[i];

			CGraphSocket* destination = con->GetDestination( true );

			if ( destination )
			{
				String linkOption;

				if( !destination->GetName().AsString().TrimCopy().Empty() )
				{
					linkOption += L'\'';
					linkOption += destination->GetName().AsString();
					linkOption += TXT( "' in " );
				}

				linkOption += L'\'';
				linkOption += destination->GetBlock()->GetCaption();
				linkOption += L'\'';

				if( !con->IsActive() )
				{
					linkOption += TXT( " (X)" );
				}

				subMenu->Append( ID_BREAK_CONNECTION_FIRST + i, linkOption.AsChar(), wxEmptyString );
				menu.Connect( ID_BREAK_CONNECTION_FIRST + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnBreakLink ), NULL, this );
			}
		}

		// Add
		menu.Append( wxID_ANY, TXT("Break Link To"), subMenu );
	}

	// Connection copy/paste
	menu.AppendSeparator();

	if ( !cons.Empty() )
	{
		menu.Append( ID_COPY_CONNECTIONS, TXT("Copy Connections"), wxEmptyString );
		menu.Connect( ID_COPY_CONNECTIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnCopyConnections ), NULL, this );
		menu.Append( ID_CUT_CONNECTIONS, TXT("Cut Connections"), wxEmptyString );
		menu.Connect( ID_CUT_CONNECTIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnCutConnections ), NULL, this );
	}

	if ( !m_connectionClipboard.m_connections.Empty() )
	{
		menu.Append( ID_PASTE_CONNECTIONS, TXT("Paste Connections"), wxEmptyString );
		menu.Connect( ID_PASTE_CONNECTIONS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnPasteConnections ), NULL, this );
	}

	menu.AppendSeparator();

	// Hide output option
	if ( socket->CanHide() )
	{
		menu.Append( ID_HIDE_SOCKET, TXT("Hide Socket"), wxEmptyString );
		menu.Connect( ID_HIDE_SOCKET, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdGraphEditor::OnHideSocket ), NULL, this );
	}
}

void CEdGraphEditor::OnSpawnBlock( wxCommandEvent& event )
{
	ASSERT( m_graph );

	// Get info
	PopupClassWrapper* wrapper = ( PopupClassWrapper* ) event.m_callbackUserData;
	CClass* blockClass = wrapper->m_objectClass;
	ASSERT( blockClass );
	ASSERT( !blockClass->IsAbstract() );

	// Create block
	SpawnBlock( blockClass, m_lastClickPoint );
}

CGraphBlock* CEdGraphEditor::SpawnBlock( CClass* blockClass, wxPoint point )
{
	CGraphBlock* ret = NULL;
	if ( ModifyGraphStructure() )
	{
		wxPoint graphPoint = ClientToCanvas( point );
		GraphBlockSpawnInfo info( blockClass );
		info.m_position = Vector( graphPoint.x, graphPoint.y, 0, 1 );
		ret = m_graph->GraphCreateBlock( info );

		if ( m_undoManager )
		{
			CUndoGraphBlockExistance::PrepareCreationStep( *m_undoManager, this, ret );
			CUndoGraphBlockExistance::FinalizeStep( *m_undoManager );
		}

		GraphStructureModified();
	}

	return ret;
}

void CEdGraphEditor::OnBreakAllLinks( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	if ( ModifyGraphStructure() )
	{
		if ( m_activeItem.Get()->IsA< CGraphBlock >() )
		{
			CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );

			if ( m_undoManager )
			{
				const TDynArray< CGraphSocket* >& sockets = block->GetSockets( );
				for ( Uint32 i=0; i<sockets.Size(); ++i )
				{
					const TDynArray< CGraphConnection* >& cons = sockets[i]->GetConnections(); 
					for ( Uint32 j=0; j<cons.Size(); ++j )
					{
						CUndoGraphConnectionExistance::PrepareDeletionStep( *m_undoManager, this, cons[j] );
					}
				}

				CUndoGraphConnectionExistance::FinalizeStep( *m_undoManager );
			}

			block->BreakAllLinks();
		}
		else if ( m_activeItem.Get()->IsA< CGraphSocket >() )
		{
			CGraphSocket* socket = static_cast< CGraphSocket* >( m_activeItem.Get() );
			if ( m_undoManager )
			{
				const TDynArray< CGraphConnection* >& cons = socket->GetConnections(); 
				for ( Uint32 i=0; i<cons.Size(); ++i )
				{
					CUndoGraphConnectionExistance::PrepareDeletionStep( *m_undoManager, this, cons[i] );
				}

				CUndoGraphConnectionExistance::FinalizeStep( *m_undoManager );
			}

			socket->BreakAllLinks();
		}
		GraphStructureModified();
	}
}

void CEdGraphEditor::OnBreakLink( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	if ( ModifyGraphStructure() )
	{
		Int32 index = event.GetId() - ID_BREAK_CONNECTION_FIRST;

		if ( m_activeItem.Get()->IsA< CGraphSocket >() )
		{
			CGraphSocket* socket = static_cast< CGraphSocket* >( m_activeItem.Get() );
			const TDynArray< CGraphConnection* >& cons = socket->GetConnections(); 

			if ( m_undoManager )
			{
				CUndoGraphConnectionExistance::PrepareDeletionStep( *m_undoManager, this, cons[index] );
				CUndoGraphConnectionExistance::FinalizeStep( *m_undoManager );
			}

			socket->DisconnectFrom( cons[index]->GetDestination( true ) );
			GraphStructureModified();
		}
	}
}

void CEdGraphEditor::OnHideSocket( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	if ( ModifyGraphStructure() )
	{
		// Hide socket
		if ( m_activeItem.Get()->IsA< CGraphSocket >() )
		{
			CGraphSocket* socket = static_cast< CGraphSocket* >( m_activeItem.Get() );
			Bool hadConnections = socket->HasConnections();
		
			if ( m_undoManager )
			{
				CUndoGraphSocketVisibility::PrepareStep( *m_undoManager, this, socket, false );
				CUndoGraphSocketVisibility::FinalizeStep( *m_undoManager );
			}
		
			socket->SetSocketVisibility( false );

			// Update
			if ( !hadConnections )
			{
				// Redraw only
				Repaint();
			}
			GraphStructureModified();
		}
	}
}

void CEdGraphEditor::OnShowSocket( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	// Get socket
	Int32 socketIndex = event.GetId() - ID_EXPOSE_SOCKET_FIRST;
	CGraphSocket* socket = m_exposableSockets[ socketIndex ];

	if ( m_undoManager )
	{
		CUndoGraphSocketVisibility::PrepareStep( *m_undoManager, this, socket, true );
		CUndoGraphSocketVisibility::FinalizeStep( *m_undoManager );
	}

	socket->SetSocketVisibility( true );

	// Redraw
	Repaint();
}

void CEdGraphEditor::OnHideSockets( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	// Hide sockets
	if ( m_activeItem.Get()->IsA< CGraphBlock >() )
	{
		CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
		TDynArray< CGraphSocket* > hiddenSockets = block->HideUnusedSockets();
	
		if ( m_undoManager )
		{
			for ( Uint32 i=0; i<hiddenSockets.Size(); ++i )
			{
				CUndoGraphSocketVisibility::PrepareStep( *m_undoManager, this, hiddenSockets[i], false );
			}

			CUndoGraphSocketVisibility::FinalizeStep( *m_undoManager );
		}

		// Redraw
		Repaint();
	}
}

void CEdGraphEditor::OnShowSockets( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	// Show sockets
	if ( m_activeItem.Get()->IsA< CGraphBlock >() )
	{
		CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
		TDynArray< CGraphSocket* > shownSockets = block->ShowAllSockets();

		if ( m_undoManager )
		{
			for ( Uint32 i=0; i<shownSockets.Size(); ++i )
			{
				CUndoGraphSocketVisibility::PrepareStep( *m_undoManager, this, shownSockets[i], true );
			}

			CUndoGraphSocketVisibility::FinalizeStep( *m_undoManager );
		}

		// Redraw
		Repaint();
	}
}

void CEdGraphEditor::OnUpdateVersion( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	// Show sockets
	if ( m_activeItem.Get()->IsA< CGraphBlock >() )
	{
		CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );

		CQuestGraphBlock* graphBlock = Cast< CQuestGraphBlock >( block );
		if ( graphBlock )
		{
			graphBlock->SortOutputBlocks();
		}

		block->UpdateVersion();

		// Redraw
		Repaint();
	}
}

void CEdGraphEditor::DoAlignSelection( Bool horz )
{
	if ( !CanBeModify() )
	{
		return;
	}

	TDynArray<CGraphBlock*> selection;
	GetSelectedBlocks( selection );
	ASSERT ( selection.Size() > 1 );

	CGraphBlock* base = selection[0];
	Vector basePos  = base->GetPosition();
	Vector baseSize = GetSizeFromLayout( base );
	Float center = horz ? ( basePos.X + 0.5*baseSize.X ) : ( basePos.Y + 0.5*baseSize.Y );

	for ( Uint32 i=1; i<selection.Size(); ++i )
	{
		Vector pos  = selection[i]->GetPosition();
		Vector size = GetSizeFromLayout( selection[i] );

		if ( horz )
		{
			pos.X = center - 0.5*size.X; 
		}
		else 
		{
			pos.Y = center - 0.5*size.Y;
		}

		if ( m_undoManager )
		{
			CUndoGraphBlockMove::PrepareStep( *m_undoManager, this, selection[i] );
		}

		selection[i]->SetPosition( pos );
	}

	if ( m_undoManager )
	{
		CUndoGraphBlockMove::FinalizeStep( *m_undoManager );
	}

	Repaint();
}

void CEdGraphEditor::OnAlignHorz( wxCommandEvent& event )
{
	DoAlignSelection( true );
}

void CEdGraphEditor::OnAlignVert( wxCommandEvent& event )
{
	DoAlignSelection( false );
}

void CEdGraphEditor::DoDistributeSelection( Bool horz )
{
	if ( !CanBeModify() )
	{
		return;
	}

	TDynArray<CGraphBlock*> selection;
	GetSelectedBlocks( selection );
	ASSERT ( selection.Size() > 2 );

	// Sort blocks by position to preserve the order
	Sort( selection.Begin(), selection.End(),
		[ horz ]( CGraphBlock* b1, CGraphBlock* b2 )
		{
			return horz ? ( b1->GetPosition().X < b2->GetPosition().X ) : ( b1->GetPosition().Y < b2->GetPosition().Y );
		} );

	TDynArray< Float > sizes;
	sizes.Reserve( selection.Size() );
	Float totalSize = 0.0;

	for ( auto blockIt = selection.Begin(); blockIt != selection.End(); ++blockIt )
	{
		Vector sizeV = GetSizeFromLayout( *blockIt );
		Float size = horz ? sizeV.X : sizeV.Y;
		totalSize += size;
		sizes.PushBack( size );
	}

	CGraphBlock* startBlock = selection[0];
	CGraphBlock* endBlock   = selection.Back();

	Float startBlockPos  = horz ? startBlock->GetPosition().X : startBlock->GetPosition().Y;
	Float startBlockSize = sizes[0];
	Float endBlockPos    = horz ? endBlock->GetPosition().X : endBlock->GetPosition().Y;
	Float endBlockSize   = sizes.Back();

	Float occupiedSpace = ( endBlockPos + endBlockSize - startBlockPos );
	Float availableMargin = occupiedSpace - totalSize;
	Float margin = availableMargin / ( selection.Size()-1 );

	// can be negative if there is no enough space
	if ( margin < 5 )
	{
		margin = 5;
	}

	Float curPos = startBlockPos + startBlockSize + margin;

	for ( Uint32 i=1; i<selection.Size(); ++i )
	{
		Vector pos  = selection[i]->GetPosition();

		horz ? pos.X = curPos : pos.Y = curPos;

		if ( m_undoManager )
		{
			CUndoGraphBlockMove::PrepareStep( *m_undoManager, this, selection[i] );
		}

		selection[i]->SetPosition( pos );

		curPos += sizes[i] + margin;
	}

	if ( m_undoManager )
	{
		CUndoGraphBlockMove::FinalizeStep( *m_undoManager );
	}

	Repaint();
}

void CEdGraphEditor::OnDistributeHorz( wxCommandEvent& event )
{
	DoDistributeSelection( true );
}

void CEdGraphEditor::OnDistributeVert( wxCommandEvent& event )
{
	DoDistributeSelection( false );
}

Bool CEdGraphEditor::GetConnectedSocketLayouts( CGraphBlock* left, CGraphBlock* right, SocketLayoutInfo*& leftSocket, SocketLayoutInfo*& rightSocket )
{
	TDynArray< CGraphSocket* > leftSockets = left->GetSockets();
	TDynArray< CGraphSocket* > rightSockets = right->GetSockets();

	for( Uint32 iLeftSocket = 0; iLeftSocket < leftSockets.Size(); ++iLeftSocket )
	{
		CGraphSocket* leftOut = leftSockets[ iLeftSocket ];

		if( leftOut->GetDirection() == ELinkedSocketDirection::LSD_Output )
		{
			for( Uint32 iRightSocket = 0; iRightSocket < rightSockets.Size(); ++iRightSocket )
			{
				CGraphSocket* rightIn = rightSockets[ iRightSocket ];

				if( rightIn->GetDirection() == ELinkedSocketDirection::LSD_Input )
				{
					if( leftOut->HasConnectionsTo( rightIn ) )
					{
						BlockLayoutInfo* leftBlockInfo = m_layout.FindPtr( left );
						BlockLayoutInfo* rightBlockInfo = m_layout.FindPtr( right );
						RED_FATAL_ASSERT( leftBlockInfo, "Missing Block information" );
						RED_FATAL_ASSERT( rightBlockInfo, "Missing Block information" );

						leftSocket = leftBlockInfo->m_sockets.FindPtr( leftOut );
						rightSocket = rightBlockInfo->m_sockets.FindPtr( rightIn );
						RED_FATAL_ASSERT( leftSocket, "Missing Socket information" );
						RED_FATAL_ASSERT( rightSocket, "Missing Socket information" );

						return true;
					}
				}
			}
		}
	}

	return false;
}

void CEdGraphEditor::OnAlignConnectedNode( wxCommandEvent& )
{
	if ( !CanBeModify() )
	{
		return;
	}

	TDynArray< CGraphBlock* > selection;
	GetSelectedBlocks( selection );

	// Sort blocks by position to preserve the order
	Sort
	(
		selection.Begin(),
		selection.End(),
		[]( CGraphBlock* b1, CGraphBlock* b2 )
		{
			return ( b1->GetPosition().X < b2->GetPosition().X );
		}
	);

	for( Uint32 iLeft = 0; iLeft < selection.Size(); ++iLeft )
	{
		CGraphBlock* left = selection[ iLeft ];
		TDynArray< CGraphSocket* > leftSockets = left->GetSockets();

		for( Uint32 iRight = iLeft + 1; iRight < selection.Size(); ++iRight )
		{
			CGraphBlock* right = selection[ iRight ];

			SocketLayoutInfo* leftSocket = nullptr;
			SocketLayoutInfo* rightSocket = nullptr;

			if( GetConnectedSocketLayouts( left, right, leftSocket, rightSocket ) )
			{
				RED_LOG( Dan, TXT( "%ls is connected to %ls!" ), left->GetBlockName().AsChar(), right->GetBlockName().AsChar() );

				const Vector& leftPosition = left->GetPosition();
				Vector rightPosition = right->GetPosition();

				Int32 offset = leftSocket->m_socketRect.GetTop() - rightSocket->m_socketRect.GetTop();

				rightPosition.Y = leftPosition.Y + offset;

				if ( m_undoManager )
				{
					CUndoGraphBlockMove::PrepareStep( *m_undoManager, this, right );
				}

				right->SetPosition( rightPosition );
			}
		}
	}

	if ( m_undoManager )
	{
		CUndoGraphBlockMove::FinalizeStep( *m_undoManager );
	}
}

void CEdGraphEditor::OnCopyConnections( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	if ( !m_activeItem.Get()->IsA< CGraphSocket >() )
	{
		return;
	}

	CGraphSocket* socket = static_cast< CGraphSocket* >( m_activeItem.Get() );

	if ( socket == nullptr || !socket->IsVisible( ) )
	{
		return;
	}

	m_connectionClipboard.m_socket = socket;
	m_connectionClipboard.m_connections.Clear();

	// Grab connections to the clipboard
	const TDynArray< CGraphConnection* > &cons = socket->GetConnections();
	for ( Uint32 j=0; j<cons.Size(); j++ )
	{
		CGraphConnection* con = cons[j];
		if ( con->GetDestination( true ) )
		{
			m_connectionClipboard.m_connections.PushBack( 
				CGraphConnectionClipboard::Connection( 
					socket->GetName(), con->GetDestination( true )->GetName(), con->GetDestination( true )->GetBlock(), con->IsActive() 
					) );
		}
	}
}

void CEdGraphEditor::OnCutConnections( wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	if ( !m_activeItem.Get()->IsA< CGraphSocket >() )
	{
		return;
	}

	CGraphSocket* socket = static_cast< CGraphSocket* >( m_activeItem.Get() );

	if ( socket == nullptr || !socket->IsVisible( ) )
	{
		return;
	}

	if ( !ModifyGraphStructure() )
	{
		return;
	}

	OnCopyConnections( event );

 	if ( m_undoManager && socket->HasConnections( true ) )
 	{
		TDynArray< CGraphConnection* > connections = socket->GetConnections();
		for ( Uint32 i=0; i<connections.Size(); ++i )
		{
			CUndoGraphConnectionExistance::PrepareDeletionStep( *m_undoManager, this, connections[i] );
		}

		CUndoGraphConnectionExistance::FinalizeStep( *m_undoManager );
	}

	socket->BreakAllLinks();

	GraphStructureModified();
}

void CEdGraphEditor::OnPasteConnections(wxCommandEvent& event )
{
	ASSERT( m_activeItem.Get() );

	if ( !m_activeItem.Get()->IsA< CGraphSocket >() )
	{
		return;
	}

	CGraphSocket* srcSocket = static_cast< CGraphSocket* >( m_activeItem.Get() );

	if ( srcSocket == nullptr || !srcSocket->IsVisible( ) )
	{
		return;
	}

	if ( !ModifyGraphStructure() )
	{
		return;
	}

	for ( Uint32 i=0; i<m_connectionClipboard.m_connections.Size(); i++ )
	{
		const CGraphConnectionClipboard::Connection& con = m_connectionClipboard.m_connections[i];

		if ( CGraphSocket* destSocket = con.m_block->FindSocket( con.m_destination ) )
		{
			// If not already connected and can be connected
			if ( !srcSocket->HasConnectionsTo( destSocket, true ) &&
				destSocket->CanConnectTo( srcSocket ) && srcSocket->CanConnectTo( destSocket )
				)
			{
				if ( m_autoUnplugSingleConnections )
				{
					TDynArray< CGraphConnection* > blocking = CollectBlockingConnections( srcSocket, destSocket );
	
					// Make the room for the connection
					for ( auto rI = blocking.Begin(); rI != blocking.End(); ++rI )
					{
						CGraphConnection* blockingCon = *rI;
						if ( m_undoManager )
						{
							CUndoGraphConnectionExistance::PrepareDeletionStep( *m_undoManager, this, blockingCon );
						}

						blockingCon->GetSource( true )->DisconnectFrom( blockingCon->GetDestination( true ) );
					}
				}

				// Make sure sockets with connections are visible
				srcSocket->SetSocketVisibility( true );
				destSocket->SetSocketVisibility( true );

				if ( CGraphConnection* connection = srcSocket->ConnectTo( destSocket, con.m_isActive ) )
				{
					if ( m_undoManager )
					{
						CUndoGraphConnectionExistance::PrepareCreationStep( *m_undoManager, this, connection );
					}
				}
			}
		}
	}

	if ( m_undoManager )
	{
		CUndoGraphConnectionExistance::FinalizeStep( *m_undoManager );
	}

	GraphStructureModified();
}