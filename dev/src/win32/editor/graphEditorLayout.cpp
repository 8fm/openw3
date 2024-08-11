/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/behaviorGraphVariableNode.h"
#include "../../common/engine/descriptionGraphBlock.h"
#include "graphEditor.h"
#include "../../common/engine/graphConnection.h"
#include "../../common/engine/graphContainer.h"

#define LAYOUT_CAPTION_MARGIN_X		15
#define LAYOUT_CAPTION_MARGIN_Y		3

#define LAYOUT_SOCKET_GROUP_V_MARGIN_X		4
#define LAYOUT_SOCKET_GROUP_V_MARGIN_Y		5
#define LAYOUT_SOCKET_GROUP_V_SEPARATION	18

#define LAYOUT_SOCKET_GROUP_H_MARGIN_X		4
#define LAYOUT_SOCKET_GROUP_H_MARGIN_Y		5
#define LAYOUT_SOCKET_GROUP_H_SEPARATION	6

#define LAYOUT_CONNECTOR_WIDTH		10
#define LAYOUT_CONNECTOR_HEIGHT		8	

#define LAYOUT_INNER_AREA_MARGIN_Y	5
#define LAYOUT_INNER_AREA_MARGIN_X	4

#define LAYOUT_CIRCLE_STEPS 32

wxPoint CEdGraphEditor::CalcTitleBarSize( const String &caption )
{
	// Get text width
	wxPoint size = TextExtents( GetGdiBoldFont(), caption.AsChar() );

	// Add some margin
	Int32 width = Max( size.x, 20 );
	width += LAYOUT_CAPTION_MARGIN_X * 2; // Left and Right margins

	// Calculate bar height
	Int32 height = size.y;
	height += LAYOUT_CAPTION_MARGIN_Y * 2; // Top and Bottom margins

	// Return size
	return wxPoint( width, height );
}

wxPoint CEdGraphEditor::CalcSocketGroupSizeV( const TDynArray< CGraphSocket* > &sockets )
{
	// Calculate maximum text width of socket captions
	Int32 maxWidth = 0;
	for ( Uint32 i=0; i<sockets.Size(); i++ )
	{
		// Process only visible sockets
		CGraphSocket *socket = sockets[i];

		// Get socket caption
		String caption;
		socket->GetCaption( caption );

		// Calculate socket name width
		wxPoint size = TextExtents( GetGdiDrawFont(), caption );
		maxWidth = Max( size.x, maxWidth );
	}

	// Assemble size
	if ( sockets.Size() )
	{
		// Add margins
		Int32 height = LAYOUT_SOCKET_GROUP_V_SEPARATION * sockets.Size();
		height += LAYOUT_SOCKET_GROUP_V_MARGIN_Y * 2;
		return wxPoint( LAYOUT_SOCKET_GROUP_V_MARGIN_X + maxWidth, height );
	}
	else
	{
		// No sockets, collapse whole region
		return wxPoint(0,0);
	}
}

wxPoint CEdGraphEditor::CalcSocketGroupSizeH( const TDynArray< CGraphSocket* > &sockets, bool& hasCaptions )
{
	// No bottom side captions for now
	hasCaptions = false;

	// Calculate maximum text width of socket captions
	Int32 sumWidth = 0;
	Int32 maxWidth = 0;
	for ( Uint32 i=0; i<sockets.Size(); i++ )
	{
		// Get socket name
		CGraphSocket *socket = sockets[i];

		// Get socket caption
		String caption;
		socket->GetCaption( caption );

		// Do we have valid caption
		if ( caption.GetLength() && socket->CanDrawCaption() )
		{
			hasCaptions = true;
		}

		// Calculate socket name width
		wxPoint size = TextExtents( GetGdiDrawFont(), caption );
		sumWidth += size.x;
		maxWidth = Max( size.x, maxWidth );
	}

	// Assemble size
	if ( sockets.Size() )
	{
		// Add margins
		Int32 totalWidth = sumWidth;
		totalWidth += ( sockets.Size() - 1 ) * LAYOUT_SOCKET_GROUP_H_SEPARATION;
		return wxPoint( totalWidth, LAYOUT_SOCKET_GROUP_V_SEPARATION );
	}
	else
	{
		// No sockets, collapse whole region
		return wxPoint(0,0);
	}
}

void CEdGraphEditor::CalcSocketLayoutVLeft( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, BlockLayoutInfo& layout, bool center )
{
	// Calculate total size
	Int32 totalHeight = sockets.Size() * LAYOUT_SOCKET_GROUP_V_SEPARATION;
	Int32 startY = center ? ( clientRect.height - totalHeight ) / 2 : LAYOUT_SOCKET_GROUP_V_MARGIN_Y;

	// Place sockets
	for ( Uint32 i=0; i<sockets.Size(); i++ )
	{
		CGraphSocket *socket = sockets[i];

		// Calculate region height
		Int32 regionPos = startY + LAYOUT_SOCKET_GROUP_V_SEPARATION * i;
		Int32 regionHeight = LAYOUT_SOCKET_GROUP_V_SEPARATION;
		Int32 regionCenter = regionPos + regionHeight / 2;

		// Get socket caption
		String caption;
		socket->GetCaption( caption );

		// Get text extents
		wxPoint textSize = TextExtents( GetGdiDrawFont(), caption );

		// Create socket layout info
		SocketLayoutInfo socketLayout;

		// Calculate caption position
		socketLayout.m_captionPos.x = clientRect.x + LAYOUT_SOCKET_GROUP_V_MARGIN_X;
		socketLayout.m_captionPos.y = clientRect.y + regionCenter - textSize.y / 2;

		// Calculate connector position
		socketLayout.m_socketRect.x = clientRect.x + ( -LAYOUT_CONNECTOR_WIDTH / 2 ) - 2;
		socketLayout.m_socketRect.y = clientRect.y + ( regionCenter - ( LAYOUT_CONNECTOR_HEIGHT / 2) );
		socketLayout.m_socketRect.width = LAYOUT_CONNECTOR_WIDTH;
		socketLayout.m_socketRect.height = LAYOUT_CONNECTOR_HEIGHT;

		// Link position
		socketLayout.m_linkDir = wxPoint( -1, 0 );
		socketLayout.m_linkPos.x = clientRect.x - LAYOUT_CONNECTOR_WIDTH/2 - 2;
		socketLayout.m_linkPos.y = clientRect.y + regionCenter;

		// Remember layout info
		layout.m_sockets.Insert( socket, socketLayout );
	}
}

void CEdGraphEditor::CalcSocketLayoutVRight( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, BlockLayoutInfo& layout, bool center )
{
	// Calculate total size
	Int32 totalHeight = sockets.Size() * LAYOUT_SOCKET_GROUP_V_SEPARATION;
	Int32 startY = center ? ( clientRect.height - totalHeight ) / 2 : LAYOUT_SOCKET_GROUP_V_MARGIN_Y;

	// Place sockets
	for ( Uint32 i=0; i<sockets.Size(); i++ )
	{
		CGraphSocket *socket = sockets[i];

		// Calculate region height
		Int32 regionPos = startY + LAYOUT_SOCKET_GROUP_V_SEPARATION * i;
		Int32 regionHeight = LAYOUT_SOCKET_GROUP_V_SEPARATION;
		Int32 regionCenter = regionPos + regionHeight / 2;

		// Get socket caption
		String caption;
		socket->GetCaption( caption );

		// Get text extents
		wxPoint textSize = TextExtents( GetGdiDrawFont(), caption );

		// Create socket layout info
		SocketLayoutInfo socketLayout;

		// Calculate caption position
		socketLayout.m_captionPos.x = clientRect.x + clientRect.width - LAYOUT_SOCKET_GROUP_V_MARGIN_X - textSize.x;
		socketLayout.m_captionPos.y = clientRect.y + regionCenter - textSize.y / 2;

		// Calculate connector position
		socketLayout.m_socketRect.x = clientRect.x + clientRect.width - ( LAYOUT_CONNECTOR_WIDTH / 2) + 3;
		socketLayout.m_socketRect.y = clientRect.y + regionCenter - ( LAYOUT_CONNECTOR_HEIGHT / 2);
		socketLayout.m_socketRect.width = LAYOUT_CONNECTOR_WIDTH;
		socketLayout.m_socketRect.height = LAYOUT_CONNECTOR_HEIGHT;

		// Link position
		socketLayout.m_linkDir = wxPoint( 1, 0 );
		socketLayout.m_linkPos.x = clientRect.x + clientRect.width + ( LAYOUT_CONNECTOR_WIDTH / 2) + 3;
		socketLayout.m_linkPos.y = clientRect.y + regionCenter;

		// Remember layout info
		layout.m_sockets.Insert( socket, socketLayout );
	}
}

