/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "treeEditor.h"

#include "undoTreeEditor.h"

BEGIN_EVENT_TABLE( CEdTreeEditor, CEdCanvas )
	EVT_MOUSEWHEEL( CEdTreeEditor::OnMouseWheel )
END_EVENT_TABLE()

namespace
{
	const Int32 HORZ_MARGIN  = 10;
	const Int32 ICON_SIZE    = 30;
	const Int32 BLOCK_HEIGHT = 33;
	const Int32 MIN_H_DIST   = 15;
	const Int32 MIN_V_DIST   = 50;
}

/************************************************************************/
/* Generic TODO: link selection                                         */
/************************************************************************/

CEdTreeEditor::CEdTreeEditor( wxWindow * parent, IHook * hook, Bool blockPosRelativeToParent, Bool allowAddByDragging, Bool allowDecorateByDragging )
	: CEdCanvas( parent )
	, m_lastMousePos( 0,0 )
	, m_selectRectStart( 0,0 )
	, m_selectRectEnd( 0,0 )
	, m_autoScroll( 0,0 )
	, m_moveTotal( 0 )
	, m_action( MA_None )
	, m_activeItem( NULL )
	, m_activeItemGizmo( GL_None )
	, m_hook  ( hook )
	, m_allowAddByDragging( allowAddByDragging )
	, m_allowDecorateByDragging( allowDecorateByDragging )
	, m_blockPosRelativeToParent( blockPosRelativeToParent )
	, m_undoManager( NULL )
	, m_blockMoved( false )
	, m_lockBitmap( NULL )
{
	wxBitmap wxBmp = SEdResources::GetInstance().LoadBitmap( TXT("IMG_CHECK_IN") );
	if ( wxBmp.IsOk() )
	{
		m_lockBitmap = ConvertToGDI( wxBmp );
	}
}

CEdTreeEditor::~CEdTreeEditor()
{
	if ( m_lockBitmap ) delete m_lockBitmap;
	ClearLayout();
}

/************************************************************************/
/* Block layout                                                         */
/************************************************************************/
void CEdTreeEditor::ClearLayout()
{
	THashMap< IScriptable*, LayoutInfo* >::iterator
		currBlock = m_layout.Begin(),
		lastBlock = m_layout.End();
	for ( ; currBlock != lastBlock; ++currBlock )
		delete currBlock->m_second;
	m_layout.Clear();
	SetActiveItem( NULL );
}

const wxColour CEdTreeEditor::LayoutInfo::DEFAULT_TEXT_COLOR( 255, 255, 255 );
const wxColour CEdTreeEditor::LayoutInfo::DEFAULT_BG_COLOR( 142, 142, 142 );
const wxColour CEdTreeEditor::LayoutInfo::INVALID_BG_COLOR( 0, 0, 0 );

CEdTreeEditor::LayoutInfo * CEdTreeEditor::UpdateBlockLayout( IScriptable * block, LayoutInfo * parentLayout )
{
	// Find or create layout info
	LayoutInfo * layout = NULL;
	if ( ! m_layout.Find( block, layout ) )
	{
		m_layout.Insert( block, new LayoutInfo() );
		m_layout.Find( block, layout );
	}
	layout->m_owner  = block;
	layout->m_parent = parentLayout;

	// Calculate text size
	wxPoint size = TextExtents( GetGdiBoldFont(), GetBlockName( *block ) );

	// Setup size to some default size
	layout->m_windowSize.x = 2*HORZ_MARGIN + size.x;
	layout->m_windowSize.y = BLOCK_HEIGHT;

	// Bitmap
	String bmpName = GetBitmapName( *block );
	if ( !bmpName.Empty() )
	{
		if ( layout->m_bitmap == NULL )
		{
			wxBitmap wxBmp = SEdResources::GetInstance().LoadBitmap( bmpName.AsChar() );
			if ( wxBmp.IsOk() )
			{
				layout->m_bitmap = ConvertToGDI( wxBmp );
			}
		}

		layout->m_windowSize.x += ICON_SIZE + HORZ_MARGIN;
	}

	if ( ! LoadBlockPosition( block, layout->m_windowPos ) )
	{
		layout->m_windowPos.x = layout->m_parent ? layout->m_parent->m_windowPos.x + 50 : 0;
		layout->m_windowPos.y = layout->m_parent ? layout->m_parent->m_windowPos.y + 50 : 0;
	}
	else if ( m_blockPosRelativeToParent && layout->m_parent )
	{
		layout->m_windowPos += layout->m_parent->m_windowPos + layout->m_parent->m_windowSize/2;
	}

	layout->m_windowPos -= layout->m_windowSize/2;

	// Create connector
	layout->m_connectorSrc.x = layout->m_windowSize.x / 2;
	layout->m_connectorSrc.y = layout->m_windowSize.y;
	layout->m_connectorDest.x = layout->m_windowSize.x / 2;
	layout->m_connectorDest.y = 0;

	layout->m_textColor = LayoutInfo::DEFAULT_TEXT_COLOR;
	layout->m_bgColor = LayoutInfo::DEFAULT_BG_COLOR;

	// top gizmo
	if ( m_allowDecorateByDragging && !IsLocked( *block ) && layout->m_parent && CanHaveChildren( *layout->m_parent->m_owner ) )
	{
		layout->m_topGizmo = LayoutInfo::CONNECTOR;
	}
	else
	{
		layout->m_topGizmo = LayoutInfo::NONE;
	}

	// Bottom gizmo
	if ( IsHiddenBranch( *block ) )
	{
		layout->m_bottomGizmo = LayoutInfo::ELLIPSIS;
	}
	else if ( m_allowAddByDragging && !IsLocked( *block ) && CanHaveChildren( *block ) )
	{
		layout->m_bottomGizmo = LayoutInfo::CONNECTOR;
	}
	else
	{
		layout->m_bottomGizmo = LayoutInfo::NONE;
	}

	return layout;
}

