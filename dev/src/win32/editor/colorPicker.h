/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "spinSliderControl.h"

/// Simple color picker
class CEdColorPicker : public wxFrame
{
	DECLARE_EVENT_TABLE();

protected:
	enum EDragMode
	{
		DM_None,
		DM_Color,
		DM_Lightness,
	};

protected:
	Float		m_hsl[3];
	Float		m_rgb[3];
	Float		m_a;
	Color		m_color;
	Color		m_initialColor;
	wxImage		m_backImage;
	EDragMode	m_dragMode;
	wxPanel*    m_panel;
	wxColor		m_bgColor;

	CEdSpinSliderControl	m_red;
	CEdSpinSliderControl	m_green;
	CEdSpinSliderControl	m_blue;
	CEdSpinSliderControl	m_alpha;

public:
	CEdColorPicker( wxWindow* parent );
	~CEdColorPicker();

	void Show( Color initialColor );

	RED_INLINE Color GetColor() const { return m_color; }

protected:
	void OnActivate( wxActivateEvent& event );
	void OnEraseBackground( wxEraseEvent& event );
	void OnPaint( wxPaintEvent& event );
	void OnMouseClick( wxMouseEvent& event );
	void OnMouseMove( wxMouseEvent& event );

protected:
	void DrawColorCircle( wxImage &image, Int32 x, Int32 y, Int32 radius );
	void DrawLightRing( wxImage &image, Int32 x, Int32 y, Int32 radiusInner, Int32 radiusOuter );
	wxRect CalcColorDragRect();
	wxRect CalcLightnessDragRect();
	void OnColorRedChanged( wxCommandEvent &event );
	void OnColorGreenChanged( wxCommandEvent &event );
	void OnColorBlueChanged( wxCommandEvent &event );
	void OnColorAlphaChanged( wxCommandEvent &event );

	void UpdateColor();
	bool ClickedColor( wxMouseEvent& event );
	bool ClickedLightness( wxMouseEvent& event, bool exact = true );

private:
	void AfterRGBSpinSliderChange();
};

/// Convert HSL to RGB (normalized values)
void HSLToRGB( Float hue, Float saturation, Float lightness, Float& r, Float& g, Float& b );

/// Convert RGB to HSL (normalized values)
void RGBToHSL( Float r, Float g, Float b, Float &hue, Float &saturation, Float &lightness );
