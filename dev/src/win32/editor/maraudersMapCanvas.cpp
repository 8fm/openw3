/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "maraudersMapCanvas.h"
#include "maraudersMapItems.h"
#include "../../common/engine/renderFrame.h"

// Event table
BEGIN_EVENT_TABLE( CMaraudersMapCanvas, CMaraudersMapCanvasDrawing )
	EVT_MOUSEWHEEL( CMaraudersMapCanvas::OnMouseWheel )
	EVT_LEFT_DCLICK( CMaraudersMapCanvas::OnMouseLeftDblClick )
	EVT_CHAR( CMaraudersMapCanvas::OnCharPressed )
END_EVENT_TABLE()

// #define MARAUDERS_MAP_DEBUG

CMaraudersMapCanvas::CMaraudersMapCanvas( wxWindow* parent )
	: CMaraudersMapCanvasDrawing( parent )
	, m_action( MA_None )
	, m_lastMousePosition( 0.0f, 0.0f, 0.0f )
	, m_clickedMousePosition( 0.0f, 0.0f, 0.0f )
	, m_moveTotal( 0 )
	, m_itemMouseOver( NULL )
	, m_itemSelected( NULL )
	, m_leftMouseOption( LMO_NONE )
	, m_isFollowingPlayer( false )
	, m_isWaypointEnabled( false )
	, m_logTimer( 10.0f )
	, m_logTimerValue( 10.0f )
	, m_logLastTime( 0 )
{
}

CMaraudersMapCanvas::~CMaraudersMapCanvas()
{
}

void CMaraudersMapCanvas::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	if ( m_itemSelected && m_itemSelected->IsValid() )
	{
		Vector selItemWorldPos = m_itemSelected->GetWorldPosition();

		frame->AddDebugSphere( selItemWorldPos, 1.0f, Matrix::IDENTITY, Color::BLUE );
		frame->AddDebugSphere( selItemWorldPos, 0.9f, Matrix::IDENTITY, Color::RED );
	}
	if ( m_isWaypointEnabled )
	{
		frame->AddDebugSphere( m_waypoint, 1, Matrix::IDENTITY, Color::WHITE );
	}

	FastUpdateItemsTick();
}

void CMaraudersMapCanvas::PaintCanvas( Int32 width, Int32 height )
{
	RemoveInvalidItems();

	if ( m_isFollowingPlayer )
	{
		if ( const CPlayer *player = GCommonGame->GetPlayer() )
		{
			Center( player->GetWorldPosition() );
		}
	}

	// Colors
	wxColour back( 0, 0, 0 ); // GetCanvasColor()

	// Paint background
	Clear( back );
	DrawGrid( width, height, true, true );

	// draw items
	for ( TDynArray< CMaraudersMapItemBase * >::const_iterator item = m_items.Begin();
		  item != m_items.End();
		  ++item )
	{
		if ( IsItemVisible( *item ) )
		{
			if ( (*item)->DoesRequireFastUpdate() )
			{
				AddItemToFastUpdate( *item );
			}
			if ( (*item)->Update( this ) )
			{
				GGame->Pause( TXT( "CMaraudersMapCanvas::PaintCanvas" ) );
				SelectItem( *item );
			}
			(*item)->Draw( this );
		}
	}

	// selected item
	if ( m_itemSelected ) //&& m_itemSelected->IsVisible() && !m_hiddenItemsIDs.Exist( m_itemSelected->GetLayerID() ) )
	{
		m_itemSelected->DrawSelected( this );
		m_itemSelected->DrawTooltip( this );
	}

	// mouse over item
	if ( m_itemMouseOver && m_itemMouseOver->IsVisible() && !m_hiddenItemsIDs.Exist( m_itemMouseOver->GetLayerID() ) )
	{
		m_itemMouseOver->DrawTooltip( this );
	}

	// camera
	DrawFreeCamera();

	// Draw dragged obj
	if ( m_action == MA_MovingActor )
	{
		static wxColor color( 255, 255, 255 );
		DrawCircleConstRadiusCanvas( m_lastMousePosition, 12, color );
	}

	if ( m_isWaypointEnabled )
	{
		static wxColor color( 100, 255, 255 );
		DrawCircleConstRadiusCanvas( m_waypoint, 9, color );
	}

	DrawLog();

#ifdef MARAUDERS_MAP_DEBUG
	for ( TDynArray< CMaraudersMapItemBase * >::const_iterator item = m_items.Begin();
		item != m_items.End();
		++item )
	{
		wxRect rect = (*item)->GetClientBorder( this );
		wxColor color(255,0,0);
		
		DrawRect( ClientToCanvas( rect ), color );
	}
#endif
}

Vector CMaraudersMapCanvasDrawing::CanvasToWorld( wxPoint point ) const
{
	return Vector( point.x * m_scaleWorld.X, -point.y * m_scaleWorld.Y, 0.0f );
}

wxPoint CMaraudersMapCanvasDrawing::WorldToCanvas( const Vector& point ) const
{
	return wxPoint( point.X / m_scaleWorld.X, -point.Y / m_scaleWorld.Y );
}

Vector CMaraudersMapCanvasDrawing::ClientToWorld( wxPoint point ) const
{
	return CanvasToWorld( ClientToCanvas( point ) );
}

wxPoint CMaraudersMapCanvasDrawing::WorldToClient( const Vector& point ) const
{
	return CanvasToClient( WorldToCanvas( point ) );
}

