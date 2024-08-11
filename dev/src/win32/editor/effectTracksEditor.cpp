/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "effectTracksEditor.h"
#include "entityPreviewPanel.h"
#include "../../common/engine/fxSystem.h"
#include "entityPreviewPanel.h"
#include "../../common/engine/playedAnimation.h"
#include "../../common/engine/animatedSkeleton.h"
#include "../../common/engine/curve.h"

BEGIN_EVENT_TABLE( CEdEffectTracksEditor, CEdCanvas )
	EVT_MOUSEWHEEL( CEdEffectTracksEditor::OnMouseWheel )
	EVT_LEFT_DCLICK( CEdEffectTracksEditor::OnMouseLeftDblClick )
	EVT_CHAR( CEdEffectTracksEditor::OnCharPressed )
END_EVENT_TABLE()

CEdEffectTracksEditor::CEdEffectTracksEditor( wxWindow* parent, CFXDefinition *effect, Int32 sidePanelWidth, CEdEffectCurveEditor *effectCurveEditor, CEdEffectEditor *effectEditor )
	: CEdCanvas( parent )
	, m_scaleTime( 1.0f, 1.0f, 1.0f )
	, m_fxDefinition( effect )
	, m_sidePanelWidth( sidePanelWidth )
	, m_action( MA_None )
	, m_moveTotal( 0 )
	, m_lastMousePosition( 0.0f, 0.0f, 0.0f )
	, m_clickedMousePosition( 0.0f, 0.0f, 0.0f )
	, m_buttonHeight( 30 )
	, m_buttonWidth( sidePanelWidth )
	, m_buttonExpandRectSize( 10 )
	, m_buttonMouseOver( NULL )
	, m_buttonIsMouseOverTreeExpander( false )
	, m_buttonMouseOverClicked( NULL )
	, m_trackItemMouseOver( NULL )
	, m_trackItemIsMouseOverSizer( false )
	, m_trackMouseOver( NULL )
	, m_trackMouseOverClicked( NULL )
	, m_trackItemMouseOverClicked( NULL )
	, m_effectCurveEditor( effectCurveEditor )
	, m_effectEditor( effectEditor )
	, m_trackGroupMouseOverClicked( NULL )
	, m_trackGroupMouseOver( NULL )
	, m_buttonsScroolPos( 0 )
	, m_gradientEditor( NULL )
	, m_animationLength( 0.0f )
{
	const Int32 downPanelHeight = 40;

	// set bars
	m_trackTimeBar = new CEdEffectTracksEditorTimeBar( 10, downPanelHeight, this );
	m_trackIntervalTimeBeginBar = new CEdEffectTracksEditorIntervalTimeBeginBar( 15, 15, downPanelHeight, this );
	m_trackIntervalTimeEndBar = new CEdEffectTracksEditorIntervalTimeEndBar( 15, 15, downPanelHeight, this );
	m_trackTimeEndBar = new CEdEffectTracksEditorTimeEndBar( 15, 15, downPanelHeight, this );
	m_trackBars.PushBack( m_trackTimeBar );
	m_trackBars.PushBack( m_trackIntervalTimeBeginBar );
	m_trackBars.PushBack( m_trackIntervalTimeEndBar );
	m_trackBars.PushBack( m_trackTimeEndBar );

	// load bars values
	m_trackTimeEndBar->SetPos( m_fxDefinition->GetEndTime() );
	m_trackIntervalTimeBeginBar->SetPos( m_fxDefinition->GetLoopStart() );
	m_trackIntervalTimeEndBar->SetPos( m_fxDefinition->GetLoopEnd() );

	// Tree view icons
	wxBitmap iconTreeExpand = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TREE_EXPAND") );
	wxBitmap iconTreeCollapse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TREE_COLLAPSE") );
	m_iconTreeExpand = ConvertToGDI( iconTreeExpand );
	m_iconTreeCollapse = ConvertToGDI( iconTreeCollapse );

	SRTTI::GetInstance().EnumClasses( ClassID<CFXTrackItem>(), m_trackItemClasses );
	m_trackItemClasses.Remove( ClassID<CFXTrackItem>() );

	// Initialize gradient editor
	{
		m_gradientEditor = new CEdGradientEditor( this, true );

		// Assign Curve Editor to Gradient Editor and Gradient Editor to Curve Editor
		m_effectCurveEditor->SetGradientEditor( m_gradientEditor );
		m_gradientEditor->SetCurveEditor( m_effectCurveEditor );
	}

	SetMinSize( wxSize( sidePanelWidth, 300 ) );
}

CEdEffectTracksEditor::~CEdEffectTracksEditor()
{
	m_fxDefinition->SetLength( m_trackTimeEndBar->GetPos() );
	m_fxDefinition->SetLoopStart( m_trackIntervalTimeBeginBar->GetPos() );
	m_fxDefinition->SetLoopEnd( m_trackIntervalTimeEndBar->GetPos() );

	//delete m_trackTimeBar;
	delete m_iconTreeExpand;
	delete m_iconTreeCollapse;
}

void CEdEffectTracksEditor::UpdateBars()
{
	m_trackTimeEndBar->SetPos( m_fxDefinition->GetEndTime() );
	m_trackIntervalTimeBeginBar->SetPos( m_fxDefinition->GetLoopStart() );
	m_trackIntervalTimeEndBar->SetPos( m_fxDefinition->GetLoopEnd() );
	Refresh( false );
	Update();
}

void CEdEffectTracksEditor::SetEffectTime( Float time )
{
	m_trackTimeBar->SetPos( time );
	Refresh( false );
	Update();
}

void CEdEffectTracksEditor::SetAnimationLength( Float time )
{
	m_animationLength = time;
	Refresh( false );
	Update();
}

void CEdEffectTracksEditor::UpdateLayout()
{
	// clear layout map and calculate every layout map again

	int lineNum = 0;
	int scroolPos = m_buttonsScroolPos;
	LayoutInfo info;
	m_layout.Clear();

	// Create layouts for 'm_fxDefinition'

	// TRACK GROUPS
	TDynArray< CFXTrackGroup* > &effectTrackGroupArray = m_fxDefinition->GetEffectTrackGroup();
	for ( TDynArray< CFXTrackGroup* >::iterator effectTrackGroup = effectTrackGroupArray.Begin(); effectTrackGroup != effectTrackGroupArray.End(); ++effectTrackGroup )
	{
		if ( scroolPos == 0 )
		{
			info.m_windowRect = wxRect( 0, m_buttonHeight * lineNum++ - lineNum, m_buttonWidth, m_buttonHeight );
			info.m_expandWindowRect = wxRect( m_buttonExpandRectSize/2, info.m_windowRect.y + info.m_windowRect.GetHeight()/2 - m_buttonExpandRectSize/2,
				m_buttonExpandRectSize, m_buttonExpandRectSize );
			m_layout.Insert( *effectTrackGroup, info );
		}
		else
		{
			--scroolPos;
		}

		// Do not expand collapsed groups
		if ( !(*effectTrackGroup)->IsExpanded() )
		{
			continue;
		}

		// TRACKS
		TDynArray< CFXTrack* > &effectTrackArray = (*effectTrackGroup)->GetTracks();
		for ( TDynArray< CFXTrack* >::iterator effectTrack = effectTrackArray.Begin(); effectTrack != effectTrackArray.End(); ++effectTrack )
		{
			if ( scroolPos > 0 )
			{
				--scroolPos;
				continue;
			}

			info.m_windowRect = wxRect( 0, m_buttonHeight * lineNum++ - lineNum, m_buttonWidth, m_buttonHeight );
			m_layout.Insert( *effectTrack, info );

			// TRACK ITEMS
			TDynArray< CFXTrackItem* > &effectTrackItemArray = (*effectTrack)->GetTrackItems();
			for ( TDynArray< CFXTrackItem* >::iterator effectTrackItem = effectTrackItemArray.Begin(); effectTrackItem != effectTrackItemArray.End(); ++effectTrackItem )
			{
				const int border = 3;
				wxPoint clientX = TimeToClient( Vector( (*effectTrackItem)->GetTimeBegin(), 0, 0 ) );
				wxPoint clientEndX = TimeToClient( Vector( (*effectTrackItem)->GetTimeEnd(), 0, 0 ) );
				int clientWidth = clientEndX.x - clientX.x;

				// Main window rect
				info.m_windowRect = wxRect( clientX.x, m_buttonHeight * (lineNum - 1) - (lineNum - 1) + border, clientWidth, m_buttonHeight-(border*2) );

				// Expand window rect
				const int x1 = info.m_windowRect.x + info.m_windowRect.GetWidth() - 1;
				const int y1 = info.m_windowRect.y + info.m_windowRect.GetHeight()/2;
				const int x2 = x1 - info.m_windowRect.GetHeight()/2;
				const int y2 = info.m_windowRect.y + info.m_windowRect.GetHeight() - 1;
				info.m_expandWindowRect = wxRect( x2, y1, x1-x2, y2-y1 );

				m_layout.Insert( *effectTrackItem, info );
			}
		}
	}

	// Update track bars
	for ( TDynArray< CEdEffectTracksEditorBar * >::iterator trackBar = m_trackBars.Begin(); trackBar != m_trackBars.End(); ++trackBar )
	{
		(*trackBar)->UpdateLayout();
	}
}

Vector CEdEffectTracksEditor::CanvasToTime( wxPoint point ) const
{
	return Vector( point.x * m_scaleTime.X, -point.y * m_scaleTime.Y, 0.0f );
}

wxPoint CEdEffectTracksEditor::TimeToCanvas( const Vector& point ) const
{
	return wxPoint( point.X / m_scaleTime.X, -point.Y / m_scaleTime.Y );
}

Vector CEdEffectTracksEditor::ClientToTime( wxPoint point ) const
{
	return CanvasToTime( ClientToCanvas( point ) );
}

wxPoint CEdEffectTracksEditor::TimeToClient( const Vector& point ) const
{
	return CanvasToClient( TimeToCanvas( point ) );
}

void CEdEffectTracksEditor::OnMouseLeftDblClick( wxMouseEvent &event )
{
	if ( !m_trackItemMouseOverClicked ) return;

	if ( m_trackItemMouseOverClicked->SupportsColor() )
	{
		OnEditTrackItemColor( wxCommandEvent() );
	}
	else if ( m_trackItemMouseOverClicked->SupportsCurve() )
	{
		OnAddTrackItemCurves( wxCommandEvent() );
	}
}

