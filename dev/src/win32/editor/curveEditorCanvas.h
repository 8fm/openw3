/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CEdCurveEditor;
class CurveParameter;
class CCurve;

#define CURVE_EDITOR_CANVAS_SIDE_PANEL_WIDTH 100

class IEdCurveEditorCanvasHook
{
public:
	virtual void OnCanvasHookSelectionChanged() = 0;
	virtual void OnCanvasHookControlPointsChanged() = 0;
	virtual void OnCanvasHookControlPointsChangedComplete() = 0;
};

/// Curve editor
class CEdCurveEditorCanvas : public CEdCanvas, public IEdEventListener
{
	friend class CurveInfoWrapper;
	friend class CurveControlPointInfoWrapper;

	DECLARE_EVENT_TABLE();

protected:
	/** To add new button to CurveInfo:
	- increase m_buttonsNum
	- add reaction to button click in OnButtonScelected method
	- add change of button filling in MatchFilledButtons
	- add tooltip text to GetButtonTooltip */
	struct CurveInfo
	{
		CCurveEditionWrapper*			m_curve;
		Bool							m_showControlPoints;
		Bool							m_showTangents;
		Bool							m_showApproximated;
		Uint32							m_buttonsNum;
		Uint32							m_buttonRectSize;
		Uint32							m_buttonMargin;
		Uint32							m_rectWidth;
		Uint32							m_rectHeight;
		Uint32							m_buttonsPerRow;
		Bool*							m_isButtonFilled;

		CurveInfo()
			: m_curve( NULL )
			, m_showControlPoints( true )
			, m_showTangents( false )
			, m_showApproximated( false )
			, m_buttonsNum( 1 )
			, m_buttonRectSize( 10 )
			, m_buttonMargin( 5 )
			, m_rectWidth( CURVE_EDITOR_CANVAS_SIDE_PANEL_WIDTH )
			, m_isButtonFilled( new Bool[ m_buttonsNum ] )
		{
			CalculateSize();
			MatchFilledButtons();
		}

		const CurveInfo& operator=( const CurveInfo& other )
		{
			m_curve = other.m_curve;
			m_showControlPoints = other.m_showControlPoints;
			m_showTangents = other.m_showTangents;
			m_showApproximated = other.m_showApproximated;
			m_buttonsNum = other.m_buttonsNum;
			m_buttonRectSize = other.m_buttonRectSize;
			m_buttonMargin = other.m_buttonMargin;
			m_rectWidth = other.m_rectWidth;
			m_rectHeight = other.m_rectHeight;
			m_buttonsPerRow = other.m_buttonsPerRow;
			m_isButtonFilled = new Bool[ m_buttonsNum ];
			Red::System::MemoryCopy( m_isButtonFilled, other.m_isButtonFilled, sizeof( Bool ) * m_buttonsNum );
			return *this;
		}
		CurveInfo( const CurveInfo& other )
		{
			m_curve = other.m_curve;
			m_showControlPoints = other.m_showControlPoints;
			m_showTangents = other.m_showTangents;
			m_showApproximated = other.m_showApproximated;
			m_buttonsNum = other.m_buttonsNum;
			m_buttonRectSize = other.m_buttonRectSize;
			m_buttonMargin = other.m_buttonMargin;
			m_rectWidth = other.m_rectWidth;
			m_rectHeight = other.m_rectHeight;
			m_buttonsPerRow = other.m_buttonsPerRow;
			m_isButtonFilled = new Bool[ m_buttonsNum ];
			Red::System::MemoryCopy( m_isButtonFilled, other.m_isButtonFilled, sizeof( Bool ) * m_buttonsNum );
		}

		~CurveInfo()
		{
			delete m_isButtonFilled;
		}

		void MatchFilledButtons()
		{
			m_isButtonFilled[0] = m_showControlPoints;
		}

		void CalculateSize()
		{
			m_buttonsPerRow = ( m_rectWidth - m_buttonMargin ) / ( m_buttonRectSize + m_buttonMargin );
			m_rectHeight = m_buttonMargin + ( m_buttonRectSize + m_buttonMargin ) * ( m_buttonsNum + m_buttonsPerRow - 1 ) / m_buttonsPerRow;
		}

		Bool operator==( const CurveInfo& other ) const
		{
			return ( m_curve == other.m_curve );
		}

