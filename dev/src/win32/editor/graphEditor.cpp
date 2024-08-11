/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "undoGraph.h"
#include "undoProperty.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/engine/graphContainer.h"
#include "../../common/engine/graphConnection.h"

// Event table
BEGIN_EVENT_TABLE( CEdGraphEditor, CEdCanvas )
    EVT_MOUSEWHEEL( CEdGraphEditor::OnMouseWheel )
    EVT_SIZE( CEdGraphEditor::OnSize )
    EVT_MIDDLE_DOWN( CEdGraphEditor::OnMouseMiddleEvent )
END_EVENT_TABLE()

CEdGraphEditor::CEdGraphEditor( wxWindow *parent, Bool mirrored, Bool autoUnplugSingleConnections )
	: CEdCanvas( parent )
	, m_graph( NULL )
	, m_activeItem( NULL )
	, m_contextMenuItem( NULL )
	, m_sourceSocket( NULL )
	, m_destSocket( NULL )
	, m_moveTotal( 0 )
	, m_mirrored( mirrored )
	, m_action( MA_None )
	, m_hook( NULL )
	, m_desiredScale( 1 )
	, m_windowsMoved( false )
	, m_LastSizeEventHad0Height( true )
	, m_LastFocusedBlock( NULL )
	, m_canBeModify( true )
	, m_shouldRepaint( false )
	, m_shouldZoom( false )
	, m_undoManager( NULL )
	, m_autoUnplugSingleConnections( autoUnplugSingleConnections )
{
	// Register as event processor
	SEvents::GetInstance().RegisterListener( CNAME( EditorTick ), this );

    Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CEdGraphEditor::OnKeyDown ), NULL, this );
}