CEdTreeEditor::LayoutInfo* CEdTreeEditor::GetLayout( IScriptable* block )
{
	LayoutInfo* layout;
	if ( m_layout.Find( block, layout ) )
	{
		return layout;
	}
	else
	{
		return NULL;
	}
}

Int32 CEdTreeEditor::MinDistanceBetween( LayoutInfo* layout1, LayoutInfo* layout2 ) const
{
	return ( layout1->m_parent == layout2->m_parent ) ? MIN_H_DIST : MIN_H_DIST*2;
}

void CEdTreeEditor::BuildLevels( LayoutInfo* parent, TDynArray< TreeLayerInfo >& layers, Uint32 level ) const
{
	ASSERT ( layers.Size() >= level );

	if ( layers.Size() == level )
	{
		layers.PushBack( TreeLayerInfo() );
	}

	Uint32 thisParentRangeStart = layers[level].Size();

  	for ( auto layoutI = m_layout.Begin(); layoutI != m_layout.End(); ++layoutI )
	{
		if ( layoutI->m_second->m_parent == parent )
		{
			layers[level].PushBack( layoutI->m_second );
		}
	}

	if ( thisParentRangeStart < layers[level].Size() )
	{
		// Sort children by the original horizontal order
		Sort( layers[level].Begin() + thisParentRangeStart, layers[level].End(), 
			[ ]( const LayoutInfo* l1, const LayoutInfo* l2 ) { 
				return l1->m_windowPos.x < l2->m_windowPos.x;
			});

		// Note sort BEFORE going down
		for ( Uint32 newChildIdx = thisParentRangeStart; newChildIdx < layers[level].Size(); ++newChildIdx )
		{
			BuildLevels( layers[level][newChildIdx], layers, level+1 );
		}
	}
}

Bool CEdTreeEditor::CalculateChildrenExtend( const LayoutInfo* parent, const TreeLayerInfo & childLayer, Int32& start, Int32& end ) const
{
	Bool found = false;

	for ( Uint32 i=0; i<childLayer.Size(); ++i )
	{
		if ( childLayer[i]->m_parent == parent )
		{
			start = childLayer[i]->m_windowPos.x;
			found = true;
			break;
		}
	}

	if ( !found )
	{
		return false;
	}

	for ( Int32 i = static_cast< Int32 >( childLayer.Size() ) - 1; i>=0; --i )
	{
		if ( childLayer[i]->m_parent == parent )
		{
			end = childLayer[i]->m_windowPos.x + childLayer[i]->m_windowSize.x;
			found = true;
			break;
		}
	}

	ASSERT ( found );

	return true;
}

void CEdTreeEditor::MoveOnLevel( const TreeLayerInfo& layer, Uint32 index, Int32 newPos )
{
	LayoutInfo* parent = layer[ index ];

	Int32 oldPos = parent->m_windowPos.x;
	Int32 shift = newPos - oldPos;

	parent->m_windowPos.x = newPos;

	if ( shift < 0 )
	{ 
		if ( index > 0 )
		{
			LayoutInfo* prevParent = layer[ index-1 ];
			Int32 distToPrev = parent->m_windowPos.x - ( prevParent->m_windowPos.x + prevParent->m_windowSize.x );

			Int32 minDist = MinDistanceBetween( parent, prevParent );
			if ( distToPrev < minDist )
			{
				// Correct already processed (to the left) parents recursively
				Int32 corr = distToPrev - minDist;
				for ( Uint32 leftIdx = 0; leftIdx < index; ++leftIdx )
				{
					layer[ leftIdx ]->m_windowPos.x += corr;
					RecursivelyMoveBlocks( *layer[ leftIdx ], wxPoint( corr, 0 ) );
				}
				for ( Uint32 rightIdx = index+1; rightIdx < layer.Size(); ++rightIdx )
				{
					layer[ rightIdx ]->m_windowPos.x += corr;
				}
			}
		}
	}
	else
	{
		if ( index < layer.Size()-1 )
		{
			LayoutInfo* nextParent = layer[ index+1 ];
			Int32 distToNext = nextParent->m_windowPos.x - ( parent->m_windowPos.x + parent->m_windowSize.x );

			Int32 minDist = MinDistanceBetween( parent, nextParent );
			if ( distToNext < minDist )
			{
				// Correct not yet processed (to the right) parents WITHOUT recurse
				Int32 corr = minDist - distToNext;
				for ( Uint32 rightIdx = index+1; rightIdx < layer.Size(); ++rightIdx )
				{
					layer[ rightIdx ]->m_windowPos.x += corr;
				}
			}
		}
	}
}

