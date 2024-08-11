/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


enum ECurveDrawerType
{
	CDT_Scalar,
	CDT_Color
};


enum ECurveEditControlPointType
{
	CPT_None,		//<! Not a valid control point (could mean no selection)
	CPT_Value,		//<! Regular value control point
	CPT_Tangent		//<! Tangent control point (for Float-valued segmented curves)
};

struct SCurveEditControlPoint
{
	ECurveEditControlPointType m_type;
	Int32 m_index;						//<! Index of the CP. Identifies it within the curve.
	wxPoint m_position;					//<! Position on the drawer's canvas of either the control point, or where the user clicked (if nothing selected)
	SCurveBase* m_curve;				//<! The curve the CP belongs to.

	SCurveEditControlPoint()
	{
		m_type = CPT_None;
	}

	Bool operator==( const SCurveEditControlPoint& point2 )
	{
		if( this->m_index == point2.m_index &&
			this->m_curve == point2.m_curve )
		{
			return true;
		}
		return false;
	}
};


struct SCurveEditDrawerSettings
{
	Bool m_enableCurve;
	Bool m_enableGrid;
	Bool m_enableGridMetrics;
	Bool m_enableCurrentTimePreview;
	Bool m_enableKeypoints;

	Float m_valueRangeMin;
	Float m_valueRangeMax;
};


class CCurveEditDrawer
{
public:
	static const Float TIME_MAXIMUM;

	Float m_displayAreaTimeMin;
	Float m_displayAreaTimeMax;
	Float m_displayAreaValueMin;
	Float m_displayAreaValueMax;

	SCurveEditDrawerSettings m_settings;

	wxDC* m_dc;
	wxRect m_fullRect;
	wxRect m_graphRect;

	SCurveBase* m_curve;

protected:
	SCurveEditControlPoint	m_currentMouseOver;
	SCurveEditControlPoint	m_currentSelection;
	Bool					m_selectionActive;	//<! If true, the current selection is actively selected (mouse down)

	Bool					m_isSelectionAreaActive;
	wxPoint					m_selectionAreaStartPoint;
	wxPoint					m_selectionAreaEndPoint;

	TDynArray< SCurveEditControlPoint > m_selectedPoints;

public:
	CCurveEditDrawer();
	virtual ~CCurveEditDrawer();

	virtual void Reset( const SCurveEditDrawerSettings& settings );

	// Set the DC and rectangle where all drawing will be done.
	virtual void SetFullRect( const wxRect& fullRect );
	void SetDC( wxDC& dc );

	void SetCurve( SCurveBase* curve );

	virtual void Draw() const = 0;

	Bool TransformScalarPoint( Float x, Float y, wxPoint& outPoint, Bool allowOutOfRange = false ) const;
	Bool TransformScalarPointInverse( const wxPoint& point, Float& outX, Float& outY, Bool allowOutOfRange = false ) const;

	virtual bool	IsInsideColorPickArea( const wxPoint& pos ) const = 0;
	virtual bool	GenerateNewControlPoint( const wxPoint& pos, Float& outTime, Vector& outValue ) const = 0;
	virtual wxRect	GetGraphRect( const wxRect &fullRect ) const = 0;
	virtual bool	HasVerticalDependency() const = 0;

	virtual ECurveDrawerType GetDrawerType() const = 0;


	void PanView( const wxPoint& mouseDelta, Bool constrainX, Bool constrainY, PropertiesPageDayCycleSettings* dayCycleSettings = NULL );
	void ZoomView( const wxPoint& center, Float scale, Bool constrainX, Bool constrainY, PropertiesPageDayCycleSettings* dayCycleSettings = NULL );

	// Copies the current selection to the clipboard, if possible. Return true on success.
	Bool CopySelection() const;
	// Pastes a new control point from the clipboard, selects that point. Return true on success.
	Bool PasteControlPoints( const wxPoint& pos );
	// Remove selected points from curve
	Bool RemoveControlPoints();


	void MouseOverControlPoint( const wxPoint& pos );
	// Begin selecting a control point. This is generally done on mouse down. Returns true if a point was selected.
	Bool BeginSelectControlPoint( const wxPoint& pos, Bool multiselect );
	// End selecting a control point. This is done on mouse up.
	void EndSelectControlPoint( const wxPoint& pos );
	// Clear current selection.
	void ClearSelection();
	// Move the currently selected control point (if one is selected) to the given position (in the same coordinates as FullRect).
	virtual void MoveSelection( const wxPoint& pos ) = 0;
	// Explicitly set the selected value control point. No change for out-of-bounds index.
	void SetSelection( Uint32 index );
	// Delete current selected point
	void DeleteCurrentSelectedPoint();

	// Change distance between current selected point and all other selected points
	void AddDistanceMultipier( Int32 value );

	Bool AreaSelectionActive() const { return m_isSelectionAreaActive; }
	void UpdateSelectionArea( const wxPoint& pos );

	const SCurveEditControlPoint& GetCurrentSelection() const { return m_currentSelection; }
	const TDynArray< SCurveEditControlPoint >& GetSelectedPoints() const { return m_selectedPoints; }

	Bool HasSelection() const { return m_currentSelection.m_type != CPT_None; }
	Bool HasActiveSelection() const { return HasSelection() && m_selectionActive; }
	Bool IsPointSelected( Uint32 index ) const;

protected:
	virtual Int32 FindNearestEntry( const wxPoint& pt, Int32 *outDist, Bool *outIsTangent ) = 0;
	virtual Bool IsPosOnControlPoint( const wxPoint &pos, const SCurveEditControlPoint &controlPoint ) const = 0;
	virtual void FillControlPointPosition( SCurveEditControlPoint& point ) const = 0;