CEdGraphEditor::~CEdGraphEditor()
{
	// Unregister event processor
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdGraphEditor::OnSize(wxSizeEvent &event)
{
	// This hack fixes block focusing on the debugger creation - the graphEditors
	// height is 0 at this time, so refocus the block on the first OnSize
	// event with non-zero height.
	if (event.m_size.y != 0 && m_LastSizeEventHad0Height && m_LastFocusedBlock)
	{
		FocusOnBlock(m_LastFocusedBlock);
	}
	m_LastSizeEventHad0Height = event.m_size.y == 0;

	// Calculate visibility and repaint
	CheckBlocksVisibility();
	m_shouldRepaint = true;
}

void CEdGraphEditor::SetHook( GraphEditorHook* hook )
{
	m_hook = hook;
}

void CEdGraphEditor::SetBackgroundOffset( wxPoint offset )
{
	if ( m_graph )
	{
		// Set background offset
		m_graph->GraphSetBackgroundOffset( Vector( offset.x, offset.y, 0 ) );

		// Scroll canvas
		SetOffset( offset );
	}
}

void CEdGraphEditor::ScrollBackgroundOffset( wxPoint delta )
{
	if ( m_graph )
	{
		// Get current background offset
		Vector point = m_graph->GraphGetBackgroundOffset();

		// Scroll background offset
		point.X += delta.x;			
		point.Y += delta.y;

		// Write back to graph
		m_graph->GraphSetBackgroundOffset( point );

		// Scroll canvas
		SetOffset( wxPoint( point.X, point.Y ) );
	}
}

void CEdGraphEditor::ScaleGraph( Float newScale )
{
	// Different ?
	if ( GetScale() != newScale )
	{
		// Apply new scale
		Float oldScale = GetScale();
		SetScale( newScale );

		// Get mouse cursor position
		wxPoint mouse;
		::GetCursorPos( (POINT*) &mouse );

		// Transform to client space
		mouse = ScreenToClient( mouse );

		// If we are outside the client area then use the center of client area as a zoom focus point
		wxRect clientRect = GetClientRect();
		if ( !clientRect.Contains( mouse ) )
		{
			// Calculate center of client area
			mouse.x = clientRect.x + clientRect.width / 2;
			mouse.y = clientRect.y + clientRect.height / 2;
		}

		// Repair background offset so we stay focused on the same space
		wxPoint offset;
		Float diff = 1.0f / newScale - 1.0f / oldScale;
		offset.x = mouse.x * diff;
		offset.y = mouse.y * diff;
		ScrollBackgroundOffset( offset );

		CheckBlocksVisibility();
	}
}

void CEdGraphEditor::ZoomExtentsLater()
{
	m_shouldZoom = true;
}

void CEdGraphEditor::ZoomExtents()
{
	if ( m_graph )
	{
		TDynArray< CGraphBlock* >& blocks = m_graph->GraphGetBlocks();

		Float minX =  NumericLimits<Float>::Max(),
			minY =  NumericLimits<Float>::Max(),
			maxX = -NumericLimits<Float>::Max(),
			maxY = -NumericLimits<Float>::Max();

		if ( blocks.Empty() )
		{
			wxSize windowSize = GetGraphArea();
			minX = 0;
			minY = 0;
			maxX = windowSize.x;
			maxY = windowSize.y;
		}
		else
		{
			for( Uint32 i=0; i<blocks.Size(); ++i )
			{
				CGraphBlock *block = blocks[i];

				const Vector &currBlockPos = blocks[i]->GetPosition();
				minX = Min( minX, currBlockPos.X );
				minY = Min( minY, currBlockPos.Y );

				UpdateBlockLayout( block );

				BlockLayoutInfo* layout = m_layout.FindPtr( block );
				if ( layout )
				{
					wxSize currBlockSize = layout->m_windowSize;

					maxX = Max( maxX, currBlockPos.X + currBlockSize.x );
					maxY = Max( maxY, currBlockPos.Y + currBlockSize.y );
				}
			}
		}

		wxSize windowSize = GetGraphArea();
		wxPoint graphCenter( ( maxX + minX ) / 2, ( maxY + minY ) / 2 );
		wxSize graphSize( (Int32)( ( maxX - minX ) * 1.2f ) , (Int32)( ( maxY - minY ) * 1.2f ) );

		Float xScale = (Float)windowSize.GetWidth() / graphSize.GetWidth();
		Float yScale = (Float)windowSize.GetHeight() / graphSize.GetHeight();

		Float scale = Min( xScale, yScale );
		scale = Clamp( scale, 0.1f, 2.0f );

		wxPoint graphCorner( graphCenter.x - ( (windowSize.x / 2) / scale ), graphCenter.y - ( (windowSize.y / 2) / scale ) );	
		wxPoint newOffset( -graphCorner.x , -graphCorner.y );

		SetScale( scale );
		m_desiredScale = scale;
		SetBackgroundOffset( newOffset );

		CheckBlocksVisibility();
	}
}

void CEdGraphEditor::FocusOnBlock( CGraphBlock* block )
{
	m_LastFocusedBlock = block;

	// Get block layout info
	BlockLayoutInfo* layout = m_layout.FindPtr( block );
	if ( layout )
	{
		// Reset scale
		m_desiredScale = 1.0f;
		SetScale( 1.0f );

		// Center on block
		wxSize windowSize = GetGraphArea();
		const Vector &currBlockPos = block->GetPosition();
		wxPoint blockCenter( currBlockPos.X + layout->m_windowSize.x - windowSize.x / 2, currBlockPos.Y + layout->m_windowSize.y - windowSize.y / 2 );
		SetBackgroundOffset( -blockCenter );

		CheckBlocksVisibility();
	}
}

void CEdGraphEditor::SetGraph( IGraphContainer *graph )
{
	// Set graph
	m_graph = graph;
	m_autoScroll = wxPoint(0,0);
	m_activeItem = NULL;

	// Initialize rendering
	if ( m_graph )
	{
		// Get blocks
		TDynArray< CGraphBlock* > blocks;
		blocks = m_graph->GraphGetBlocks();

		// Draw them
		for ( Int32 i=(Int32)blocks.Size()-1; i>=0; i-- )
		{
			CGraphBlock* block = blocks[i];

			// Update layout
			UpdateBlockLayout( block );
			block->m_needsLayoutUpdate = false;
		}
	}

	// Redraw window - for update canvas data
	Repaint( true );

	CheckBlocksVisibility();

	// Paint visible blocks
	m_shouldRepaint = true;
	m_shouldZoom = true;
}

void CEdGraphEditor::SetUndoManager( CEdUndoManager* undoManager )
{
	m_undoManager = undoManager;
}

void CEdGraphEditor::FilterBlockClassList( TDynArray< CClass* >& blockClasses )
{
	// Filter classes
	TDynArray< CClass* > filteredClasses;
	for ( Uint32 i=0; i<blockClasses.Size(); i++ )
	{
		if ( m_graph && m_graph->GraphSupportsBlockClass( blockClasses[i] ) )
		{
			filteredClasses.PushBackUnique( blockClasses[i] );
		}
	}

	// Replace array
	blockClasses = filteredClasses;
}

void CEdGraphEditor::GetSelectedBlocks( TDynArray<CGraphBlock*> &blocks )
{
	// Collect selected blocks
	for ( THashSet< CGraphBlock* >::iterator i=m_selected.Begin(); i != m_selected.End(); ++i )
	{
		blocks.PushBack( *i );
	}
}

Bool CEdGraphEditor::CanBeModify() const
{
	return m_canBeModify;
}

Bool CEdGraphEditor::ModifyGraphStructure()
{
	Bool ret = CanBeModify();

	// Call graph event
	if ( m_graph )
	{
		return ret && m_graph->ModifyGraphStructure();
	}

	return ret;
}

void CEdGraphEditor::OnGraphStructureWillBeModified()
{
	if ( m_hook )
	{
		m_hook->OnGraphStructureWillBeModified( m_graph );
	}
}

void CEdGraphEditor::GraphStructureModified()
{
	ASSERT( CanBeModify() );

	if ( m_graph )
	{
		m_graph->GraphStructureModified();
	}

	if ( m_hook )
	{
		m_hook->OnGraphStructureModified( m_graph );
	}

	// Update layout info
	TDynArray< CGraphBlock* > blocks;
	blocks = m_graph->GraphGetBlocks();

	for ( Uint32 i=0; i<blocks.Size(); i++ )
	{
		UpdateBlockLayout( blocks[i] );
	}

	CheckBlocksVisibility();

	Repaint();
}

void CEdGraphEditor::SelectAllBlocks()
{
	// Clear selection
	m_selected.Clear();

	// Select all blocks
	if ( m_graph )
	{
		// Get blocks
		TDynArray< CGraphBlock* > blocks;
		blocks = m_graph->GraphGetBlocks();

		// Select all if them
		for ( Uint32 i=0; i<blocks.Size(); i++ )
		{
			m_selected.Insert( blocks[i] );
		}

		// Selection changed
		if ( m_hook )
		{
			m_hook->OnGraphSelectionChanged();
		}
	}
}

void CEdGraphEditor::DeselectAllBlocks()
{
	Bool wasSelected = !m_selected.Empty();

	// Clear selection
	m_selected.Clear();

	// Selection changed
	if ( wasSelected && m_hook )
	{
		m_hook->OnGraphSelectionChanged();
	}
}

Bool CEdGraphEditor::IsBlockSelected( CGraphBlock* block )
{
	return m_selected.Find( block ) != m_selected.End();
}

void CEdGraphEditor::SelectBlock( CGraphBlock* block, Bool select, Bool clear )
{
	// Update only if selection differs
	if ( IsBlockSelected( block ) != select || clear )
	{
		if ( clear )
		{
			m_selected.Clear();
		}

		// Set selection flag
		if ( select )
		{
			m_selected.Insert( block );
		}
		else
		{
			m_selected.Erase( block );
		}

		// Move block to front
		if ( select )
		{
			// Find block in block list
			TDynArray< CGraphBlock* >& blocks = m_graph->GraphGetBlocks();
			TDynArray< CGraphBlock* >::iterator index = Find( blocks.Begin(), blocks.End(), block );

			// Move selected block to the front
			//ASSERT( index != blocks.End() );
			if ( index != blocks.Begin() )
			{
				blocks.Remove( block );
				blocks.Insert( 0, block );
			}
		}

		// Selection changed
		if ( m_hook )
		{
			m_hook->OnGraphSelectionChanged();
		}
	}
}

void CEdGraphEditor::SelectBlocks( const TDynArray< CGraphBlock* > &blocks, Bool clear )
{
	Bool changed = false;

	if ( clear )
	{
		if ( !m_selected.Empty() )
		{
			changed = true;
		}
		m_selected.Clear();
	}

	for ( Uint32 i=0; i<blocks.Size(); ++i ) 
	{
		if ( m_selected.Insert( blocks[i] ) )
		{
			changed = true;
		}
	}

	// Selection changed
	if ( m_hook && changed )
	{
		m_hook->OnGraphSelectionChanged();
	}
}


void CEdGraphEditor::MoveSelectedBlocks( wxPoint graphMousePoint )
{
	if ( CanBeModify() )
	{
		// Move blocks
		for ( Uint32 i=0; i<m_blocksToMove.Size(); i++ )
		{
			CGraphBlock* block = m_blocksToMove[i];
			wxPoint pressPoint;
			Bool found = m_movePressPoints.Find( block, pressPoint );
			ASSERT( found );
			wxPoint newPos = graphMousePoint - pressPoint;

			if ( m_undoManager )
			{
				CUndoGraphBlockMove::PrepareStep( *m_undoManager, this, block );
			}

			block->SetPosition( Vector( newPos.x, newPos.y, 0 ) );
		}
	}
}

void CEdGraphEditor::SelectBlocksFromArea( wxRect area, Bool clear )
{
	// Get block
	if ( m_graph )
	{
		TDynArray< CGraphBlock* > &blocks = m_graph->GraphGetBlocks();

		TDynArray< CGraphBlock* > blocksToSelect;

		// Linear search
		CGraphSocket* activeSocket = NULL;
		for ( Uint32 i=0; i<blocks.Size(); i++ )
		{
			// Get layout
			CGraphBlock* block = blocks[i];
			const BlockLayoutInfo *info = m_layout.FindPtr( block );

			// Select only if layout data is valid
			if ( info && info->m_visible && !info->m_freeze )
			{
				// Get block rect
				wxRect blockRect;
				blockRect.x = block->GetPosition().X;
				blockRect.y = block->GetPosition().Y;
				blockRect.width = info->m_windowSize.x;
				blockRect.height = info->m_windowSize.y;

				if ( block->IsInnerAreaTransparent() && blockRect.Contains( wxPoint( area.x, area.y ) ) && blockRect.Contains( wxPoint( area.x + area.width, area.y + area.height ) ) )
				{
					continue;
				}
				// Selected ?				
				if ( area.Intersects( blockRect ) )
				{
					blocksToSelect.PushBack( block );
				}
			}
		}

		SelectBlocks( blocksToSelect, clear );
	}
}

void CEdGraphEditor::DispatchEditorEvent( const CName& systemEvent, IEdEventData* data )
{
	// Only for valid graph
	if ( !m_graph )
		return;

	// Auto update
	if ( systemEvent == CNAME( EditorTick ) )
	{
		// Automatic background offset
		if ( m_autoScroll.x || m_autoScroll.y )
		{
			// Move to client space
			wxPoint mousePos = CanvasToClient( m_lastMousePos );

			// Scroll
			ScrollBackgroundOffset( m_autoScroll );

			// Fake mouse movement
			wxMouseEvent event( wxEVT_MOTION );
			event.m_x = mousePos.x;
			event.m_y = mousePos.y;
			MouseMove( event, -m_autoScroll );			
		}

		// Automatic scaling
		if ( GetScale() != m_desiredScale )
		{
			// Set new scale
			ScaleGraph( m_desiredScale );//( getScale(), m_DesiredScale, 0.05f ) );

			// Repaint
			Repaint( true );
		}

		// Marked to repaint at some point later
		if ( IsShownOnScreen() )
		{
			if ( m_shouldRepaint )
			{
				Repaint( true );
				m_shouldRepaint = false;
			}

			if ( m_shouldZoom )
			{
				ZoomExtents();
				m_shouldZoom = false;
			}
		}
	}
}

void CEdGraphEditor::GetSocketLinkParams( CGraphSocket* socket, wxPoint& pos, wxPoint &dir )
{
	if ( socket )
	{
		// Get block
		CGraphBlock* block = socket->GetBlock();
		BlockLayoutInfo* info = m_layout.FindPtr( block );

		// Get socket
		if ( info )
		{
			// Get socket info
			SocketLayoutInfo* sinfo = info->m_sockets.FindPtr( socket );
			if ( sinfo )
			{
				// Set position
				pos.x = sinfo->m_linkPos.x + block->GetPosition().X;
				pos.y = sinfo->m_linkPos.y + block->GetPosition().Y;

				// Set direction
				dir.x = sinfo->m_linkDir.x;
				dir.y = sinfo->m_linkDir.y;

				// Done
				return;
			}
		}	
	}

	// Use floating mouse position
	pos = m_lastMousePos;
	dir = wxPoint(0,0);
}

THandle< ISerializable > CEdGraphEditor::GetActiveItem( const wxPoint& graphPoint )
{
	// No graph
	if ( !m_graph )
	{
		return NULL;
	}

	// Are we link dragging ? 
	Bool isLinkDragging = m_action == MA_DraggingLink;

	// Get all blocks
	TDynArray< CGraphBlock* > &blocks = m_graph->GraphGetBlocks();

	// Linear search

	// Sockets
	CGraphSocket* activeSocket = NULL;
	for ( Uint32 i=0; i<blocks.Size(); i++ )
	{
		// Get layout
		CGraphBlock* block = blocks[i];
		const BlockLayoutInfo *info = m_layout.FindPtr( block );

		// Test only blocks with valid layout
		if ( info && info->m_onScreen && info->m_visible && !info->m_freeze )
		{
			// Block offset
			wxPoint offset(block->GetPosition().X, block->GetPosition().Y);

			// Test each socket from block
			wxPoint localPoint = graphPoint - offset;

			for ( THashMap< THandle< CGraphSocket >, SocketLayoutInfo>::const_iterator j = info->m_sockets.Begin(); j != info->m_sockets.End(); ++j )
			{
				const SocketLayoutInfo& sinfo = j->m_second;
				
				if( j->m_first.Get() == NULL )
				{
					continue;
				}

				CGraphSocket* socket = j->m_first.Get();

				// Filter :)
				if ( (isLinkDragging && socket->CanEndLink()) || (!isLinkDragging && socket->CanStartLink() ) )
				{
					// Get socket info
					if ( sinfo.m_socketRect.Contains( localPoint ) )
					{
						// Over a socket
						return socket;
					}
				}
			}
		}
	}

	CGraphBlock* candidate = NULL;
	// Blocks
	for ( Uint32 i=0; i<blocks.Size(); i++ )
	{
		// Get layout
		CGraphBlock* block = blocks[i];
		const BlockLayoutInfo *info = m_layout.FindPtr( block );

		// Test only blocks with valid layout
		if ( info && info->m_onScreen && info->m_visible && !info->m_freeze )
		{
			// Get block rect
			wxRect blockRect;
			blockRect.x = block->GetPosition().X;
			blockRect.y = block->GetPosition().Y;

			wxRect blockResizeRect = GetResizeRect( block, info );

			if ( block->IsInnerAreaTransparent() )
			{
				blockRect.width = info->m_titleRect.GetWidth();
				blockRect.height = info->m_titleRect.GetHeight();
			}
			else
			{
				blockRect.width = info->m_windowSize.x;
				blockRect.height = info->m_windowSize.y;
			}

			if ( ( blockRect.Contains( graphPoint ) || blockResizeRect.Contains( graphPoint ) )	&& 
				 ( !candidate || block->GetBlockDepthGroup()>candidate->GetBlockDepthGroup() ) )
			{
				// Over a window
				candidate = block;
			}
		}
	}

	// Links
	for ( Uint32 i=0; i<blocks.Size(); i++ )
	{
		// Get layout
		CGraphBlock* block = blocks[i];
		const BlockLayoutInfo *info = m_layout.FindPtr( block );

		// Test only blocks with valid layout
		if ( info && info->m_visible && !info->m_freeze )
		{
			// Block offset
			wxPoint offset(block->GetPosition().X, block->GetPosition().Y);

			// Test each socket from block
			wxPoint localPoint = graphPoint - offset;

			for ( THashMap< THandle< CGraphSocket >, SocketLayoutInfo>::const_iterator j = info->m_sockets.Begin(); j != info->m_sockets.End(); ++j )
			{
				const SocketLayoutInfo& sinfo = j->m_second;

				if( j->m_first.Get() == NULL )
				{
					continue;
				}

				CGraphSocket* socket = j->m_first.Get();

				// Test each socket connection

				// Connection is hide
				if ( socket->IsNoDraw() )//&& !socket->ForceDrawConnections() )
				{
					continue;
				}

				// Only for output - connection is between input and output :)
				if ( socket->GetDirection() == LSD_Input )
				{
					continue;
				}

				// Get socket connections
				const TDynArray< CGraphConnection* >& connections = socket->GetConnections();
				for ( Uint32 j=0; j<connections.Size(); j++ )
				{
					CGraphConnection* con = connections[j];
					if ( con->GetSource( true ) == socket )
					{
						// Need 4 points for Bezier curve
						wxPoint src, srcDir, dest, destDir;

						// Source
						src = sinfo.m_linkPos + offset;
						srcDir = sinfo.m_linkDir;

						// Destination
						GetSocketLinkParams( con->GetDestination( true ), dest, destDir );

						// Check AABB for fast selection test ( HitTestLink is slow )
						if ( !HitTestLinkAABB( src, dest, graphPoint ) )
						{
							continue;
						}

						// Case for loop connection
						if ( con->GetSource( true )->GetBlock() == con->GetDestination( true )->GetBlock() )
						{
							srcDir.y = -2;
							destDir.y = -2;
						}

						// Calculate distance between end points
						Int32 dist = Max( Abs( src.x-dest.x ), Abs( src.y-dest.y ) );
						Int32 offset = 5.0f + Min( dist / 3.0f, 200.0f );

						// Setup link
						wxPoint points[4];
						points[0].x = src.x;
						points[0].y = src.y;
						points[1].x = src.x + srcDir.x * offset;
						points[1].y = src.y + srcDir.y * offset;
						points[2].x = dest.x + destDir.x * offset;
						points[2].y = dest.y + destDir.y * offset;
						points[3].x = dest.x;
						points[3].y = dest.y;

						// How far
						Float range = 10.0f;

						// Min distance from socket
						Float minDistFromSocket = 15.0f;

						if ( HitTestLink( points, graphPoint, minDistFromSocket, range) )
						{
							return con;
						}
					}
				}
			}
		}
	}

	return candidate;
}

void CEdGraphEditor::UpdateActiveItem( wxPoint mousePoint )
{
	// Convert to graph coords
	wxPoint pos = ClientToCanvas( mousePoint );

	// Get active item under cursor
	THandle< ISerializable > item = GetActiveItem( pos );
	if ( item != m_activeItem )
	{
		// Set new active item
		m_activeItem = item;

		// Repaint
		Repaint();
	}
}


void CEdGraphEditor::GetBlocksOnBlock( TDynArray< CGraphBlock* >& blocks, CGraphBlock* block ) const
{
	wxRect blockRect;
	const BlockLayoutInfo *info = m_layout.FindPtr( block );
	blockRect.x = block->GetPosition().X;
	blockRect.y = block->GetPosition().Y;
	blockRect.width = info->m_windowSize.x;
	blockRect.height = info->m_windowSize.y;
	// collecting overlaying blocks

	// Get all blocks
	TDynArray< CGraphBlock* > tblocks;
	tblocks = m_graph->GraphGetBlocks();
	for ( Uint32 i = 0; i < tblocks.Size(); ++i )
	{
		// Get layout
		CGraphBlock* block2 = tblocks[i];

		if ( block2 == block )
			continue;

		const BlockLayoutInfo *info2 = m_layout.FindPtr( block2 );

		// Test only blocks with valid layout
		if ( info2 )
		{
			if ( blockRect.Contains( wxPoint( block2->GetPosition().X + ( info2->m_windowSize.x >> 1 ), block2->GetPosition().Y + ( info2->m_windowSize.y >> 1 ) ) ) )
			{
				blocks.PushBack( block2 );
			}
		}
	}

}

Bool CEdGraphEditor::IsDraggedByClickOnInnerArea( wxMouseEvent& event, CGraphBlock* block )
{
	return block->IsDraggedByClickOnInnerArea();
}

void CEdGraphEditor::MouseClick( wxMouseEvent& event )
{
	wxPoint mousePoint = event.GetPosition();
	wxPoint graphPoint = ClientToCanvas( mousePoint );
	// Pass to base class
	CEdCanvas::MouseClick( event );	

	// Deleting connection on ALT+click
	if ( event.AltDown() && event.LeftDown() )
	{
		if ( m_activeItem.IsValid() && m_activeItem.Get()->IsA< CGraphConnection >() && ModifyGraphStructure() )
		{
			CGraphConnection* connection = static_cast< CGraphConnection* >( m_activeItem.Get() );
			DeleteConnection( connection );
			GraphStructureModified();
			return;
		}
		else if ( m_activeItem.IsValid() && m_activeItem.Get()->IsA< CGraphSocket >() && ModifyGraphStructure() )
		{
			CGraphSocket* socket = static_cast< CGraphSocket* >( m_activeItem.Get() );
			DeleteSocketConnections( socket );
			GraphStructureModified();
			return;
		}
		else if ( m_activeItem.IsValid() && m_activeItem.Get()->IsA< CGraphBlock >() && ModifyGraphStructure() )
		{
			CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );
			DeleteBlockConnections( block );
			GraphStructureModified();
			return;
		}
	}

	// Zooming
	if ( event.ShiftDown() )
	{
		// Only when in no mode
		if ( m_action == MA_None )
		{
			if ( event.LeftDown() )
			{
				// Full zoom in
				m_desiredScale = 1.0f;
				ScaleGraph( 1.0f );
				// Don't do any more processing
				return;
			}
			else if ( event.RightDown() )
			{
				// Full zoom out
				m_desiredScale = 0.1f;
				ScaleGraph( 0.1f );
				// Don't do any more processing
				return;
			}
		}
	}

	// Background drag
	if ( m_action == MA_None && event.RightDown() )
	{
		m_moveTotal	 = 0;
		m_action = MA_BackgroundScroll;
		CaptureMouse( true, true );
	}
	else if ( m_action == MA_BackgroundScroll && event.RightUp() )
	{
		// Uncapture
		m_action = MA_None;
		CaptureMouse( false, true );

		UpdateActiveItem( mousePoint );

		// Minimal movement, show menu
		if ( m_moveTotal < 5 )
		{
			OpenContextMenu();
		}
	}

	// Click
	if ( m_action == MA_None && event.LeftDown() )
	{
		// Block clicked, select
		if ( m_activeItem.Get() && m_activeItem.Get()->IsA< CGraphBlock >() )
		{
			CGraphBlock* block = static_cast< CGraphBlock* >( m_activeItem.Get() );

			const BlockLayoutInfo *info = m_layout.FindPtr( block );

			// Select only if layout data is valid
			if ( info && info->m_visible && !info->m_freeze )
			{
				if ( event.ControlDown() )
				{
					// invert selection of active block if control is pressed
					SelectBlock( block, !IsBlockSelected( block ) );
				}
				else
				{
					if ( !IsBlockSelected( block ) )
					{
						// if block is not selected, deselect all others and select it
						SelectBlock( block, true, true );
					}				

					wxRect blockResizeRect = GetResizeRect( block, info );
					wxRect blockFreezeRect = GetFreezeRect( block, info );					
					wxRect moveWithoutContentRect = GetMoveWithoutContentRect( block, info );

					if ( block->IsResizable() && blockResizeRect.Contains( graphPoint ) )
					{
						CaptureMouse( true, false );
						m_action = MA_RescalingWindows;
						m_rescalePressPoint = wxPoint( graphPoint.x - blockResizeRect.x, graphPoint.y - blockResizeRect.y );
					}
					else
					{
						if ( block->CanFreezeContent() && blockFreezeRect.Contains( graphPoint ) )
						{
							TDynArray< CGraphBlock* > blocks;
							GetBlocksOnBlock( blocks, block );
							bool containsNonFreezed = false;
							for ( Uint32 i = 0; i < blocks.Size(); ++i )
							{
								BlockLayoutInfo* layout = m_layout.FindPtr( blocks[i] );
								if ( layout && !layout->m_freeze )
								{
									containsNonFreezed = true;
									break;
								}
							}
							for ( Uint32 i = 0; i < blocks.Size(); ++i )
							{
								BlockLayoutInfo* layout = m_layout.FindPtr( blocks[i] );
								if ( layout )
								{
									layout->m_freeze = containsNonFreezed;
								}
							}
						}
						else
						{
							BlockLayoutInfo* layout = m_layout.FindPtr( block );
							wxRect innerRect;
							innerRect.x = block->GetPosition().X + layout->m_titleRect.x;
							innerRect.y = block->GetPosition().Y + layout->m_titleRect.y;
							innerRect.width = info->m_titleRect.width;
							innerRect.height = info->m_titleRect.height;
							if ( !IsDraggedByClickOnInnerArea( event, block ) && !innerRect.Contains( graphPoint ) )
							{
								if ( block->IsInnerAreaTransparent() )
								{
									m_activeItem = NULL;
								}
							}
							else
							{
								// Get list of selected block
								TDynArray< CGraphBlock* > selectedBlocks;
								GetSelectedBlocks( selectedBlocks );

								// If there are selected windows begin dragging
								if ( selectedBlocks.Size() )
								{
									m_windowsMoved = false;
									m_action = MA_MovingWindows;
									m_blocksToMove.ClearFast();
									// Add selected blocks to the moving set
									m_blocksToMove.PushBackUnique( selectedBlocks );
									CaptureMouse( true, false );
									if ( block && block->IsMovingOverlayingBlocks() && !moveWithoutContentRect.Contains( graphPoint ) )
									{
										GetBlocksOnBlock( m_blocksToMove, block );
									}
									// Store click offsets for later use
									m_movePressPoints.ClearFast();
									for ( Uint32 i=0; i<m_blocksToMove.Size(); ++i )
									{
										Vector blockPos = m_blocksToMove[i]->GetPosition();
										wxPoint pressPoint = graphPoint - wxPoint( blockPos.X, blockPos.Y );
										m_movePressPoints.Insert( m_blocksToMove[i], pressPoint );
									}
								}
							}
						}
					}
				}
			}
		}

		// Link drag
		if ( m_activeItem.Get() && m_activeItem.Get()->IsA< CGraphSocket >() )
		{
			// Setup link
			m_sourceSocket = static_cast< CGraphSocket* >( m_activeItem.Get() );
			m_destSocket = NULL;

			// Begin drag
			m_action = MA_DraggingLink;
			CaptureMouse( true, false );
		}

		// Connection clicked, select
		if ( m_activeItem.Get() && m_activeItem.Get()->IsA< CGraphConnection >() && ModifyGraphStructure() )
		{
			CGraphConnection* con = static_cast<CGraphConnection*>( m_activeItem.Get() );

			TDynArray<CGraphConnection*> conDest =  con->GetDestination( true )->GetConnections();

			// Switch state
			Bool newState = !con->IsActive();
			if ( m_undoManager )
			{
				CUndoGraphConnectionActivity::PrepareStep( *m_undoManager, this, con, newState );
			}
			con->SetActive( newState );

			// Click detection is only for output. Switch for input.
			for( Uint32 i=0; i<conDest.Size(); ++i )
			{
				// Need to check both sides
				if
				(
					conDest[i]->GetDestination( true ) == con->GetSource( true ) && 
					conDest[i]->GetSource( true ) == con->GetDestination( true )
				)
				{
					if ( m_undoManager )
					{
						CUndoGraphConnectionActivity::PrepareStep( *m_undoManager, this, conDest[i], newState );
					}
					conDest[i]->SetActive( newState );
				}
			}

			if ( m_undoManager )
			{
				CUndoGraphConnectionActivity::FinalizeStep( *m_undoManager );
			}

			GraphStructureModified();
		}

		// Selection rect
		if ( NULL == m_activeItem.Get() )
		{
			// Initialize selection rect
			wxPoint point = event.GetPosition();
			m_selectRectEnd = m_selectRectStart = ClientToCanvas( point );

			// Start mode
			m_action = MA_SelectingWindows;
			CaptureMouse( true, false );
		}

		// Repaint
		Repaint();
	}

	// Finished movement
	if ( m_action==MA_MovingWindows && event.LeftUp() )
	{
		if ( !m_windowsMoved )
		{
			// select active block
			CGraphBlock* block = Cast<CGraphBlock>( m_activeItem.Get() );
			SelectBlock( block, true, true );

			Repaint();
		}
		else
		{
			if ( m_undoManager )
			{
				CUndoGraphBlockMove::FinalizeStep( *m_undoManager );
			}
		}

		m_action = MA_None;
		CaptureMouse( false, false );
	}

	// Finished rescaling
	if ( m_action==MA_RescalingWindows && event.LeftUp() )
	{
		if ( m_undoManager )
		{
			CUndoGraphBlockLayout::FinalizeStep( *m_undoManager );
		}

		m_action = MA_None;
		CaptureMouse( false, false );
	}

	// Finished link dragging
	if ( m_action==MA_DraggingLink && event.LeftUp() )
	{
		// Uncapture
		m_action = MA_None;
		CaptureMouse( false, false );

		// Create link
		if ( m_sourceSocket && m_destSocket && ModifyGraphStructure() )
		{
			// Connect
			ConnectSockets( m_sourceSocket, m_destSocket );		
			GraphStructureModified();
		}

		// Repaint
		Repaint();
	}

	// Finished window selection
	if ( m_action==MA_SelectingWindows && event.LeftUp() )
	{
		// End drag
		m_action = MA_None;
		CaptureMouse( false, false );

		// Assemble rect
		wxRect selectionRect;
		selectionRect.x = Min( m_selectRectStart.x, m_selectRectEnd.x );
		selectionRect.y = Min( m_selectRectStart.y, m_selectRectEnd.y );
		selectionRect.width = Abs( m_selectRectEnd.x - m_selectRectStart.x );
		selectionRect.height = Abs( m_selectRectEnd.y - m_selectRectStart.y );

		// Select blocks from area
		SelectBlocksFromArea( selectionRect, !event.ControlDown() );

		// Repaint
		Repaint();
	}
}