void CEdTreeEditor::AutoLayout( CObject* block )
{
	TDynArray< TreeLayerInfo > levels;

	wxPoint rootPos( 0, 0 );
	LayoutInfo* layout = NULL;
	Uint32 level = 0;
	if ( block )
	{
		if ( m_layout.Find( block, layout ) )
		{
			rootPos = layout->m_windowPos;
			TreeLayerInfo rootLayer;
			rootLayer.PushBack( layout );
			levels.PushBack( rootLayer );
			++level;
		}
	}

	BuildLevels( layout, levels, level );

	// Remove empty level generated by BuildLevels
	ASSERT ( levels.Back().Empty() );
	levels.PopBack();

	if ( levels.Size() == 0 )
	{
		return;
	}

	// There should be at least one element on the root level
	ASSERT ( !levels[0].Empty() );

	// Store the current positions (for undo)
	THashMap< LayoutInfo*, wxPoint > oldPositions;
	if ( m_undoManager )
	{
		for ( auto layoutI = m_layout.Begin(); layoutI != m_layout.End(); ++layoutI )
		{
			oldPositions.Insert( layoutI->m_second, layoutI->m_second->m_windowPos );
		}
	}

	// Initial (packed) layout
	for ( Uint32 levelIdx = 0; levelIdx < levels.Size(); ++levelIdx )
	{
		int posX = 0;
		for ( Uint32 layoutIdx = 0, layoutLast = levels[levelIdx].Size()-1; layoutIdx <= layoutLast; ++layoutIdx )
		{
			levels[levelIdx][layoutIdx]->m_windowPos.x = posX;
			if ( layoutIdx != layoutLast )
			{
				LayoutInfo* thisLayout = levels[levelIdx][layoutIdx];
				LayoutInfo* nextLayout = levels[levelIdx][layoutIdx+1];
				posX += thisLayout->m_windowSize.x + MinDistanceBetween( thisLayout, nextLayout );
			}
		}
	}

	TDynArray< Int32 > maxExtendPerLevel;
	maxExtendPerLevel.Resize( levels.Size() );

	// Center parents
	for ( Int32 levelIdx = static_cast< Int32 >( levels.Size()-1 ); levelIdx > 0; --levelIdx )
	{
		maxExtendPerLevel[ levelIdx-1 ] = 0;

		const TreeLayerInfo& parentLayouts = levels[ levelIdx-1 ];

		for ( Uint32 parentIdx = 0; parentIdx < parentLayouts.Size(); ++parentIdx )
		{
			Int32 start, end;
			if ( CalculateChildrenExtend( parentLayouts[ parentIdx ], levels[ levelIdx ], start, end ) )
			{
				Int32 extend = end - start;

				if ( extend > maxExtendPerLevel[ levelIdx-1 ] )
				{
					maxExtendPerLevel[ levelIdx-1 ] = extend;
				}

				Int32 newPos = (start + end)/2 - parentLayouts[ parentIdx ]->m_windowSize.x/2;
				MoveOnLevel( parentLayouts, parentIdx, newPos );
			}
		}
	}

	// Calculate vertical layout - spacing is based on the maximum children horizontal extend (looks better this way)
	int posY = rootPos.y;
	for ( Uint32 levelIdx = 0; levelIdx != levels.Size(); ++levelIdx )
	{
		for ( Uint32 layoutIdx = 0; layoutIdx != levels[levelIdx].Size(); ++layoutIdx )
		{
			levels[levelIdx][layoutIdx]->m_windowPos.y = posY;
		}

		posY += Max( maxExtendPerLevel[levelIdx]/3, MIN_V_DIST );
	}

	// Move the root back to original position (or 0,0 if we layout the whole tree)
	wxPoint finalOffset = rootPos - levels[0][0]->m_windowPos;
	for ( Uint32 layoutIdx = 0; layoutIdx < levels[0].Size(); ++layoutIdx )
	{
		LayoutInfo& layout = *levels[0][layoutIdx];
		layout.m_windowPos += finalOffset;
		RecursivelyMoveBlocks( layout, finalOffset );
	}

	// Write newly calculated positions back to associated objects
	for ( Uint32 layoutIdx = 0; layoutIdx < levels[0].Size(); ++layoutIdx )
	{
		LayoutInfo& layout = *levels[0][layoutIdx];
		DoSavePosition( layout );
		RecursivelySavePosition( layout );
	}

	if ( m_undoManager )
	{
		for ( auto layoutI = m_layout.Begin(); layoutI != m_layout.End(); ++layoutI )
		{
			wxPoint oldPos;
			if ( oldPositions.Find( layoutI->m_second, oldPos ) )
			{
				wxPoint offset = layoutI->m_second->m_windowPos - oldPos;
				if ( offset != wxPoint( 0, 0 ) )
				{
					CUndoTreeBlockMove::PrepareStep( *m_undoManager, this, layoutI->m_first, offset, true );
				}
			}
		}

		CUndoTreeBlockMove::FinalizeStep( *m_undoManager );
	}

	Refresh();
}


/************************************************************************/
/* Mouse handling                                                       */
/************************************************************************/
void CEdTreeEditor::OnMouseWheel( wxMouseEvent& event )
{
	// Scale
	Float scale = GetScale();
	Float delta = ( event.GetWheelDelta() / (FLOAT)event.GetWheelRotation() );
	scale = Clamp< Float >( scale + delta * 0.10f, 0.10f, 2.0f );

	// Set new desired scale
	m_desiredScale = scale;

	// Automatic scaling
	if ( GetScale() != m_desiredScale )
	{
		// Set new scale
		ScaleGraph( m_desiredScale );

		// Repaint
		Repaint( true );
	}
}