void CEdGraphEditor::CalcSocketLayoutHBottom( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, BlockLayoutInfo& layout )
{
	// Calculate total size and start offset
	bool hasCaptions = false;
	wxPoint neededSize = CalcSocketGroupSizeH( sockets, hasCaptions );
	Int32 startX = ( clientRect.width - neededSize.x ) / 2;

	// Place sockets
	for ( Uint32 i=0; i<sockets.Size(); i++ )
	{
		CGraphSocket *socket = sockets[i];

		// Get socket caption
		String caption;
		socket->GetCaption( caption );

		// Get text extents
		wxPoint textSize = TextExtents( GetGdiDrawFont(), caption );

		// Calculate region height
		Int32 regionPos = startX;
		Int32 regionWidth = textSize.x;
		Int32 regionCenter = regionPos + regionWidth / 2;

		// Create socket layout info
		SocketLayoutInfo socketLayout;

		// Calculate caption position
		socketLayout.m_captionPos.x = clientRect.x + regionCenter - textSize.x / 2;
		socketLayout.m_captionPos.y = clientRect.y + clientRect.height - LAYOUT_SOCKET_GROUP_H_MARGIN_Y - textSize.y;

		// Calculate connector position
		socketLayout.m_socketRect.x = clientRect.x + regionCenter - ( LAYOUT_CONNECTOR_HEIGHT / 2);
		socketLayout.m_socketRect.y = clientRect.y + clientRect.height - ( LAYOUT_CONNECTOR_WIDTH / 2) + 7;
		socketLayout.m_socketRect.width = LAYOUT_CONNECTOR_HEIGHT;
		socketLayout.m_socketRect.height = LAYOUT_CONNECTOR_WIDTH;

		// Link position
		socketLayout.m_linkDir = wxPoint( 0, 1 );
		socketLayout.m_linkPos.x = clientRect.x + regionCenter;
		socketLayout.m_linkPos.y = socketLayout.m_socketRect.y + LAYOUT_CONNECTOR_WIDTH;

		// Remember layout info
		layout.m_sockets.Insert( socket, socketLayout );

		// Move to next
		startX += textSize.x + LAYOUT_SOCKET_GROUP_H_SEPARATION;
	}
}

void CEdGraphEditor::CalcSocketLayoutCenter( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, BlockLayoutInfo& layout )
{
	// Place sockets
	for ( Uint32 i=0; i<sockets.Size(); i++ )
	{
		CGraphSocket *socket = sockets[i];

		// Create socket layout info
		SocketLayoutInfo socketLayout;

		// Calculate caption position
		socketLayout.m_captionPos.x = clientRect.x + clientRect.width / 2;
		socketLayout.m_captionPos.y = clientRect.y + clientRect.height / 2;

		// Calculate connector position
		socketLayout.m_socketRect.x = clientRect.x;
		socketLayout.m_socketRect.y = clientRect.y;
		socketLayout.m_socketRect.width = clientRect.width;
		socketLayout.m_socketRect.height = clientRect.height;

		// Link position
		socketLayout.m_linkDir = wxPoint( 0, 0 );
		socketLayout.m_linkPos.x = clientRect.x + clientRect.width / 2;
		socketLayout.m_linkPos.y = clientRect.y + clientRect.height / 2;

		// Remember layout info
		layout.m_sockets.Insert( socket, socketLayout );
	}
}

void CEdGraphEditor::CalcSocketLayoutTitle( const TDynArray< CGraphSocket* > &sockets, const wxRect &clientRect, const wxRect& titleRect, BlockLayoutInfo& layout )
{
	// Place sockets
	for ( Uint32 i=0; i<sockets.Size(); i++ )
	{
		CGraphSocket *socket = sockets[i];

		// Create socket layout info
		SocketLayoutInfo socketLayout;

		// Calculate caption position
		socketLayout.m_captionPos.x = titleRect.x + titleRect.width / 2;
		socketLayout.m_captionPos.y = titleRect.y + titleRect.height / 2;

		// Calculate connector position
		socketLayout.m_socketRect.x = titleRect.x;
		socketLayout.m_socketRect.y = titleRect.y;
		socketLayout.m_socketRect.width = titleRect.width;
		socketLayout.m_socketRect.height = titleRect.height;

		// Link position
		socketLayout.m_linkDir = wxPoint( 0, -1 );
		socketLayout.m_linkPos.x = titleRect.x + titleRect.width / 2;
		socketLayout.m_linkPos.y = titleRect.y;

		// Remember layout info
		layout.m_sockets.Insert( socket, socketLayout );
	}
}

void CEdGraphEditor::CalcTitleIconArea( CGraphBlock* block, wxSize &size )
{
	// Implemented in sub classes
}

void CEdGraphEditor::DrawTitleIconArea( CGraphBlock* block, const wxRect& rect )
{
	// Implemented in sub classes
}

void CEdGraphEditor::CalcBlockInnerArea( CGraphBlock* block, wxSize& size )
{
	// Implemented in sub classes
}

void CEdGraphEditor::DrawBlockInnerArea( CGraphBlock* block, const wxRect& rect )
{
	// Implemented in sub classes
}

