/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "spinSliderControl.h"

/// Advanced color picker
class CEdAdvancedColorPicker : public wxFrame
{
	DECLARE_EVENT_TABLE();

public:
	// -----------------------------------
	enum { NUM_CUSTOM_COLORS = 30 };
	// -----------------------------------
	struct SCustomColor
	{
		SCustomColor ()
		{
			Float v = 240.f / 255.f;
			rgb.Set3( v, v, v );
		}

		Vector rgb;
	};
	// -----------------------------------

protected:	
	static SCustomColor	s_customColors[NUM_CUSTOM_COLORS];

protected:
	Float		m_hsl[3];
	Float		m_rgb[3];
	Color		m_color;
	Vector		m_comparisonRefColor;

	Int32			m_currCustomColorIndex;
	wxPanel*	m_customColorPanels[NUM_CUSTOM_COLORS];

	wxImage		m_imageMainPanel;
	Float		m_imageMainPanelLightness;

	wxPanel*				m_panel;
	wxPanel*				m_panelMain;
	wxPanel*				m_panelRed;
	wxPanel*				m_panelGreen;
	wxPanel*				m_panelBlue;
	wxPanel*				m_panelHue;
	wxPanel*				m_panelSaturation;
	wxPanel*				m_panelLightness;
	wxPanel*				m_panelWhiteness;
	wxPanel*				m_panelColorComparison;
	CEdSpinSliderControl	m_red;
	CEdSpinSliderControl	m_green;
	CEdSpinSliderControl	m_blue;
	CEdSpinSliderControl	m_hue;
	CEdSpinSliderControl	m_saturation;
	CEdSpinSliderControl	m_lightness;

	Bool					m_isWindowed;
	String					m_lastUsedPalettePath;

public:
	CEdAdvancedColorPicker( wxWindow* parent, Color initialColor, Bool isWindowed = false );
	~CEdAdvancedColorPicker();

public:
	RED_INLINE Vector GetVectorRGB( bool normalized ) const { return Vector (m_rgb[0], m_rgb[1], m_rgb[2], 1.f) * (normalized ? 1.f : 255.f); }
	RED_INLINE Vector GetVectorHSL()					const { return Vector (m_hsl[0], m_hsl[1], m_hsl[2], 1.f); }
	RED_INLINE Color  GetColor()						const { return m_color; }

public:
	static Uint32		GetNumCustomColors();
	static Vector	GetCustomColor( Uint32 customColorIndex );
	static void		SetCustomColor( Uint32 customColorIndex, const Vector &color );

public:
	void SetColorRGB( const Vector &rgb, bool normalized );
	void SetColorHSL( const Vector &hsl );
	void SetColor( const Color &color );

	void SetReferenceColor( const Color &color );

	const String& GetLastPaletteFilePath() { return m_lastUsedPalettePath; }
	Bool LoadPalette( const String &palettePath );
	
protected:
	bool SetSelectedColorRGB( const Vector &rgb, bool normalized );
	bool SetSelectedColorHSL( const Vector &hsl );
	bool SetSelectedColor( const Color &color );
	void PostSelectedColorUpdate();
	void UpdateTextControlsValues();
	void UpdateImages();

protected:
	void OnActivate( wxActivateEvent& event );
	void OnEraseBackground( wxEraseEvent& event );
	void OnPaint( wxPaintEvent& event );
	void OnColorSelectionChanged( wxMouseEvent& event );
	void OnColorValueChangedRGB( wxCommandEvent& event );
	void OnColorValueChangedHSL( wxCommandEvent& event );
	void OnCustomColorSelect( wxMouseEvent& event );
	void OnCustomColorSet( wxCommandEvent& event );
	void OnCustomColorGet( wxCommandEvent& event );
	void OnLoadPalette( wxCommandEvent& event );
	void OnSavePalette( wxCommandEvent& event );
};