		wxRect GetClientRect( wxPoint position )
		{
			return wxRect( position.x + 0, position.y + 0, m_rectWidth, m_rectHeight );
		}

		wxRect GetButtonRect( Int32 index, wxPoint position )
		{
			int row = index / m_buttonsPerRow;
			Uint32 n = ( row == m_buttonsNum / m_buttonsPerRow ) ? m_buttonsNum % m_buttonsPerRow : m_buttonsPerRow;
			Uint32 sx = ( m_rectWidth - m_buttonRectSize - ( n - 1 ) * ( m_buttonRectSize + m_buttonMargin ) ) >> 1;
			return wxRect( position.x + sx + ( m_buttonRectSize + m_buttonMargin ) * ( index % m_buttonsPerRow ), position.y + m_buttonRectSize + ( index / m_buttonsPerRow ) * ( m_buttonRectSize + m_buttonMargin ), m_buttonRectSize, m_buttonRectSize );
		}

		CCurveEditionWrapper* GetCurve() { return m_curve; }
		const CCurveEditionWrapper* GetCurve() const { return m_curve; }

		const String GetButtonTooltip( Uint32 buttonId )
		{
			switch ( buttonId )
			{
			case 0 : return String( TXT( "Show points" ) );
			default: return String::EMPTY;
			}
		}

		void Draw( CEdCanvas* canvas, const wxPoint position, wxColour borderColor )
		{
			// Calc colors
			wxColour baseColor( m_curve->GetColor().R, m_curve->GetColor().G, m_curve->GetColor().B );
			wxColour shadowColor( baseColor.Red() * 3 / 2, baseColor.Green() * 3 / 2, baseColor.Blue() * 3 / 2 );

			// Calc curveInfoRect in Canvas space
			wxRect curveInfoRect = wxRect( canvas->ClientToCanvas( GetClientRect( position ) ) );

			// Draw rounded rect with border
			canvas->FillRoundedRect( curveInfoRect.Deflate(1), baseColor, 5 );

			for ( Uint32 i = 0; i < m_buttonsNum; ++ i )
			{
				wxRect buttonRect = wxRect( canvas->ClientToCanvas( GetButtonRect( i, position ) ) );
				buttonRect.Offset( 0, -4 );
				buttonRect.Inflate(44,4);
				
				// Making a selection rectangle if it is not filled.
				if ( !m_isButtonFilled[ i ] )
				{
					canvas->FillRoundedRect( curveInfoRect.Deflate(1), shadowColor, 5 );
				}
			}
		}

		Bool MouseClick( wxPoint point, wxPoint position )
		{
			for ( Uint32 i = 0; i < m_buttonsNum; ++i )
			{
				wxRect nRect = GetButtonRect( i, position );
				nRect.Inflate(50,2);
				nRect.Offset( +1, -4 );
				if ( nRect.Contains( point ) )
				{
					OnButtonSelected( i );
					return true;
				}
			}
			return false;
		}

		void OnButtonSelected( Uint32 buttonId )
		{
			switch ( buttonId )
			{
			case 0: m_showControlPoints = !m_showControlPoints; break;
			}
			MatchFilledButtons();
		}
	};

	struct CurveGroupInfo
	{
		String							m_curveGroupName;
		TDynArray< CurveInfo >			m_curveInfos;
		Vector							m_curveScale;
		wxColour						m_color;
		Bool							m_pinned;
		
		CurveGroupInfo( const String groupName )
		{
			m_curveGroupName = groupName;
			m_color = wxColour(50,90,90);
			m_pinned = false;
		}
		CurveGroupInfo( const CurveGroupInfo& other )
		{
			m_curveGroupName = other.m_curveGroupName;
			m_curveInfos = other.m_curveInfos;
			m_curveScale = other.m_curveScale;		
			m_color = other.m_color;
			m_pinned = other.m_pinned;
		}
		
		const CurveGroupInfo& operator=( const CurveGroupInfo& other )
		{
			m_curveGroupName = other.m_curveGroupName;
			m_curveInfos = other.m_curveInfos;
			m_curveScale = other.m_curveScale;
			return *this;
		}

		Bool operator==( const CurveGroupInfo& other ) const
		{
			return ( m_curveGroupName == other.m_curveGroupName );
		}