wxRect CEdGraphEditor::GetResizeRect( const CGraphBlock* block, const BlockLayoutInfo* info ) const
{
	wxRect blockResizeRect;
	blockResizeRect.x = block->GetPosition().X + info->m_windowSize.x - GRAPH_BLOCK_RESIZE_RECT_SIZE;
	blockResizeRect.y = block->GetPosition().Y + info->m_windowSize.y - GRAPH_BLOCK_RESIZE_RECT_SIZE;
	blockResizeRect.width = GRAPH_BLOCK_RESIZE_RECT_SIZE;
	blockResizeRect.height = GRAPH_BLOCK_RESIZE_RECT_SIZE;

	return blockResizeRect;
}

wxRect CEdGraphEditor::GetFreezeRect( const CGraphBlock* block, const BlockLayoutInfo* info ) const
{
	wxRect blockFreezeRect;
	blockFreezeRect.x = block->GetPosition().X;
	blockFreezeRect.y = block->GetPosition().Y + info->m_windowSize.y - GRAPH_BLOCK_RESIZE_RECT_SIZE;
	blockFreezeRect.width = GRAPH_BLOCK_RESIZE_RECT_SIZE;
	blockFreezeRect.height = GRAPH_BLOCK_RESIZE_RECT_SIZE;

	return blockFreezeRect;
}