void CEdEffectTracksEditor::OnMouseWheel( wxMouseEvent& event )
{
	wxPoint point = event.GetPosition();

	if ( IsMouseOverCanvasPanel( point ) )
	{
		// Find zooming pivot
		Vector zoomingPivot = ClientToTime( point );

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
		Float wheelScale = event.ShiftDown() ? 0.25f : 0.05f;

		// Calculate new zoomed region
		Vector newZoomedMin = zoomingPivot + (zoomedMin - zoomingPivot) * (1.0f - delta * wheelScale);
		Vector newZoomedMax = zoomingPivot + (zoomedMax - zoomingPivot) * (1.0f - delta * wheelScale);
		SetZoomedRegion( newZoomedMin, newZoomedMax );
	}
	else
	{
		ScrollButtons( event.GetWheelRotation() / event.GetWheelDelta() );
		Repaint();
	}
}

void CEdEffectTracksEditor::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	wxPoint point = event.GetPosition();

	if ( IsMouseOverCanvasPanel( point ) )
	{
		const Int32 autoScrollBorderSize = 10;

		// We do not repaint if it's not necessary, because preview panel stops when we repaint
		if ( m_action == MA_Zooming )
		{
			Float wheelScale = 0.0001f;
			Vector curveDelta = ClientToTime( delta ) * wheelScale;
			m_scaleTime.X = Clamp< Float >( m_scaleTime.X + delta.x * wheelScale, 0.0001f, 100.0f );
			m_scaleTime.Y = Clamp< Float >( m_scaleTime.Y + delta.y * wheelScale, 0.0001f, 100.0f );
			Refresh();
			Update();

			// We don't want auto scroll, and lastMousePosition updated
			return;
		}
		else if ( m_action == MA_None )
		{
			// Check bars
			for ( TDynArray< CEdEffectTracksEditorBar * >::iterator trackBar = m_trackBars.Begin();
				  trackBar != m_trackBars.End();
				  ++trackBar )
			{
				Bool isOverTrackBarOld = (*trackBar)->IsHightlighted();
				(*trackBar)->Hightlight( IsOverTrackBar( point, *trackBar ) );
				if ( (*trackBar)->IsHightlighted() != isOverTrackBarOld ) Repaint();
			}

			// Check track items and tracks
			LayoutInfo info;
			CFXBase *trackItemMouseOverOld = m_trackItemMouseOver;
			Bool trackItemFound = false;
			Bool trackItemOverSizerFound = false;
			Bool trackFound = false;
			Bool trackGroupFound = false;
			TDynArray< CFXTrackGroup* > &effectTrackGroupArray = m_fxDefinition->GetEffectTrackGroup();
			for ( TDynArray< CFXTrackGroup* >::iterator effectTrackGroup = effectTrackGroupArray.Begin();
				  effectTrackGroup != effectTrackGroupArray.End();
				  ++effectTrackGroup )
			{
				if ( trackItemFound ) break;

				LayoutInfo* pInfo = m_layout.FindPtr( *effectTrackGroup );
				if ( !pInfo )
				{
					continue;
				}

				info = *pInfo;

				// Check track
				if ( point.y > info.m_windowRect.y && point.y < info.m_windowRect.y + info.m_windowRect.height && point.y < GetClientSize().GetHeight() - GetDownPanelHeight() )
				{
					m_trackGroupMouseOver = *effectTrackGroup;
					trackGroupFound = true;
				}

				if ( !(*effectTrackGroup)->IsExpanded() ) continue;

				TDynArray< CFXTrack* > &effectTrackArray = (*effectTrackGroup)->GetTracks();
				for ( TDynArray< CFXTrack* >::iterator effectTrack = effectTrackArray.Begin();
					  effectTrack != effectTrackArray.End();
					  ++effectTrack )
				{
					if ( trackItemFound ) break;

					LayoutInfo* pInfo = m_layout.FindPtr( *effectTrack );
					if ( !pInfo )
					{
						continue;
					}

					info = *pInfo;

					// Check track
					if ( point.y > info.m_windowRect.y && point.y < info.m_windowRect.y + info.m_windowRect.height && point.y < GetClientSize().GetHeight() - GetDownPanelHeight() )
					{
						m_trackMouseOver = *effectTrack;
						trackFound = true;
					}

					TDynArray< CFXTrackItem* > &effectTrackItemArray = (*effectTrack)->GetTrackItems();
					for ( TDynArray< CFXTrackItem* >::iterator effectTrackItem = effectTrackItemArray.Begin();
						  effectTrackItem != effectTrackItemArray.End();
						  ++effectTrackItem )
					{
						if ( trackItemFound ) break;

						LayoutInfo* pInfo = m_layout.FindPtr( *effectTrackItem );
						if ( !pInfo )
						{
							continue;
						}

						info = *pInfo;

						if ( (*effectTrackItem)->IsTick() )
						{
							info.m_windowRect.width = info.m_windowRect.GetHeight() / 2;
							info.m_windowRect.x -= info.m_windowRect.GetHeight() / 4;
						}
						else
						{
							// Deal with zero width
							const int widthTreshold = 2;
							if ( info.m_windowRect.GetWidth() < widthTreshold )
							{
								info.m_windowRect.SetWidth( widthTreshold );
							}
						}

						if ( info.m_windowRect.Contains( point ) )
						{
							m_trackItemMouseOver = *effectTrackItem;
							trackItemFound = true;

							if ( !(*effectTrackItem)->IsTick() && info.m_expandWindowRect.Contains( point ) )
							{
								m_trackItemIsMouseOverSizer = true;
								trackItemOverSizerFound = true;
								SetCursor( wxCURSOR_SIZEWE );
							}
							else
							{
								SetCursor( wxCURSOR_HAND );
							}
						}
					}
				}
			}
			if ( !trackItemFound )
			{
				m_trackItemMouseOver = NULL;
				SetCursor( wxCURSOR_ARROW );
			}
			if ( !trackItemOverSizerFound ) m_trackItemIsMouseOverSizer = false;
			if ( !trackFound ) m_trackMouseOver = NULL;
			if ( !trackGroupFound ) m_trackGroupMouseOver = NULL;
			if ( m_trackItemMouseOver != trackItemMouseOverOld ) Repaint();
		}
		else if ( m_action == MA_BackgroundScroll )
		{
			ScrollBackgroundOffset( delta );
			Repaint( true );
		}
		else if ( m_action == MA_MovingTrackTimeBar )
		{
			// Preserve constraints and set new pos
			const Float newPosX = ClientToTime( point ).X;
			UpdateAnimationDrag( newPosX );
		}
		else if ( m_action == MA_MovingTrackIntervalTimeBeginBar )
		{
			// Preserve constraints and set new pos
			const Float newPosX = ClientToTime( point ).X;
			m_fxDefinition->SetLoopStart( newPosX );
			m_trackIntervalTimeBeginBar->SetPos( m_fxDefinition->GetLoopStart() );
			Repaint();
			m_effectEditor->RefreshProperties();
		}
		else if ( m_action == MA_MovingTrackIntervalTimeEndBar )
		{
			// Preserve constraints and set new pos
			const Float newPosX = ClientToTime( point ).X;
			m_fxDefinition->SetLoopEnd( newPosX );
			m_trackIntervalTimeEndBar->SetPos( m_fxDefinition->GetLoopEnd() );
			Repaint();
			m_effectEditor->RefreshProperties();
		}
		else if ( m_action == MA_MovingTrackTimeEndBar )
		{
			// Preserve constraints and set new pos
			const Float newPosX = ClientToTime( point ).X;
			m_fxDefinition->SetLength( newPosX );
			m_trackTimeEndBar->SetPos( m_fxDefinition->GetEndTime() );
			Repaint();
			m_effectEditor->RefreshProperties();
		}
		else if ( m_action == MA_MovingTrackItem )
		{
			Vector deltaInCurveSpace = ClientToTime( point ) - m_lastMousePosition;
			if ( m_selectedTrackItems.Size() > 0 ) 
			{
				CFXTrackItem* item = m_selectedTrackItems[0];

				// Update first item
				Float newPosition = item->GetTimeBegin() + deltaInCurveSpace.X;
				if ( event.ShiftDown() )
				{
					newPosition = SnapToBar( newPosition );
				}
				float base = item->GetTimeBegin();
				item->SetTimeBeginWithConstDuration( newPosition );

				Vector curveScale( item->GetTimeBegin(), item->GetTimeDuration(), 0.0f );
				m_effectCurveEditor->UpdateCurveParam( &item->GetCurve()->GetCurveData(), curveScale );

				// Update the rest of the items to be relative to the first
				for ( Uint32 i=1; i<m_selectedTrackItems.Size(); ++i )
				{
					item = m_selectedTrackItems[i];

					item->SetTimeBeginWithConstDuration( item->GetTimeBegin() - base + m_selectedTrackItems[0]->GetTimeBegin() );

					Vector curveScale( item->GetTimeBegin(), item->GetTimeDuration(), 0.0f );
					m_effectCurveEditor->UpdateCurveParam( &item->GetCurve()->GetCurveData(), curveScale );
				}

				Repaint();
			}
		}
		else if ( m_action == MA_ResizingTrackItem )
		{
			ASSERT( m_trackItemMouseOver );

			// Preserve constraints and set new pos
			Float newPosX = ClientToTime( point ).X;
			if ( newPosX > m_trackItemMouseOver->GetTimeBegin() )
			{
				if ( event.ShiftDown() )
				{
					newPosX = SnapToBar( newPosX );
				}
				m_trackItemMouseOver->SetTimeEnd( newPosX );
			}
			else
			{
				m_trackItemMouseOver->SetTimeEnd( m_trackItemMouseOver->GetTimeBegin() + 0.1f );
			}

			Vector curveScale( m_trackItemMouseOver->GetTimeBegin(), m_trackItemMouseOver->GetTimeDuration(), 0.0f );
			m_effectCurveEditor->UpdateCurveParam( &m_trackItemMouseOver->GetCurve()->GetCurveData(), curveScale );

			Repaint();
		}

		// Common part, auto scroll, calc total mouse move, reset auto scroll and calc last mouse position
		{
			// Accumulate move
			m_moveTotal += Abs( delta.x ) + Abs( delta.y );

			// Remember mouse position
			m_lastMousePosition = ClientToTime( point );

			// Auto scroll setup
			m_autoScroll = wxPoint(0,0);
			/*
			if ( m_action == MA_MovingControlPoints || m_action == MA_SelectingWindows || m_action == MA_MovingSnappingLine )
			{
				Int32 width, height;
				Int32 sidePanelSize = GetSidePanelWidth();
				GetClientSize( &width, &height );
				m_autoScroll.x += (point.x > sidePanelSize && point.x < sidePanelSize + autoScrollBorderSize) ? 5 : 0;
				m_autoScroll.x -= (point.x > ( width - autoScrollBorderSize )) ? 5 : 0;
				m_autoScroll.y += (point.y < autoScrollBorderSize) ? 5 : 0;
				m_autoScroll.y -= (point.y > ( height - autoScrollBorderSize )) ? 5 : 0;

				Repaint();
			}
			*/
		}

		if ( m_buttonMouseOver )
		{
			m_buttonMouseOver = NULL;
			m_buttonMouseOverTrackGroup = NULL;
			m_buttonIsMouseOverTreeExpander = false;
			Repaint();
		}
	}
	else // Mouse is over side panel
	{
		// Check buttons
		LayoutInfo info;
		CFXBase *buttonMouseOverOld = m_buttonMouseOver;
		Bool buttonFound = false;
		Bool buttonOverTreeExpanderFound = false;
		TDynArray< CFXTrackGroup* > &effectTrackGroupArray = m_fxDefinition->GetEffectTrackGroup();
		for ( TDynArray< CFXTrackGroup* >::iterator effectTrackGroup = effectTrackGroupArray.Begin();
			effectTrackGroup != effectTrackGroupArray.End();
			++effectTrackGroup )
		{
			if ( buttonFound ) break;

			LayoutInfo* pInfo = m_layout.FindPtr( *effectTrackGroup );
			if ( !pInfo )
			{
				continue;
			}

			info = *pInfo;

			if ( info.m_windowRect.Contains( point ) )
			{
				m_buttonMouseOver = *effectTrackGroup;
				buttonFound = true;

				if ( info.m_expandWindowRect.Contains( point ) )
				{
					m_buttonMouseOverTrackGroup = *effectTrackGroup;
					m_buttonIsMouseOverTreeExpander = true;
					buttonOverTreeExpanderFound = true;
				}
			}

			if ( !(*effectTrackGroup)->IsExpanded() ) continue;

			TDynArray< CFXTrack* > &effectTrackArray = (*effectTrackGroup)->GetTracks();
			for ( TDynArray< CFXTrack* >::iterator effectTrack = effectTrackArray.Begin();
				effectTrack != effectTrackArray.End();
				++effectTrack )
			{
				if ( buttonFound ) break;
				LayoutInfo* pInfo = m_layout.FindPtr( *effectTrack );
				if ( !pInfo )
				{
					continue;
				}
				info = *pInfo;
				if ( info.m_windowRect.Contains( point ) )
				{
					m_buttonMouseOver = *effectTrack;
					buttonFound = true;
				}
			}
		}
		if ( !buttonFound ) m_buttonMouseOver = NULL;
		if ( !buttonOverTreeExpanderFound ) m_buttonIsMouseOverTreeExpander = false;
		if ( m_buttonMouseOver != buttonMouseOverOld ) Repaint();
	}
}