		TDynArray< CurveInfo >& GetCurveInfos() { return m_curveInfos; }
		const TDynArray< CurveInfo >& GetCurveInfos() const { return m_curveInfos; }

		wxRect GetClientRect( wxPoint position )
		{
			Uint32 h = 40;
			if ( m_curveInfos.Size() == 0 )
			{
				return wxRect( position.x + 0, position.y + 0, 100, h );
			}
			for ( Uint32 i = 0; i < m_curveInfos.Size(); ++ i )
			{
				h += m_curveInfos[i].GetClientRect( position ).GetHeight();
			}
			return wxRect( position.x + 0, position.y + 0, m_curveInfos[0].GetClientRect( position ).GetWidth(), h );
		}

		void SetColor( Color bckgColor )
		{
			m_color = wxColour(bckgColor.R,bckgColor.G,bckgColor.B);
		}

		void Draw( CEdCanvas* canvas, wxPoint position, wxColour borderColor )
		{
			// Calc and draw curve group rect
			wxRect curveInfoRect = wxRect( canvas->ClientToCanvas( GetClientRect( position ) ) );
			
			canvas->FillRoundedRect( curveInfoRect, m_color, 5 );
			canvas->DrawRoundedRect( curveInfoRect, borderColor, 5 );
			
			// Draw curve group name
			// We need to split the name on the : sign
			String newName = m_curveGroupName;
			newName.Replace( TXT(":"), TXT("\n") );
			
			canvas->DrawText( wxPoint( curveInfoRect.x + 8 + 0, curveInfoRect.y + 5 + 0 ), canvas->GetGdiDrawFont(), newName, borderColor, CEdCanvas::CVA_Top, CEdCanvas::CHA_Left );

			// Draw curves infos.
			position.y += 40;
			for( Uint32 i = 0; i < m_curveInfos.Size(); ++i )
			{
				m_curveInfos[i].Draw( canvas, position, borderColor );
				position.y += m_curveInfos[i].GetClientRect( position ).GetHeight();
			}
		}

		Bool MouseClick( wxPoint point, wxPoint position )
		{
			position.y += 40;
			for( Uint32 i = 0; i < m_curveInfos.Size(); ++i )
			{
				if ( m_curveInfos[i].MouseClick( point, position ) == true )
				{
					return true;
				}
				position.y += m_curveInfos[i].GetClientRect( position ).GetHeight();
			}
			return false;
		}

		Float GetTimeAdjusted( Float time ) const
		{
			return ( time * m_curveScale.Y ) + m_curveScale.X;
		}

		Float GetTimeAdjustedRevert( Float time ) const
		{
			return ( time - m_curveScale.X ) / m_curveScale.Y;
		}

		Float GetTimeAdjustedRevertScaleOnly( Float time ) const
		{
			return time / m_curveScale.Y;
		}
	};


public:
	struct ControlPointInfo
	{
		CurveGroupInfo*						m_curveGroupInfo;
		Int32								m_curveGroupInfoIndex;
		CCurveEditionWrapper::ControlPoint*	m_controlPoint;
		Int32								m_controlTangentIndex;

		ControlPointInfo()
			: m_curveGroupInfo( NULL )
			, m_curveGroupInfoIndex( -1 )
			, m_controlPoint( NULL )
			, m_controlTangentIndex( -1 )
		{
		}
		ControlPointInfo( CurveGroupInfo* curveGroupInfo, Int32 curveGroupInfoIndex, CCurveEditionWrapper::ControlPoint* controlPoint, Int32 tangentIndex )
			: m_curveGroupInfo( curveGroupInfo )
			, m_curveGroupInfoIndex( curveGroupInfoIndex )
			, m_controlPoint( controlPoint )
			, m_controlTangentIndex( tangentIndex )
		{
		}
		Bool operator==( const ControlPointInfo& other ) const
		{
			return ( m_curveGroupInfo == other.m_curveGroupInfo ) && ( m_curveGroupInfoIndex == other.m_curveGroupInfoIndex) && ( m_controlPoint == other.m_controlPoint ) && ( m_controlTangentIndex == other.m_controlTangentIndex );
		}