wxRect CEdGraphEditor::GetMoveWithoutContentRect( const CGraphBlock* block, const BlockLayoutInfo* info ) const
{
	wxRect moveWithoutContentRect;
	moveWithoutContentRect.x = block->GetPosition().X + info->m_windowSize.x - GRAPH_BLOCK_RESIZE_RECT_SIZE;
	moveWithoutContentRect.y = block->GetPosition().Y;
	moveWithoutContentRect.width = GRAPH_BLOCK_RESIZE_RECT_SIZE;
	moveWithoutContentRect.height = GRAPH_BLOCK_RESIZE_RECT_SIZE;

	return moveWithoutContentRect;
}

void CEdGraphEditor::DeleteBlockConnections( CGraphBlock* block )
{
	const auto& sockets = block->GetSockets();
	for ( auto socket : sockets )
	{
		DeleteSocketConnections( socket );
	}
}

void CEdGraphEditor::DeleteSocketConnections( CGraphSocket* socket )
{
	const auto& connections = socket->GetConnections();
	for ( auto connection : connections )
	{
		if ( m_undoManager )
		{
			CUndoGraphConnectionExistance::PrepareDeletionStep( *m_undoManager, this, connection );
		}
	}
	CUndoGraphConnectionExistance::FinalizeStep( *m_undoManager );
	socket->BreakAllLinks();
}