void CEdEffectTracksEditor::MouseClick( wxMouseEvent& event )
{
	// Pass to base class
	CEdCanvas::MouseClick( event );

	if ( event.LeftDown() || event.RightDown() )
	{
		m_clickedMousePosition = ClientToTime( event.GetPosition() );
	}

	// Check if mouse is over side panel or curve canvas panel
	if ( IsMouseOverCanvasPanel( event.GetPosition() ) )
	{
		MouseClickCanvasPanel( event );
	}
	else
	{
		MouseClickSidePanel( event );
	}

	// Always repaint canvas when clicked
	Repaint();
}

void CEdEffectTracksEditor::PaintCanvas( Int32 width, Int32 height )
{
	// Colors
	wxColour back = GetCanvasColor();
	static wxColour text( 255, 255, 255 );
	static wxColour texth( 255, 255, 0 );
	static wxColour highlight( 0, 0, 80 );

	static wxColour colorSelecting( 0, 0, 255 );
	static wxColour colorZooming( 0, 255, 0 );

	 UpdateLayout();

	// Paint background
	Clear( back );

	// Draw elements
	DrawGrid( width, height, true, true );
	DrawTrackItems();
	DrawDownPanel();	//time lines are drawn here 
	DrawSidePanel();  
}

void CEdEffectTracksEditor::DrawSidePanel()
{
	wxColour backgroundColor = GetCanvasColor();
	static wxColour lineColor( 255, 255, 255 );
	static wxColour buttonLineColor( 215, 215, 215 );

	// Draw side panel background
	wxRect sidePanelRect( 0, 0, GetSidePanelWidth(), GetSidePanelHeight() );
	FillRect( ClientToCanvas( sidePanelRect ), backgroundColor );

	// Draw vertical line
	wxSize clientSize = GetClientSize();
	DrawLine( ClientToTime( wxPoint(GetSidePanelWidth()-1, 0) ), ClientToTime( wxPoint(GetSidePanelWidth()-1, clientSize.GetHeight()) ), lineColor, 1.5f );

	// Draw side panel buttons
	{
		int lineNum = 0;
		int scroolPos = m_buttonsScroolPos;
		LayoutInfo info;

		// Create layouts for 'm_fxDefinition'
		TDynArray< CFXTrackGroup* > &effectTrackGroupArray = m_fxDefinition->GetEffectTrackGroup();
		for ( TDynArray< CFXTrackGroup* >::iterator effectTrackGroup = effectTrackGroupArray.Begin();
			effectTrackGroup != effectTrackGroupArray.End();
			++effectTrackGroup )
		{
			if ( scroolPos == 0 )
			{
				LayoutInfo* pInfo = m_layout.FindPtr( *effectTrackGroup );
				if ( !pInfo )
				{
					continue;
				}

				info = *pInfo;

				// keep boundaries
				if ( info.m_windowRect.GetY() + info.m_windowRect.GetHeight() > GetClientSize().GetHeight() - GetDownPanelHeight() ) break;

				// draw button boundaries
				DrawRect( ClientToCanvas( info.m_windowRect ), buttonLineColor );

				// draw button background
				{
					wxColour btnBackgroundColor( (*effectTrackGroup)->GetColor().R, (*effectTrackGroup)->GetColor().G, (*effectTrackGroup)->GetColor().B,
						m_buttonMouseOver == *effectTrackGroup ? 255 : (*effectTrackGroup)->GetColor().A );
					wxColour btnBackgroundSideColor( btnBackgroundColor.Red() / 2, btnBackgroundColor.Green() / 2, btnBackgroundColor.Blue() / 2, btnBackgroundColor.Alpha() );

					// remove vertical right button line
					wxColour backColor = GetCanvasColor();
					//DrawLine( ClientToTime( wxPoint(GetSidePanelWidth(), info.m_windowRect.GetY()) ), ClientToTime( wxPoint(GetSidePanelWidth(), info.m_windowRect.GetY() + info.m_windowRect.GetHeight()) ), backColor, 1.5f );
					DrawLine( ClientToTime( wxPoint(info.m_windowRect.GetX() + info.m_windowRect.GetWidth()-1, info.m_windowRect.GetY()) ), ClientToTime( wxPoint(info.m_windowRect.GetX() + info.m_windowRect.GetWidth()-1, info.m_windowRect.GetY() + info.m_windowRect.GetHeight()) ), backColor, 1.0f );

					wxRect btnBackgroundRect = info.m_windowRect;
					btnBackgroundRect.SetWidth( GetClientSize().GetWidth() );
					FillGradientVerticalRect( ClientToCanvas( btnBackgroundRect ), btnBackgroundSideColor, btnBackgroundSideColor, btnBackgroundColor );
				}


				//DrawRect( ClientToCanvas( info.m_expandWindowRect ), buttonLineColor );

				// Draw button text
				//if ( m_buttonMouseOver != *effectTrackGroup ) SetClip( ClientToCanvas( info.m_windowRect ) );
				CEdCanvas::DrawText( ClientToCanvas( wxPoint( info.m_expandWindowRect.x + m_buttonExpandRectSize + 2, info.m_windowRect.y + m_buttonHeight / 2 ) ), 
					GetGdiDrawFont(), (*effectTrackGroup)->GetName(),
					buttonLineColor, CEdCanvas::CVA_Center, CEdCanvas::CHA_Left );
				//ResetClip();

				if ( (*effectTrackGroup)->IsExpanded() )
				{
					// Draw minus
					DrawImage( m_iconTreeCollapse, ClientToCanvas( info.m_expandWindowRect ) );
				}
				else
				{
					// Draw plus
					DrawImage( m_iconTreeExpand, ClientToCanvas( info.m_expandWindowRect ) );

					continue;
				}
			}
			else
			{
				--scroolPos;
				if ( !(*effectTrackGroup)->IsExpanded() ) continue;
			}

			TDynArray< CFXTrack* > &effectTrackArray = (*effectTrackGroup)->GetTracks();
			for ( TDynArray< CFXTrack* >::iterator effectTrack = effectTrackArray.Begin();
				effectTrack != effectTrackArray.End();
				++effectTrack )
			{
				if ( scroolPos > 0 )
				{
					--scroolPos;
					continue;
				}

				LayoutInfo* pInfo = m_layout.FindPtr( *effectTrack );
				if ( !pInfo )
				{
					continue;
				}

				info = *pInfo;

				// keep boundaries
				if ( info.m_windowRect.GetY() + info.m_windowRect.GetHeight() > GetClientSize().GetHeight() - GetDownPanelHeight() ) break;

				// draw button boundaries
				DrawRect( ClientToCanvas( info.m_windowRect ), lineColor );

				// draw button background
				//wxColour btnBackgroundGroupColor( btnBackgroundColor.Red() / 0.8, btnBackgroundColor.Green() / 0.8, btnBackgroundColor.Blue() / 0.8, btnBackgroundColor.Alpha() );
				wxColour btnBackgroundGroupColor( (*effectTrackGroup)->GetColor().R * 0.8, (*effectTrackGroup)->GetColor().G * 0.8, (*effectTrackGroup)->GetColor().B * 0.8,
					m_buttonMouseOver == *effectTrack ? 255 : (*effectTrackGroup)->GetColor().A );
				if ( m_buttonMouseOver != *effectTrack )
				{
					FillRect( ClientToCanvas( info.m_windowRect ), btnBackgroundGroupColor );
				}
				else
				{
					// draw button extended background
					wxPoint testExtents = TextExtents( GetGdiDrawFont(), (*effectTrack)->GetName() );
					wxRect extendedButtonBackgroundRect = info.m_windowRect;
					int protrusion = testExtents.x - extendedButtonBackgroundRect.width;
					const int margin = 10;
					if ( protrusion > 0 )
					{
						// remove vertical right button line
						wxColour backColor = GetCanvasColor();
						//DrawLine( ClientToTime( wxPoint(GetSidePanelWidth(), info.m_windowRect.GetY()) ), ClientToTime( wxPoint(GetSidePanelWidth(), info.m_windowRect.GetY() + info.m_windowRect.GetHeight()) ), backColor, 1.5f );
						DrawLine( ClientToTime( wxPoint(info.m_windowRect.GetX() + info.m_windowRect.GetWidth()-1, info.m_windowRect.GetY()) ), ClientToTime( wxPoint(info.m_windowRect.GetX() + info.m_windowRect.GetWidth()-1, info.m_windowRect.GetY() + info.m_windowRect.GetHeight()) ), backColor, 1.0f );

						// extend button background
						extendedButtonBackgroundRect.width += protrusion + margin;
					}
					FillRect( ClientToCanvas( extendedButtonBackgroundRect ), btnBackgroundGroupColor );
				}

				

				// Draw button text
				{
					if ( m_buttonMouseOver != *effectTrack ) SetClip( ClientToCanvas( info.m_windowRect ) );
					CEdCanvas::DrawText( ClientToCanvas( wxPoint( info.m_windowRect.x + 2, info.m_windowRect.y + m_buttonHeight / 2 ) ), 
						GetGdiDrawFont(), (*effectTrack)->GetName(),
						buttonLineColor, CEdCanvas::CVA_Center, CEdCanvas::CHA_Left );
					ResetClip();
				}

				//TDynArray< CEffectTrackItem* > &effectTrackItemArray = (*effectTrack)->GetEffectTrackItem();
				//for ( TDynArray< CEffectTrackItem* >::iterator effectTrackItem = effectTrackItemArray.Begin();
				//	effectTrackItem != effectTrackItemArray.End();
				//	++effectTrackItem )
				//{
				//	// TODO
				//	//m_layout.Insert( *effectTrackItem, LayoutInfo() );
				//}
			}
		}
	}
}