void CEdTreeEditor::MouseClick( wxMouseEvent& event )
{
	// Pass to base class
	CEdCanvas::MouseClick( event );	

	// Convert to graph coords
	wxPoint pos = ClientToCanvas( event.GetPosition() );

	// Background drag
	if ( m_action == MA_None && event.RightDown() )
	{
		m_moveTotal	 = 0;
		m_action = MA_BackgroundScroll;
		m_activeItem = NULL;
		CaptureMouse( true, true );
	}
	else if ( m_action == MA_BackgroundScroll && event.RightUp() )
	{
		// Uncapture
		m_action = MA_None;
		CaptureMouse( false, true );
		UpdateActiveItem( event.GetPosition() );

		// Minimal movement, show menu
		if ( m_moveTotal < 5 )
		{
			OnOpenContextMenu();
		}
	}

	// Click
	if ( m_action == MA_None && event.LeftDown() )
	{
		// Connector drag
		if ( m_allowAddByDragging )
		{
			for ( auto currBlock : m_layout )
			{
				LayoutInfo * layout = currBlock.m_second;

				Bool top    = layout->m_topGizmo    == LayoutInfo::CONNECTOR && GetGizmoRect( *layout, GL_Top )   .Contains( pos );
				Bool bottom = layout->m_bottomGizmo == LayoutInfo::CONNECTOR && GetGizmoRect( *layout, GL_Bottom ).Contains( pos );
				if ( top || bottom )
				{
					m_activeItem = layout;
					m_action     = MA_DraggingLink;
					CaptureMouse( true, false );
				}
			}
		}

		if ( m_allowDecorateByDragging )
		{
		}

		// Default actions
		if ( m_action == MA_None )
		{
			if ( m_activeItem )
			{
				if ( m_activeItemGizmo != GL_None )
				{
					// Let the descendant class decide what to do
					OnGizmoClicked( m_activeItem, m_activeItemGizmo );
				}
				else
				{
					// Block has been clicked, select it and move
					if ( ! m_selected.Exist( m_activeItem->m_owner ) )
					{
						// No shift, deselect all blocks first
					
						if ( ! event.ShiftDown() )
						{
							DeselectAllObjects();
						}

						if ( m_activeItem ) // m_activeItem may be cleared from within DeselectAllObjects
						{
							// Select clicked object
							SelectObject( m_activeItem->m_owner, true );
						}
					}
					// Select children with Control
					if ( event.ControlDown() )
						SelectRecursive( m_activeItem, true );

					// Movement
					m_action = event.ControlDown() ? MA_MovingWindowsAlt : MA_MovingWindows;
					CaptureMouse( true, false );
				}
			}
			else
			{
				// Initialize selection rect
				m_selectRectStart = ClientToCanvas( event.GetPosition() );
				m_selectRectEnd = ClientToCanvas( event.GetPosition() ); 

				// Start mode
				m_action = MA_SelectingWindows;
				CaptureMouse( true, false );

				// No shift, deselect all blocks first
				if ( ! event.ShiftDown() )
				{
					DeselectAllObjects();
				}
			}
		}

		// Repaint
		Repaint();
	}

	// Finished movement
	if ( ( m_action==MA_MovingWindows || m_action==MA_MovingWindowsAlt ) && event.LeftUp() )
	{
		m_action = MA_None;

		if ( m_blockMoved ) // call only if sth was actually moved
		{
			if ( m_undoManager )
			{
				CUndoTreeBlockMove::FinalizeStep( *m_undoManager );
			}

			OnMoveEnded();
			m_blockMoved = false;
		}

		CaptureMouse( false, false );
	}

	// Finished link dragging
	if ( m_action==MA_DraggingLink && event.LeftUp() )
	{
		// Uncapture
		m_action = MA_None;
		CaptureMouse( false, false );

		// Finalize link
		if ( m_activeItem )
		{
			ASSERT ( m_activeItemGizmo != GL_None );

			wxPoint addedPos = pos;
			if ( m_blockPosRelativeToParent )
			{
				if ( m_activeItemGizmo == GL_Top && m_activeItem->m_parent )
				{ // decoration mode - the new block will be added under a parent
					addedPos -= m_activeItem->m_parent->m_windowPos + m_activeItem->m_parent->m_windowSize/2;
					addedPos.y -= BLOCK_HEIGHT/2;
				}
				else
				{
					addedPos -= m_activeItem->m_windowPos + m_activeItem->m_windowSize/2;
					addedPos.y += BLOCK_HEIGHT/2;
				}
			}

			OnAddByDragging( addedPos, m_activeItemGizmo );
			m_activeItemGizmo = GL_None;
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

		UpdateDraggedSelection( event.GetPosition(), true );

		// Repaint
		Repaint();
	}
}

void CEdTreeEditor::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	// Update active item;
	if ( m_action == MA_None )
	{
		UpdateActiveItem( event.GetPosition() );
	}

	// Accumulate move
	m_moveTotal += Abs( delta.x ) + Abs( delta.y );

	// Reset auto scroll
	m_autoScroll = wxPoint(0,0);

	// Remember mouse position
	m_lastMousePos = ClientToCanvas( event.GetPosition() );

	// Background drag
	if ( m_action == MA_BackgroundScroll )
	{
		// Update background offset
		Float scale = GetScale();
		SetOffset( GetOffset() + wxPoint( delta.x / scale, delta.y / scale ) ); 
		// Repaint
		Repaint();
	}

	// Block movement
	if ( m_action == MA_MovingWindows || m_action == MA_MovingWindowsAlt )
	{
		// Move blocks
		Float scale = GetScale();
		MoveSelectedBlocks( wxPoint( delta.x / scale, delta.y / scale ), m_action == MA_MovingWindowsAlt  );

		// Repaint
		Repaint();
	}

	// Selection rect
	if ( m_action == MA_SelectingWindows )
	{
		UpdateDraggedSelection( event.GetPosition(), false );
		// Repaint
		Repaint();
	}

	// Setup auto scroll
	if ( m_action == MA_MovingWindows || m_action == MA_MovingWindowsAlt || m_action == MA_DraggingLink || m_action == MA_SelectingWindows )
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
	}

	// Link dragging
	if ( m_action == MA_DraggingLink )
	{
		// Repaint
		Repaint();
	}
}