void CEdGraphEditor::UpdateBlockLayout( CGraphBlock* block )
{
	// Find or create layout info
	BlockLayoutInfo* layout = m_layout.FindPtr( block );

	// Not defined, create
	if ( !layout )
	{
		m_layout.Insert( block, BlockLayoutInfo() );
		layout = m_layout.FindPtr( block );
	}

	// Reset visibility and freeze flag
	layout->m_visible = true;
	layout->m_freeze = false;
	layout->m_onScreen = true;

	// Reset socket info
	layout->m_sockets.Clear();

	// Get left side sockets
	TDynArray< CGraphSocket* > leftSockets;
	for ( SocketIterator< CGraphSocket > i( block ); i; ++i )
	{
		if ( i->GetPlacement() == LSP_Left && i->ShouldDraw() )
		{
			leftSockets.PushBack( *i );
		}
	}

	// Get right side sockets
	TDynArray< CGraphSocket* > rightSockets;
	for ( SocketIterator< CGraphSocket > i( block ); i; ++i )
	{
		if ( i->GetPlacement() == LSP_Right && i->ShouldDraw() )
		{
			rightSockets.PushBack( *i );
		}
	}

	// Get bottom side sockets
	TDynArray< CGraphSocket* > bottomSockets;
	for ( SocketIterator< CGraphSocket > i( block ); i; ++i )
	{
		if ( i->GetPlacement() == LSP_Bottom && i->ShouldDraw() )
		{
			bottomSockets.PushBack( *i );
		}
	}

	// Get center sockets
	TDynArray< CGraphSocket* > centerSockets;
	for ( SocketIterator< CGraphSocket > i( block ); i; ++i )
	{
		if ( i->GetPlacement() == LSP_Center && i->ShouldDraw() )
		{
			centerSockets.PushBack( *i );
		}
	}

	// Get title bar sockets
	TDynArray< CGraphSocket* > titleSockets;
	for ( SocketIterator< CGraphSocket > i( block ); i; ++i )
	{
		if ( i->GetPlacement() == LSP_Title && i->ShouldDraw() )
		{
			titleSockets.PushBack( *i );
		}
	}

	// Calculate block layout
	EGraphBlockShape shape = block->GetBlockShape();
	if ( shape == GBS_LargeCircle || shape == GBS_DoubleCircle )
	{
		// Calculate client area size
		Int32 clientAreaHeight = 48;
		Int32 clientAreaWidth = 48;

		// Calculate final window size
		layout->m_windowSize.x = clientAreaWidth;
		layout->m_windowSize.y = clientAreaHeight;

		// Calculate title bar area
		layout->m_titleRect.x = 0;
		layout->m_titleRect.y = 0;
		layout->m_titleRect.width = clientAreaWidth;
		layout->m_titleRect.height = 0;

		// Calculate title icon area
		layout->m_iconRect.x = 0;
		layout->m_iconRect.y = 0;
		layout->m_iconRect.width = 0;
		layout->m_iconRect.height = 0;

		// Calculate client area
		layout->m_clientRect.x = 0;
		layout->m_clientRect.y = 0;
		layout->m_clientRect.width = clientAreaWidth;
		layout->m_clientRect.height = clientAreaHeight;

		// No inner rect
		layout->m_innerRect = wxRect( 0, 0, clientAreaWidth,clientAreaHeight );

		// Place sockets in the center
		CalcSocketLayoutCenter( centerSockets, layout->m_clientRect, *layout );

		// left
		CalcSocketLayoutVLeft( leftSockets, layout->m_clientRect, *layout, true );

		// right
		CalcSocketLayoutVRight( rightSockets, layout->m_clientRect, *layout, true );

		// and bottom :)
		CalcSocketLayoutHBottom( bottomSockets, layout->m_clientRect, *layout );
	}
	else
	{
		// Title bar
		String caption = block->GetCaption();
		if ( block->IsObsolete() ) caption += TXT(" (Obsolete)");
		wxPoint titleBarSize = CalcTitleBarSize( caption );

		// Add icon width and height
		wxSize titleIconSize( 0, 0 ); 
		CalcTitleIconArea( block, titleIconSize );

		titleBarSize.x += titleIconSize.x;
		titleBarSize.y = Max( titleBarSize.y, titleIconSize.y );

		// Calculate client area size
		Int32 clientAreaHeight = 0;
		Int32 clientAreaWidth = 0;

		// Have some minimal client area
		clientAreaWidth = Max( clientAreaWidth, 20 );
		clientAreaHeight = Max( clientAreaHeight, 15 );

		// Calculate size of socket areas
		bool hasBottomCaptions = false;
		wxPoint inputSocketSize = CalcSocketGroupSizeV( leftSockets );
		wxPoint outputSocketSize = CalcSocketGroupSizeV( rightSockets );
		wxPoint bottomSocketSize = CalcSocketGroupSizeH( bottomSockets, hasBottomCaptions );

		// Calculate width of client area
		if ( inputSocketSize.x || outputSocketSize.x )
		{
			// Merge two blocks widths
			clientAreaWidth = inputSocketSize.x + outputSocketSize.x;
			clientAreaWidth = Max( clientAreaWidth, bottomSocketSize.x );

			// Get larger of heights
			clientAreaHeight = Max( inputSocketSize.y, outputSocketSize.y );
			clientAreaHeight += bottomSocketSize.y;

			// Extra margin between sides
			clientAreaWidth += 4;

			// Both input and output connections are visible, add extra margin
			if ( inputSocketSize.x && outputSocketSize.x )
			{
				clientAreaWidth += 4;
			}
		}
		else
		{
			// No left or right sockets, if we have bottom captions add a margin
			if ( hasBottomCaptions )
			{
				clientAreaHeight += 10;
			}
		}

		// Inner area size
		wxSize innerAreaSize( 0,0 );
		CalcBlockInnerArea( block, innerAreaSize );

		// Add inner area to client area
		if ( innerAreaSize.x && innerAreaSize.y )
		{
			clientAreaWidth += innerAreaSize.x + LAYOUT_INNER_AREA_MARGIN_X*2;
			clientAreaHeight = Max( clientAreaHeight, innerAreaSize.y + LAYOUT_INNER_AREA_MARGIN_Y*2 );
		}

		// Variable sockets
		wxPoint varLinksSize( 0,0 );

		if ( shape == GBS_Triangle || shape == GBS_TriangleLeft || shape == GBS_Octagon || shape == GBS_Arrow || shape == GBS_ArrowLeft )
		{
			titleBarSize.x = 0;
			titleBarSize.y = 0;
		}

		// Calculate final window size
		wxSize prevSize = layout->m_windowSize;
		layout->m_windowSize.x = Max<Uint32>( titleBarSize.x, Max( clientAreaWidth, varLinksSize.x ) );
		layout->m_windowSize.y = titleBarSize.y + clientAreaHeight + varLinksSize.y + 2;
		Vector size = block->GetSize();
		if ( size.X!=0 && size.Y!=0 )
		{
			layout->m_windowSize.x = Max<Uint32>( layout->m_windowSize.x, size.X );
			layout->m_windowSize.y = Max<Uint32>( layout->m_windowSize.y, size.Y );
		}
		if ( block->IsResizable() )
		{
			layout->m_windowSize.x = Max<Uint32>( layout->m_windowSize.x, prevSize.x );
			layout->m_windowSize.y = Max<Uint32>( layout->m_windowSize.y, prevSize.y );
		}
		 if ( block->IsA< CBehaviorGraphVariableBaseNode >() )
		 {
			 layout->m_windowSize.y += BEHAVIOR_GRAPH_VARIABLE_NODE_BUTTON_SIZE;
		 }

		// Calculate final client area width
		clientAreaWidth = layout->m_windowSize.x;

		// Calculate title bar area
		layout->m_titleRect.x = 0;
		layout->m_titleRect.y = 0;
		layout->m_titleRect.width = clientAreaWidth;
		layout->m_titleRect.height = titleBarSize.y;

		// Calculate title icon area
		layout->m_iconRect.x = layout->m_titleRect.x + layout->m_titleRect.width - titleIconSize.x - (LAYOUT_CAPTION_MARGIN_X / 2);
		layout->m_iconRect.y = layout->m_titleRect.y + ( layout->m_titleRect.height - titleIconSize.y ) / 2;
		layout->m_iconRect.width = titleIconSize.x;
		layout->m_iconRect.height = titleIconSize.y;

		// Calculate client area
		layout->m_clientRect.x = 0;
		layout->m_clientRect.y = titleBarSize.y;
		layout->m_clientRect.width = clientAreaWidth;
		layout->m_clientRect.height = clientAreaHeight;

		if ( shape == GBS_TriangleLeft )
		{
			// ok, kinda hacky, but it's used in one editor only so far, so nothing unexcpected will happen
			layout->m_clientRect.width *= 1.5f;
			layout->m_clientRect.height *= 2.f;
		}

		// Place sockets
		if ( m_mirrored )
		{
			CalcSocketLayoutVLeft( rightSockets, layout->m_clientRect, *layout, false );
			CalcSocketLayoutVRight( leftSockets, layout->m_clientRect, *layout, false );
		}
		else
		{
			CalcSocketLayoutVLeft( leftSockets, layout->m_clientRect, *layout, false );
			CalcSocketLayoutVRight( rightSockets, layout->m_clientRect, *layout, false );
		}

		// Calculate bottom sockets layout
		CalcSocketLayoutHBottom( bottomSockets, layout->m_clientRect, *layout );

		// Add center sockets
		CalcSocketLayoutCenter( centerSockets, layout->m_clientRect, *layout );

		// Add title bar sockets
		CalcSocketLayoutTitle( titleSockets, layout->m_clientRect, layout->m_titleRect, *layout );

		// Setup inner area rect
		if ( innerAreaSize.x && innerAreaSize.y )
		{
			// Set x position
			if ( inputSocketSize.x || outputSocketSize.x )
			{
				// Place rect after input sockets
				if ( m_mirrored )
				{
					layout->m_innerRect.x = outputSocketSize.x + LAYOUT_INNER_AREA_MARGIN_X;
				}
				else
				{
					layout->m_innerRect.x = inputSocketSize.x + LAYOUT_INNER_AREA_MARGIN_X;
				}
			}
			else
			{
				// Place rect in the center of the dialog
				layout->m_innerRect.x = ( clientAreaWidth - innerAreaSize.x ) / 2;
			}

			// Set y position
			layout->m_innerRect.y = titleBarSize.y + LAYOUT_INNER_AREA_MARGIN_Y + 1;

			// Set size
			layout->m_innerRect.width = innerAreaSize.x;
			layout->m_innerRect.height = innerAreaSize.y;
		}
		else
		{
			// No inner rect
			layout->m_innerRect = wxRect(0,0,0,0);
		}
	}
}

void CEdGraphEditor::DrawConnector( const wxRect& rect, const wxColour& borderColor, const wxColour &baseColor, ELinkedSocketDrawStyle style, ELinkedSocketDirection direction, Float width )
{
	// Draw connector
	if ( style == LSDS_Arrow )
	{
		// Draw arrow
		DrawDownArrow( rect, baseColor, borderColor );
	}
	else if ( style == LSDS_InArrow )
	{
		// Draw arrow
		DrawUpArrow( rect, baseColor, borderColor );
	}
	else
	{
		// Draw default box connector
		FillRect( rect.x+0, rect.y+0, rect.width, rect.height, baseColor );
		DrawRect( rect.x, rect.y, rect.width, rect.height, borderColor, width );
	}
}

void CEdGraphEditor::DrawTitleBar( const wxRect& rect, const wxRect& iconRect, const wxColour& borderColor, const wxColour& clientColor, const wxColour& titleColor, const String &caption, EGraphBlockShape shape, float width /* = 1.0f */, Bool shouldCaptionBeInConstantSize /* = false */ )
{
	static wxColour textColor( 255, 255, 255 );
	static wxColour shadowColor( 0, 0, 0 );

	// Set clipping to draw only in the title area
	wxRect clipRect = rect;
	clipRect.width += 1;
	SetClip( clipRect );

	// Calculate caption position
	wxPoint size = TextExtents( GetGdiBoldFont(), caption );

	wxPoint position;
	position.x = rect.x + ( rect.width - iconRect.width - size.x ) / 2 - 1;
	position.y = rect.y + LAYOUT_CAPTION_MARGIN_Y + 1;

	// Draw title bar
	if ( shape == GBS_Slanted )
	{
		FillCuttedRect( rect.x, rect.y, rect.width, rect.height + 9, titleColor, 8 );
		DrawCuttedRect( rect.x, rect.y, rect.width, rect.height + 9, borderColor, 8, width  );
	}
	else if ( shape == GBS_Rounded )
	{
		FillRoundedRect( rect.x, rect.y, rect.width, rect.height + 9, titleColor, 8 );
		DrawRoundedRect( rect.x, rect.y, rect.width, rect.height + 9, borderColor, 8, width  );
	}
	else if ( shape == GBS_Triangle || shape == GBS_TriangleLeft )
	{
		position.y = rect.y - LAYOUT_CAPTION_MARGIN_Y - size.y;
		ResetClip();
	}
	else if ( shape == GBS_Octagon )
	{
		const Int32 triangleHeight = 16;
		position.y = rect.y - LAYOUT_CAPTION_MARGIN_Y - size.y - triangleHeight;
		ResetClip();
	}
	else if ( shape == GBS_Arrow )
	{
		const Int32 arrowHeight = 16;
		position.y = rect.y - LAYOUT_CAPTION_MARGIN_Y - size.y - arrowHeight;
		ResetClip();
	}
	else if ( shape == GBS_ArrowLeft )
	{
		const Int32 arrowHeight = 16;
		position.y = rect.y - LAYOUT_CAPTION_MARGIN_Y - size.y - arrowHeight;
		ResetClip();
	}
	else if ( shape == GBS_DoubleCircle || shape == GBS_LargeCircle )
	{
		const Int32 triangleHeight = 5;
		position.y = rect.y - LAYOUT_CAPTION_MARGIN_Y - size.y - triangleHeight;
		ResetClip();
	}
	else
	{
		FillRect( rect.x, rect.y, rect.width, rect.height + 9, titleColor );
		DrawRect( rect.x, rect.y, rect.width, rect.height + 9, borderColor, width );
	}

	if ( caption!=String::EMPTY )
	{
		// Draw text
		if ( shouldCaptionBeInConstantSize )
		{
			Float scaleCopy = GetScale();
			position.x = rect.x + ( rect.width - iconRect.width - size.x / scaleCopy ) / 2 - 1;
			position.y = rect.y + ( rect.height - size.y / scaleCopy ) / 2;
			if ( scaleCopy > 1.0f )
			{
				DrawText( position + wxPoint( 1, 1 ), GetGdiBoldFont(), caption, shadowColor );
				DrawText( position, GetGdiBoldFont(), caption, textColor );
			}
			else
			{
				wxPoint pos = position;
				pos.x *= scaleCopy;
				pos.y *= scaleCopy;
				pos.x += GetOffset().x * ( scaleCopy - 1.0f ); 
				pos.y += GetOffset().y * ( scaleCopy - 1.0f ); 
				SetScale( 1.0f, false );
				const wxRect &clipRect = GetClip();
				ResetClip();
				DrawText( pos + wxPoint( 1, 1 ), GetGdiBoldFont(), caption, shadowColor );
				DrawText( pos, GetGdiBoldFont(), caption, textColor );
				SetClip( clipRect );
				SetScale( scaleCopy, false );
			}
		}
		else
		{
			DrawText( position + wxPoint( 1, 1 ), GetGdiBoldFont(), caption, shadowColor );
			DrawText( position, GetGdiBoldFont(), caption, textColor );
		}
	}

	// Disable clipping
	ResetClip();
}