void CEdGraphEditor::DeleteConnection( CGraphConnection* connection )
{
	if ( m_undoManager )
	{
		CUndoGraphConnectionExistance::PrepareDeletionStep( *m_undoManager, this, connection );
		CUndoGraphConnectionExistance::FinalizeStep( *m_undoManager );
	}
	connection->GetSource( true )->DisconnectFrom( connection->GetDestination( true ) );
}

void CEdGraphEditor::ConnectSockets( CGraphSocket* srcSocket, CGraphSocket* destSocket )
{
	if ( m_autoUnplugSingleConnections )
	{
		TDynArray< CGraphConnection* > blocking = CollectBlockingConnections( srcSocket, destSocket );
	
		// make the room for the connection
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

	if ( CGraphConnection* created = srcSocket->ConnectTo( destSocket, true ) )
	{
		if ( m_undoManager )
		{
			CUndoGraphConnectionExistance::PrepareCreationStep( *m_undoManager, this, created );
		}
	}

	CUndoGraphConnectionExistance::FinalizeStep( *m_undoManager );
}

void CEdGraphEditor::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	wxPoint point = event.GetPosition();
	wxPoint graphPoint = ClientToCanvas( point );

	THandle< ISerializable > prevActiveItem = m_activeItem;

	// Block resizment
	if ( m_action == MA_RescalingWindows )
	{
		Float scale = GetScale();
		CGraphBlock* block = Cast<CGraphBlock>( m_activeItem.Get() );
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( layout )
		{
			if ( m_undoManager )
			{
				CUndoGraphBlockLayout::PrepareStep( *m_undoManager, this, block, layout );
			}

			layout->m_windowSize.x = Max<Int32>( 0, graphPoint.x - block->GetPosition().X + m_rescalePressPoint.x );
			layout->m_windowSize.y = Max<Int32>( 0, graphPoint.y - block->GetPosition().Y + m_rescalePressPoint.y );

			block->SetSize( Vector( layout->m_windowSize.x, layout->m_windowSize.y, 0.0f ) );
			UpdateBlockLayout( block );

			CheckBlocksVisibility();

			// Repaint
			Repaint();
		}
	}

	// Update active item;
	if ( m_action != MA_BackgroundScroll )
	{
		UpdateActiveItem( point );
	}

	if ( m_action == MA_RescalingWindows )
	{
		m_activeItem = prevActiveItem;
	}

	// Accumulate move
	m_moveTotal += Abs( delta.x ) + Abs( delta.y );

	// Reset auto scroll
	m_autoScroll = wxPoint(0,0);

	// Remember mouse position
	m_lastMousePos = ClientToCanvas( point );

	// Background drag
	if ( m_action == MA_BackgroundScroll )
	{
		// Update background offset
		Float scale = GetScale();
		ScrollBackgroundOffset( wxPoint( delta.x / scale, delta.y / scale ) );

		CheckBlocksVisibility();

		// Repaint
		Repaint();
	}

	// Block movement
	if ( m_action == MA_MovingWindows )
	{
		// windows were moved
		m_windowsMoved = true;

		// Move blocks
		Float scale = GetScale();
		MoveSelectedBlocks( graphPoint );

		CheckBlocksVisibility();

		// Repaint
		Repaint();
	}

	// Selection rect
	if ( m_action == MA_SelectingWindows )
	{
		// Set selection rect
		m_selectRectEnd = ClientToCanvas( point );

		// Repaint
		Repaint();
	}

	// Link dragging
	if ( m_action == MA_DraggingLink )
	{
		// Grab destination socket from active element
		CGraphSocket* socket = Cast< CGraphSocket >( m_activeItem.Get() );
		if ( socket && m_sourceSocket->CanConnectTo( socket ) && socket->CanConnectTo( m_sourceSocket ) )
		{
			// Set as possible destination
			m_destSocket = socket;
		}
		else
		{
			// No socket
			m_destSocket = NULL;
		}

		// Repaint
		Repaint();
	}

	// Setup auto scroll
	if ( m_action == MA_MovingWindows || m_action == MA_DraggingLink || m_action == MA_SelectingWindows || m_action == MA_RescalingWindows )
	{
		// Get client size
		Int32 width, height;
		GetClientSize( &width, &height );

		// X edges
		m_autoScroll.x += (event.GetX() < 10) ? 5 : 0;
		m_autoScroll.x -= (event.GetX() > ( width - 10 )) ? 5 : 0;

		// Y edge
		m_autoScroll.y += (event.GetY() < 10) ? 5 : 0;
		m_autoScroll.y -= (event.GetY() > ( height - 10 )) ? 5 : 0;

		CheckBlocksVisibility();
	}
}