void CMaraudersMapCanvas::OnMouseLeftDblClick( wxMouseEvent &event )
{
	//if ( !m_trackItemMouseOverClicked ) return;

	//if ( m_trackItemMouseOverClicked->SupportsColor() )
	//{
	//	OnEditTrackItemColor( wxCommandEvent() );
	//}
	//else if ( m_trackItemMouseOverClicked->SupportsCurve() )
	//{
	//	OnAddTrackItemCurve( wxCommandEvent() );
	//}
}

void CMaraudersMapCanvas::OnMouseWheel( wxMouseEvent& event )
{
	if ( IsMouseOverCanvasPanel( event.GetPosition() ) )
	{
		// Find zooming pivot
		Vector zoomingPivot = ClientToWorld( event.GetPosition() );

		// Calculate zoomed region
		Vector zoomedRegion = GetZoomedRegion();
		Vector zoomedMin = Vector( zoomedRegion.X, zoomedRegion.Y, 0.0f );
		Vector zoomedMax = Vector( zoomedRegion.Z, zoomedRegion.W, 0.0f );

		// Calculate scale, but sometimes GetWheelRotation() returns zero
		Float delta = 0.0f;
		if ( event.GetWheelRotation() != 0 )
		{
			delta = ( event.GetWheelDelta() / (FLOAT)event.GetWheelRotation() );
		}
		// increase speed if shift is pressed
		Float wheelScale = event.ShiftDown() ? 0.25f : 0.05f;

		// Calculate new zoomed region
		Vector newZoomedMin = zoomingPivot + (zoomedMin - zoomingPivot) * (1.0f - delta * wheelScale);
		Vector newZoomedMax = zoomingPivot + (zoomedMax - zoomingPivot) * (1.0f - delta * wheelScale);
		SetZoomedRegion( newZoomedMin, newZoomedMax );
	}
//	else
	{
//		ScrollButtons( event.GetWheelRotation() / event.GetWheelDelta() );
//		Repaint();
	}
}

void CMaraudersMapCanvas::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	//if ( IsMouseOverCanvasPanel( event.GetPosition() ) )
	{
		const Int32 autoScrollBorderSize = 10;

		// We do not repaint if it's not necessary, because preview panel stops when we repaint
		//if ( m_action == MA_Zooming )
		//{
		//	Float wheelScale = 0.0001f;
		//	Vector curveDelta = ClientToWorld( delta ) * wheelScale;
		//	m_scaleWorld.X = Clamp< Float >( m_scaleWorld.X + delta.x * wheelScale, 0.0001f, 100.0f );
		//	m_scaleWorld.Y = Clamp< Float >( m_scaleWorld.Y + delta.y * wheelScale, 0.0001f, 100.0f );
		//	Refresh();
		//	Update();

		//	// We don't want auto scroll, and lastMousePosition updated
		//	return;
		//}
		//else
		if ( m_action == MA_None )
		{
			Bool itemSelected = false;
			m_itemsMouseOver.Clear();
			for ( TDynArray< CMaraudersMapItemBase * >::const_iterator item = m_items.Begin();
				  item != m_items.End();
				  ++item )
			{
				if ( (*item)->IsValid() && (*item)->GetClientBorder( this ).Contains( event.GetPosition() ) && IsItemSelectable( *item ) )
				{
					m_itemMouseOver = *item;
					itemSelected = true;
					m_itemsMouseOver.PushBack( *item );
					//break; // uncomment to take only the first
				}
			}
			if ( !itemSelected ) m_itemMouseOver = NULL;
		}
		else if ( m_action == MA_BackgroundScroll )
		{
			ScrollBackgroundOffset( delta );
			Repaint( true );
		}
		else if ( m_action == MA_MovingActor )
		{
			Repaint( true );
		}

		// Common part, auto scroll, calc total mouse move, reset auto scroll and calc last mouse position
		{
			// Accumulate move
			m_moveTotal += Abs( delta.x ) + Abs( delta.y );

			// Remember mouse position
			m_lastMousePosition = ClientToWorld( event.GetPosition() );
		}
	}
}

void CMaraudersMapCanvas::MouseClick( wxMouseEvent& event )
{
	// Pass to base class
	CEdCanvas::MouseClick( event );

	if ( event.LeftDown() || event.RightDown() )
	{
		m_clickedMousePosition = ClientToWorld( event.GetPosition() );
	}

	if ( IsMouseOverCanvasPanel( event.GetPosition() ) )
	{
		MouseClickCanvasPanel( event );
	}

	// Always repaint canvas when clicked
	Repaint();
}