		CurveGroupInfo* GetCurveGroupInfo() { return m_curveGroupInfo; }
		CurveGroupInfo* GetCurveGroupInfo() const { return m_curveGroupInfo; }
		CurveInfo* GetCurveInfo() { return &(GetCurveGroupInfo()->m_curveInfos[ GetCurveGroupInfoIndex() ]); }
		CurveInfo* GetCurveInfo() const { return &(GetCurveGroupInfo()->m_curveInfos[ GetCurveGroupInfoIndex() ]); }
		CCurveEditionWrapper* GetCurve() { return GetCurveInfo()->m_curve; }
		CCurveEditionWrapper* GetCurve() const { return GetCurveInfo()->m_curve; }
		CCurveEditionWrapper::ControlPoint* GetControlPoint() { return m_controlPoint; }
		CCurveEditionWrapper::ControlPoint* GetControlPoint() const { return m_controlPoint; }
		Int32 GetCurveGroupInfoIndex() const { return m_curveGroupInfoIndex; }

		Vector GetPosition() const
		{
			Vector controlPointPosition( m_curveGroupInfo->GetTimeAdjusted(m_controlPoint->GetTime()), m_controlPoint->GetValue(), 0.0f );

			// If it's tangent control point, offset it from root control point
			if ( m_controlTangentIndex != -1 )
			{
				controlPointPosition += m_controlPoint->GetTangentValue( m_controlTangentIndex );
			}
			return controlPointPosition;
		}
		void SetPosition( const Vector& newPosition ) const
		{
			// If it's tangent control point, calc offset it from root control point
			if ( m_controlTangentIndex != -1 )
			{
				Vector controlPointPosition( m_curveGroupInfo->GetTimeAdjusted(m_controlPoint->GetTime()), m_controlPoint->GetValue(), 0.0f );
				m_controlPoint->SetTangentValue( m_controlTangentIndex, newPosition - controlPointPosition );
			}
			else
			{
				m_controlPoint->SetTime( newPosition.X );
				m_controlPoint->SetValue( newPosition.Y );
			}
		}
	};

	class CanvasRegion
	{
		wxPoint m_regionStart;
		wxPoint m_regionEnd;

	public:
		CanvasRegion( const wxPoint& startPoint = wxPoint(0,0), const wxPoint& endPoint = wxPoint(0,0) )
			: m_regionStart( startPoint )
			, m_regionEnd( endPoint )
		{
		}
		void	SetStartAndEndPoint( const wxPoint& point ) { m_regionStart = m_regionEnd = point; }
		wxPoint GetStartPoint() const { return m_regionStart; }
		void	SetStartPoint( const wxPoint& startPoint ) { m_regionStart = startPoint; }
		wxPoint GetEndPoint() const	{ return m_regionEnd; }
		void	SetEndPoint( const wxPoint& endPoint )	{ m_regionEnd = endPoint; }
		
		wxRect	GetRect() const
		{
			wxRect regionRect;
			regionRect.x = Min( m_regionStart.x, m_regionEnd.x );
			regionRect.y = Min( m_regionStart.y, m_regionEnd.y );
			regionRect.width  = Abs( m_regionEnd.x - m_regionStart.x );
			regionRect.height = Abs( m_regionEnd.y - m_regionStart.y );
			return regionRect;
		}
	};

protected:
	enum EMouseAction
	{
		MA_None,
		MA_Zooming,
		MA_MovingControlPoints,
		MA_MovingSnappingLine,
		MA_BackgroundScroll,
		MA_SelectingWindows,
	};
	EMouseAction					m_action;				// Action actually performed

protected:
	TDynArray< CurveGroupInfo >		m_curveGroups;
	TDynArray< String >				m_curveGroupsPinned;

	TDynArray< ControlPointInfo >	m_controlPointInfosSelected;
	TDynArray< ControlPointInfo >	m_controlPointInfosMouseOver;

	// Horizontal or vertical snapping lines
	TDynArray< Float >				m_snappingLines;
	Int32							m_snappingLineSelectedIndex;

	// Active region (region where control points are valid, only x component is used )
	Bool							m_activeRegionShow;
	Float							m_activeRegionStart;
	Float							m_activeRegionEnd;

	// Selection region (rect when selecting control points)
	CanvasRegion					m_selectionRegion;

	// Global options / switches
	Bool							m_showTangents;			// Global switch to show/hide tangents
	Bool							m_showControlPoints;	// Global switch to show/hide control points
	Bool							m_showSnappingLines;	// Global switch to show/hide snapping lines
	Bool							m_xAxisMoveEnabled; //<! if false, then moving in x axis is locked
	Bool							m_yAxisMoveEnabled; //<! if false, then moving in y axis is locked