void CEdGraphEditor::DrawLink( const wxColour &color, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float width /* = 1.0f */, Bool srcCapArrow /*= false*/, Bool destCapArrow /*= false*/ )
{
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

	// Draw link curve with arrow !
	//const Int32 alpha = 255;//30 + 100 * GetScale();
	//wxColour tempColor( color.Red(), color.Green(), color.Blue(), alpha );

	DrawCurve( points, color, width );

	// Draw arrow caps if needed
	if ( srcCapArrow )
	{
		Float dirX = srcDir.x;
		Float dirY = srcDir.y;
		if ( dirY == 0 && dirY == 0 )
		{
			Vector direction(src.x - dest.x, src.y - dest.y, 0.f, 0.f);
			direction.Normalize2();
			dirX = direction.X;
			dirY = direction.Y;
		}

		const Float arrowSize = 2 * (4.f + width);
		const Int32 x1 = dest.x;
		const Int32 y1 = dest.y;
		const Int32 x2 = dest.x - arrowSize * dirX + (0.5f * arrowSize) * dirY;
		const Int32 y2 = dest.y - arrowSize * dirY - (0.5f * arrowSize) * dirX;
		const Int32 x3 = dest.x - arrowSize * dirX - (0.5f * arrowSize) * dirY;
		const Int32 y3 = dest.y - arrowSize * dirY + (0.5f * arrowSize) * dirX;
		FillTriangle( x1, y1, x2, y2, x3, y3, color );
	}

	if ( destCapArrow )
	{
		Float dirX = srcDir.x;
		Float dirY = srcDir.y;
		if ( dirX == 0 && dirY == 0 )
		{
			Vector direction(dest.x - src.x, dest.y - src.y, 0.f, 0.f);
			direction.Normalize2();
			dirX = direction.X;
			dirY = direction.Y;
		}

		const Float arrowSize = 2 * (4.f + width);
		const Int32 x1 = dest.x;
		const Int32 y1 = dest.y;
		const Int32 x2 = dest.x - arrowSize * dirX + (0.5f * arrowSize) * dirY;
		const Int32 y2 = dest.y - arrowSize * dirY - (0.5f * arrowSize) * dirX;
		const Int32 x3 = dest.x - arrowSize * dirX - (0.5f * arrowSize) * dirY;
		const Int32 y3 = dest.y - arrowSize * dirY + (0.5f * arrowSize) * dirX;
		FillTriangle( x1, y1, x2, y2, x3, y3, color );
	}
}

wxPoint CEdGraphEditor::CalculateLinkMidpoint( const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float size )
{
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

	wxPoint crossPoint;
	crossPoint.x = 0.125f*points[0].x + 0.375f*points[1].x + 0.375f*points[2].x + 0.125f*points[3].x - size/2.0f;
	crossPoint.y = 0.125f*points[0].y + 0.375f*points[1].y + 0.375f*points[2].y + 0.125f*points[3].y - size/2.0f;

	return crossPoint;
}

void CEdGraphEditor::DrawCrossOnLink( const wxColour &color, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float size, float width /* = 1.0f */ )
{
	wxPoint crossPoint = CalculateLinkMidpoint( src, srcDir, dest, destDir, size );

	wxSize crossSize(size,size);
	wxRect rect(crossPoint, crossSize);

	DrawCross( rect, color, width);
}

void CEdGraphEditor::DrawCrossOnLink( const wxColour &color, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float size, float width, float place )
{
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

	wxPoint crossPoint;

	crossPoint.x = (1.0f-place)*(1.0f-place)*(1.0f-place)*points[0].x + 3.0f*(1.0f-place)*(1.0f-place)*place*points[1].x + 3.0f*(1.0f-place)*place*place*points[2].x + place*place*place*points[3].x - size/2.0f;
	crossPoint.y = (1.0f-place)*(1.0f-place)*(1.0f-place)*points[0].y + 3.0f*(1.0f-place)*(1.0f-place)*place*points[1].y + 3.0f*(1.0f-place)*place*place*points[2].y + place*place*place*points[3].y - size/2.0f;

	wxSize crossSize(size,size);
	wxRect rect(crossPoint, crossSize);

	DrawCross( rect, color, width);
}
void CEdGraphEditor::DrawBlockShadows( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data )
{
	// Draw shadow
	if ( GetScale() > 0.85f && !block->IsInnerAreaTransparent() )
	{
		if ( data.shape == GBS_LargeCircle || data.shape == GBS_DoubleCircle )
		{
			FillCircle( data.location.x+3, data.location.y+3, layout->m_windowSize.x, layout->m_windowSize.y, data.shadow );
		}
		else if ( data.shape == GBS_Slanted )
		{
			FillCuttedRect( data.location.x+3, data.location.y+3, layout->m_windowSize.x, layout->m_windowSize.y, data.shadow, 8 );
		}
		else if ( data.shape == GBS_Rounded )
		{
			FillRoundedRect( data.location.x+3, data.location.y+3, layout->m_windowSize.x, layout->m_windowSize.y, data.shadow, 8 );
		}
		else if ( data.shape == GBS_Triangle )
		{
			wxPoint bottom( data.location.x, data.location.y + layout->m_clientRect.height );
			wxPoint right( data.location.x + layout->m_windowSize.x * 3 / 2, data.location.y + layout->m_clientRect.height / 2 );

			FillTriangle( data.location.x + 3, data.location.y + 3, right.x + 3, right.y + 3, bottom.x + 3, bottom.y + 3, data.shadow );
		}
		else if ( data.shape == GBS_TriangleLeft )
		{
			wxPoint left( data.location.x, data.location.y + layout->m_clientRect.height / 4 );
			wxPoint top( data.location.x + layout->m_clientRect.width, data.location.y );
			wxPoint bottom( data.location.x + layout->m_clientRect.width, data.location.y + layout->m_clientRect.height / 2 );	

			FillTriangle( left.x + 3, left.y + 3, top.x + 3, top.y + 3, bottom.x + 3, bottom.y + 3, data.shadow );
		}
		else if ( data.shape == GBS_Octagon )
		{
			const Int32 triangleHeight = 16;
			wxPoint points[6] = 
			{
				wxPoint( data.location.x + 3, data.location.y + 3 ),
				wxPoint( data.location.x + layout->m_windowSize.x / 2 + 3, data.location.y - triangleHeight + 3 ),
				wxPoint( data.location.x + layout->m_windowSize.x + 3, data.location.y + 3 ),
				wxPoint( data.location.x + layout->m_windowSize.x + 3, data.location.y + layout->m_windowSize.y + 3 ),
				wxPoint( data.location.x + layout->m_windowSize.x / 2 + 3, data.location.y + layout->m_windowSize.y + triangleHeight + 3 ),
				wxPoint( data.location.x + 3, data.location.y + layout->m_windowSize.y + 3 ),
			};

			FillPoly(&points[0], sizeof(points) / sizeof(wxPoint), data.shadow);
		}
		else if ( data.shape == GBS_Arrow )
		{
			const Int32 arrowheadHeight = 10;
			const Int32 arrowheadWidth = 30;
			const Int32 arrowLength = data.blockRect.width - arrowheadWidth;
			const wxPoint arrowPos(data.location.x + layout->m_windowSize.x - arrowLength - arrowheadWidth, data.location.y);
			wxPoint points[7] = 
			{
				wxPoint( arrowPos.x + 3, arrowPos.y + 3 ),
				wxPoint( arrowPos.x + arrowLength + 3, arrowPos.y + 3 ),
				wxPoint( arrowPos.x + arrowLength + 3, arrowPos.y - arrowheadHeight + 3 ),
				wxPoint( arrowPos.x + arrowLength + arrowheadWidth + 3, arrowPos.y + layout->m_windowSize.y / 2 + 3 ),
				wxPoint( arrowPos.x + arrowLength + 3, arrowPos.y + layout->m_windowSize.y + arrowheadHeight + 3 ),
				wxPoint( arrowPos.x + arrowLength + 3, arrowPos.y + layout->m_windowSize.y + 3 ),
				wxPoint( arrowPos.x + 3, arrowPos.y + layout->m_windowSize.y + 3 ),
			};

			FillPoly(&points[0], sizeof(points) / sizeof(wxPoint), data.shadow);
		}
		else if ( data.shape == GBS_ArrowLeft )
		{
			const Int32 arrowheadHeight = 10;
			const Int32 arrowheadWidth = 30;
			const Int32 arrowLength = data.blockRect.width - arrowheadWidth;
			const wxPoint arrowPos(data.location.x + layout->m_windowSize.x - arrowLength - arrowheadWidth, data.location.y);
			wxPoint points[7] = 
			{
				wxPoint( arrowPos.x - 3,									arrowPos.y - 3 ),
				wxPoint( arrowPos.x - (arrowLength + 3),					arrowPos.y - 3 ),
				wxPoint( arrowPos.x - (arrowLength + 3),					arrowPos.y + (arrowheadHeight + 3) ),
				wxPoint( arrowPos.x - (arrowLength + arrowheadWidth + 3),	arrowPos.y - (layout->m_windowSize.y / 2 + 3) ),
				wxPoint( arrowPos.x - (arrowLength + 3),					arrowPos.y - (layout->m_windowSize.y + arrowheadHeight + 3) ),
				wxPoint( arrowPos.x - (arrowLength + 3),					arrowPos.y - (layout->m_windowSize.y + 3) ),
				wxPoint( arrowPos.x - 3,									arrowPos.y - (layout->m_windowSize.y + 3) ),
			};

			FillPoly(&points[0], sizeof(points) / sizeof(wxPoint), data.shadow);
		}
		else
		{
			FillRect( data.location.x + 3, data.location.y + 3, layout->m_windowSize.x, layout->m_windowSize.y, data.shadow );
		}
	}
}