void CEdGraphEditor::OnMouseWheel( wxMouseEvent& event )
{
	// Scale
	Float scale = GetScale();
	Float delta = ( event.GetWheelDelta() / (FLOAT)event.GetWheelRotation() );
	scale = Clamp< Float >( scale + delta * 0.10f, 0.10f, 2.0f );

	// Set new desired scale
	m_desiredScale = scale;
	ScaleGraph( scale );
}

void CEdGraphEditor::OnMouseMiddleEvent( wxMouseEvent& event )
{
	if ( m_graph->GetLayerNum() > 0 )
	{
		/*TDynArray< CGraphBlock* > blocks;
		GetSelectedBlocks( blocks );

		wxPoint position = CanvasToClient( m_lastMousePos );
		if ( !blocks.Empty() )
		{
		// Move selected blocks to layer
		CEdGraphLayerEditor layerEditor( this, position );
		layerEditor.SelectLayersForBlocks( blocks );
		}
		else
		{
		// Show layer editor for layers selection
		CEdGraphLayerEditor layerEditor( this, position );
		layerEditor.ChooseLayersStates();
		}

		ModifyGraphStructure();*/
	}
	else
	{
		event.Skip();
	}
}

void CEdGraphEditor::OnKeyDown( wxKeyEvent& event )
{
    if ( event.GetKeyCode() == WXK_HOME )
    {
        ZoomExtents();
    }

    event.Skip();
}

void CEdGraphEditor::OnSetCursor( wxSetCursorEvent& event )
{
}