	// Global states
	Vector							m_scaleCurves;			// Actual scale (used when painting canvas panel)

protected:
	Int32							m_moveTotal;			// Distance moved when right mouse button was pressed
	wxPoint							m_autoScroll;			// Used for autoscroll when mouse is near edge of canvas panel
	Vector							m_lastMousePosition;	// Used when dynamically scaling canvas via mouse move
	CEdCurveEditor*					m_editor;				//<! pointer to parent editor
	TDynArray< CObject* >			m_curveContainers;

	IEdCurveEditorCanvasHook*		m_hook;
public:
	CEdCurveEditorCanvas( wxWindow *parentWin );
	~CEdCurveEditorCanvas();

	void SetHook( IEdCurveEditorCanvasHook* hook );
	void SetParentCurveEditor( CEdCurveEditor* editor) { m_editor = editor; }

protected:
	Vector  CanvasToCurve( wxPoint point ) const;
	wxPoint CurveToCanvas( const Vector&  point ) const;
	Vector  ClientToCurve( wxPoint point ) const;
	wxPoint CurveToClient( const Vector&  point ) const;

	void	DrawLine( Float x1, Float y1, Float x2, Float y2, const wxColour& color, Float width = 1.0f );
	void	DrawLine( const Vector& start, const Vector& end, const wxColour& color, Float width = 1.0f );
	void	DrawText( const Vector& offset, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign );
	void	DrawText( const Float x, const Float y, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign );

	// Draw vertical or horizontal line (from one edge of panel to other edge)
	void	DrawVLine( Float x, const wxColour& color, Float width = 1.0f );
	void	DrawHLine( Float y, const wxColour& color, Float width = 1.0f );

protected:
	Float	SnapFloat( const Float& floatToSnap ) const;

	// Scroll background, used by autoscroll
	virtual void ScrollBackgroundOffset( wxPoint delta );

	// Called when selection changed, it signal other parts of editor (for ex. to update UI in Curve Editor)
	void	SelectionChanged();

	// Called when some control points changed, it signal other parts of editor (for ex. to update Gradient Editor with new Curve's values)
	void	ControlPointsChanged();

	// Called when a control point change is completed (user drags, and then releases), signals other parts of editor
	void	ControlPointsChangedComplete();

public:
	CCurveEditionWrapper*	AddCurve( SCurveData* curve, const String& curveName, const Vector &curveScale = Vector(0,1.0,0), CObject* curveContainer = NULL, const Color& color = Color( 255, 255, 255 ) );
	CCurveEditionWrapper*	AddCurve( CCurve* curve, const String& curveName, const Vector &curveScale = Vector(0,1.0,0), const Color& color = Color( 255, 255, 255 ) );
	void	RemoveCurve( SCurveData* curve );
	TDynArray< CCurveEditionWrapper* >	AddCurveGroup( CurveParameter* curve, const Color& bckgColor, const Vector &curveScale = Vector(0,1.0,0), const Color& color = Color( 255, 255, 255 ) );
	TDynArray< CCurveEditionWrapper* >	AddCurveGroup( CurveParameter* curve, const String &moduleName, const Color& bckgColor, Bool pinned = false, const Vector &curveScale = Vector(0,1.0,0), const Color& color = Color( 255, 255, 255 ) );
	void	RemoveCurveGroup( const CName& curveGroupName );
	void	RemoveAllCurveGroups();
	void	RemoveAllCurves();
	Bool	IsCurveGroupAdded( const CName& curveGroupName ) const;
	Bool	IsCurveAdded( const SCurveData* curve) const;

	// Within active region control points have valid time and value, it's used to snap control points to some region (for ex. [0-1])
	void	SetActiveRegion( Float start, Float end );
	void	GetActiveRegion( Float& start, Float& end ) const;

	// Zoomed region is the region where Curve Editor Canvas is actually zoomed
	Vector	GetZoomedRegion() const;
	virtual void SetZoomedRegion( const Vector& corner1, const Vector& corner2 );
	void	SetZoomedRegionToFit();