void CMaraudersMapCanvas::MouseClickCanvasPanel( wxMouseEvent& event )
{
	if ( m_action == MA_None && event.LeftDown() && m_leftMouseOption != LMO_NONE )
	{
		ExecuteTool();
		return;
	}

	// Selecting items with middle mouse click
	//if ( m_action == MA_None && !event.ShiftDown() && !event.ControlDown() && !event.AltDown() &&
	//	event.MiddleDown() && m_itemMouseOver )
	//{
	//	if ( m_itemsMouseOver.Size() > 1 )
	//	{
	//		for ( TDynArray< CMaraudersMapItemBase* >::iterator itemMouseOver = m_itemsMouseOver.Begin();
	//			  itemMouseOver != m_itemsMouseOver.End();
	//			  ++itemMouseOver )
	//		{
	//			if ( (*itemMouseOver)->GetMarItemType() == MMIT_NPC )
	//			{
	//				m_itemSelected = *itemMouseOver;
	//				return;
	//			}
	//		}
	//	}
	//	else
	//	{
	//		m_itemSelected = m_itemMouseOver;
	//		return;
	//	}
	//}

	// Selecting items
	if ( m_action == MA_None && !event.ShiftDown() && !event.ControlDown() && !event.AltDown() &&
		 event.LeftDown() && m_itemMouseOver )
	{
		if ( m_itemsMouseOver.Size() > 1 )
		{
			ShowItemSelectionContextMenu( event.GetPosition() );
		}
		else
		{
			m_itemSelected = m_itemMouseOver;
		}
	}
	else if ( m_action == MA_None && event.LeftDown() && m_itemMouseOver == NULL )
	{
		m_itemSelected = NULL;
	}

	// Context menu
	if ( m_action == MA_None && event.RightDown() && m_itemMouseOver )
	{
		// If only one item under cursor select it
		if ( m_itemsMouseOver.Size() == 1 )
		{
			m_itemSelected = m_itemsMouseOver[ 0 ];
		}

		// Show menu for selected item
		if( m_itemSelected && m_itemSelected->GetOptionsSize() > 0 )
			{
				ShowItemContextMenu( event.GetPosition() );
				return;
			}
		}

	// Dragging items
	if ( m_action == MA_None && event.LeftDown() && event.ShiftDown()
		&& m_itemSelected && m_itemSelected->CanBeDragged() )
	{
		m_action = MA_MovingActor;
	}
	else if ( m_action == MA_MovingActor && event.LeftUp() )
	{
		if ( m_itemSelected )
		{
			Vector collisionPos;
			if ( CMaraudersMapCanvas::GetTerrainCollision( m_lastMousePosition, collisionPos ) )
			{
				m_itemSelected->SetDraggedPos( collisionPos );
			}
		}
		m_action = MA_None;
		return;
	}

	// Zooming via mouse move (x and y scale are not locked)
	if ( m_action == MA_None && event.RightDown() && event.ControlDown() )
	{
		m_action = MA_Zooming;
		CaptureMouse( true, false );
		return;
	}
	else if ( m_action == MA_Zooming && event.RightUp() )
	{
		m_action = MA_None;
		CaptureMouse( false , false );
		return;
	}

	// Background drag
	if ( m_action == MA_None && event.RightDown() )
		//&& ( !m_trackMouseOver || ( m_clickedMousePosition.DistanceTo( m_lastMousePosition ) >= maxMouseTravelDistToShowMenu ) ) )
	{
		m_action = MA_BackgroundScroll;
		CaptureMouse( true, true );
		m_moveTotal	= 0;
	}
	else if ( m_action == MA_BackgroundScroll && event.RightUp() )
	{
		m_action = MA_None;
		CaptureMouse( false, true );

		//// Minimal movement, show menu
		//if ( m_moveTotal < maxMoveTotal )
		//{
		//	// Tracks menu
		//	if ( m_action == MA_None && event.RightUp() && m_trackMouseOver && !m_trackItemMouseOver 
		//		/*&& m_clickedMousePosition.DistanceTo( m_lastMousePosition ) < maxMouseTravelDistToShowMenu*/ )
		//	{
		//		m_trackMouseOverClicked = m_trackMouseOver;
		//		m_trackItemMouseOver = NULL;
		//		m_trackMouseOver = NULL;

		//		// show context menu for button
		//		wxMenu menu;

		//		{
		//			wxMenu *submenuTrackItems = new wxMenu();
		//			for ( Uint32 trackItemClassNum = 0; trackItemClassNum < m_trackItemClasses.Size(); ++trackItemClassNum )
		//			{
		//				submenuTrackItems->Append( trackItemClassNum, m_trackItemClasses[trackItemClassNum]->GetName().AsChar() );
		//				menu.Connect( trackItemClassNum, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnAddTrackItem ), NULL, this );
		//			}

		//			menu.AppendSubMenu( submenuTrackItems, TXT("Add track item") );
		//		}

		//		//menu.Append( trackItemClasses.Size() + 0, TXT("Add track item") );
		//		//menu.Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnButtonAppendTrack ), NULL, this );

		//		PopupMenu( &menu );

		//		return;
		//	}
		//}
	}
}

void CMaraudersMapCanvas::DrawGrid( Int32 width, Int32 height, Bool drawLines, Bool drawText )
{
	const Int32 brightness = 60;
	static wxColour textColor( 255, 255, 255 );
	static wxColour lines1( 100 - brightness, 100 - brightness, 100 - brightness );
	static wxColour lines2( 140 - brightness, 140 - brightness, 140 - brightness );
	static wxColour lines3( 170 - brightness, 170 - brightness, 170 - brightness );

	Vector gridSpacing;
	Vector zoomedRegion = GetZoomedRegion();
	wxSize clientSize = GetClientSize();
	gridSpacing.X = SnapFloat( (zoomedRegion.Z - zoomedRegion.X) / ( (clientSize.GetWidth()+1) / 80 ) );
	gridSpacing.Y = SnapFloat( (zoomedRegion.W - zoomedRegion.Y) / ( (clientSize.GetHeight()+1) / 40 ) );

	// Vertical
	for( Int32 i = (Int32)( zoomedRegion.X / gridSpacing.X ); i <= (Int32)( zoomedRegion.Z / gridSpacing.X ); ++i )
	{
		if ( drawLines )
		{
			wxColour color = ( i == 0 ) ? lines3 : ( ( i % 8 ) ? lines1 : lines2 );
			DrawLineCanvas( i * gridSpacing.X, zoomedRegion.Y, i * gridSpacing.X, zoomedRegion.W, color );
		}	
		if ( drawText )
		{
			String string = String::Printf( TXT("%.2f"), i * gridSpacing.X );
			DrawTextCanvas( i * gridSpacing.X, zoomedRegion.Y, GetGdiDrawFont(), string, textColor, CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Center );
		}
	}

	// Horizontal
	for( Int32 i = (Int32)( zoomedRegion.Y / gridSpacing.Y ); i <= (Int32)( zoomedRegion.W / gridSpacing.Y ); ++i )
	{
		if ( drawLines )
		{
			wxColour color = ( i == 0 ) ? lines3 : ( ( i % 8 ) ? lines1 : lines2 );
			DrawLineCanvas( zoomedRegion.X, i * gridSpacing.Y, zoomedRegion.Z, i * gridSpacing.Y, color );
		}	
		if ( drawText )
		{
			String string = String::Printf( TXT("%.2f"), i * gridSpacing.Y );
			DrawTextCanvas( zoomedRegion.X, i * gridSpacing.Y, GetGdiDrawFont(), string, textColor, CEdCanvas::CVA_Center, CEdCanvas::CHA_Left );
		}
	}
}