void CEdTreeEditor::ScaleGraph( Float newScale )
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
		SetOffset( GetOffset() + offset ); 
	}
}

void CEdTreeEditor::CleanUpSelection( Bool removeChildren, Bool removeLocked )
{
	for ( Int32 i = static_cast< Int32 >( m_selected.Size() ) - 1; i >= 0; --i )
	{
		LayoutInfo* layout;
		if ( m_layout.Find( m_selected[i], layout ) )
		{
			if ( removeLocked && IsLocked( *layout->m_owner ) )
			{
				m_selected.EraseFast( m_selected.Begin() + i );
			}
			else if ( removeChildren )
			{
				LayoutInfo* parentLayout = layout->m_parent;
				while ( parentLayout )
				{
					if ( m_selected.Exist( parentLayout->m_owner ) )
					{
						m_selected.EraseFast( m_selected.Begin() + i );
						break;
					}

					parentLayout = parentLayout->m_parent;
				}
			}
		}
	}
}

void CEdTreeEditor::RecursivelyMoveBlocks( const LayoutInfo& parent, wxPoint offset )
{
	for ( auto layoutIt = m_layout.Begin(); layoutIt != m_layout.End(); ++layoutIt )
	{
		LayoutInfo* layout = layoutIt->m_second;
		if ( layout->m_parent == &parent )
		{
			layout->m_windowPos += offset;		
			RecursivelyMoveBlocks( *layout, offset );
		}
	}
}

void CEdTreeEditor::DoSavePosition( const LayoutInfo& layout )
{
	wxPoint posToSave = layout.m_windowPos + layout.m_windowSize/2;

	if ( m_blockPosRelativeToParent && layout.m_parent )
	{
		posToSave -= layout.m_parent->m_windowPos + layout.m_parent->m_windowSize/2;
	}

	SaveBlockPosition( layout.m_owner, posToSave );
}

void CEdTreeEditor::RecursivelySavePosition( const LayoutInfo& parent )
{
	for ( auto layoutIt = m_layout.Begin(); layoutIt != m_layout.End(); ++layoutIt )
	{
		LayoutInfo* layout = layoutIt->m_second;
		if ( layout->m_parent == &parent )
		{
			DoSavePosition( *layout );
			RecursivelySavePosition( *layout );
		}
	}
}

void CEdTreeEditor::MoveBlock( LayoutInfo* layout, wxPoint totalOffset, Bool alternate )
{
	layout->m_windowPos += totalOffset;

	wxPoint posToSave = layout->m_windowPos + layout->m_windowSize/2;

	if ( m_blockPosRelativeToParent )
	{
		if ( alternate )
		{
			RecursivelySavePosition( *layout );
		}
		else
		{
			RecursivelyMoveBlocks( *layout, totalOffset );
		}

		if ( layout->m_parent )
		{
			posToSave -= layout->m_parent->m_windowPos + layout->m_parent->m_windowSize/2;
		}
	}

	SaveBlockPosition( layout->m_owner, posToSave );
}

void CEdTreeEditor::MoveSelectedBlocks( wxPoint offset, Bool alternate )
{
	CleanUpSelection( m_blockPosRelativeToParent, true );

	for ( Uint32 i = 0; i < m_selected.Size(); ++i )
	{
		LayoutInfo* layout;
		if ( m_layout.Find( m_selected[i], layout ) )
		{
			if ( m_undoManager )
			{
				CUndoTreeBlockMove::PrepareStep( *m_undoManager, this, layout->m_owner, offset, alternate );
			}

			MoveBlock( layout, offset, alternate );
			m_blockMoved = true;
		}
	}
}

void CEdTreeEditor::ZoomExtents()
{
	Int32	minX =  NumericLimits<Int32>::Max(),
		minY =  NumericLimits<Int32>::Max(),
		maxX = -NumericLimits<Int32>::Max(),
		maxY = -NumericLimits<Int32>::Max();

	if ( m_layout.Empty() )
	{
		wxSize windowSize = GetParent()->GetSize();
		minX = 0;
		minY = 0;
		maxX = windowSize.x;
		maxY = windowSize.y;
	}
	else
	{
		auto currBlock = m_layout.Begin(), lastBlock = m_layout.End();
		for ( ; currBlock != lastBlock; ++currBlock )
		{
			minX = Min( minX, currBlock->m_second->m_windowPos.x );
			minY = Min( minY, currBlock->m_second->m_windowPos.y );
			maxX = Max( maxX, currBlock->m_second->m_windowPos.x+currBlock->m_second->m_windowSize.x );
			maxY = Max( maxY, currBlock->m_second->m_windowPos.y+currBlock->m_second->m_windowSize.y );
		}
	}

	wxSize  windowSize = GetParent()->GetSize();
	wxPoint graphCenter( ( maxX + minX ) / 2, ( maxY + minY ) / 2 );
	wxSize  graphSize( (Int32)( ( maxX - minX ) * 1.2f ) , (Int32)( ( maxY - minY ) * 1.2f ) );

	Float xScale = (Float)windowSize.GetWidth() / graphSize.GetWidth();
	Float yScale = (Float)windowSize.GetHeight() / graphSize.GetHeight();

	Float scale = Min( xScale, yScale );
	scale = Clamp( scale, 0.1f, 2.0f );

	wxPoint graphCorner( graphCenter.x - ( (windowSize.x / 2) / scale ), graphCenter.y - ( (windowSize.y / 2) / scale ) );	
	wxPoint newOffset( -graphCorner.x , -graphCorner.y );

	SetScale( scale );
	m_desiredScale = scale;
	SetOffset( newOffset );
}
/************************************************************************/
/* Selection handling                                                   */
/************************************************************************/
void CEdTreeEditor::DeselectAllObjects( Bool notifyHook /*= true*/ )
{
	// Clear selection list
	m_selected.Clear();

	// Send event
	if ( notifyHook && m_hook )
	{
		m_hook->OnGraphSelectionChanged();
	}
}