void CEdGraphEditor::DrawBlockSockets( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data )
{
	// Draw sockets
	const Bool drawConnectors = GetScale() > 0.2f;
	for ( THashMap< THandle< CGraphSocket >, SocketLayoutInfo>::iterator i = layout->m_sockets.Begin(); i != layout->m_sockets.End(); ++i )
	{
		if( i->m_first.Get() == NULL )
		{
			continue;
		}

		// Get socket info
		CGraphSocket* socket = i->m_first.Get();
		const SocketLayoutInfo& info = i->m_second;

		// Draw socket connector
		if ( drawConnectors )
		{
			if ( !socket->IsNoDraw() )
			{
				wxRect socketRect = info.m_socketRect;
				socketRect.Offset( data.location );
				Color socketColor = (socket == m_activeItem.Get()) ? Color::YELLOW : socket->GetColor();
				wxColour socketDrawColor( socketColor.R, socketColor.G, socketColor.B );

				if ( layout->m_freeze )
				{
					socketDrawColor = ConvertBlockColorToFreezeMode( socketDrawColor );
				}
				if( m_activeItem.Get() == socket)
				{
					DrawConnector( socketRect, wxColour( 255,255,255 ), socketDrawColor, socket->GetDrawStyle(), socket->GetDirection(), 2.0f );
				}
				else
				{
					DrawConnector( socketRect, data.border, socketDrawColor, socket->GetDrawStyle(), socket->GetDirection() );
				}
			}
		}
	}
}

void CEdGraphEditor::DrawBlockBackground( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data )
{
	// Draw inner area
	if ( data.shape == GBS_LargeCircle )
	{
		// Draw circle :)
		FillCircle( data.location.x, data.location.y, layout->m_windowSize.x, layout->m_windowSize.y, data.client );
	}
	else if ( data.shape == GBS_DoubleCircle )
	{
		// Draw circle :)
		FillCircle( data.location.x-4, data.location.y-4, layout->m_windowSize.x+8, layout->m_windowSize.y+8, data.outerBorder  );
		FillCircle( data.location.x, data.location.y, layout->m_windowSize.x, layout->m_windowSize.y, data.client );
	}
	else
	{
		// Draw title bar
		wxRect titleRect = layout->m_titleRect;
		titleRect.Offset( data.location );

		wxRect iconRect = layout->m_iconRect;
		iconRect.Offset( data.location );

		// Set window area clipping
		wxRect windowRect;
		windowRect.x = data.location.x;
		windowRect.y = titleRect.y + titleRect.height;
		windowRect.width = layout->m_windowSize.x + 1;
		windowRect.height = layout->m_windowSize.y - layout->m_titleRect.height + 1;
		SetClip( windowRect );

		// Draw window area
		if ( data.shape == GBS_Slanted )
		{
			FillCuttedRect( data.location.x, data.location.y, layout->m_windowSize.x, layout->m_windowSize.y, data.client, 8 );
		}
		else if ( data.shape == GBS_Rounded )
		{
			FillRoundedRect( data.location.x, data.location.y, layout->m_windowSize.x, layout->m_windowSize.y, data.client, 8 );
		}
		else if ( data.shape == GBS_Triangle )
		{
			ResetClip();

			wxPoint bottom( data.location.x, data.location.y + layout->m_clientRect.height );
			wxPoint right( data.location.x + layout->m_windowSize.x * 3 / 2, data.location.y + layout->m_clientRect.height / 2 );

			FillTriangle( data.location.x, data.location.y, right.x, right.y, bottom.x, bottom.y, data.client );
		}
		else if ( data.shape == GBS_TriangleLeft )
		{
			ResetClip();

			wxPoint left( data.location.x, data.location.y + layout->m_clientRect.height / 4 );
			wxPoint top( data.location.x + layout->m_clientRect.width, data.location.y );
			wxPoint bottom( data.location.x + layout->m_clientRect.width, data.location.y + layout->m_clientRect.height / 2 );	

			FillTriangle( left.x, left.y, top.x, top.y, bottom.x, bottom.y, data.client );
		}
		else if ( data.shape == GBS_Octagon )
		{
			ResetClip();

			const Int32 triangleHeight = 16;
			wxPoint points[6] = 
			{
				wxPoint( data.location.x, data.location.y ),
				wxPoint( data.location.x + layout->m_windowSize.x / 2, data.location.y - triangleHeight ),
				wxPoint( data.location.x + layout->m_windowSize.x, data.location.y ),
				wxPoint( data.location.x + layout->m_windowSize.x, data.location.y + layout->m_windowSize.y ),
				wxPoint( data.location.x + layout->m_windowSize.x / 2, data.location.y + layout->m_windowSize.y + triangleHeight ),
				wxPoint( data.location.x, data.location.y + layout->m_windowSize.y ),
			};

			FillPoly( &points[0], sizeof(points) / sizeof(wxPoint), data.client );
		}
		else if ( data.shape == GBS_Arrow )
		{
			ResetClip();

			const Int32 arrowheadHeight = 10;
			const Int32 arrowheadWidth = 30;
			const Int32 arrowLength = data.blockRect.width - arrowheadWidth;
			const wxPoint arrowPos(data.location.x + layout->m_windowSize.x - arrowLength - arrowheadWidth, data.location.y);
			wxPoint points[7] = 
			{
				wxPoint( arrowPos.x, arrowPos.y ),
				wxPoint( arrowPos.x + arrowLength, arrowPos.y ),
				wxPoint( arrowPos.x + arrowLength, arrowPos.y - arrowheadHeight ),
				wxPoint( arrowPos.x + arrowLength + arrowheadWidth, arrowPos.y + layout->m_windowSize.y / 2 ),
				wxPoint( arrowPos.x + arrowLength, arrowPos.y + layout->m_windowSize.y + arrowheadHeight ),
				wxPoint( arrowPos.x + arrowLength, arrowPos.y + layout->m_windowSize.y ),
				wxPoint( arrowPos.x, arrowPos.y + layout->m_windowSize.y ),
			};

			FillPoly( &points[0], sizeof(points) / sizeof(wxPoint), data.client );
		}
		else if ( data.shape == GBS_ArrowLeft )
		{
			ResetClip();

			const Int32 arrowheadHeight = 10;
			const Int32 arrowheadWidth = 30;
			const Int32 arrowLength = data.blockRect.width - arrowheadWidth;
			const wxPoint arrowPos(data.location.x, data.location.y + (layout->m_windowSize.y / 2));
			wxPoint points[7] = 
			{
				wxPoint( arrowPos.x,								arrowPos.y),
				wxPoint( arrowPos.x + arrowheadWidth,				arrowPos.y + ((layout->m_windowSize.y / 2) + arrowheadHeight) ),
				wxPoint( arrowPos.x + arrowheadWidth,				arrowPos.y + (layout->m_windowSize.y / 2) ),
				wxPoint( arrowPos.x + arrowheadWidth + arrowLength,	arrowPos.y + (layout->m_windowSize.y / 2) ),
				wxPoint( arrowPos.x + arrowheadWidth + arrowLength,	arrowPos.y - (layout->m_windowSize.y / 2) ),
				wxPoint( arrowPos.x + arrowheadWidth,				arrowPos.y - (layout->m_windowSize.y / 2) ),
				wxPoint( arrowPos.x + arrowheadWidth,				arrowPos.y - ((layout->m_windowSize.y / 2) + arrowheadHeight) ),
			};

			FillPoly( &points[0], sizeof(points) / sizeof(wxPoint), data.client );
		}
		else
		{
			FillRect( data.location.x, data.location.y, layout->m_windowSize.x, layout->m_windowSize.y, data.client );
		}

		// Reset clipping area
		ResetClip();
	}
}