void CMaraudersMapCanvas::DrawFreeCamera()
{
	static wxColor cameraPosColor( 0, 155, 0 );
	static wxColor cameraRotColor( 0, 255, 0 );

	if ( GGame->IsFreeCameraEnabled() )
	{
		Vector freeCamPos( 0, 0, 0 );
		EulerAngles freeCamRot( 0, 0, 0 );
		GGame->GetFreeCameraWorldPosition( &freeCamPos, &freeCamRot, NULL );

		DrawCircleConstRadiusCanvas( freeCamPos, 14, cameraPosColor, 2.5f );

		freeCamRot.Pitch = 0.0f;
		freeCamRot.Roll = 0.0f;
		Vector rotVec = freeCamRot.TransformVector( Vector::EY );
		DrawCircleConstRadiusCanvas( freeCamPos + rotVec * 4, 8, cameraRotColor, 0.9f );
	}
}

Vector CMaraudersMapCanvasDrawing::GetZoomedRegion() const
{
	wxSize clientSize = GetClientSize();
	Vector corner1 = ClientToWorld( wxPoint( 0, 0 ) );
	Vector corner2 = ClientToWorld( wxPoint( clientSize.GetWidth(), clientSize.GetHeight() ) );
	return Vector( corner1.X, corner2.Y, corner2.X, corner1.Y );
}

Float CMaraudersMapCanvasDrawing::SnapFloat( const Float& floatToSnap ) const
{
	ASSERT( floatToSnap > 0.0f );
	Float snappedValue = 1024.0f;
	while( floatToSnap < snappedValue )
	{
		snappedValue /= 2.0f;
	}
	return snappedValue;
}

void CMaraudersMapCanvasDrawing::DrawTextCanvas( const Vector& offset, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign )
{
	wxPoint v1 = WorldToCanvas( offset );
	CEdCanvas::DrawText( v1, font, text, color, vAlign, hAlign );
}

void CMaraudersMapCanvasDrawing::DrawTextCanvas( const Float x, const Float y, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign )
{
	wxPoint v1 = WorldToCanvas( Vector( x, y, 0.0f ) );
	CEdCanvas::DrawText( v1, font, text, color, vAlign, hAlign );
}

void CMaraudersMapCanvasDrawing::DrawLineCanvas( Float x1, Float y1, Float x2, Float y2, const wxColour& color, Float width/*=1.0f*/ )
{
	wxPoint v1 = WorldToCanvas( Vector( x1, y1, 0.0f ) );
	wxPoint v2 = WorldToCanvas( Vector( x2, y2, 0.0f ) );
	CEdCanvas::DrawLine( v1, v2, color, width );
}

void CMaraudersMapCanvasDrawing::FillRectCanvas( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color )
{
	wxPoint vc = WorldToCanvas( Vector( x, y, 0.0f ) );
	wxPoint wc = WorldToCanvas( Vector( w, 0.0f, 0.0f ) );
	wxPoint hc = WorldToCanvas( Vector( h, 0.0f, 0.0f ) );
	CEdCanvas::FillRect( vc.x, vc.y, wc.x, hc.x, color );
}

void CMaraudersMapCanvasDrawing::DrawLineCanvas( const Vector& start, const Vector& end, const wxColour& color, Float width/*=1.0f*/ )
{
	wxPoint v1 = WorldToCanvas( start );
	wxPoint v2 = WorldToCanvas( end );
	CEdCanvas::DrawLine( v1, v2, color, width );
}

void CMaraudersMapCanvasDrawing::DrawCircleCanvas( Float x, Float y, Float radius, const wxColour& color, Float width /* = 1.0f */ )
{
	wxPoint centerCanvas = WorldToCanvas( Vector( x, y, 0 ) );
	wxPoint radiusCanvas = WorldToCanvas( Vector( radius, 0, 0 ) );
	centerCanvas += wxPoint( -radiusCanvas.x, -radiusCanvas.x );
	CEdCanvas::DrawCircle( centerCanvas.x, centerCanvas.y, radiusCanvas.x * 2, color, width );
}

void CMaraudersMapCanvasDrawing::DrawCircleCanvas( const Vector& center, Float radius, const wxColour& color, Float width /* = 1.0f */ )
{
	DrawCircleCanvas( center.X, center.Y, radius, color, width );
}

void CMaraudersMapCanvasDrawing::DrawCircleConstRadiusCanvas( const Vector& center, Int32 radius, const wxColour& color, Float width /* = 1.0f */ )
{
	wxPoint centerCanvas = WorldToCanvas( center );
	centerCanvas += wxPoint( -radius, -radius );
	CEdCanvas::DrawCircle( centerCanvas.x, centerCanvas.y, radius * 2, color, width );
}