Bool CEdTreeEditor::IsObjectSelected( IScriptable * object )
{
	ASSERT( object );
	return m_selected.Exist( object );
}

void CEdTreeEditor::SelectObject( IScriptable * object, Bool select, Bool notifyHook /*= true*/ )
{
	// Generic case
	ASSERT( object );
	if ( select )
	{
		m_selected.PushBackUnique( object );
	}
	else
	{
		m_selected.Remove( object );
	}

	// Send event
	if ( notifyHook && m_hook )
	{
		m_hook->OnGraphSelectionChanged();
	}
}

void CEdTreeEditor::SelectRecursive( LayoutInfo* rootLayout, Bool select, Bool notifyHook /*= true*/ )
{
	if ( rootLayout )
		SelectObject( rootLayout->m_owner, select, false );
	
	auto currBlock = m_layout.Begin(), lastBlock = m_layout.End();
	for ( ; currBlock != lastBlock; ++currBlock )
	{
		LayoutInfo * layout = currBlock->m_second;
		if ( layout->m_parent == rootLayout )
			SelectRecursive( layout, select, false );
	}

	// Send event
	if ( notifyHook && m_hook )
	{
		m_hook->OnGraphSelectionChanged();
	}
}

void CEdTreeEditor::SelectObjectsFromArea( wxRect area, Bool sendEvent )
{
	DeselectAllObjects( false );

	auto currBlock = m_layout.Begin(), lastBlock = m_layout.End();
	for ( ; currBlock != lastBlock; ++currBlock )
	{
		LayoutInfo * layout = currBlock->m_second;

		wxRect rect( layout->m_windowPos, layout->m_windowSize );
		if ( area.Intersects( rect ) && ! IsObjectSelected( currBlock->m_first ) )
		{
			SelectObject( currBlock->m_first, true, false );
		}
	}

	// Send event
	if ( sendEvent && m_hook )
	{
		m_hook->OnGraphSelectionChanged();
	}
}

void CEdTreeEditor::UpdateDraggedSelection( const wxPoint& pos, Bool sendEvent )
{
	// Set selection rect
	m_selectRectEnd = ClientToCanvas( pos );

	wxRect selectionRect;
	selectionRect.x = Min( m_selectRectStart.x, m_selectRectEnd.x );
	selectionRect.y = Min( m_selectRectStart.y, m_selectRectEnd.y );
	selectionRect.width = Abs( m_selectRectEnd.x - m_selectRectStart.x );
	selectionRect.height = Abs( m_selectRectEnd.y - m_selectRectStart.y );

	// Select blocks from area
	SelectObjectsFromArea( selectionRect, sendEvent );
}

void CEdTreeEditor::UpdateActiveItem( wxPoint mousePoint )
{
	// Reset active item
	LayoutInfo * activeItem = NULL;
	GizmoLocation activeItemGizmo = GL_None;

	// Convert to graph coords
	wxPoint pos = ClientToCanvas( mousePoint );

	auto currBlock = m_layout.Begin(), lastBlock = m_layout.End();
	for ( ; currBlock != lastBlock; ++currBlock )
	{
		LayoutInfo * layout = currBlock->m_second;

		if ( layout->m_topGizmo != LayoutInfo::NONE && GetGizmoRect( *layout, GL_Top ).Contains( pos ) )
		{ // first check the gizmo (is on the top)
			activeItem = layout;
			activeItemGizmo = GL_Top;
		}
		else if ( layout->m_bottomGizmo != LayoutInfo::NONE && GetGizmoRect( *layout, GL_Bottom ).Contains( pos ) )
		{
			activeItem = layout;
			activeItemGizmo = GL_Bottom;
		}
		else if ( wxRect( layout->m_windowPos, layout->m_windowSize ).Contains( pos ) )
		{ // then check the whole node rectangle
			activeItem = layout;
		}
		// do not break if found, select the last (topmost one)
	}

	// New active node/gizmo
	if ( activeItem != m_activeItem || activeItemGizmo != m_activeItemGizmo )
	{
		m_activeItem = activeItem;
		m_activeItemGizmo = activeItemGizmo;
		Repaint();
	}
}

/************************************************************************/
/* Drawing on the canvas                                                */
/************************************************************************/
void CEdTreeEditor::PaintCanvas( Int32 width, Int32 height )
{
	// Colors
	wxColour back = GetCanvasColor();
	Clear( back );

	// First, draw links, to keep them under blocks
	for ( auto currBlock = m_layout.Begin(); currBlock != m_layout.End(); ++currBlock )
	{
		DrawBlockLinks( *currBlock->m_second );
	}

	for ( auto currBlock = m_layout.Begin(); currBlock != m_layout.End(); ++currBlock )
	{
		DrawBlockLayout( *currBlock->m_second );
	}

	// Draw selection rect
	if ( m_action == MA_SelectingWindows )
	{
		// Get size and pos
		wxRect selRect;
		selRect.x = Min( m_selectRectStart.x, m_selectRectEnd.x );
		selRect.y = Min( m_selectRectStart.y, m_selectRectEnd.y );
		selRect.width = Abs( m_selectRectEnd.x - m_selectRectStart.x );
		selRect.height = Abs( m_selectRectEnd.y - m_selectRectStart.y );

		// Selection rect
		FillRect( selRect, wxColour( 0, 50, 100, 96 ) );
		DrawRect( selRect, wxColour( 0, 100, 200 ) );
	}

	// Link drawing
	if ( m_action == MA_DraggingLink )
	{
		DrawDraggedLink();
	}
}

