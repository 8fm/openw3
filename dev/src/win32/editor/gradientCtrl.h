/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#define GRADIENT_GRID_SIZE		6
#define GRADIENT_TICKER_HEIGHT	6

#include "../../win32/editor/curveEditionWrapper.h"

class CEdColorPicker;
class CEdGradientPicker : public wxControl
{
	DECLARE_EVENT_TABLE();

protected:
	class ControlPointInfo
	{
		TDynArray< CCurveEditionWrapper::ControlPoint* > m_controlPoints;

	public:
		ControlPointInfo()
		{
		}
		const ControlPointInfo& operator=( const ControlPointInfo& other )
		{
			m_controlPoints = other.m_controlPoints;
			return *this;
		}

		Bool operator==( const ControlPointInfo& other )
		{
			return m_controlPoints == other.m_controlPoints;
		}

		TDynArray< CCurveEditionWrapper::ControlPoint* >& GetControlPoints()
		{
			return m_controlPoints;
		}

		void PushBack( CCurveEditionWrapper::ControlPoint* controlPoint )
		{
			m_controlPoints.PushBack( controlPoint );
		}
		wxRect GetClientRect( const Int32 width )
		{
			Float fraction = m_controlPoints[0]->GetTime();
			Int32 dx = fraction * (width-1);
			Int32 xStart = Max( dx - GRADIENT_TICKER_HEIGHT , 0 );
			Int32 xEnd   = Min( dx + GRADIENT_TICKER_HEIGHT + 1, width-1 );
			wxRect rect( xStart, 0, xEnd-xStart, GRADIENT_TICKER_HEIGHT );
			return rect;
		}
		Float GetTime() const
		{
			return m_controlPoints[0]->GetTime();
		}
		void SetTime( const Float time )
		{
			for( Uint32 i = 0; i < m_controlPoints.Size(); ++i )
			{
				m_controlPoints[i]->SetTime( time );
			}
		}
		void SetValue( const Int32 index, const Float value )
		{
			m_controlPoints[index]->SetValue( value );
		}
	};

	TDynArray< ControlPointInfo > m_controlPointsMouseOver;
	TDynArray< ControlPointInfo > m_controlPointsSelected;

public:
	enum EUpdateMode
	{
		UM_Color,
		UM_Alpha,
		UM_Both
	};

	enum EDisplayMode
	{
		DM_Color,
		DM_Alpha,
		DM_Both
	};

protected:
	enum EDragMode
	{
		DM_None,
		DM_MoveSelected,
	};

	CCurveEditionWrapper*			m_curve[4];
	wxImage			m_backImage;
	wxPoint			m_lastMousePosition;

	EDragMode		m_dragMode;
	EUpdateMode		m_updateMode;
	EDisplayMode	m_displayMode;

	CEdColorPicker*	m_ctrlColorPicker;

public:
	CEdGradientPicker();
	~CEdGradientPicker();

	void SetCurves( SCurveData* curveR, SCurveData* curveG, SCurveData* curveB, SCurveData* curveA );
	void SetUpdateMode( EUpdateMode updateMode ) { m_updateMode = updateMode; }
	void SetDisplayMode( EDisplayMode displayMode ) { m_displayMode = displayMode; }
	void ClearSelection() { m_controlPointsSelected.Clear(); }

	void ForceAlphaToColor();
	Bool CheckAlphaToColor();

protected:
	void OnEraseBackground( wxEraseEvent& event ) {};
	void OnSize( wxSizeEvent& event );
	void OnPaint( wxPaintEvent& event );
	void OnKeyDown( wxKeyEvent &event );
	void OnMouseMove( wxMouseEvent& event );
	void OnMouseClick( wxMouseEvent& event );
	void OnMouseLeftDoubleClick( wxMouseEvent &event );

	void ColorOpenBrowser( const Color& color );
	void OnColorPicked( wxCommandEvent& event );

	void ControlPointsChanged();

	void MouseCapture();
	void MouseRelease();

protected:
	void ControlPointsFromCurves( TDynArray< ControlPointInfo >& outControlPoints ) const;
	void DrawCursor( wxImage &image, const Float fraction, const Color& color );
	void DrawGradient( wxImage &image );
	void DrawTickers( wxImage &image );
};