void CMaraudersMapCanvasDrawing::FillCircleCanvas( const Vector& center, Float radius, const wxColour& color )
{
	wxPoint centerCanvas = WorldToCanvas( center );
	wxPoint radiusCanvas = WorldToCanvas( Vector( radius, 0, 0 ) );
	centerCanvas += wxPoint( -radiusCanvas.x, -radiusCanvas.x );
	CEdCanvas::FillCircle( centerCanvas.x, centerCanvas.y, radiusCanvas.x * 2, color );
}

void CMaraudersMapCanvasDrawing::FillCircleConstRadiusCanvas( const Vector& center, Int32 radius, const wxColour& color )
{
	wxPoint centerCanvas = WorldToCanvas( center );
	centerCanvas += wxPoint( -radius, -radius );
	CEdCanvas::FillCircle( centerCanvas.x, centerCanvas.y, radius * 2, color );
}

void CMaraudersMapCanvasDrawing::DrawCrossCavnas( const Vector &center, Float size, const wxColour& color, Float width /* = 1.0f */ )
{
	wxPoint centerCanvas = WorldToCanvas( center );
	wxPoint sizeCanvas = WorldToCanvas( Vector( size, 0, 0 ) );
	wxPoint topLeft( centerCanvas.x - sizeCanvas.x, centerCanvas.y + sizeCanvas.x );
	wxPoint bottomRight( centerCanvas.x + sizeCanvas.x, centerCanvas.y - sizeCanvas.x );
	wxRect rect( topLeft, bottomRight );
	CEdCanvas::DrawCross( rect, color, width );
}

void CMaraudersMapCanvasDrawing::DrawCrossCavnasConstSize( const Vector &center, Int32 size, const wxColour& color, Float width /* = 1.0f */ )
{
	wxPoint centerCanvas = WorldToCanvas( center );
	wxPoint topLeft( centerCanvas.x - size, centerCanvas.y + size );
	wxPoint bottomRight( centerCanvas.x + size, centerCanvas.y - size );
	wxRect rect( topLeft, bottomRight );
	CEdCanvas::DrawCross( rect, color, width );
}

void CMaraudersMapCanvasDrawing::DrawPolyCanvas( const Vector points[], Int32 numPoints, const wxColour& color, Float width /* = 1.0f */ )
{
	static TDynArray< wxPoint > pts;
	pts.ClearFast();
	for( Int32 i=0; i<numPoints; i++ )
	{
		pts.PushBack( WorldToCanvas( points[i] ) );
	}
	CEdCanvas::DrawPoly( ( wxPoint * )pts.Data(), pts.Size(), color, width );
}

void CMaraudersMapCanvasDrawing::FillPolyCanvas( const Vector points[], Int32 numPoints, const wxColour& color )
{
	static TDynArray< wxPoint > pts;
	pts.ClearFast();
	for( Int32 i=0; i<numPoints; i++ )
	{
		pts.PushBack( WorldToCanvas( points[i] ) );
	}
	CEdCanvas::FillPoly( ( wxPoint * )pts.Data(), pts.Size(), color );
}

void CMaraudersMapCanvas::ScrollBackgroundOffset( wxPoint delta )
{
	wxPoint point = GetOffset();

	// Scroll background offset
	point.x += delta.x;			
	point.y += delta.y;

	// Scroll canvas
	SetOffset( point );
}

void CMaraudersMapCanvasDrawing::SetZoomedRegion( const Vector& corner1, const Vector& corner2 )
{
	Vector previousScaleWorld = m_scaleWorld;
	wxPoint previousOffset = GetOffset();

	if ( corner1 == corner2 )
	{
		// Zoomed region has no area
		return;
	}

	// Calculate upper left corner and lower right corner
	Vector start = Vector::Min4( corner1, corner2 );
	Vector end   = Vector::Max4( corner1, corner2 );

	// Calculate proper clientRect (which take into account left side curvesInfo)
	wxRect clientRect = GetClientRect();

	// Calculate new scale
	m_scaleWorld.X = (end.X - start.X) / Max(clientRect.width , 1);
	m_scaleWorld.Y = (end.Y - start.Y) / Max(clientRect.height, 1);

	// X and Y scales have to be the same
	if ( m_scaleWorld.X > m_scaleWorld.Y )
		m_scaleWorld.Y = m_scaleWorld.X;
	else
		m_scaleWorld.X = m_scaleWorld.Y;

	// Calculate new offset
	wxPoint offset;
	offset.x = -(start.X / m_scaleWorld.X);
	offset.y = +(start.Y / m_scaleWorld.Y) + clientRect.height;
	SetOffset( offset );

	// Make sure we dont get an invalid zoomed region from GetZoomedRegion
	// and if so, revert to the previous values
	Vector zoomedRegion = GetZoomedRegion();
	if ( (zoomedRegion.Z - zoomedRegion.X) / ( (clientRect.width+1) / 80 ) < 0.0001 ||
		 (zoomedRegion.W - zoomedRegion.Y) / ( (clientRect.height+1) / 40 ) < 0.0001 )
	{
		m_scaleWorld = previousScaleWorld;
		SetOffset( previousOffset );
	}

	Repaint();
}

void CMaraudersMapCanvasDrawing::Center( const Vector &centerPos )
{
	wxPoint newOffset;
	wxRect clientRect = GetClientRect();
	newOffset.x = -(centerPos.X / m_scaleWorld.X);
	newOffset.y = +(centerPos.Y / m_scaleWorld.Y) + clientRect.height;
	newOffset.x += clientRect.width/2;
	newOffset.y -= clientRect.height/2;
	SetOffset( newOffset );
}