void CEdGraphEditor::DeleteSelection()
{
	if ( !CanBeModify() )
	{
		return;
	}

	// Get selected blocks
	TDynArray< CGraphBlock* > blocks;
	GetSelectedBlocks( blocks );

	// Ask
	if ( blocks.Size() && YesNo( TXT("Delete selected blocks ?") ) && ModifyGraphStructure() )
	{
		// Remove blocks
		for ( Uint32 i=0; i<blocks.Size(); i++ )
		{
			if ( m_graph->GraphCanRemoveBlock( blocks[i] ) )
			{
				if ( m_undoManager )
				{
					CUndoGraphBlockExistance::PrepareDeletionStep( *m_undoManager, this, blocks[i] );
				}

				m_graph->GraphRemoveBlock( blocks[i] );
			}
		}

		if ( m_undoManager )
		{
			CUndoGraphBlockExistance::FinalizeStep( *m_undoManager );
		}

		// Deselect removed blocks
		DeselectAllBlocks();

		GraphStructureModified();
	}
}

Bool CEdGraphEditor::CopyBlocksToClipboard( const TDynArray< CGraphBlock* >& blocks, bool isCopy )
{
	// Only if there are any blocks to copy
	if ( !blocks.Size() )
	{
		return false;
	}

	// Serialize to memory package
	TDynArray< Uint8 > buffer;
	CMemoryFileWriter writer( buffer );
	CDependencySaver saver( writer, NULL );

	// Save object
	DependencySavingContext context( (const DependencyMappingContext::TObjectsToMap&) blocks );
	if ( !saver.SaveObjects( context ) )
	{
		WARN_EDITOR( TXT("Unable to copy selected objects to clipboard") );
		return false;
	}

	// Open clipboard
	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new CClipboardData( String(TXT("Blocks")) + ClipboardChannelName() , buffer, isCopy ) );
		wxTheClipboard->Close();
	}		

	return true;
}

void CEdGraphEditor::CopySelection( bool isCopy )
{
	TDynArray< CGraphBlock* > blocks;
	GetSelectedBlocks( blocks );

	CopyBlocksToClipboard( blocks, isCopy );
}

void CEdGraphEditor::CutSelection()
{
	if ( !CanBeModify() )
	{
		return;
	}

	// Get selection
	TDynArray< CGraphBlock* > blocks;
	GetSelectedBlocks( blocks );

	if ( ModifyGraphStructure() && CopyBlocksToClipboard( blocks, false ) )
	{
		// Remove blocks
		for ( Uint32 i=0; i<blocks.Size(); i++ )
		{
			if ( m_graph->GraphCanRemoveBlock( blocks[i] ) )
			{
				if ( m_undoManager )
				{
					CUndoGraphBlockExistance::PrepareDeletionStep( *m_undoManager, this, blocks[i] );
				}

				m_graph->GraphRemoveBlock( blocks[i] );
			}
		}

		if ( m_undoManager )
		{
			CUndoGraphBlockExistance::FinalizeStep( *m_undoManager );
		}

		// Selection changed
		if ( m_hook )
		{
			m_hook->OnGraphSelectionChanged();
		}
		GraphStructureModified();
	}
}

TDynArray< CGraphBlock* > CEdGraphEditor::Paste( const Vector* position, Bool atLeftUpper /*= false*/, Bool doNotCreateUndoSteps /*= false*/ )
{
	TDynArray< CGraphBlock* > pastedBlocks;

	if ( !CanBeModify() )
	{
		return pastedBlocks;
	}

	// Paste only to valid graph
	if ( m_graph )
	{
		// Open clipboard
		if ( wxTheClipboard->Open())
		{
			CClipboardData data( String(TXT("Blocks")) + ClipboardChannelName() );
			if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
			{
				// Extract data from the clipboard
				if ( wxTheClipboard->GetData( data ) )
				{
					// Determine spawn position
					Bool relativePosition = false;
					Vector spawnPosition = Vector::ZEROS;
					if ( position )
					{
						// Use given position
						relativePosition = true;
						spawnPosition = *position;
					}
					else 
					{
						// Get selection
						TDynArray< CGraphBlock* > blocks;
						GetSelectedBlocks( blocks );

						if ( blocks.Size() )
						{
							// Use position from selection
							relativePosition = true;
							spawnPosition = blocks[0]->GetPosition() + Vector( 32, 32, 0 );
						}
					}

					if ( ModifyGraphStructure() )
					{
						OnGraphStructureWillBeModified();

						// Paste entities
						if ( m_graph->GraphPasteBlocks( data.GetData(), pastedBlocks, relativePosition, atLeftUpper, spawnPosition ) )
						{
							for ( Uint32 i=0; i<pastedBlocks.Size(); i++ )
							{
								pastedBlocks[ i ]->OnPasted( data.IsCopy() );
							}

							// Select pasted blocks
							SelectBlocks( pastedBlocks, true );

							// make pasting reversible by storing the undo step
							if ( m_undoManager )
							{
								for ( Uint32 i=0; i<pastedBlocks.Size(); i++ )
								{
									CUndoGraphBlockExistance::PrepareCreationStep( *m_undoManager, this, pastedBlocks[i] );
								}

								CUndoGraphBlockExistance::FinalizeStep( *m_undoManager );
							}
						}
						GraphStructureModified();
					}
				}

				// Selection changed
				if ( m_hook )
				{
					m_hook->OnGraphSelectionChanged();
				}
			}

			// Close clipboard
			wxTheClipboard->Close();
		}
	}

	return pastedBlocks;
}

Bool CEdGraphEditor::HitTestLinkAABB( const wxPoint& source, const wxPoint& dest, const wxPoint& testPoint ) const
{
	wxPoint leftTop( ::Min<Int32>( source.x, dest.x ), ::Min<Int32>( source.y, dest.y ) );
	wxPoint rightDown( ::Max<Int32>( source.x, dest.x ), ::Max<Int32>( source.y, dest.y ) );

	wxRect rect( leftTop, rightDown );

	return rect.Contains( testPoint ) ? true : false;
}

Bool CEdGraphEditor::HitTestLinkAABB( const wxPoint& source, const wxPoint& dest, const wxRect& testRect ) const
{
	wxPoint leftTop( ::Min<Int32>( source.x, dest.x ), ::Min<Int32>( source.y, dest.y ) );
	wxPoint rightDown( ::Max<Int32>( source.x, dest.x ), ::Max<Int32>( source.y, dest.y ) );

	wxRect rect( leftTop, rightDown );

	return testRect.Intersects( rect ) ? true : false;
}