void CEdEffectTracksEditor::DrawGrid( Int32 width, Int32 height, Bool drawLines, Bool drawText )
{
	static wxColour text( 235, 235, 235 );
	static wxColour lines1( 100, 100, 100 );
	static wxColour lines2( 140, 140, 140 );
	static wxColour lines3( 180, 180, 180 );

	Vector gridSpacing;
	Vector zoomedRegion = GetZoomedRegion();
	wxSize clientSize = GetClientSize();

	// Safety check
	if( clientSize.GetHeight() < 80 || clientSize.GetWidth() < 80 )
	{
		return;
	}

	gridSpacing.X = SnapFloat( (zoomedRegion.Z - zoomedRegion.X) / ( (clientSize.GetWidth()+1) / 80 ) );
	gridSpacing.Y = SnapFloat( (zoomedRegion.W - zoomedRegion.Y) / ( (clientSize.GetHeight()+1) / 40 ) );

	// Draw background
	{
		wxColour backgroundColor( 90, 90, 90 ); // = GetCanvasColor();

		// left side (to zero)
		wxPoint lsLeftUpCorner = ClientToCanvas( wxPoint( GetSidePanelWidth(), 0 ) );
		int lsWidth = TimeToClient( Vector(0, 0, 0) ).x - GetSidePanelWidth();
		FillRect( lsLeftUpCorner.x, lsLeftUpCorner.y, lsWidth, GetClientSize().GetHeight() - GetDownPanelHeight(), backgroundColor );

		// right side
		int rsStartX = TimeToClient( Vector(m_trackTimeEndBar->GetPos(), 0, 0) ).x;
		wxPoint rsLeftUpCorner = ClientToCanvas( wxPoint( rsStartX, 0 ) );
		int rsWidth = GetClientSize().GetWidth() - rsStartX;
		FillRect( rsLeftUpCorner.x, rsLeftUpCorner.y, rsWidth, GetClientSize().GetHeight() - GetDownPanelHeight(), backgroundColor );
	}

	// Vertical
	for( Int32 i = (Int32)( zoomedRegion.X / gridSpacing.X ); i <= (Int32)( zoomedRegion.Z / gridSpacing.X ); ++i )
	{
		if ( drawLines )
		{
			wxColour color = ( i == 0 ) ? lines3 : ( ( i % 8 ) ? lines1 : lines2 );
			DrawLine( i * gridSpacing.X, zoomedRegion.Y, i * gridSpacing.X, zoomedRegion.W, color );
		}	
		if ( drawText )
		{
			String string = String::Printf( TXT("%.2f"), i * gridSpacing.X );
			DrawText( i * gridSpacing.X, zoomedRegion.Y, GetGdiDrawFont(), string, text, CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Center );
		}
	}

	if ( m_animationLength > 0.0f )
	{
		wxColour lineColor( 150, 150, 0 );
		DrawLine( m_animationLength, zoomedRegion.Y, m_animationLength, zoomedRegion.W, lineColor );
	}
}

Vector CEdEffectTracksEditor::GetZoomedRegion() const
{
	wxSize clientSize = GetClientSize();
	Vector corner1 = ClientToTime( wxPoint( GetSidePanelWidth(), 0 ) );
	Vector corner2 = ClientToTime( wxPoint( clientSize.GetWidth(), clientSize.GetHeight() - GetDownPanelHeight() ) );
	return Vector( corner1.X, corner2.Y, corner2.X, corner1.Y );
}

Float CEdEffectTracksEditor::SnapFloat( const Float& floatToSnap ) const
{
	ASSERT( floatToSnap > 0.0f, TXT("A negative value was passed to SnapFloat - that is so wrong it hurts!") );

	// Lets avoid hanging forever, shall we?
	if ( floatToSnap > 0.0f )
	{
		Float snappedValue = 1024.0f;
		while( floatToSnap < snappedValue )
		{
			snappedValue /= 2.0f;
		}
		return snappedValue;
	}
	else
	{
		return 0.0f;
	}
}

void CEdEffectTracksEditor::DrawText( const Vector& offset, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign )
{
	wxPoint v1 = TimeToCanvas( offset );
	CEdCanvas::DrawText( v1, font, text, color, vAlign, hAlign );
}

void CEdEffectTracksEditor::DrawText( const Float x, const Float y, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign )
{
	wxPoint v1 = TimeToCanvas( Vector( x, y, 0.0f ) );
	CEdCanvas::DrawText( v1, font, text, color, vAlign, hAlign );
}

void CEdEffectTracksEditor::DrawLine( Float x1, Float y1, Float x2, Float y2, const wxColour& color, Float width/*=1.0f*/ )
{
	wxPoint v1 = TimeToCanvas( Vector( x1, y1, 0.0f ) );
	wxPoint v2 = TimeToCanvas( Vector( x2, y2, 0.0f ) );
	CEdCanvas::DrawLine( v1, v2, color, width );
}

void CEdEffectTracksEditor::DrawLine( const Vector& start, const Vector& end, const wxColour& color, Float width/*=1.0f*/ )
{
	wxPoint v1 = TimeToCanvas( start );
	wxPoint v2 = TimeToCanvas( end );
	CEdCanvas::DrawLine( v1, v2, color, width );
}

Bool CEdEffectTracksEditor::IsMouseOverCanvasPanel( const wxPoint& mousePosition ) const
{
	// Calculate size of curveInfos side panel
	Int32 panelWidth  = GetSidePanelWidth();
	Int32 panelHeight = GetSidePanelHeight();
	wxRect sidePanelRect( 0, 0, panelWidth, panelHeight );
	return sidePanelRect.Contains( mousePosition ) ? false : true;
}

Int32 CEdEffectTracksEditor::GetSidePanelWidth() const
{
	// Width of side panel is the same as curveInfo width
	//return CurveInfo::GetClientRect( wxPoint(0,0) ).GetWidth();
	return m_sidePanelWidth;
}

Int32 CEdEffectTracksEditor::GetSidePanelHeight() const
{
	// Height of side panel is full canvas height
	return GetClientSize().GetHeight();
}

void CEdEffectTracksEditor::ScrollBackgroundOffset( wxPoint delta )
{
	wxPoint point = GetOffset();

	// Scroll background offset
	point.x += delta.x;			
	// point.y += delta.y; // don't scrool vertically

	// Scroll canvas
	SetOffset( point );

	SynchronizeWithOther( point, m_scaleTime );
}

void CEdEffectTracksEditor::MouseClickSidePanel( wxMouseEvent& event )
{
	wxPoint point = event.GetPosition();

	ASSERT( IsMouseOverCanvasPanel( point ) == false );

	// Check side panel buttons click
	if ( m_action == MA_None && event.LeftDown() && m_buttonMouseOver )
	{
		if ( m_buttonIsMouseOverTreeExpander )
		{
			if ( m_buttonMouseOverTrackGroup->IsExpanded() )
			{
				m_buttonMouseOverTrackGroup->Collapse();
			}
			else
			{
				m_buttonMouseOverTrackGroup->Expand();
			}

			Repaint();
		}
		else
		{
			//edit parameters
			m_effectEditor->SetObjectToEdit( m_buttonMouseOver );
		}
	}
	else if ( m_action == MA_None && event.RightDown() && m_buttonMouseOver )
	{
		m_buttonMouseOverClicked = m_buttonMouseOver;

		// show context menu for button
		wxMenu menu;

		menu.Append( 0, TXT("Append track") );
		menu.Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnButtonAppendTrack ), NULL, this );

		menu.AppendSeparator();

		menu.Append( 1, TXT("Insert track group") );
		menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnButtonInsertTrackGroup ), NULL, this );
		
		menu.Append( 2, TXT("Append track group") );
		menu.Connect( 2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnButtonAppendTrackGroup ), NULL, this );

		menu.AppendSeparator();

		menu.Append( 3, TXT("Rename item") );
		menu.Connect( 3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnButtonRename ), NULL, this );

		menu.Append( 4, TXT("Remove item") );
		menu.Connect( 4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnButtonRemove ), NULL, this );

		PopupMenu( &menu );
	}
	else if ( m_action == MA_None && event.RightDown() )
	{
		m_buttonMouseOverClicked = NULL;

		// show default context menu

		wxMenu menu;

		menu.Append( 0, TXT("Insert track group") );
		menu.Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnButtonInsertTrackGroup ), NULL, this );

		menu.Append( 1, TXT("Append track group") );
		menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnButtonAppendTrackGroup ), NULL, this );

		PopupMenu( &menu );
	}

	//(*effectTrackGroup)->SwitchExpand();
	//Repaint();
}