	void DrawLine( const wxPoint& pt1, const wxPoint& pt2, const wxColour& colour, Int32 width = 1 ) const;
	void DrawFilledRectangle( const wxRect& rect, const wxColour& colour ) const;
	void DrawRectangle( const wxRect& rect, const wxColour& colour, Int32 lineWidth = 1 ) const;
	void DrawFrameRectangle( const wxRect& rect, const wxColour& fillColour, const wxColour& frameColour, Int32 lineWidth = 1 ) const;
	void DrawFilledCircle( const wxPoint& c, Int32 radius, const wxColour& colour ) const;
	void DrawCircle( const wxPoint& c, Int32 radius, const wxColour& colour, Int32 lineWidth = 1 ) const;
	void DrawFrameCircle( const wxPoint& c, Int32 radius, const wxColour& fillColour, const wxColour& frameColour, Int32 lineWidth = 1 ) const;

	void DrawTimePreview( Float time ) const;
	void DrawTimeSelection( Float minSelection, Float maxSelection ) const;
	wxRect DrawGrid( bool enableMetricsBar, const wxColour& colorTransparent ) const;
	wxRect DrawGridTime( bool enableMetricsBar, const wxColour& colorTransparent ) const;
	wxRect DrawGridNum( Bool enableMetricsBar, const wxColour& colorTransparent, Bool isUnit ) const;

	void UpdateSelectedPointsPosition();
};


class CCurveEditDrawerScalar : public CCurveEditDrawer
{
public:
	wxColour	m_backgroundColorActive;
	wxColour	m_backgroundColorInactive;
	wxColour	m_backgroundTypesBorderColor;
	wxColour	m_lineColor;
	wxColour	m_keyPointsColor;
	wxColour	m_keyPointsColorSelected;
	wxColour	m_keyPointsColorLastSelected;
	wxColour	m_keyPointsBorder;
	wxColour	m_keyPointsBorderMouseOver;
	wxColour	m_keyPointsLastSelectedBorder;
	Float		m_keyPointsRadius;
	wxColour	m_lineColorRangeExceeded;
	wxColour	m_keyPointsBorderRangeExceeded;


public:
	CCurveEditDrawerScalar();

	virtual void Reset( const SCurveEditDrawerSettings& settings );

	virtual void Draw() const;

	virtual wxRect GetGraphRect( const wxRect& fullRect ) const { return fullRect; }
	virtual bool HasVerticalDependency() const { return true; }
	virtual bool IsInsideColorPickArea( const wxPoint& pos ) const { return false; }
	virtual bool GenerateNewControlPoint( const wxPoint& pos, Float& outTime, Vector& outValue ) const;

	virtual ECurveDrawerType GetDrawerType() const { return CDT_Scalar; }

	virtual void MoveSelection( const wxPoint& pos );


protected:
	virtual Int32 FindNearestEntry( const wxPoint& pt, Int32* outDist, Bool* outIsTangent );
	virtual Bool IsPosOnControlPoint( const wxPoint& pos, const SCurveEditControlPoint& controlPoint ) const;
	virtual void FillControlPointPosition( SCurveEditControlPoint& point ) const;

	Int32 GetKeyPointsRadius() const;

	Bool ShouldDrawTangent( Uint32 ctlIdx, Uint32 tanIdx ) const;

	void DrawControlPoint( const wxPoint& point, const wxColour& borderColor, const wxColour& baseColor ) const;
	void DrawTangentPoint( const wxPoint& point, const wxColour& borderColor, const wxColour& baseColor ) const;

	void GetControlPointColour( Uint32 idx, wxColour& base, wxColour& border ) const;
	void GetTangentPointColour( Uint32 idx, wxColour& base, wxColour& border ) const;
};


class CCurveEditDrawerColor : public CCurveEditDrawer
{
public:
	wxColour	m_backgroundColorActive;
	wxColour	m_backgroundColorInactive;
	Int32			m_keysWidth;
	Int32			m_keysHeight;
	Int32			m_keysBarHeight;
	wxColour	m_keysBarBackColor;
	wxColour	m_keysBorderColor;
	wxColour	m_keysBorderColorSelected;
	wxColour	m_keysBorderColorLastSelected;

	wxRect		m_keysBarRect;

public:
	CCurveEditDrawerColor();

	virtual void Reset( const SCurveEditDrawerSettings& settings );

	virtual void SetFullRect( const wxRect& fullRect );

	virtual void Draw() const;

	virtual wxRect GetGraphRect( const wxRect& fullRect ) const;
	virtual bool HasVerticalDependency() const { return false; }
	virtual bool IsInsideColorPickArea( const wxPoint& pos ) const;
	virtual bool GenerateNewControlPoint( const wxPoint& pos, Float& outTime, Vector& outValue ) const;

	virtual ECurveDrawerType GetDrawerType() const { return CDT_Color; }

	virtual void MoveSelection( const wxPoint& pos );

protected:
	virtual Int32 FindNearestEntry( const wxPoint& pt, Int32* outDist, Bool* outIsTangent );
	virtual Bool IsPosOnControlPoint( const wxPoint& pos, const SCurveEditControlPoint& controlPoint ) const;
	virtual void FillControlPointPosition( SCurveEditControlPoint& point ) const;

	bool CalcTriangleKeyTopVertex( Float time, wxPoint& outPoint ) const;
};