void CEdGraphEditor::DrawBlockBorder( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data )
{
	if ( data.shape == GBS_LargeCircle )
	{
		DrawCircle( data.location.x, data.location.y, layout->m_windowSize.x, layout->m_windowSize.y, data.border, data.borderWidth  );
	}
	else if ( data.shape == GBS_DoubleCircle )
	{
		DrawCircle( data.location.x, data.location.y, layout->m_windowSize.x, layout->m_windowSize.y, data.border, data.borderWidth  );
	}
	else
	{
		// Draw title bar
		wxRect titleRect = layout->m_titleRect;
		titleRect.Offset( data.location );

		wxRect iconRect = layout->m_iconRect;
		iconRect.Offset( data.location );

		// Set window area clipping
		wxRect windowRect;
		windowRect.x = data.location.x;
		windowRect.y = titleRect.y + titleRect.height;
		windowRect.width = layout->m_windowSize.x + 1;
		windowRect.height = layout->m_windowSize.y - layout->m_titleRect.height + 1;
		SetClip( windowRect );

		// Draw window area
		if ( data.shape == GBS_Slanted )
		{
			DrawCuttedRect( data.location.x, data.location.y, layout->m_windowSize.x, layout->m_windowSize.y, data.border, 8, data.borderWidth );
		}
		else if ( data.shape == GBS_Rounded )
		{
			DrawRoundedRect( data.location.x, data.location.y, layout->m_windowSize.x, layout->m_windowSize.y, data.border, 8, data.borderWidth );
		}
		else if ( data.shape == GBS_Triangle )
		{
			ResetClip();

			wxPoint bottom( data.location.x, data.location.y + layout->m_clientRect.height );
			wxPoint right( data.location.x + layout->m_windowSize.x * 3 / 2, data.location.y + layout->m_clientRect.height / 2 );

			DrawTriangle( data.location.x, data.location.y, right.x, right.y, bottom.x, bottom.y, data.border );
		}
		else if ( data.shape == GBS_TriangleLeft )
		{
			ResetClip();

			wxPoint left( data.location.x, data.location.y + layout->m_clientRect.height / 4 );
			wxPoint top( data.location.x + layout->m_clientRect.width, data.location.y );
			wxPoint bottom( data.location.x + layout->m_clientRect.width, data.location.y + layout->m_clientRect.height / 2 );	

			DrawTriangle( left.x, left.y, top.x, top.y, bottom.x, bottom.y, data.border );
		}
		else if ( data.shape == GBS_Octagon )
		{
			ResetClip();

			const Int32 triangleHeight = 16;
			wxPoint points[6] = 
			{
				wxPoint( data.location.x, data.location.y ),
				wxPoint( data.location.x + layout->m_windowSize.x / 2, data.location.y - triangleHeight ),
				wxPoint( data.location.x + layout->m_windowSize.x, data.location.y ),
				wxPoint( data.location.x + layout->m_windowSize.x, data.location.y + layout->m_windowSize.y ),
				wxPoint( data.location.x + layout->m_windowSize.x / 2, data.location.y + layout->m_windowSize.y + triangleHeight ),
				wxPoint( data.location.x, data.location.y + layout->m_windowSize.y ),
			};

			DrawPoly( &points[0], sizeof(points) / sizeof(wxPoint), data.border, data.borderWidth );
		}
		else if ( data.shape == GBS_Arrow )
		{
			ResetClip();

			const Int32 arrowheadHeight = 10;
			const Int32 arrowheadWidth = 30;
			const Int32 arrowLength = data.blockRect.width - arrowheadWidth;
			const wxPoint arrowPos(data.location.x + layout->m_windowSize.x - arrowLength - arrowheadWidth, data.location.y);
			wxPoint points[7] = 
			{
				wxPoint( arrowPos.x, arrowPos.y ),
				wxPoint( arrowPos.x + arrowLength, arrowPos.y ),
				wxPoint( arrowPos.x + arrowLength, arrowPos.y - arrowheadHeight ),
				wxPoint( arrowPos.x + arrowLength + arrowheadWidth, arrowPos.y + layout->m_windowSize.y / 2 ),
				wxPoint( arrowPos.x + arrowLength, arrowPos.y + layout->m_windowSize.y + arrowheadHeight ),
				wxPoint( arrowPos.x + arrowLength, arrowPos.y + layout->m_windowSize.y ),
				wxPoint( arrowPos.x, arrowPos.y + layout->m_windowSize.y ),
			};

			DrawPoly( &points[0], sizeof(points) / sizeof(wxPoint), data.border, data.borderWidth );
		}
		else if ( data.shape == GBS_ArrowLeft )
		{
			ResetClip();

			const Int32 arrowheadHeight = 10;
			const Int32 arrowheadWidth = 30;
			const Int32 arrowLength = data.blockRect.width - arrowheadWidth;
			const wxPoint arrowPos(data.location.x, data.location.y + (layout->m_windowSize.y / 2));
			wxPoint points[7] = 
			{
				wxPoint( arrowPos.x,								arrowPos.y),
				wxPoint( arrowPos.x + arrowheadWidth,				arrowPos.y + ((layout->m_windowSize.y / 2) + arrowheadHeight) ),
				wxPoint( arrowPos.x + arrowheadWidth,				arrowPos.y + (layout->m_windowSize.y / 2) ),
				wxPoint( arrowPos.x + arrowheadWidth + arrowLength,	arrowPos.y + (layout->m_windowSize.y / 2) ),
				wxPoint( arrowPos.x + arrowheadWidth + arrowLength,	arrowPos.y - (layout->m_windowSize.y / 2) ),
				wxPoint( arrowPos.x + arrowheadWidth,				arrowPos.y - (layout->m_windowSize.y / 2) ),
				wxPoint( arrowPos.x + arrowheadWidth,				arrowPos.y - ((layout->m_windowSize.y / 2) + arrowheadHeight) ),
			};

			DrawPoly( &points[0], sizeof(points) / sizeof(wxPoint), data.border, data.borderWidth );
		}
		else
		{
			DrawRect( data.location.x, data.location.y, layout->m_windowSize.x, layout->m_windowSize.y, data.border, data.borderWidth );
		}

		// Reset clipping area
		ResetClip();

		// Cut the rounded rect with a line
		if ( data.shape != GBS_Triangle && data.shape != GBS_TriangleLeft && data.shape != GBS_Octagon && data.shape != GBS_Arrow && data.shape != GBS_ArrowLeft )
		{
			DrawLine( titleRect.x, titleRect.y + titleRect.height, titleRect.x + titleRect.width, titleRect.y + titleRect.height, data.border );
		}
	}
}
void CEdGraphEditor::DrawBlockSocketsCaptions( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data )
{
	// Draw sockets captions
	const Bool drawCaptions = GetScale() > 0.1f;
	for ( THashMap< THandle< CGraphSocket >, SocketLayoutInfo>::iterator i = layout->m_sockets.Begin(); i != layout->m_sockets.End(); ++i )
	{
		if( i->m_first.Get() == NULL )
		{
			continue;
		}

		// Get socket info
		CGraphSocket* socket = i->m_first.Get();
		const SocketLayoutInfo& info = i->m_second;

		// Draw socket caption
		if ( drawCaptions )
		{
			if ( !socket->IsNoDraw() && socket->CanDrawCaption() )
			{
				// Get socket caption
				String caption;
				socket->GetCaption( caption );

				wxColor color( data.text );

				AdjustSocketCaption( socket, caption, color );

				// Draw text
				wxPoint drawOffset = info.m_captionPos + data.location;
				DrawText( drawOffset, GetGdiDrawFont(), caption, color );
			}
		}
	}
}


void CEdGraphEditor::DrawBlockButtons( CGraphBlock* block, BlockLayoutInfo* layout, const SGraphBlockLayoutHelperData& data )
{
	if ( block->IsResizable() )
	{
		const Int32 cornerSize = GRAPH_BLOCK_RESIZE_RECT_SIZE;
		const wxPoint cornerPos(data.location.x + layout->m_windowSize.x, data.location.y + layout->m_windowSize.y );
		wxPoint points[3] = 
		{
			wxPoint( cornerPos.x, cornerPos.y),
			wxPoint( cornerPos.x, cornerPos.y - cornerSize ),
			wxPoint( cornerPos.x - cornerSize, cornerPos.y ),
		};
		FillPoly( &points[0], sizeof(points) / sizeof(wxPoint), data.border );
	}

	if ( block->CanFreezeContent() )
	{
		FillRect( data.location.x + 1 , data.location.y + layout->m_windowSize.y - GRAPH_BLOCK_RESIZE_RECT_SIZE - 1 , GRAPH_BLOCK_RESIZE_RECT_SIZE, GRAPH_BLOCK_RESIZE_RECT_SIZE, wxColour( 4, 170, 166 ) );
		DrawRect( data.location.x + 1 , data.location.y + layout->m_windowSize.y - GRAPH_BLOCK_RESIZE_RECT_SIZE - 1 , GRAPH_BLOCK_RESIZE_RECT_SIZE, GRAPH_BLOCK_RESIZE_RECT_SIZE, wxColour( 0, 0, 0) );
	}

	if ( block->IsMovingOverlayingBlocks() )
	{
		FillRect( data.location.x + layout->m_windowSize.x - GRAPH_BLOCK_RESIZE_RECT_SIZE - 1 , data.location.y , GRAPH_BLOCK_RESIZE_RECT_SIZE, GRAPH_BLOCK_RESIZE_RECT_SIZE, wxColour( 195, 195, 195 ) );
		DrawRect( data.location.x + layout->m_windowSize.x - GRAPH_BLOCK_RESIZE_RECT_SIZE - 1 , data.location.y , GRAPH_BLOCK_RESIZE_RECT_SIZE, GRAPH_BLOCK_RESIZE_RECT_SIZE, wxColour( 0, 0, 0 ) );
	}
}