void CEdEffectTracksEditor::BeginAnimationDrag()
{
	// Animation preview is only possible for effects bound to animation
	if ( m_fxDefinition->IsBoundToAnimation() )
	{
		CEntity* entity = m_effectEditor->GetEntity();
		if ( entity )
		{
			// Stop all effects
			m_effectEditor->ForceStopEffects();

			// Begin preview of animation on the root component
			CAnimatedComponent* ac = entity->GetRootAnimatedComponent();
			if ( ac )
			{
				const CName& animName = m_fxDefinition->GetAnimationName();
				ac->PlayAnimationAtTimeOnSkeleton( animName, m_trackTimeBar->GetPos() );
			}
		}
	}
}

void CEdEffectTracksEditor::EndAnimationDrag()
{
	// End animation preview
	CEntity* entity = m_effectEditor->GetEntity();
	if ( entity )
	{
		CAnimatedComponent* ac = entity->GetRootAnimatedComponent();
		if ( ac )
		{
			ac->StopAllAnimationsOnSkeleton();
		}
	}
}

void CEdEffectTracksEditor::UpdateAnimationDrag( Float time )
{
	// Update time bar
	m_trackTimeBar->SetPos( time );
	Refresh();

	// Update animation preview
	CEntity* entity = m_effectEditor->GetEntity();
	if ( entity )
	{
		CAnimatedComponent* ac = entity->GetRootAnimatedComponent();
		if ( ac && m_fxDefinition )
		{
			const Float animTime = m_trackTimeBar->GetPos();
			const CName animName = m_fxDefinition->GetAnimationName();
			CPlayedSkeletalAnimation* animation = ac->GetAnimatedSkeleton()->GetPlayedAnimation( animName );
			
			if ( animation )
			{
				animation->SetTime( animTime );
			}
			else
			{
				ASSERT( animation );
			}
		}
	}
}

void CEdEffectTracksEditor::MouseClickCanvasPanel( wxMouseEvent& event )
{
	wxPoint point = event.GetPosition();

	ASSERT( IsMouseOverCanvasPanel( point ) == true );

	const Float maxMouseTravelDistToShowMenu = 1.0f;
	const Int32 maxMoveTotal = 5;
	const Int32 snappingBorderSize = 15;

	// Bars

	if ( m_action == MA_None && event.LeftDown() && m_trackTimeBar->IsHightlighted() )
	{
		m_action = MA_MovingTrackTimeBar;
		BeginAnimationDrag();
	}
	else if ( m_action == MA_MovingTrackTimeBar && event.LeftUp() )
	{
		EndAnimationDrag();
		m_action = MA_None;
		return;
	}

	if ( m_action == MA_None && event.LeftDown() && m_trackTimeEndBar->IsHightlighted() )
	{
		m_action = MA_MovingTrackTimeEndBar;
	}
	else if ( m_action == MA_MovingTrackTimeEndBar && event.LeftUp() )
	{
		m_action = MA_None;
		return;
	}

	if ( m_action == MA_None && event.LeftDown() && m_trackIntervalTimeEndBar->IsHightlighted() )
	{
		m_action = MA_MovingTrackIntervalTimeEndBar;
	}
	else if ( m_action == MA_MovingTrackIntervalTimeEndBar && event.LeftUp() )
	{
		m_action = MA_None;
		return;
	}

	if ( m_action == MA_None && event.LeftDown() && m_trackIntervalTimeBeginBar->IsHightlighted() )
	{
		m_action = MA_MovingTrackIntervalTimeBeginBar;
	}
	else if ( m_action == MA_MovingTrackIntervalTimeBeginBar && event.LeftUp() )
	{
		m_action = MA_None;
		return;
	}

	// Track items

	if ( m_action == MA_None && event.LeftDown() && m_trackItemMouseOver )
	{
		m_trackItemMouseOverClicked = m_trackItemMouseOver;
		if ( !m_selectedTrackItems.Exist( m_trackItemMouseOver ) )
		{
			if ( !event.ShiftDown() ) m_selectedTrackItems.Clear();
			m_selectedTrackItems.PushBackUnique( m_trackItemMouseOver );
		}
		else if ( event.ShiftDown() && m_selectedTrackItems.Size() > 0 )
		{
			m_selectedTrackItems.Remove( m_trackItemMouseOver );
		}
	}
	else if ( m_action == MA_None && event.LeftDown() && !m_trackItemMouseOver )
	{
		if ( !event.ShiftDown() ) m_selectedTrackItems.Clear();
	}

	// Set property
	if ( m_action == MA_None && event.LeftDown() && m_trackItemMouseOver )
	{
		m_effectEditor->SetObjectToEdit( m_trackItemMouseOver );
	}
	else if ( m_action == MA_None && event.LeftDown() && m_trackMouseOver && !m_trackItemMouseOver )
	{
		m_effectEditor->SetObjectToEdit( m_trackMouseOver );
	}
	else if ( m_action == MA_None && event.LeftDown() && m_trackGroupMouseOver && !m_trackMouseOver && !m_trackItemMouseOver )
	{
		m_effectEditor->SetObjectToEdit( m_trackGroupMouseOver );
	}
	else if ( m_action == MA_None && event.LeftDown() )
	{
		m_effectEditor->SetObjectToEdit( m_fxDefinition );
	}

	if ( m_action == MA_None && event.LeftDown() && m_trackItemMouseOver && m_trackItemIsMouseOverSizer )
	{
		m_action = MA_ResizingTrackItem;
	}
	else if ( m_action == MA_ResizingTrackItem && event.LeftUp() )
	{
		m_action = MA_None;
		return;
	}

	if ( m_action == MA_None && event.LeftDown() && m_trackItemMouseOver && !m_trackItemIsMouseOverSizer )
	{
		m_action = MA_MovingTrackItem;
	}
	else if ( m_action == MA_MovingTrackItem && event.LeftUp() )
	{
		m_action = MA_None;
		return;
	}

	if ( m_action == MA_None && event.RightDown() && m_trackItemMouseOver )
	{
		m_trackItemMouseOverClicked = m_trackItemMouseOver;
		m_trackMouseOverClicked = m_trackMouseOver;
		m_trackItemMouseOver = NULL;
		m_trackMouseOver = NULL;
		if ( !m_selectedTrackItems.Exist( m_trackItemMouseOverClicked ) )
		{
			if ( !event.ShiftDown() )
			{
				m_selectedTrackItems.Clear();
			}
			m_selectedTrackItems.PushBack( m_trackItemMouseOverClicked );
		}
		Repaint( true );

		// show context menu for button
		wxMenu menu;

		menu.Append( 0, TXT("Remove track item") );
		menu.Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnRemoveTrackItem ), NULL, this );

		menu.Append( 1, TXT("Copy track item") );
		menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnCopyTrackItem ), NULL, this );

		//menu.Append( 2, TXT("Rename track item") );
		//menu.Connect( 2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnRenameTrackItem ), NULL, this );

		if ( m_trackItemMouseOverClicked->SupportsCurve() )
		{
			menu.AppendSeparator();

			if ( m_trackItemMouseOverClicked->GetCurvesCount() > 1 )
			{
				menu.Append( 5, TXT("Show curves") );
				menu.Connect( 5, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnAddTrackItemCurves ), NULL, this );
			}
			else
			{
				menu.Append( 3, TXT("Show curve") );
				menu.Connect( 3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnAddTrackItemCurve ), NULL, this );
			}

			menu.Append( 4, TXT("Hide curve") );
			menu.Connect( 4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnRemoveTrackItemCurve ), NULL, this );
		}

		PopupMenu( &menu );

		return;
	}

	// Selecting window
	/*
	if ( m_action == MA_None && event.LeftDown() && m_controlPointInfosMouseOver.Size() == 0 )
	{
		// Deselecting all selected points
		m_controlPointInfosSelected.Clear();
		SelectionChanged();

		// Going to Selecting Window Mode
		m_action = MA_SelectingWindows;
		CaptureMouse( true, false );

		m_selectionRegion.SetStartAndEndPoint( ClientToCanvas( point ) );
	}
	else if ( m_action == MA_SelectingWindows && event.LeftUp() )
	{
		m_action = MA_None;
		CaptureMouse( false, false );

		// Assemble selection rect
		m_selectionRegion.SetEndPoint( ClientToCanvas( point ) );

		// If SHIFT is pressed, so zoom in, if not add control points to selection array
		if ( event.ShiftDown() )
		{
			Vector start = CanvasToCurve( m_selectionRegion.GetStartPoint() );
			Vector end   = CanvasToCurve( m_selectionRegion.GetEndPoint() );
			SetZoomedRegion( start, end );
		}
		else
		{
			m_controlPointInfosSelected.PushBackUnique( m_controlPointInfosMouseOver );
			SelectionChanged();
		}
		return;
	}
	*/

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

		// Minimal movement, show menu
		if ( m_moveTotal < maxMoveTotal )
		{
			// Tracks menu
			if ( m_action == MA_None && m_trackMouseOver && !m_trackItemMouseOver 
				/*&& m_clickedMousePosition.DistanceTo( m_lastMousePosition ) < maxMouseTravelDistToShowMenu*/ )
			{
				m_trackMouseOverClicked = m_trackMouseOver;
				m_selectedTrackItems.Clear();
				m_trackItemMouseOver = NULL;
				m_trackMouseOver = NULL;
				Repaint( true );

				// show context menu for button
				wxMenu menu;

				{
					wxMenu *submenuTrackItems = new wxMenu();
					for ( Uint32 trackItemClassNum = 0; trackItemClassNum < m_trackItemClasses.Size(); ++trackItemClassNum )
					{
						submenuTrackItems->Append( trackItemClassNum, m_trackItemClasses[trackItemClassNum]->GetName().AsString().AsChar() );
						menu.Connect( trackItemClassNum, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnAddTrackItem ), NULL, this );
					}

					menu.AppendSubMenu( submenuTrackItems, TXT("Add track item") );
				}

				if ( GObjectClipboard.HasObjects() )
				{
					menu.Append( m_trackItemClasses.Size() + 0, TXT("Paste track item") );
					menu.Connect( m_trackItemClasses.Size() + 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnPasteTrackItem ), NULL, this );
				}

				//menu.Append( trackItemClasses.Size() + 0, TXT("Add track item") );
				//menu.Connect( 0, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdEffectTracksEditor::OnButtonAppendTrack ), NULL, this );

				PopupMenu( &menu );

				return;
			}
		}
	}
}