void CMaraudersMapCanvas::OnCharPressed( wxKeyEvent& event )
{
	if ( event.GetKeyCode() == WXK_HOME )
	{
		// Move to the map
		if ( event.AltDown() )
		{
			if ( !m_items.Empty() )
			{
				Vector minVec( m_items[0]->GetWorldPosition() );
				Vector maxVec( minVec );

				for ( TDynArray< CMaraudersMapItemBase * >::const_iterator item = m_items.Begin();
					item != m_items.End();
					++item )
				{
					minVec = Vector::Min4( minVec, (*item)->GetWorldPosition() ); 
					maxVec = Vector::Max4( maxVec, (*item)->GetWorldPosition() ); 
				}

				// make symmetric scale (scale.x == scale.y)
				Float xSize = fabs( minVec.X - maxVec.X );
				Float ySize = fabs( minVec.Y - maxVec.Y );
				Float diffSize = xSize - ySize;
				if ( diffSize > 0 )
				{
					minVec.Y -= diffSize / 2.0f;
					maxVec.Y += diffSize / 2.0f;
				}
				else if ( diffSize < 0 )
				{
					minVec.X += diffSize / 2.0f;
					maxVec.X -= diffSize / 2.0f;
				}

				SetZoomedRegion( minVec, maxVec );
			}
		}
		// Center on (0,0)
		else if ( event.ControlDown() )
		{
			// Shift position to start (zero)
			wxRect clientRect = GetClientRect();
			wxPoint point( clientRect.width/2, clientRect.height/2 );
			SetOffset( point );
		}
		// Center on player
		else
		{
			if ( const CPlayer *player = GCommonGame->GetPlayer() )
			{
				Center( player->GetWorldPosition() );
			}
		}
	}

	event.Skip();
}

Bool CMaraudersMapCanvas::IsMouseOverCanvasPanel( const wxPoint& mousePosition ) const
{
	return true;
}

void CMaraudersMapCanvas::UpdateLayout()
{

}

void CMaraudersMapCanvas::TeleportPlayer()
{
	SetCursor( wxCURSOR_PENCIL );

	m_leftMouseOption = LMO_PLAYER_TELEPORT;
}

void CMaraudersMapCanvas::TeleportCamera()
{
	SetCursor( wxCURSOR_PENCIL );

	m_leftMouseOption = LMO_CAMERA_TELEPORT;
}

void CMaraudersMapCanvas::SetFollowingPlayer( Bool isFollowingPlayer )
{
	m_isFollowingPlayer = isFollowingPlayer;
}

void CMaraudersMapCanvas::SetWaypoint( const Vector& worldPos )
{
	m_waypoint = worldPos;
	m_isWaypointEnabled = true;
}

void CMaraudersMapCanvas::ClearWaypoint()
{
	m_isWaypointEnabled = false;
}