void CEdGraphEditor::DrawBlockLayout( CGraphBlock* block )
{
	// Find layout info
	BlockLayoutInfo* layout = m_layout.FindPtr( block );
	if ( !layout || !layout->m_visible || !layout->m_onScreen )
	{
		return;
	}
	
	SGraphBlockLayoutHelperData data;
	// Calculate window offset
	data.location = wxPoint( block->GetPosition().X, block->GetPosition().Y );

	// Check if block is visible on the screen
	data.locationInScreen = CanvasToClient( data.location );
	data.canvasSize = GetCanvasSize();
	data.scale = GetScale();

	if ( data.locationInScreen.x > data.canvasSize.GetWidth() || data.locationInScreen.x + layout->m_windowSize.x * data.scale < 0 || data.locationInScreen.y > data.canvasSize.GetHeight() || data.locationInScreen.y + layout->m_windowSize.y * data.scale < 0 )
	{
		return;
	}

	// Get colors
	data.shape = block->GetBlockShape();
	data.clientColor = block->GetClientColor();
	data.borderColor = block->GetBorderColor();
	data.outerBorderColor = block->GetOuterBorderColor();
	data.titleColor = block->GetTitleColor();

	// Assemble selection rect
	data.selectionRect.x = Min( m_selectRectStart.x, m_selectRectEnd.x );
	data.selectionRect.y = Min( m_selectRectStart.y, m_selectRectEnd.y );
	data.selectionRect.width = Abs( m_selectRectEnd.x - m_selectRectStart.x );
	data.selectionRect.height = Abs( m_selectRectEnd.y - m_selectRectStart.y );

	// Get block rect
	data.blockRect.x = block->GetPosition().X;
	data.blockRect.y = block->GetPosition().Y;
	data.blockRect.width = layout->m_windowSize.x;
	data.blockRect.height = layout->m_windowSize.y;


	data.borderWidth = 1.0f;

	// Border highlights
	if ( ( m_activeItem.Get() == block ) || ( m_action == MA_SelectingWindows && data.selectionRect.Intersects( data.blockRect ) ) )
	{
		data.borderColor = Color( 200, 200, 200 );		
	}
	else if ( block->IsObsolete() )
	{
		data.borderColor = Color( 255, 0, 0 );
	}
	else if ( IsBlockSelected( block ) ) 
	{
		data.borderColor = Color( 255, 255, 0 );
	}

	// Block activation
	if ( IsBlockActivated( block ) )
	{
		// Get activation weight from block itself
		Float activationAlpha = GetBlockActivationAlpha( block );

		// Override color and border width
		data.borderWidth = 1.0f + activationAlpha * 5.0f;
		data.borderColor = Color( (Uint8)( data.borderColor.R * ( 1.0f-activationAlpha ) + 255.0f * activationAlpha ),
			(Uint8)( data.borderColor.G * ( 1.0f-activationAlpha ) + 255.0f * activationAlpha ),
			(Uint8)( data.borderColor.B * ( 1.0f-activationAlpha ) + 0.0f * activationAlpha ),
			255 );
	}

	// Title highlights
	if ( block->IsObsolete() )
	{
		data.titleColor = Color( 220, 32, 32 );
	}

	// Per block override
	AdjustBlockColors( block, data.borderColor, data.borderWidth );

	// Setup wx colors
	data.border = wxColour( data.borderColor.R, data.borderColor.G, data.borderColor.B );
	data.outerBorder = wxColour( data.outerBorderColor.R, data.outerBorderColor.G, data.outerBorderColor.B );
	data.title = wxColour( data.titleColor.R, data.titleColor.G, data.titleColor.B );
	data.client = wxColour( data.clientColor.R, data.clientColor.G, data.clientColor.B );
	data.shadow = wxColour( 40, 40, 40 );
	data.text = wxColour( 255, 255, 255 );

	if ( block->IsInnerAreaTransparent() )
	{
		data.client = wxColour( 0, 0, 0, 0 );
	}

	// If block is freeze color is in gray mode
	if ( layout->m_freeze )
	{
		data.border =		ConvertBlockColorToFreezeMode( data.border );
		data.outerBorder =	ConvertBlockColorToFreezeMode( data.outerBorder );
		data.title =		ConvertBlockColorToFreezeMode( data.title );
		data.client =		ConvertBlockColorToFreezeMode( data.client );
		data.shadow =		ConvertBlockColorToFreezeMode( data.shadow );
		data.text =			ConvertBlockColorToFreezeMode( data.text );
	}

	DrawBlockShadows( block, layout, data );
	DrawBlockSockets( block, layout, data );
	DrawBlockBackground( block, layout, data );
	DrawBlockBorder( block, layout, data );
	DrawBlockSocketsCaptions( block, layout, data );


	if ( block->IsExactlyA< CDescriptionGraphBlock >( ) )
	{
		// Calculate client rect
		wxRect rect;
		rect.x = data.location.x + layout->m_clientRect.x + LAYOUT_INNER_AREA_MARGIN_X;
		rect.y = data.location.y + layout->m_clientRect.y + LAYOUT_INNER_AREA_MARGIN_Y;
		rect.width  = layout->m_clientRect.width - 2*LAYOUT_INNER_AREA_MARGIN_X;
		rect.height = layout->m_windowSize.y - layout->m_titleRect.height - 2*LAYOUT_INNER_AREA_MARGIN_Y;

		CDescriptionGraphBlock* descBlock = SafeCast< CDescriptionGraphBlock >( block );
		String text = descBlock->GetDescriptionText();

		// Setup clipping and draw the text. The text is not clipped on active items.

		bool doClipText = ( m_activeItem.Get() != block );

		if ( doClipText ) 
		{
			SetClip( rect );
		}
		else
		{
			ResetClip();
			rect.height = 1000;
		}

		DrawText( rect.GetLeftTop(), rect.width, rect.height, GetGdiDrawFont(), text, wxColour(255,255,255) );

		if ( doClipText ) 
		{
			ResetClip();
		}
	}
	else if ( layout->m_innerRect.width && layout->m_innerRect.height )
	{
		// Calculate client rect
		wxRect rect;
		rect.x = data.location.x + layout->m_innerRect.x;
		rect.y = data.location.y + layout->m_innerRect.y;
		rect.width = layout->m_innerRect.width;
		rect.height = layout->m_innerRect.height;

		// Setup clipping rect and draw area
		SetClip( rect );
		DrawBlockInnerArea( block, rect );
		ResetClip();
	}

	// Draw display value for rounded blocks
	if ( data.shape == GBS_LargeCircle || data.shape == GBS_DoubleCircle )
	{
		String caption = GetBlockDisplayValue( block );

		// Draw text
		wxPoint size = TextExtents( GetGdiDrawFont(), caption.AsChar() );
		wxPoint offset( data.location.x + layout->m_windowSize.x/2 - size.x/2, data.location.y + layout->m_windowSize.y/2 - size.y/2 );
		DrawText( offset + wxPoint(1,1), GetGdiDrawFont(), caption.AsChar(), wxColour(0,0,0) );
		DrawText( offset, GetGdiDrawFont(), caption.AsChar(), wxColour(255,255,255) );
	}

	wxRect titleRect = layout->m_titleRect;
	titleRect.Offset( data.location );

	wxRect iconRect = layout->m_iconRect;
	iconRect.Offset( data.location );

	String caption = block->GetCaption();
	if ( block->IsObsolete() ) caption += TXT(" (Obsolete)");
	DrawTitleBar( titleRect, iconRect, data.border, data.client, data.title, caption, data.shape, data.borderWidth, block->ShouldCaptionBeInConstantSize() );

	if ( data.shape != GBS_LargeCircle && data.shape != GBS_DoubleCircle )
	{
		SetClip( iconRect );
		DrawTitleIconArea( block, iconRect );
		ResetClip();
	}

	DrawBlockButtons( block, layout, data );
}

String CEdGraphEditor::GetBlockDisplayValue( CGraphBlock* block )
{
	// Use the block value
	if ( block )
	{
		return block->GetDisplayValue();
	}

	// No caption
	return String::EMPTY;
}

void CEdGraphEditor::DrawDraggedLink()
{
	wxPoint src, srcDir, dest, destDir;

	// Setup params
	Color linkColor = m_sourceSocket->GetLinkColor();
	GetSocketLinkParams( m_sourceSocket, src, srcDir );
	GetSocketLinkParams( m_destSocket, dest, destDir );

	// Case for loop connection
	if ( m_destSocket && m_sourceSocket->GetBlock() == m_destSocket->GetBlock() )
	{
		srcDir.y = -2;
		destDir.y = -2;
	}

	// Draw curve
	DrawLink( wxColour( linkColor.R, linkColor.G, linkColor.B ), src, srcDir, dest, destDir );
}