void CEdEffectTracksEditor::SetZoomedRegion( const Vector& corner1, const Vector& corner2 )
{
	if ( corner1 == corner2 )
	{
		// Zoomed region has no area
		return;
	}

	// Calculate upper left corner and lower right corner
	Vector start = Vector::Min4( corner1, corner2 );
	Vector end   = Vector::Max4( corner1, corner2 );

	// Calculate proper clientRect (which take into account left side curvesInfo
	wxRect clientRect = GetClientRect();
	clientRect.width -= GetSidePanelWidth();

	// Calculate new scale
	m_scaleTime.X = (end.X - start.X) / Max(clientRect.width , 1);
	//m_scaleTime.Y = (end.Y - start.Y) / Max(clientRect.height, 1); // don't scale vertically

	// Calculate new offset
	wxPoint offset;
	offset.x = -(start.X / m_scaleTime.X) + GetSidePanelWidth();
	//offset.y = +(start.Y / m_scaleTime.Y) + clientRect.height; // don't move vertically
	SetOffset( offset );
	Repaint();

	SynchronizeWithOther( offset, m_scaleTime );
}

void CEdEffectTracksEditor::Synchronize( wxPoint offset, const Vector& scale )
{
	SetOffset( offset );
	m_scaleTime = scale;
	m_scaleTime.Y = 1.f;
	Repaint( true );
}

Int32 CEdEffectTracksEditor::GetDownPanelHeight() const
{
	return m_trackTimeBar->GetTrackBarHeight();
}

void CEdEffectTracksEditor::DrawDownPanel()
{
	static wxColour lineColor( 255, 255, 255 );

	// Draw bars
	m_trackTimeBar->Draw();
	m_trackIntervalTimeBeginBar->Draw();
	m_trackIntervalTimeEndBar->Draw();
	m_trackTimeEndBar->Draw();

	// Draw horizontal line
	wxSize clientSize = GetClientSize();
	DrawLine( ClientToTime( wxPoint(GetSidePanelWidth(), clientSize.GetHeight() - GetDownPanelHeight()) ),
		ClientToTime( wxPoint(clientSize.GetWidth(), clientSize.GetHeight() - GetDownPanelHeight()) ), lineColor, 1.5f );
}

Bool CEdEffectTracksEditor::IsOverTrackBar( wxPoint point, CEdEffectTracksEditorBar *trackBar )
{
	wxRect trackTimeBarRect = trackBar->GetRect();
	wxPoint mousePos = ClientToCanvas( point );
	return trackTimeBarRect.Contains( mousePos );
}

void CEdEffectTracksEditor::OnButtonRemove( wxCommandEvent &event )
{
	if ( !m_buttonMouseOverClicked ) return;

	
	m_buttonMouseOverClicked->Remove();
	m_buttonMouseOver = NULL;
	m_buttonMouseOverClicked = NULL;
	Repaint();
}

void CEdEffectTracksEditor::OnButtonAppendTrackGroup( wxCommandEvent &event )
{
	OnButtonInsertTrackGroup( event );
	/*if ( m_buttonMouseOverClicked )
	{
		String trackGroupName;
		if ( !InputBox( this, TXT("Track Group Name"), TXT("Write the track group name"), trackGroupName ) ) return;
		if ( m_buttonMouseOverClicked->IsA< CFXDefinition >() )
		{
			Cast< CFXDefinition >( m_buttonMouseOverClicked )->AddTrackGroup( trackGroupName );
		}
	}
	else
	{
		String trackGroupName;
		if ( !InputBox( this, TXT("Track Group Name"), TXT("Write the track group name"), trackGroupName ) ) return;
		m_fxDefinition->AddTrackGroup( trackGroupName );
	}
	*/
}

void CEdEffectTracksEditor::OnButtonInsertTrackGroup( wxCommandEvent &event )
{
	if ( m_buttonMouseOverClicked )
	{
		String trackGroupName;
		if ( !InputBox( this, TXT("Track Group Name"), TXT("Write the track group name"), trackGroupName ) ) return;
#if 0 // CFXDefinition and CFXBase are unrelated types !!! Below "if" would always fail
		if ( m_buttonMouseOverClicked->IsA< CFXDefinition >() )
		{
			Cast< CFXDefinition >( m_buttonMouseOverClicked )->AddTrackGroup( trackGroupName );
		}
#endif
		
	}
	else
	{
		String trackGroupName;
		if ( !InputBox( this, TXT("Track Group Name"), TXT("Write the track group name"), trackGroupName ) ) return;
		m_fxDefinition->AddTrackGroup( trackGroupName );
	}
}

void CEdEffectTracksEditor::OnButtonAppendTrack( wxCommandEvent &event )
{
	if ( m_buttonMouseOverClicked )
	{
		String trackName;
		if ( !InputBox( this, TXT("Track Name"), TXT("Write the track name"), trackName ) ) return;
		if ( m_buttonMouseOverClicked->IsA< CFXTrackGroup >() )
		{
			Cast< CFXTrackGroup >(m_buttonMouseOverClicked)->AddTrack( trackName );
		}
	}
}

void CEdEffectTracksEditor::DrawTrackItems()
{
	LayoutInfo info;
	int scroolPos = m_buttonsScroolPos;

	// Create layouts for 'm_fxDefinition'
	TDynArray< CFXTrackGroup* > &effectTrackGroupArray = m_fxDefinition->GetEffectTrackGroup();
	for ( TDynArray< CFXTrackGroup* >::iterator effectTrackGroup = effectTrackGroupArray.Begin(); effectTrackGroup != effectTrackGroupArray.End(); ++effectTrackGroup )
	{
		// Adjust scroll
		if ( scroolPos != 0 )
		{
			--scroolPos;
		}

		// Skip non expanded groups
		if ( !(*effectTrackGroup)->IsExpanded() )
		{
			continue;
		}

		TDynArray< CFXTrack* > &effectTrackArray = (*effectTrackGroup)->GetTracks();
		for ( TDynArray< CFXTrack* >::iterator effectTrack = effectTrackArray.Begin(); effectTrack != effectTrackArray.End(); ++effectTrack )
		{
			// Adjust scroll
			if ( scroolPos > 0 )
			{
				--scroolPos;
				continue;
			}

			LayoutInfo* pInfo = m_layout.FindPtr( *effectTrack );
			if ( !pInfo )
			{
				continue;
			}

			// Get layout info
			info = *pInfo;

			// keep boundaries
			if ( info.m_windowRect.GetY() + info.m_windowRect.GetHeight() > GetClientSize().GetHeight() - GetDownPanelHeight() )
			{
				break;
			}

			TDynArray< CFXTrackItem* > &effectTrackItemArray = (*effectTrack)->GetTrackItems();
			for ( TDynArray< CFXTrackItem* >::iterator effectTrackItem = effectTrackItemArray.Begin(); effectTrackItem != effectTrackItemArray.End(); ++effectTrackItem )
			{
				LayoutInfo* pInfo = m_layout.FindPtr( *effectTrackItem );
				if ( !pInfo )
				{
					continue;
				}

				info = *pInfo;

				float bgColorModifier = 0.8f;

				// Hightlight
				if ( m_selectedTrackItems.Exist( *effectTrackItem ) || m_trackItemMouseOver == *effectTrackItem /*&& !m_trackItemIsMouseOverSizer*/ )
				{
					bgColorModifier = 1.0f;
					
					// Draw text
					if ( (*effectTrackItem)->IsTick() )
					{
						Float yPosTime = ClientToTime( wxPoint(0, info.m_windowRect.y + info.m_windowRect.height/2 ) ).Y;
						DrawTooltip( (*effectTrackItem)->GetTimeBegin(), yPosTime, (*effectTrackItem)->GetName() );
					}
					else
					{
						Float yPosTime = ClientToTime( wxPoint(0, info.m_windowRect.y) ).Y;
						DrawTooltip( (*effectTrackItem)->GetTimeBegin(), yPosTime );
						DrawTooltip( (*effectTrackItem)->GetTimeEnd(), yPosTime );
					}
				}
				else
				{
					if ( (*effectTrackItem)->IsTick() )
					{
						//Float yPosTime = ClientToTime( wxPoint(0, info.m_windowRect.y + info.m_windowRect.height/2 ) ).Y;
						//DrawTooltip( (*effectTrackItem)->GetTimeBegin(), yPosTime );
					}
				}

				// Draw area
				wxColour trackItemBackgroundTrackItemColor( (*effectTrackGroup)->GetColor().R * bgColorModifier, (*effectTrackGroup)->GetColor().G * bgColorModifier, (*effectTrackGroup)->GetColor().B * bgColorModifier, 255 );
				wxColour trackItemBorder( 255, 255, 255 );
				FillRect( ClientToCanvas( info.m_windowRect ), trackItemBackgroundTrackItemColor );
				DrawRect( ClientToCanvas( info.m_windowRect ), trackItemBorder );

				// Draw text
				if ( !(*effectTrackItem)->IsTick() )
				{
					wxRect textRect = info.m_windowRect;
					textRect.SetWidth( textRect.width - 5 );
					SetClip( ClientToCanvas( textRect ) );
					CEdCanvas::DrawText( ClientToCanvas( wxPoint( info.m_windowRect.x + 5, info.m_windowRect.y + info.m_windowRect.GetHeight() / 2 ) ),
						GetGdiDrawFont(), (*effectTrackItem)->GetName(),
						trackItemBorder, CEdCanvas::CVA_Center, CEdCanvas::CHA_Left );
					ResetClip();
				}
				
				// Draw left triangle
				{
					const int x1 = info.m_windowRect.x;
					const int y1 = info.m_windowRect.y + info.m_windowRect.GetHeight() / 2 /*+ info.m_windowRect.GetHeight()/4*/;
					const int x2 = info.m_windowRect.x + info.m_windowRect.GetHeight() / 4;
					const int y2 = info.m_windowRect.y + info.m_windowRect.GetHeight() - 1;
					const int x3 = info.m_windowRect.x - info.m_windowRect.GetHeight() / 4;
					const int y3 = info.m_windowRect.y + info.m_windowRect.GetHeight() - 1;
					const wxPoint p1(x1, y1);
					const wxPoint p2(x2, y2);
					const wxPoint p3(x3, y3);
					wxPoint p1c = ClientToCanvas( p1 );
					wxPoint p2c = ClientToCanvas( p2 );
					wxPoint p3c = ClientToCanvas( p3 );
					FillTriangle( p1c.x, p1c.y, p2c.x, p2c.y, p3c.x, p3c.y, trackItemBackgroundTrackItemColor );
					DrawTriangle( p1c.x, p1c.y, p2c.x, p2c.y, p3c.x, p3c.y, trackItemBorder );
				}

				// Draw right triangle
				if ( info.m_windowRect.GetWidth() > info.m_expandWindowRect.width )
				{
					const int x1 = info.m_expandWindowRect.x + info.m_expandWindowRect.width;
					const int y1 = info.m_expandWindowRect.y;
					const int x2 = info.m_expandWindowRect.x;
					const int y2 = info.m_expandWindowRect.y + info.m_expandWindowRect.height;
					const wxPoint p1(x1, y1);
					const wxPoint p2(x2, y2);
					wxPoint p1c = ClientToCanvas( p1 );
					wxPoint p2c = ClientToCanvas( p2 );
					CEdCanvas::DrawLine( p1c, p2c, trackItemBorder );
				}
			}
		}
	}
}