void CMaraudersMapCanvas::GotoWaypoint()
{
	if ( m_isWaypointEnabled )
	{
		Center( m_waypoint );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// reset canvas state
void CMaraudersMapCanvas::Reset()
{
	// clear selection
	m_itemMouseOver = NULL;
	m_itemSelected = NULL;

	// remove items
	for ( TDynArray< CMaraudersMapItemBase * >::iterator item = m_items.Begin();
		  item != m_items.End();
		  ++item )
	{
		delete *item;
	}

	// clear arrays
	m_items.Clear();
	m_fastUpdateItems.Clear();
	m_itemsMouseOver.Clear();
	m_hiddenItemsIDs.Clear();
}

Bool CMaraudersMapCanvas::ItemExist( const CMaraudersMapItemBase &item )
{
	const Uint32 itemsCount = m_items.Size();
	for ( Uint32 i = 0; i < itemsCount; ++i )
	{
		if ( *m_items[i] == item ) return true;
	}

	return false;
}

Bool CMaraudersMapCanvas::AddItem( CMaraudersMapItemBase *item, Bool checkUniqueness /* = true */ )
{
	if ( checkUniqueness )
	{
		if( ItemExist( *item ) )
			return false;
	}

	m_items.PushBack( item );
	return true;
}

void CMaraudersMapCanvasDrawing::DrawTooltip( Float xPosTime, Float yPosTime, const String &text )
{
	static wxColour textColor( 0, 0, 0 );
	static wxColour bgColor( 255, 255, 0 );

	Float position = xPosTime;

	// Shift text if it extends over client size, so it will be always visible
	wxPoint textExtents = TextExtents( GetGdiDrawFont(), text );
	if ( WorldToClient( Vector(xPosTime,0,0) ).x + textExtents.x > GetClientSize().GetWidth() )
	{
		// shift to left
		position = ClientToWorld( wxPoint( WorldToClient( Vector(xPosTime,0,0) ).x - textExtents.x, 0 ) ).X;
	}
	//else if ( WorldToClient( Vector(xPosTime,0,0) ).x - textExtents.x < 0 )
	//{
	//	// shift to right
	//	position = ClientToWorld( wxPoint( WorldToClient( Vector(xPosTime,0,0) ).x + textExtents.x/2, 0 ) ).X;
	//}

	//FillRectCanvas( position, yPosTime, textExtents.x, textExtents.y, bgColor );
	//DrawTextCanvas( position, yPosTime, GetGdiDrawFont(), string, textColor, 
	//	CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Center );

	wxPoint v = WorldToCanvas( Vector( position, yPosTime, 0.0f ) );
	CEdCanvas::DrawTooltip( v, text, textColor, bgColor );
}
//////////////////////////////////////////////////////////////////////////
//
// clean items
void CMaraudersMapCanvas::RemoveInvalidItems()
{
	if ( m_itemSelected && !m_itemSelected->IsValid() )
	{
		m_itemSelected = NULL;
	}

	if ( m_itemMouseOver && !m_itemMouseOver->IsValid() )
	{
		m_itemMouseOver = NULL;
	}

	if ( m_itemSelected && !m_itemSelected->IsValid() )
	{
		m_itemSelected = NULL;
	}

	for ( Int32 i = (Int32)m_itemsMouseOver.Size() - 1; i >= 0; --i )
	{
		if ( !m_itemsMouseOver[ i ]->IsValid() )
		{
			m_itemsMouseOver.RemoveAtFast( i );
		}
	}

	for ( Int32 i = (Int32)m_fastUpdateItems.Size() - 1; i >= 0; --i )
	{
		if ( !m_fastUpdateItems[i]->IsValid() )
		{
			m_fastUpdateItems.RemoveAtFast( i );
		}
	}

	for ( Int32 i = (Int32)m_items.Size() - 1; i >= 0; --i )
	{
		if ( !m_items[i]->IsValid() )
		{
			delete m_items[i];
			m_items.RemoveAtFast( i );
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// deselect item
void CMaraudersMapCanvas::Deselect()
{
	m_itemSelected = NULL;
	m_itemMouseOver = NULL;
	m_itemsMouseOver.Clear();
}

const CMaraudersMapItemBase * CMaraudersMapCanvas::GetSelectedItem()
{
	return m_itemSelected;
}

void CMaraudersMapCanvas::HideItem( Int32 id )
{
	m_hiddenItemsIDs.PushBackUnique( id );
}

void CMaraudersMapCanvas::ShowItem( Int32 id )
{
	m_hiddenItemsIDs.Remove( id );
}

void CMaraudersMapCanvas::ActivateVisibilityFilter( Int32 id )
{
	m_activeFiltersVisibility.Set( id );
}

void CMaraudersMapCanvas::DeactivateVisibilityFilter( Int32 id )
{
	m_activeFiltersVisibility.Clear( id );
}

void CMaraudersMapCanvas::ActivateSelectingFilter( Int32 id )
{
	m_activeFiltersSelecting.Set( id );
}

void CMaraudersMapCanvas::DeactivateSelectingFilter( Int32 id )
{
	m_activeFiltersSelecting.Clear( id );
}

void CMaraudersMapCanvas::ShowItemSelectionContextMenu( wxPoint menuPos )
{
	wxMenu menu;

	for ( Uint32 i = 0; i < m_itemsMouseOver.Size(); ++i )
	{
		menu.Append( i, m_itemsMouseOver[i]->GetShortDescription().AsChar() );
		menu.Connect( i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMapCanvas::OnItemSelectionContextMenu ), NULL, this );
	}

	PopupMenu( &menu, menuPos );
}

void CMaraudersMapCanvas::ShowItemContextMenu( wxPoint menuPos )
{
	ASSERT( m_itemSelected );

	wxMenu menu;

	const TDynArray< String > *menuItemsNames = m_itemSelected->GetOptionsNames();

	if ( menuItemsNames == NULL ) return;

	for ( Int32 i = 0; i < m_itemSelected->GetOptionsSize(); ++i )
	{
		menu.Append( i, (*menuItemsNames)[i].AsChar() );
		menu.Connect( i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMapCanvas::OnItemContextMenu ), NULL, this );
	}

	PopupMenu( &menu, menuPos );
}

void CMaraudersMapCanvas::OnItemSelectionContextMenu( wxCommandEvent &event )
{
	unsigned int menuClickedPos = event.GetId();
	if ( menuClickedPos < m_itemsMouseOver.Size() )
	{
		m_itemSelected = m_itemsMouseOver[ menuClickedPos ];
	}
}

void CMaraudersMapCanvas::OnItemContextMenu( wxCommandEvent &event )
{
	if ( m_itemSelected == NULL ) return;

	Int32 menuClickedPos = event.GetId();
	if ( menuClickedPos >= 0 )
	{
		TDynArray< String > logOutput;
		m_itemSelected->ExecuteOption( menuClickedPos, logOutput );
		if ( !logOutput.Empty() )
		{
			AddLogInfo( logOutput );
		}
	}
}

void CMaraudersMapCanvas::SelectItem( CMaraudersMapItemBase *item )
{
	if ( item == NULL ) return;
	if ( !item->IsValid() ) return;

	m_itemSelected = item;
}

Bool CMaraudersMapCanvas::GotoActionPointItem( TActionPointID apID )
{
	for ( TDynArray< CMaraudersMapItemBase* >::const_iterator ci = m_items.Begin();
		  ci != m_items.End();
		  ++ci )
	{
		if ( (*ci)->GetMarItemType() == MMIT_ActionPoint )
		{
			CMaraudersMapItemActionPoint *mmiap = dynamic_cast< CMaraudersMapItemActionPoint* >( *ci );
			if ( mmiap->DoesMatchApID( apID ) )
			{
				GotoItem( *ci );
				return true;
			}
		}
	}

	return false;
}

void CMaraudersMapCanvas::AddLogInfo( const String& msg )
{
	SLogEntry logEntry;
	logEntry.m_msg = msg;
	m_log.PushBack( logEntry );
	m_logTimer = m_logTimerValue;
	Refresh();
}

void CMaraudersMapCanvas::AddLogInfo( const TDynArray<String> &msg )
{
	for ( TDynArray<String>::const_iterator msgI = msg.Begin();
		  msgI != msg.End();
		  ++msgI )
	{
		AddLogInfo( *msgI );
	}
}

void CMaraudersMapCanvas::ClearLog()
{
	m_log.Clear();
}

void CMaraudersMapCanvas::GotoItem( CMaraudersMapItemBase *item )
{
	if ( item == NULL ) return;
	if ( !item->IsValid() ) return;

	m_itemSelected = item;
	Center( item->GetWorldPosition() );
}

Bool CMaraudersMapCanvas::IsItemVisible( const CMaraudersMapItemBase *item ) const
{
	if ( item->IsVisible() && m_activeFiltersVisibility.Get( item->GetTypeID() ) &&	!m_hiddenItemsIDs.Exist( item->GetLayerID() ) )
	{
		return true;
	}

	return false;
}

Bool CMaraudersMapCanvas::IsItemSelectable( const CMaraudersMapItemBase *item ) const
{
	if ( item->IsVisible() &&
		 m_activeFiltersSelecting.Get( item->GetTypeID() ) &&
		 m_activeFiltersVisibility.Get( item->GetTypeID() ) &&
		!m_hiddenItemsIDs.Exist(item->GetLayerID()) )
	{
		return true;
	}

	return false;
}

void CMaraudersMapCanvas::AddItemToFastUpdate( CMaraudersMapItemBase *item )
{
	m_fastUpdateItems.PushBackUnique( item );
}

void CMaraudersMapCanvas::FastUpdateItemsTick()
{
	for ( Int32 i = m_fastUpdateItems.Size() - 1; i >= 0; --i )
	{
		if ( !m_fastUpdateItems[i]->IsValid() || !m_fastUpdateItems[i]->DoesRequireFastUpdate() )
		{
			m_fastUpdateItems.EraseFast( m_fastUpdateItems.Begin() + i );
			continue;
		}

		if ( m_fastUpdateItems[i]->Update( this ) )
		{
			GGame->Pause( TXT( "CMaraudersMapCanvas::FastUpdateItemsTick" ) );
			SelectItem( m_fastUpdateItems[i] );
		}
	}
}

void CMaraudersMapCanvas::ExecuteTool()
{
	if ( m_leftMouseOption == LMO_PLAYER_TELEPORT )
	{
		if ( GGame->GetPlayerEntity() )
		{
			Vector collisionPos;
			if ( CMaraudersMapCanvas::GetTerrainCollision( m_lastMousePosition, collisionPos ) )
			{
				GGame->GetPlayerEntity()->Teleport( collisionPos, GGame->GetPlayerEntity()->GetRotation() );
			}
		}
		SetCursor( *wxSTANDARD_CURSOR );

		m_leftMouseOption = LMO_NONE;
	}
	else if ( m_leftMouseOption == LMO_CAMERA_TELEPORT )
	{
		Vector gotoVec;
		if ( CMaraudersMapCanvas::GetTerrainCollision( m_lastMousePosition, gotoVec ) )
		{
			gotoVec.Z += 2.0f;
		}

		if ( GGame->IsActive() && GGame->IsFreeCameraEnabled() )
		{
			GGame->SetFreeCameraWorldPosition( gotoVec );
		}
		else if ( !GGame->IsActive() )
		{
			wxTheFrame->GetWorldEditPanel()->SetCameraPosition( gotoVec );
		}

		SetCursor( *wxSTANDARD_CURSOR );
		m_leftMouseOption = LMO_NONE;
	}
}

void CMaraudersMapCanvas::DrawLog()
{
	const Int32 maxLines = 15;
	const Float fontHeight = 16;
	const Float bottomMargin = 20;
	const Float leftMargin = 35;
	static wxColour textColor( 255, 255, 255 );
	const wxSize clientSize = GetClientSize();

	if ( m_logLastTime == 0 )
	{
		m_logLastTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	}
	Float diffTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds() - m_logLastTime;
	m_logLastTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	m_logTimer -= diffTime;
	if ( m_logTimer <= 0 )
	{
		m_logTimer = m_logTimerValue;
		m_log.Clear();
	}

	for( Int32 logEntryIdx = m_log.Size()-1, i = 0; logEntryIdx >=0 && i <= maxLines; --logEntryIdx, ++i )
	{
		SLogEntry &logEntry = m_log[ logEntryIdx ];
		const String string = logEntry.m_msg;

		Vector pos = ClientToWorld( wxPoint(leftMargin, clientSize.GetHeight() - bottomMargin - fontHeight*i ) );
		DrawTextCanvas( pos.X, pos.Y, GetGdiDrawFont(), string, textColor, CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Left );
	}
}

Bool CMaraudersMapCanvas::GetTerrainCollision( const Vector& pos, Vector& colisionPos )
{
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return false;
	}
	CPhysicsWorld* physicsWorld = nullptr;
	if ( !world->GetPhysicsWorld( physicsWorld ) )
	{
		return false;
	}
	const Vector pointA = pos + Vector( 0, 0, 100000 );
	const Vector pointB = pos + Vector( 0, 0, -100000 );
	CPhysicsEngine::CollisionMask mask = GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) );
	SPhysicsContactInfo ci;
	Bool ret = physicsWorld->RayCastWithSingleResult( pointA, pointB, mask, 0, ci ) == TRV_Hit;
	if ( ret )
	{
		colisionPos = ci.m_position;
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

void CMaraudersNavigationCanvas::AddSphere( const Vector& pos, Float radius, const Color& color )
{
	if ( m_canvas )
	{
		m_canvas->DrawCircleCanvas( pos, radius, wxColor( color.R, color.G, color.B ) );
	}
}

void CMaraudersNavigationCanvas::AddLine( const Vector& start, const Vector& end, const Color& color )
{
	if ( m_canvas )
	{
		m_canvas->DrawLineCanvas( start, end, wxColor( color.R, color.G, color.B ) );
	}
}