	// Set global options
	void	SetShowTangents( Bool showTangents ) { m_showTangents = showTangents; }
	void	SetShowControlPoints( Bool showControlPoints ) { m_showControlPoints = showControlPoints; }
	void	SetShowSnappingLines( Bool showSnappingLines ) { m_showSnappingLines = showSnappingLines; }
	void	SetXIsUnlocked( const Bool& xAxisMoveEnabled ) { m_xAxisMoveEnabled = xAxisMoveEnabled; }
	void	SetYIsUnlocked( const Bool& yAxisMoveEnabled ) { m_yAxisMoveEnabled = yAxisMoveEnabled; }

protected:
	// Internally used to operate on many control points
	void	ControlPointsFromCurve( CurveGroupInfo* curveGroupInfo, Int32 curveGroupIndex, Bool createPoints, Bool createTangents, TDynArray< ControlPointInfo >& outControlPoints ) const;
	Bool	ControlPointsNearPoint( TDynArray< ControlPointInfo >& controlPoints, const wxPoint& clientPoint, const Int32 radiusSqr, ControlPointInfo& outControlPoint ) const;
	Int32		ControlPointsInArea( TDynArray< ControlPointInfo >& controlPoints, const wxRect& canvasRect, TDynArray< ControlPointInfo >& outControlPoints ) const;
	void	ControlPointsMove( TDynArray< ControlPointInfo >& controlPoints, Vector& delta );

public:
	// Used by Curve Editor to update UI (when signaled that selection changed)
	TDynArray< ControlPointInfo >& GetSelection() {	return m_controlPointInfosSelected; }

	// Get curve objects, currently being selected
	void GetSelectedCurves( TDynArray< CCurveEditionWrapper* >& curves );
		 
	// Called by Curve Editor to move some control points (to absolute or relative time/value)
	void	MoveSelectedControlPoints( const Float time, const Float value, Bool updateTime, Bool updateValue, Bool absoluteMove );

	// Info methods
	Int32           GetSidePanelWidth() const;
	Int32           GetSidePanelHeight() const;

	void SetCurveScale( const Vector& scale ) { m_scaleCurves = scale; }
	Vector GetCurveScale() const { return m_scaleCurves; }

	Bool UpdateCurveParam( SCurveData* curve, const Vector &curveScale );

protected:
	virtual void PaintCanvas( Int32 width, Int32 height );

	// Low level stuff
	void		DrawGrid( Int32 width, Int32 height, Bool drawLines, Bool drawText );
	void		DrawSnapLines( Bool drawLines, Bool drawText );
	void		DrawControlPoint( wxPoint point, const wxColour& borderColor, const wxColour &baseColor );
	void		DrawTangentPoint( wxPoint point, const wxColour& borderColor, const wxColour &baseColor );
	void		DrawCurveInfo( String name, const wxRect& curveInfoRect, const wxColour& baseColor, const wxColour& borderColor );
	void		DrawControlPoints( const TDynArray< ControlPointInfo >& controlPoints, const wxColour& borderColor, const wxColour &baseColor, Bool drawControlPoints, Bool drawTangents );

	// High level stuff
	//void		DrawCurve( const CurveGroupInfo *curveGroupInfo, CCurve* curve, const wxColour& color, Bool drawControlPoints, Bool drawTangentPoints );
	void		DrawSampledCurve( const CurveGroupInfo *curveGroupInfo, CCurveEditionWrapper* curve, const Int32 numSamples, wxColour color );
	void		DrawCurveInfo( const CurveInfo& curveInfo, Int32 index, wxColour borderColor );

	// Mouse events
	Bool		 IsMouseOverCanvasPanel( const wxPoint& mousePosition ) const;
	virtual void MouseClick( wxMouseEvent& event );
	virtual void MouseClickSidePanel( wxMouseEvent& event );
	virtual void MouseClickCanvasPanel( wxMouseEvent& event );
	virtual void MouseMove( wxMouseEvent& event, wxPoint delta );

	// Dispatch system event
	virtual void DispatchEditorEvent( const CName& systemEvent, IEdEventData* data );

protected:
	// Context menus
	void OpenControlPointContextMenu();

	// Events from context menus
	void OnSetTangentType( wxCommandEvent& event );

	// Events
	void OnMouseLeftDoubleClick( wxMouseEvent &event );
	void OnMouseWheel( wxMouseEvent& event );
	void OnKeyDown( wxKeyEvent &event );

	void OnColorPicked( wxCommandEvent& event );
};