void CEdEffectTracksEditor::OnRemoveTrackItem( wxCommandEvent &event )
{
	for ( Uint32 i=0; i<m_selectedTrackItems.Size(); ++i )
	{
		m_effectCurveEditor->RemoveCurve( &m_selectedTrackItems[i]->GetCurve()->GetCurveData() );
		//ab fix
		m_selectedTrackItems[i]->Remove();
		if ( m_selectedTrackItems[i] == m_trackItemMouseOverClicked )
		{
			m_trackItemMouseOverClicked = NULL;
		}
	}
	m_selectedTrackItems.Clear();
}

void CEdEffectTracksEditor::OnCopyTrackItem( wxCommandEvent &event )
{
	CopySelectedTrackItem();
}

void CEdEffectTracksEditor::CopySelectedTrackItem()
{
	TDynArray<CObject*> objs;

	if ( m_selectedTrackItems.Size() == 0 ) return;
	
	if ( !GObjectClipboard.Copy( m_selectedTrackItems ) )
	{
		wxMessageBox( wxT("Failed to copy the selected item(s) to the clipboard"), wxT("Error"), wxOK|wxICON_ERROR|wxCENTRE, this );
	}
}

void CEdEffectTracksEditor::OnRenameTrackItem( wxCommandEvent &event )
{
	//if ( !m_trackItemMouseOverClicked ) return;

	//String trackItemName;
	//if ( !InputBox( this, TXT("Track Name"), TXT("Write the track item new name"), trackItemName ) ) return;
	//m_trackItemMouseOverClicked->SetName( trackItemName );
}

void CEdEffectTracksEditor::OnAddTrackItem( wxCommandEvent &event )
{
	if ( m_trackMouseOverClicked )
	{
		// Get class to create
		ASSERT( (Uint32)event.GetId() < m_trackItemClasses.Size() );
		CClass *trackItemClass = m_trackItemClasses[ event.GetId() ];

		// Create track item
		m_trackMouseOverClicked->AddTrackItem( trackItemClass, m_clickedMousePosition.X );
	}
}

void CEdEffectTracksEditor::OnPasteTrackItem( wxCommandEvent &event )
{
	PasteTrackItem( m_trackMouseOverClicked, m_clickedMousePosition.X );
}

void CEdEffectTracksEditor::PasteTrackItem( CFXTrack *parentTrack, Float position ) 
{
	ASSERT( parentTrack );

	if ( !parentTrack->MarkModified() ) return;

	TDynArray<CObject*> objs;
	if ( !GObjectClipboard.Paste( objs, parentTrack ) )
	{
		wxMessageBox( wxT("Failed to paste an item from the clipboard"), wxT("Error"), wxCENTRE|wxOK|wxICON_ERROR );
		return;
	}

	if ( objs.Size() == 0 ) return;

	Uint32 unsupportedObjects = 0;
	float base = 0;
	bool first = true;
	for ( Uint32 i=0; i<objs.Size(); ++i )
	{
		if ( !objs[i]->IsA<CFXTrackItem>() )
		{
			unsupportedObjects++;
			objs[i]->Discard();
		}
		else
		{
			CFXTrackItem* trackItem = Cast<CFXTrackItem>(objs[i]);
			if ( first )
			{
				base = trackItem->GetTimeBegin();
				first = false;
			}
			parentTrack->AddTrackItem( trackItem, position + trackItem->GetTimeBegin() - base );
		}
	}

	if ( unsupportedObjects == objs.Size() )
	{
		wxMessageBox( wxT("Unsupported object in the clipboard"), wxT("Error"), wxCENTRE|wxOK|wxICON_ERROR );
	}
	else if ( unsupportedObjects > 0 )
	{
		wxMessageBox( wxString::Format(wxT("There were %d (of %d) unsupported objects in the clipboard"), unsupportedObjects, objs.Size()), wxT("Warning"), wxCENTRE|wxOK|wxICON_WARNING );
	}

	Refresh( false );
	Update();
}

void CEdEffectTracksEditor::PasteCopiedTrackItem()
{
	if ( !m_trackMouseOver ) return;

	PasteTrackItem( m_trackMouseOver, m_lastMousePosition.X );
}

void CEdEffectTracksEditor::OnAddTrackItemCurve( wxCommandEvent &event )
{
	if ( !m_trackItemMouseOverClicked ) return;
	if ( !m_trackItemMouseOverClicked->SupportsCurve() ) return;
	if ( m_effectCurveEditor->IsCurveAdded( &m_trackItemMouseOverClicked->GetCurve()->GetCurveData() ) ) return;

	Vector curveScale( m_trackItemMouseOverClicked->GetTimeBegin(), m_trackItemMouseOverClicked->GetTimeDuration(), 0.0f );

	// Choose unique curve name
	String curveNameOrg = m_trackItemMouseOverClicked->GetName(); // = m_trackMouseOverClicked->GetName() + m_trackItemMouseOverClicked->GetName()
	String curveName = curveNameOrg;
	for ( Int32 i = 0; i < 25; ++i )
	{
		if ( m_effectCurveEditor->IsCurveGroupAdded(CName(curveName)) )
		{
			curveName = String::Printf( TXT("%s_%d"), curveNameOrg.AsChar(), i );
		}
		else
		{
			break;
		}
	}

	m_trackItemMouseOverClicked->GetCurve()->SetColor( m_trackItemMouseOverClicked->GetTrack()->GetTrackGroup()->GetColor() );
	m_effectCurveEditor->AddCurve( m_trackItemMouseOverClicked->GetCurve(), curveName, false, curveScale );
}

void CEdEffectTracksEditor::OnAddTrackItemCurves( wxCommandEvent &event )
{
	if ( !m_trackItemMouseOverClicked ) return;
	if ( !m_trackItemMouseOverClicked->SupportsCurve() ) return;
	if ( m_effectCurveEditor->IsCurveAdded( &m_trackItemMouseOverClicked->GetCurve()->GetCurveData() ) ) return;

	Vector curveScale( m_trackItemMouseOverClicked->GetTimeBegin(), m_trackItemMouseOverClicked->GetTimeDuration(), 0.0f );

	for ( Uint32 curveIdx = 0; curveIdx < m_trackItemMouseOverClicked->GetCurvesCount(); ++curveIdx )
	{
		// Choose unique curve name
		String curveNameOrg = m_trackItemMouseOverClicked->GetCurveName( curveIdx );
		String curveName = curveNameOrg;
		for ( Int32 i = 0; i < 25; ++i )
		{
			if ( m_effectCurveEditor->IsCurveGroupAdded(CName(curveName)) )
			{
				curveName = String::Printf( TXT("%s_%d"), curveNameOrg.AsChar(), i );
			}
			else
			{
				break;
			}
		}

		m_effectCurveEditor->AddCurve( m_trackItemMouseOverClicked->GetCurve(curveIdx), curveName, false, curveScale );
		//m_effectCurveEditor->AddCurve( m_trackItemMouseOverClicked->GetCurveParameter(), false, curveScale ); <- TODO: handle curve group as a group
	}
}

void CEdEffectTracksEditor::OnRemoveTrackItemCurve( wxCommandEvent &event )
{
	if ( !m_trackItemMouseOverClicked ) return;

	for ( Uint32 curveIdx = 0; curveIdx < m_trackItemMouseOverClicked->GetCurvesCount(); ++curveIdx )
	{
		m_effectCurveEditor->RemoveCurve( &m_trackItemMouseOverClicked->GetCurve(curveIdx)->GetCurveData() );
	}
}

void CEdEffectTracksEditor::OnButtonRename( wxCommandEvent &event )
{
	if ( !m_buttonMouseOverClicked ) return;

	String newName;
	if ( !InputBox( this, TXT("New name"), TXT("Write new name"), newName ) ) return;

	m_buttonMouseOverClicked->SetName( newName );

	Repaint();
}

void CEdEffectTracksEditor::ScrollButtons( Int32 shift )
{
	Int32 newButtonsScroolPos = m_buttonsScroolPos + shift;
	
	// Get max shift value
	Int32 maxButtonsScroolPos = 0;
	const TDynArray< CFXTrackGroup* > &effectTrackGroupArray = m_fxDefinition->GetEffectTrackGroup();
	for ( TDynArray< CFXTrackGroup* >::const_iterator effectTrackGroup = effectTrackGroupArray.Begin();
		  effectTrackGroup != effectTrackGroupArray.End();
		  ++effectTrackGroup )
	{
		++maxButtonsScroolPos;
		if ( (*effectTrackGroup)->IsExpanded() )
		{
			const TDynArray< CFXTrack* > &effectTrackArray = (*effectTrackGroup)->GetTracks();
			maxButtonsScroolPos += effectTrackArray.Size();
		}
	}

	if ( newButtonsScroolPos >= 0 && newButtonsScroolPos <= maxButtonsScroolPos )
	{
		m_buttonsScroolPos = newButtonsScroolPos;
	}
}