Bool CEdGraphEditor::HitTestLink( const wxPoint *points, const wxPoint& point, const Float minDistFromSocket, const Float range )
{
	const Vector testPoint( point.x, point.y, 0 );

	// Initialize bezier blend weights on first use
	const Uint32 numPoints = 20;
	static Vector bezierWeights[ numPoints+1 ];
	if ( !bezierWeights[0].X )
	{
		for ( Uint32 i=0; i<=numPoints; i++ )
		{
			Float t = i / (Float)numPoints;
			bezierWeights[i].W = t*t*t;
			bezierWeights[i].Z = 3.0f*t*t*(1.0f-t);
			bezierWeights[i].Y = 3.0f*t*(1.0f-t)*(1.0f-t);
			bezierWeights[i].X = (1.0f-t)*(1.0f-t)*(1.0f-t);
		}
	}

	// Test bounding box
	Int32 minX = Min( Min( points[0].x, points[1].x ), Min( points[2].x, points[3].x ) ) - range;
	Int32 minY = Min( Min( points[0].y, points[1].y ), Min( points[2].y, points[3].y ) ) - range;
	Int32 maxX = Max( Max( points[0].x, points[1].x ), Max( points[2].x, points[3].x ) ) + range;
	Int32 maxY = Max( Max( points[0].y, points[1].y ), Max( points[2].y, points[3].y ) ) + range;
	if ( point.x >= minX && point.y >= minY && point.x <= maxX && point.y <= maxY )
	{
		// Hit test curve
		Vector lastPoint;
		Vector sourcePoint( points[0].x, points[0].y, 0, 1);
		Vector destPoint( points[3].x, points[3].y, 0, 1);

		for ( Uint32 i=0; i<=numPoints; i++ )
		{
			const Vector weights = bezierWeights[i];
			const Float px = (points[0].x * weights.X) + (points[1].x * weights.Y) + (points[2].x * weights.Z) + (points[3].x * weights.W);
			const Float py = (points[0].y * weights.X) + (points[1].y * weights.Y) + (points[2].y * weights.Z) + (points[3].y * weights.W);
			const Vector point( px, py, 0, 1 );

			// Calculate distance to curve
			if ( i )
			{
				const Vector delta = point - lastPoint;
				const Float s = Vector::Dot3( lastPoint, delta );
				const Float e = Vector::Dot3( point, delta );
				const Float t = Vector::Dot3( testPoint, delta );
				Float dist, distSource, distDest;
				if ( t < s )
				{
					dist = lastPoint.DistanceTo( testPoint );
					distSource = lastPoint.DistanceTo( sourcePoint );
					distDest = lastPoint.DistanceTo( destPoint );
				}
				else if ( t > e )
				{
					dist = point.DistanceTo( testPoint );
					distSource = point.DistanceTo( sourcePoint );
					distDest = point.DistanceTo( destPoint );
				}
				else
				{
					const Vector projected = lastPoint + delta * (t - s);
					dist = projected.DistanceTo( testPoint );
					distSource = projected.DistanceTo( sourcePoint );
					distDest = projected.DistanceTo( destPoint );
				}

				// Close enough
				if ( dist < range && distDest > minDistFromSocket && distSource > minDistFromSocket )
				{
					return true;
				}
			}

			// Remember for next pass
			lastPoint = point;
		}
	}

	// Not hit
	return false;
}

Bool CEdGraphEditor::IsBlockVisible( CGraphBlock* block, const BlockLayoutInfo* layout /* = NULL  */) const
{
	// Get canvas data
	wxSize canvasSize = GetCanvasSize();
	Float scale = GetScale();

	// Block location
	wxPoint location;
	location.x = block->GetPosition().X;
	location.y = block->GetPosition().Y;

	wxPoint locationInScreen = CanvasToClient( location );

	if ( layout )
	{
		if (!( locationInScreen.x > canvasSize.GetWidth() || locationInScreen.x + layout->m_windowSize.x * scale < 0 || locationInScreen.y > canvasSize.GetHeight() || locationInScreen.y + layout->m_windowSize.y * scale < 0 ))
		{
			return true;
		}
	}
	else
	{
		const BlockLayoutInfo* l = m_layout.FindPtr( block );

		if ( !l )
		{
			return false;
		}

		if (!( locationInScreen.x > canvasSize.GetWidth() || locationInScreen.x + l->m_windowSize.x * scale < 0 || locationInScreen.y > canvasSize.GetHeight() || locationInScreen.y + l->m_windowSize.y * scale < 0 ))
		{
			return true;
		}
	}

	return false;
}

Vector CEdGraphEditor::GetSizeFromLayout( CGraphBlock* block ) const
{
	if ( const BlockLayoutInfo* layout = GetBlockLayout( block ) )
	{
		return Vector( layout->m_windowSize.GetX(), layout->m_windowSize.GetY(), 0.0 );
	}
	else
	{
		return Vector( 0.0, 0.0, 0.0 );
	}
}

void CEdGraphEditor::ResetBlocksVisibility( const TDynArray< CGraphBlock* >& blocks )
{
	for ( Uint32 i=0; i < blocks.Size(); i++ )
	{
		CGraphBlock* block = blocks[i];

		// Find layout info
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( !layout )
		{
			continue;
		}

		// Reset flags
		layout->m_onScreen = false;
		layout->m_visible = false;
		layout->m_freeze = false;
	}
}

void CEdGraphEditor::CheckBlocksVisibility()
{
	// Get blocks
	TDynArray< CGraphBlock* > blocks;

	if ( m_graph )
	{
		blocks = m_graph->GraphGetBlocks();
	}
	else
	{
		return;
	}

	// Layers flag
	Uint32 layersVisibleFlag = m_graph->GetLayersInStateFlag( GLS_Visible );
	Uint32 layersFreezeFlag = m_graph->GetLayersInStateFlag( GLS_Freeze );

	for ( Uint32 i=0; i < blocks.Size(); i++ )
	{
		CGraphBlock* block = blocks[i];

		// Find layout info
		BlockLayoutInfo* layout = m_layout.FindPtr( block );
		if ( !layout )
		{
			continue;
		}

		// Reset flags
		layout->m_onScreen = false;
		layout->m_visible = false;

		// Check if block is visible on the screen
		if ( IsBlockVisible( block, layout ) )
		{
			layout->m_onScreen = true;
		}

		// Set flags
		if ( block->IsInLayer( layersVisibleFlag ) )
		{
			layout->m_visible = true;
		}
		else if ( block->IsInLayer( layersFreezeFlag ) )
		{
			layout->m_freeze = true;
			layout->m_visible = true;
		} 
	}
}

wxColor CEdGraphEditor::ConvertBlockColorToFreezeMode( const wxColor &color )
{
	Int32 value = Int32(0.3f * color.Red() + 0.59 * color.Red() + 0.11f * color.Blue());
	return wxColor(value, value, value, 0.4f * color.Alpha());
}

wxColor CEdGraphEditor::ConvertLinkColorToFreezeMode( const wxColor &color )
{
	Int32 value = Int32(0.3f * color.Red() + 0.59 * color.Red() + 0.11f * color.Blue());
	return wxColor(value, value, value, 0.2f * color.Alpha());
}

wxSize CEdGraphEditor::GetGraphArea() const
{
	return GetParent()->GetSize();
}

TDynArray< CGraphConnection* > CEdGraphEditor::CollectBlockingConnections( const CGraphSocket* srcSocked, const CGraphSocket* dstSocket ) const
{
	TDynArray< CGraphConnection* > blocking;

	// if this slot is single-link, and there is already a connection, it should be removed

	if ( !srcSocked->IsMultiLink() && srcSocked->HasConnections( true ) )
	{
		ASSERT ( srcSocked->GetConnections().Size() == 1 );
		ASSERT ( srcSocked->GetConnections()[0]->GetDestination() != dstSocket );
		blocking.PushBack( srcSocked->GetConnections()[0] );
	}

	if ( !dstSocket->IsMultiLink() && dstSocket->HasConnections( true ) )
	{
		ASSERT ( dstSocket->GetConnections().Size() == 1 );
		ASSERT ( dstSocket->GetConnections()[0]->GetDestination() != srcSocked );
		blocking.PushBack( dstSocket->GetConnections()[0] );
	}

	return blocking;
}