void CEdGraphEditor::DrawBlockLinks( CGraphBlock* block )
{
	// Find layout info
	BlockLayoutInfo* layout = m_layout.FindPtr( block );
	if ( !layout )
	{
		return;
	}

	Bool modified = false; 

	// Block offset
	wxPoint offset;
	offset.x = block->GetPosition().X;
	offset.y = block->GetPosition().Y;

	// Draw sockets
	for ( THashMap< THandle< CGraphSocket >, SocketLayoutInfo>::iterator i = layout->m_sockets.Begin(); i != layout->m_sockets.End(); ++i )
	{
		if( i->m_first.Get() == NULL )
		{
			continue;
		}

		// Get socket info
		CGraphSocket* socket = i->m_first.Get();
		const SocketLayoutInfo& info = i->m_second;

		// Do not draw connections from invisible sockets
		if ( socket->IsNoDraw() && !socket->ForceDrawConnections() )
		{
			continue;
		}

		// Do not draw connections from input sockets ( because we draw the output connection :P )
		if ( socket->GetDirection() == LSD_Input )
		{
			continue;
		}

		// Get socket connections
		const TDynArray< CGraphConnection* >& connections = socket->GetConnections();
		
		// Draw connections
		for ( Uint32 j=0; j<connections.Size(); j++ )
		{
			CGraphConnection* con = connections[j];
			if ( con->GetSource( true ) == socket )
			{
				CGraphBlock* destBlock = con->GetDestination( true )->GetBlock();
				if ( !destBlock ) continue;

				BlockLayoutInfo* destLayout = m_layout.FindPtr( destBlock );
				BlockLayoutInfo* sourceLayout = layout;

				if ( !destLayout || ( !destLayout->m_visible && !sourceLayout->m_visible ) )
				{
					continue;
				}

				wxPoint src, srcDir, dest, destDir;

				// Source socket
				src = info.m_linkPos + offset;
				srcDir = info.m_linkDir;

				// Destination
				GetSocketLinkParams( con->GetDestination( true ), dest, destDir );

				// Check AABB for visibility test
				wxSize canvasSize = GetCanvasSize();
				wxRect canvasRect( wxPoint(0,0), canvasSize );

				if ( !HitTestLinkAABB( CanvasToClient(src), CanvasToClient(dest), canvasRect ) )
				{
					continue;
				}

				// Case for loop connection
				if ( con->GetSource( true )->GetBlock() == con->GetDestination( true )->GetBlock() )
				{
					srcDir.y = -2;
					destDir.y = -2;
				}

				// Draw curve
				Color linkColor = con->GetSource( true )->GetLinkColor();
				Float width = 3.0f;

				// Highlights the connection
				if ( m_activeItem.Get() == con )
				{
					linkColor = Color( 255,255,255 );
					width = 4.5f;

					if ( wxGetKeyState( WXK_SHIFT ) )
					{
						con->SetActive( false );
						TDynArray<CGraphConnection*> conDest =  con->GetDestination( true )->GetConnections();
						// Click detection is only for output. Switch for input.
						for( Uint32 i=0; i<conDest.Size(); ++i )
						{
							// Need to check both sides
							if( conDest[i]->GetDestination( true ) == con->GetSource( true ) && 
								conDest[i]->GetSource( true ) == con->GetDestination( true ) )
							{
								conDest[i]->SetActive( false );
								modified = true;
							}
						}
					}
					if ( wxGetKeyState( WXK_CONTROL ) )
					{
						con->SetActive( true );
						TDynArray<CGraphConnection*> conDest =  con->GetDestination( true )->GetConnections();
						// Click detection is only for output. Switch for input.
						for( Uint32 i=0; i<conDest.Size(); ++i )
						{
							// Need to check both sides
							if( conDest[i]->GetDestination( true ) == con->GetSource( true ) && 
								conDest[i]->GetSource( true ) == con->GetDestination( true ) )
							{
								conDest[i]->SetActive( true );
								modified = true;
							}
						}
					}
				}

				if ( IsBlockActivated( con->GetSource( true )->GetBlock() ) || IsBlockActivated( con->GetDestination( true )->GetBlock() ) )
				{
					Float alpha = IsBlockActivated( con->GetSource( true )->GetBlock() ) ? GetBlockActivationAlpha( con->GetSource( true )->GetBlock() ) : 0.0f;
					alpha = Min( alpha, IsBlockActivated( con->GetDestination( true )->GetBlock() ) ? GetBlockActivationAlpha( con->GetDestination( true )->GetBlock() ) : 0.0f );

					linkColor.R = (Uint8)(255 * alpha + (1.0f - alpha) * 0);
					linkColor.G = (Uint8)(255 * alpha + (1.0f - alpha) * 0);
					linkColor.B = (Uint8)(0 * alpha + (1.0f - alpha) * 0);
					linkColor.A = 255;
					width = 3 + alpha * 4.0f;
				}

				AdjustLinkColors( con->GetSource( true ), con->GetDestination( true ), linkColor, width );

				Bool srcCapArrow = false;
				Bool destCapArrow = false;
				AdjustLinkCaps( con->GetSource( true ), con->GetDestination( true ), srcCapArrow, destCapArrow );

				if (con->IsActive())
				{
					wxColour finalColor = wxColour( linkColor.R, linkColor.G, linkColor.B, linkColor.A );

					if ( !layout->m_visible || !layout->m_onScreen || !destLayout->m_onScreen || layout->m_freeze )
					{
						finalColor = ConvertLinkColorToFreezeMode( finalColor );
					}

					DrawLink( finalColor, src, srcDir, dest, destDir, width, srcCapArrow, destCapArrow );
				}
				else
				{
					if (linkColor.R == Color::BLACK.R && linkColor.G == Color::BLACK.G && linkColor.B == Color::BLACK.B )
					{
						// Color black, change to dark gray
						linkColor.R = linkColor.G = linkColor.B = 50;
					}
					else
					{
						// Little darker
						Float temp = 0.4f;

						if (linkColor.R*temp > 255) linkColor.R = 255;
						else linkColor.R *= temp;

						if (linkColor.G*temp > 255) linkColor.G = 255;
						else linkColor.G *= temp;

						if (linkColor.B*temp > 255) linkColor.B = 255;
						else linkColor.B *= temp;
					}

					wxColour finalColor = wxColour( linkColor.R, linkColor.G, linkColor.B, linkColor.A );

					if ( !layout->m_visible || !layout->m_onScreen || layout->m_freeze )
					{
						finalColor = ConvertLinkColorToFreezeMode( finalColor );
					}

					DrawLink( finalColor, src, srcDir, dest, destDir, width, srcCapArrow, destCapArrow );
				}

				AnnotateConnection( con, src, srcDir, dest, destDir, width );
			}
		}
	}
	if( modified )
	{
		GraphStructureModified();
		modified = false;
	}
}

void CEdGraphEditor::AnnotateConnection( const CGraphConnection* con, 
	const wxPoint &src, 
	const wxPoint& srcDir, 
	const wxPoint &dest, 
	const wxPoint &destDir, 
	float width )
{
	if ( con->IsActive() )
	{
		return;
	}

	Float size = 20;
	DrawCrossOnLink( *wxRED, src, srcDir, dest, destDir, size, 3.0f*width);
}

void CEdGraphEditor::PaintCanvas( Int32 width, Int32 height )
{
	// Colors
	wxColour back = GetCanvasColor();
	static wxColour text( 255, 255, 255 );
	static wxColour texth( 255, 255, 0 );
	static wxColour highlight( 0, 0, 80 );
	static wxColour rect( 0, 0, 255 );

	// Paint background
	Clear( back );

	// Draw graph
	if ( m_graph )
	{
		// Get all blocks
		TDynArray< CGraphBlock* > blocks;
		blocks = m_graph->GraphGetBlocks();

		Bool needCheckVisiblity = false;

		// Update layout
		for ( Int32 i=(Int32)blocks.Size()-1; i>=0; i-- )
		{
			CGraphBlock* block = blocks[i];

			// Update layout
			if ( block->m_needsLayoutUpdate )
			{
				UpdateBlockLayout( block );
				block->m_needsLayoutUpdate = false;

				needCheckVisiblity = true;
			}
		}

		if ( needCheckVisiblity )
		{
			CheckBlocksVisibility();
		}

		// Draw comment blocks
		for ( Int32 i=(Int32)blocks.Size()-1; i>=0; i-- )
		{
			CGraphBlock* block = blocks[i];

			if ( block->GetBlockDepthGroup() == GBDG_Comment )
			{
				DrawBlockLayout( block );
			}
		}

		// Draw background blocks
		for ( Int32 i=(Int32)blocks.Size()-1; i>=0; i-- )
		{
			CGraphBlock* block = blocks[i];

			if ( block->GetBlockDepthGroup() == GBDG_Background )
			{
				DrawBlockLayout( block );
			}
		}

		// Draw links
		for ( Int32 i=(Int32)blocks.Size()-1; i>=0; i-- )
		{
			CGraphBlock* block = blocks[i];
			DrawBlockLinks( block );
		}

		// Draw foreground blocks
		for ( Int32 i=(Int32)blocks.Size()-1; i>=0; i-- )
		{
			CGraphBlock* block = blocks[i];

			if ( block->GetBlockDepthGroup() == GBDG_Foreground )
			{
				DrawBlockLayout( block );
			}
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
			DrawRect( selRect, rect );
		}

		// Link drawing
		if ( m_action == MA_DraggingLink )
		{
			DrawDraggedLink();
		}
	}
}

void CEdGraphEditor::AdjustBlockColors( CGraphBlock* block, Color& borderColor, Float& borderWidth )
{

}

void CEdGraphEditor::AdjustLinkColors( CGraphSocket* source, CGraphSocket* destination, Color& linkColor, Float& linkWidth )
{

}

void CEdGraphEditor::AdjustLinkCaps( CGraphSocket* source, CGraphSocket* destination, Bool& srcCapArrow, Bool& destCapArrow )
{
	srcCapArrow = false;
	destCapArrow = false;
}

void CEdGraphEditor::AdjustSocketCaption( CGraphSocket* socket, String& caption, wxColor& color )
{

}