void CEdEffectTracksEditor::DrawTooltip( Float xPosTime, Float yPosTime, const String& customString /*= String::EMPTY*/ )
{
	static wxColour textColor( 255, 255, 255 );

	String string = String::Printf( TXT("%.2f"), xPosTime );
	Float position = xPosTime;

	// Override string
	if ( customString.GetLength() )
	{
		string = customString;
	}

	// Shift text if it extends over client size, so it will be always visible
	wxPoint testExtents = TextExtents( GetGdiDrawFont(), string );
	if ( TimeToClient( Vector(xPosTime,0,0) ).x + testExtents.x/2 > GetClientSize().GetWidth() )
	{
		// shift to left
		position = ClientToTime( wxPoint( TimeToClient( Vector(xPosTime,0,0) ).x - testExtents.x/2, 0 ) ).X;
	}
	else if ( TimeToClient( Vector(xPosTime,0,0) ).x - testExtents.x/2 < GetSidePanelWidth() )
	{
		// shift to right
		position = ClientToTime( wxPoint( TimeToClient( Vector(xPosTime,0,0) ).x + testExtents.x/2, 0 ) ).X;
	}

	DrawText( position, yPosTime, GetGdiDrawFont(), string, textColor, 
		CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Center );
}

void CEdEffectTracksEditor::OnEditTrackItemColor( wxCommandEvent &event )
{
	if ( !m_trackItemMouseOverClicked ) return;
	if ( !m_trackItemMouseOverClicked->SupportsColor() ) return;

	m_gradientEditor->SetCurves( m_trackItemMouseOverClicked->GetCurve( 0 ), m_trackItemMouseOverClicked->GetCurve( 1 ),
		m_trackItemMouseOverClicked->GetCurve( 2 ), m_trackItemMouseOverClicked->GetCurve( 3 ) );
	m_gradientEditor->Show();	
}

void CEdEffectTracksEditor::OnCharPressed( wxKeyEvent& event )
{
	if ( event.GetKeyCode() == WXK_HOME )
	{
		// Shift position to start (zero)
		wxPoint point = GetOffset();
		point.x = GetSidePanelWidth();
		SetOffset( point );
		SynchronizeWithOther( point, m_scaleTime );
	}

	event.Skip();
}

Float CEdEffectTracksEditor::SnapToBar( Float timePos )
{
	const Float SNAP_THRESHOLD = 0.2f;

	for ( TDynArray< CEdEffectTracksEditorBar * >::iterator trackBar = m_trackBars.Begin();
		  trackBar != m_trackBars.End();
		  ++trackBar )
	{
		const Float &trackBarTimePos = (*trackBar)->GetPos();
		if ( abs(trackBarTimePos - timePos) < SNAP_THRESHOLD )
		{
			return trackBarTimePos;
		}
	}

	return timePos;
}

//////////////////////////////////////////////////////////////////////////

void CEdEffectTracksEditorTimeBar::Draw()
{
	static wxColour trackBarColorNormal( 0, 255, 0, 100 );
	static wxColour trackBarMouseOverColor( 0, 255, 0 );
	static wxColour trackBarMovingColor( 0, 255, 255, 100 );
	static wxColour textColor( 255, 255, 255 );

	const wxColour &trackBarColor = m_hightlight ? trackBarMouseOverColor : trackBarColorNormal;

	m_editor->FillRoundedRect( m_x, m_y, m_trackBarWidth, m_trackBarHeight / 2, trackBarColor, 3 );

	Float timeDown = m_editor->ClientToTime( wxPoint(0, m_editor->GetClientSize().GetHeight() - m_trackBarHeight / 2 ) ).Y;
	Float timeUp = m_editor->ClientToTime( wxPoint(0, 0) ).Y;
	m_editor->DrawLine( Vector( m_position, timeUp, 0 ), Vector( m_position, timeDown, 0 ), trackBarColor );

	DrawTooltip( timeDown );
}

wxRect CEdEffectTracksEditorTimeBar::GetRect() const
{
	return wxRect( m_x, m_y, m_trackBarWidth, m_trackBarHeight / 2 );
}

void CEdEffectTracksEditorTimeBar::UpdateLayout()
{
	m_x = m_editor->TimeToCanvas( Vector( m_position, 0, 0 ) ).x;
	m_y = m_editor->ClientToCanvas( wxPoint( 0, m_editor->GetClientSize().GetHeight() - m_trackBarHeight / 2 ) ).y;

	m_x -= m_trackBarWidth / 2; // center position
}

//////////////////////////////////////////////////////////////////////////

wxRect CEdEffectTracksEditorIntervalTimeBeginBar::GetRect() const
{
	return wxRect( m_x, m_y, m_trackBarWidth, m_trackBarHeight );
}

void CEdEffectTracksEditorIntervalTimeBeginBar::UpdateLayout()
{
	m_x = m_editor->TimeToCanvas( Vector( m_position, 0, 0 ) ).x;
	m_y = m_editor->ClientToCanvas( wxPoint( 0, m_editor->GetClientSize().GetHeight() - m_trackBarHeight ) ).y;
}

void CEdEffectTracksEditorIntervalTimeBeginBar::Draw()
{
	static wxColour trackBarColorNormal( 0, 0, 255, 100 );
	static wxColour trackBarMouseOverColor( 0, 0, 255 );
	static wxColour trackBarMovingColor( 0, 255, 255, 100 );
	static wxColour textColor( 255, 255, 255 );

	const wxColour &trackBarColor = m_hightlight ? trackBarMouseOverColor : trackBarColorNormal;

	m_editor->FillTriangle( m_x, m_y,
							m_x, m_y + m_trackBarHeight,
							m_x + m_trackBarWidth, m_y + m_trackBarHeight,
							trackBarColor );

	Float timeDown = m_editor->ClientToTime( wxPoint(0, m_editor->GetClientSize().GetHeight() - m_trackBarHeight ) ).Y;
	Float timeUp = m_editor->ClientToTime( wxPoint(0, 0) ).Y;
	m_editor->DrawLine( Vector( m_position, timeUp, 0 ), Vector( m_position, timeDown, 0 ), trackBarColor );

	DrawTooltip( timeDown );
}

//////////////////////////////////////////////////////////////////////////

wxRect CEdEffectTracksEditorIntervalTimeEndBar::GetRect() const
{
	return wxRect( m_x - m_trackBarWidth, m_y, m_trackBarWidth, m_trackBarHeight );
}

void CEdEffectTracksEditorIntervalTimeEndBar::UpdateLayout()
{
	m_x = m_editor->TimeToCanvas( Vector( m_position, 0, 0 ) ).x;
	m_y = m_editor->ClientToCanvas( wxPoint( 0, m_editor->GetClientSize().GetHeight() - m_trackBarHeight ) ).y;
}

void CEdEffectTracksEditorIntervalTimeEndBar::Draw()
{
	static wxColour trackBarColorNormal( 0, 0, 255, 100 );
	static wxColour trackBarMouseOverColor( 0, 0, 255 );
	static wxColour trackBarMovingColor( 0, 255, 255, 100 );
	static wxColour textColor( 255, 255, 255 );

	const wxColour &trackBarColor = m_hightlight ? trackBarMouseOverColor : trackBarColorNormal;

	m_editor->FillTriangle( m_x, m_y,
		m_x, m_y + m_trackBarHeight,
		m_x - m_trackBarWidth, m_y + m_trackBarHeight,
		trackBarColor );

	Float timeDown = m_editor->ClientToTime( wxPoint(0, m_editor->GetClientSize().GetHeight() - m_trackBarHeight ) ).Y;
	Float timeUp = m_editor->ClientToTime( wxPoint(0, 0) ).Y;
	m_editor->DrawLine( Vector( m_position, timeUp, 0 ), Vector( m_position, timeDown, 0 ), trackBarColor );

	DrawTooltip( timeDown );
}

//////////////////////////////////////////////////////////////////////////

wxRect CEdEffectTracksEditorTimeEndBar::GetRect() const
{
	return wxRect( m_x, m_y, m_trackBarWidth, m_trackBarHeight );
}

void CEdEffectTracksEditorTimeEndBar::UpdateLayout()
{
	m_x = m_editor->TimeToCanvas( Vector( m_position, 0, 0 ) ).x;
	m_y = m_editor->ClientToCanvas( wxPoint( 0, m_editor->GetClientSize().GetHeight() - m_trackBarHeight ) ).y;

	m_x -= m_trackBarWidth / 2; // center position
}

void CEdEffectTracksEditorTimeEndBar::Draw()
{
	static wxColour trackBarColorNormal( 0, 0, 255, 100 );
	static wxColour trackBarMouseOverColor( 0, 0, 255 );
	static wxColour trackBarMovingColor( 0, 255, 255, 100 );
	static wxColour textColor( 255, 255, 255 );

	const wxColour &trackBarColor = m_hightlight ? trackBarMouseOverColor : trackBarColorNormal;

	m_editor->FillRect( m_x, m_y, m_trackBarWidth, m_trackBarHeight, trackBarColor );

	Float timeDown = m_editor->ClientToTime( wxPoint(0, m_editor->GetClientSize().GetHeight() - m_trackBarHeight ) ).Y;
	Float timeUp = m_editor->ClientToTime( wxPoint(0, 0) ).Y;
	m_editor->DrawLine( Vector( m_position, timeUp, 0 ), Vector( m_position, timeDown, 0 ), trackBarColor );

	DrawTooltip( timeDown );
}

void CEdEffectTracksEditorBar::DrawTooltip( Float yPos )
{
	//static wxColour textColor( 255, 255, 255 );

	if ( m_hightlight )
	{
		m_editor->DrawTooltip( m_position, yPos );
		//String string = String::Printf( TXT("%.2f"), m_position );
		//Float position = m_position;

		//// Shift text if it extends over client size, so it will be always visible
		//wxPoint testExtents = m_editor->TextExtents( m_editor->GetGdiDrawFont(), string );
		//if ( m_editor->TimeToClient( Vector(m_position,0,0) ).x + testExtents.x/2 > m_editor->GetClientSize().GetWidth() )
		//{
		//	// shift to left
		//	position = m_editor->ClientToTime( wxPoint( m_editor->TimeToClient( Vector(m_position,0,0) ).x - testExtents.x/2, 0 ) ).X;
		//}
		//else if ( m_editor->TimeToClient( Vector(m_position,0,0) ).x - testExtents.x/2 < m_editor->GetSidePanelWidth() )
		//{
		//	// shift to right
		//	position = m_editor->ClientToTime( wxPoint( m_editor->TimeToClient( Vector(m_position,0,0) ).x + testExtents.x/2, 0 ) ).X;
		//}

		//m_editor->DrawText( position, yPos, m_editor->GetGdiDrawFont(), string, textColor, 
		//	CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Center );
	}
}