void CEdTreeEditor::DrawBlockLayout( LayoutInfo & layout )
{
	Bool selected = IsObjectSelected( layout.m_owner );
	Bool active = ( m_activeItem == &layout );
	Bool locked = IsLocked( *layout.m_owner );

	// Get colors
	wxColour borderClr = selected
		? wxColour( 255, 255, 0 ) 
		: wxColour( 0, 0, 0 );
	wxColour shadowClr( 0, 0, 0, 128 );
	wxColour backgroundClr = locked
		? wxColour( layout.m_bgColor.Red(), layout.m_bgColor.Green(), layout.m_bgColor.Blue(), layout.m_bgColor.Alpha()/2 )
		: layout.m_bgColor;
	wxColour commentClr( 0, 255, 0 );

	wxColour textClr = layout.m_textColor;

	if ( textClr == wxColour( 255, 255, 255 ) )
	{
		Float lvl = 0.3*backgroundClr.Red() + 0.6*backgroundClr.Green() + 0.1*backgroundClr.Blue();
		if ( lvl > 128 )
		{
			textClr = wxColour( 0, 0, 0 );
		}
	}

	if ( !locked )
	{
		FillRoundedRect( layout.m_windowPos.x + 3, layout.m_windowPos.y + 3, layout.m_windowSize.x, layout.m_windowSize.y, shadowClr, 8 );
	}
	// Draw window area
	FillRoundedRect( layout.m_windowPos.x, layout.m_windowPos.y, layout.m_windowSize.x, layout.m_windowSize.y, backgroundClr, 8 );
	DrawRoundedRect( layout.m_windowPos.x, layout.m_windowPos.y, layout.m_windowSize.x, layout.m_windowSize.y, borderClr, 8, selected ? 2 : 1 );

	// Draw bitmap
	if ( layout.m_bitmap != NULL )
	{
		wxRect bmpPos( layout.m_windowPos.x + HORZ_MARGIN, layout.m_windowPos.y + layout.m_windowSize.y/2 - ICON_SIZE/2, ICON_SIZE, ICON_SIZE );
		DrawImage( layout.m_bitmap, bmpPos );
	}

	// Draw active block marker
 	if ( active && m_activeItemGizmo == GL_None )
	{
		FillRoundedRect( layout.m_windowPos.x, layout.m_windowPos.y, layout.m_windowSize.x, layout.m_windowSize.y, wxColour( 255, 255, 255, 128 ), 8 );
	}

	const String & blockName = GetBlockName( *layout.m_owner );
	if ( !blockName.Empty() )
	{
		wxPoint pt( layout.m_windowPos.x + layout.m_windowSize.x/2, layout.m_windowPos.y + 7 );
		if ( layout.m_bitmap != NULL )
		{
			pt.x += ICON_SIZE/2 + HORZ_MARGIN/2;
		}

		DrawText( pt, GetGdiBoldFont(), blockName, textClr, CEdCanvas::CVA_Top, CEdCanvas::CHA_Center );
	}

	// Draw locked state indicator
	if ( locked )
	{
		wxRect bmpPos( layout.m_windowPos.x + layout.m_windowSize.x - 16, layout.m_windowPos.y + layout.m_windowSize.y - 4, 16, 16 );
		DrawImage( m_lockBitmap, bmpPos );
	}

	const String& comment = GetBlockComment( *layout.m_owner );
	if( !comment.Empty() )
	{
		wxPoint size = TextExtents( GetGdiBoldFont(), comment );
		wxPoint pt( layout.m_windowPos.x + layout.m_windowSize.x / 2, layout.m_windowPos.y - size.y - 1 );

		if ( layout.m_topGizmo != LayoutInfo::NONE )
		{
			pt.y -= 5;
		}

		wxColour bk = GetCanvasColor();
		FillRect( pt.x-size.x/2, pt.y, size.x, size.y, wxColour( bk.Red(), bk.Green(), bk.Blue(), 160 ) );

		DrawText( pt, GetGdiBoldFont(), comment, commentClr, CEdCanvas::CVA_Top, CEdCanvas::CHA_Center );
	}

	if ( layout.m_topGizmo != LayoutInfo::NONE )
	{
		DrawGizmo( layout, GL_Top, active && m_activeItemGizmo == GL_Top, selected );
	}

	if ( layout.m_bottomGizmo != LayoutInfo::NONE )
	{
		DrawGizmo( layout, GL_Bottom, active && m_activeItemGizmo == GL_Bottom, selected );
	}
}

wxRect CEdTreeEditor::GetGizmoRect( const LayoutInfo& layout, GizmoLocation location ) const
{
	wxPoint connectorPos = ( location == GizmoLocation::GL_Bottom ) ? layout.m_connectorSrc : layout.m_connectorDest;
	LayoutInfo::Gizmo type = ( location == GizmoLocation::GL_Bottom ) ? layout.m_bottomGizmo : layout.m_topGizmo;

	switch ( type )
	{
	case LayoutInfo::CONNECTOR:
		return wxRect( 
			connectorPos.x + layout.m_windowPos.x - 5, 
			connectorPos.y + layout.m_windowPos.y - 5, 
			10, 10 );
		break;
	case LayoutInfo::ELLIPSIS:
		return wxRect( 
			connectorPos.x + layout.m_windowPos.x - 12, 
			connectorPos.y + layout.m_windowPos.y - 5, 
			20, 10 );
		break;
	default:
		ASSERT( false ); // Shouldn't be here
		return wxRect( 0, 0, 0, 0 );
		break;
	}
}

void CEdTreeEditor::DrawGizmo( const LayoutInfo& layout, GizmoLocation location, Bool active, Bool selected )
{
	ASSERT ( location != GL_None );

	wxRect rect = GetGizmoRect( layout, location );

	wxColour borderClr = selected ? wxColour( 255, 255, 0 ) : wxColour( 0, 0, 0 );

	LayoutInfo::Gizmo type = ( location == GizmoLocation::GL_Bottom ) ? layout.m_bottomGizmo : layout.m_topGizmo;
	switch ( type )
	{
	case LayoutInfo::CONNECTOR:
		{
			wxColour backgroundClr( 255, 128, 64 );
			FillCircle( rect.x, rect.y, rect.width, backgroundClr );
			if ( active )
			{
				FillCircle( rect.x, rect.y, rect.width, wxColour( 255, 255, 255, 128 ) );
			}
			DrawCircle( rect.x, rect.y, rect.width, borderClr, selected ? 2 : 1 );
		}
		break;

	case LayoutInfo::ELLIPSIS:
		{
			wxColour backgroundClr( 128, 128, 128 );
			FillRoundedRect( rect.x, rect.y, rect.width, rect.height, backgroundClr, 3 );
			if ( active )
			{
				FillRoundedRect( rect.x, rect.y, rect.width, rect.height, wxColour( 255, 255, 255, 128 ), 3 );
			}
			DrawRoundedRect( rect.x, rect.y, rect.width, rect.height, borderClr, 3, selected ? 2 : 1 );
			FillCircle( rect.x + rect.width/2 - 2,     rect.y + rect.height/2 - 2, 4, *wxBLACK );
			FillCircle( rect.x + rect.width/2 - 2 - 6, rect.y + rect.height/2 - 2, 4, *wxBLACK );
			FillCircle( rect.x + rect.width/2 - 2 + 6, rect.y + rect.height/2 - 2, 4, *wxBLACK );
		}
		break;
	}
}

void CEdTreeEditor::DrawBlockLinks( LayoutInfo & layout )
{
	if ( layout.m_parent == NULL )
		return;

	wxPoint points[4];
	wxPoint from = layout.m_parent->m_connectorSrc + layout.m_parent->m_windowPos;
	wxPoint to   = layout.m_connectorDest + layout.m_windowPos;
	CalculateBezierPoints( from, to, points );
	DrawCurve( points, wxColour( 180,180,180 ), 2.0f );
}

void CEdTreeEditor::DrawDraggedLink()
{
	wxColour borderClr( 180, 180, 180, 128 );
	wxColour dummyClr( 128, 128, 128, 128 );

	wxPoint points[4];

	if ( m_activeItemGizmo == GL_Top )
	{
		if ( m_activeItem->m_parent )
		{
			wxPoint from = m_activeItem->m_parent->m_connectorSrc + m_activeItem->m_parent->m_windowPos;
			wxPoint to   = m_lastMousePos - wxPoint( 0, BLOCK_HEIGHT );
			CalculateBezierPoints( from, to, points );
			DrawCurve( points, borderClr, 2.0f );
		}

		wxPoint from = m_lastMousePos;
		wxPoint to   = m_activeItem->m_connectorDest + m_activeItem->m_windowPos;
		CalculateBezierPoints( from, to, points );
		DrawCurve( points, borderClr, 2.0f );

		wxRect dummyRect( m_lastMousePos.x - 40, m_lastMousePos.y - BLOCK_HEIGHT, 80, BLOCK_HEIGHT );
		FillRoundedRect( dummyRect.x, dummyRect.y, dummyRect.width, dummyRect.height, dummyClr, 8 );
		DrawRoundedRect( dummyRect.x, dummyRect.y, dummyRect.width, dummyRect.height, borderClr, 8 );
	}
	else
	{
		wxPoint from = m_activeItem->m_connectorSrc + m_activeItem->m_windowPos;
		wxPoint to   = m_lastMousePos;
		CalculateBezierPoints( from, to, points );
		DrawCurve( points, borderClr, 2.0f );

		wxRect dummyRect( m_lastMousePos.x - 40, m_lastMousePos.y, 80, BLOCK_HEIGHT );
		FillRoundedRect( dummyRect.x, dummyRect.y, dummyRect.width, dummyRect.height, dummyClr, 8 );
		DrawRoundedRect( dummyRect.x, dummyRect.y, dummyRect.width, dummyRect.height, borderClr, 8 );
	}
}

void CEdTreeEditor::CalculateBezierPoints( const wxPoint& from, const wxPoint& to, wxPoint (&outPoints)[4] )
{
	wxPoint src, srcDir, dest, destDir;

	// Source
	src = from;
	//src.y += 5;
	srcDir = wxPoint( 0,1 );

	// Destination
	dest = to;
	destDir = wxPoint( 0,-1 );

	// Calculate distance between end points
	Int32 dist = Max( Abs( src.x-dest.x ), Abs( src.y-dest.y ) );
	Int32 offset = 5.0f + Min( dist / 3.0f, 200.0f );

	// Setup link
	outPoints[0].x = src.x;
	outPoints[0].y = src.y;
	outPoints[1].x = src.x + srcDir.x * offset;
	outPoints[1].y = src.y + srcDir.y * offset;
	outPoints[2].x = dest.x + destDir.x * offset;
	outPoints[2].y = dest.y + destDir.y * offset;
	outPoints[3].x = dest.x;
	outPoints[3].y = dest.y;
}
