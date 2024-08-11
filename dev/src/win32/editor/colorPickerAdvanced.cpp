/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include <wx/display.h>

#define ADV_PICKER_WIDTH	850
#define ADV_PICKER_HEIGHT	460

BEGIN_EVENT_TABLE( CEdAdvancedColorPicker, wxFrame )
	EVT_PAINT( CEdAdvancedColorPicker::OnPaint )
	EVT_ACTIVATE( CEdAdvancedColorPicker::OnActivate )
	EVT_ERASE_BACKGROUND( CEdAdvancedColorPicker::OnEraseBackground )
	EVT_BUTTON( XRCID("btnCustomSet"), CEdAdvancedColorPicker::OnCustomColorSet )
	EVT_BUTTON( XRCID("btnCustomGet"), CEdAdvancedColorPicker::OnCustomColorGet )
	EVT_BUTTON( XRCID("btnLoadPalette"), CEdAdvancedColorPicker::OnLoadPalette )
	EVT_BUTTON( XRCID("btnSavePalette"), CEdAdvancedColorPicker::OnSavePalette )
END_EVENT_TABLE()

// --------------------------------------------------------------------------
// Utility functions

namespace
{
	wxPoint CalcAdvancedPickerPosition()
	{
		POINT p;
		::GetCursorPos( &p );
		wxPoint ret = wxPoint ( p.x, p.y );

		int w = 0, h = 0;
		::wxDisplaySize( &w, &h );
		w = 0;
		for( Int32 i=0; i<(Int32)wxDisplay::GetCount(); i++ )
		{
			wxDisplay d( i );
			w += d.GetGeometry().GetWidth();
		}

		ret.x = max(0, min(ret.x, w - ADV_PICKER_WIDTH));
		ret.y = max(0, min(ret.y, h - ADV_PICKER_HEIGHT));
		return ret;
	}

	void DrawColorBar( wxPanel *panel, const Vector &valueStart, const Vector &valueEnd, bool horizontal, bool hsl, Float displayValueFactor )
	{
		wxRect rect = wxRect ( wxPoint (0, 0), panel->GetSize() );
		if ( rect.IsEmpty() )
		{
			return;
		}

		wxPaintDC dc ( panel );

		const Uint32	  linesCount	= horizontal ? rect.width : rect.height;
		const wxPoint linePosDelta	= horizontal ? wxPoint(1,0)				: wxPoint(0,1);
		const wxPoint lineDisplace	= horizontal ? wxPoint(0, rect.height)	: wxPoint(rect.width, 0);
		const Vector  valueDelta	= (valueEnd - valueStart) / Max(0.f, (Float)(linesCount - 1));

		Vector	currValue = valueStart;
		wxPoint	currPoint = rect.GetTopLeft();
		for ( Int32 linesLeft=(Int32)linesCount; linesLeft>0; --linesLeft )
		{
			Vector penColorVec = currValue;
			if ( hsl )
			{
				HSLToRGB( currValue.X, currValue.Y, currValue.Z, penColorVec.X, penColorVec.Y, penColorVec.Z );
			}
			penColorVec *= 255.f;
			wxColour penColor = wxColour( (Uint8)penColorVec.X, (Uint8)penColorVec.Y, (Uint8)penColorVec.Z, 255 );
			
			dc.SetPen( wxPen( penColor ) );
			dc.DrawLine( currPoint, currPoint + lineDisplace );
			currPoint += linePosDelta;			
			currValue += valueDelta;
			currValue.X = Clamp( currValue.X, 0.f, 1.f );
			currValue.Y = Clamp( currValue.Y, 0.f, 1.f );
			currValue.Z = Clamp( currValue.Z, 0.f, 1.f );
		}		

		wxColour colorInside (222, 60, 240);
		wxColour colorBorder (  0,  0,  0);
		Int32		displayLine		= (Int32)(displayValueFactor * (linesCount - 1));
		wxPoint displayCorner	= wxPoint ( displayLine * linePosDelta.x, displayLine * linePosDelta.y ) - linePosDelta;
		wxSize  displaySize		= wxSize ( lineDisplace.x + 3 * linePosDelta.x, lineDisplace.y + 3 * linePosDelta.y );
		dc.SetPen( wxPen( colorBorder, 1, wxSOLID ) );
		dc.SetBrush( wxBrush( colorInside ) );
		dc.DrawRectangle( displayCorner, displaySize );
	}

	void DrawRGBBar( wxPanel *panel, const Vector &valueStart, const Vector &valueEnd, bool horizontal, Float displayValueFactor )
	{
		DrawColorBar( panel, valueStart, valueEnd, horizontal, false, displayValueFactor );
	}

	void DrawHSLBar( wxPanel *panel, const Vector &valueStart, const Vector &valueEnd, bool horizontal, Float displayValueFactor )
	{
		DrawColorBar( panel, valueStart, valueEnd, horizontal, true, displayValueFactor );
	}

	void DrawMainPanelImage( wxImage &bitmap, Float lightness )
	{
		if ( !bitmap.IsOk() || bitmap.GetWidth() < 1 || bitmap.GetHeight() < 1 )
		{
			return;
		}

		Int32 width = bitmap.GetWidth();
		Int32 height = bitmap.GetHeight();
		Float scale_i = 1.f / Max( 1.f, (width - 1.f) );
		Float scale_j = 1.f / max( 1.f, (height - 1.f) );
		for ( Int32 i=0; i<width;  ++i )
		for ( Int32 j=0; j<height; ++j )
		{
			Float r, g, b;
			Float hue = i * scale_i;
			Float saturation = 1.f - j * scale_j;
			HSLToRGB( hue, saturation, lightness, r, g, b );
			bitmap.SetRGB( i, j, 255.f * r, 255.f * g, 255.f * b );
		}
	}

	void DrawMainPanel( wxPanel *panel, wxImage &bitmap, Float hue, Float saturation, Float lightness )
	{
		wxSize size = panel->GetSize();
		if ( !bitmap.IsOk() || size.GetWidth() < 2 || size.GetHeight() < 2 )
		{
			return;
		}

		wxPaintDC dc ( panel );

		wxBitmap blitBitmap( bitmap, 24 );
		dc.DrawBitmap( blitBitmap, 0, 0 );

		wxColour colorInside (222, 60, 240);
		wxColour colorBorder (  0,  0,  0);
		wxPoint  center ( (Int32)(hue * (size.GetWidth()-1)), (Int32)((1.f - saturation) * (size.GetHeight() - 1)) );
		for ( Uint32 pass_i=0; pass_i<2; ++pass_i )
		{
			Int32 size = 5;
			dc.SetPen( 0==pass_i ? wxPen ( colorBorder, 3 ) : wxPen ( colorInside, 1 ) );
			dc.DrawLine( center.x - size, center.y - size, center.x + size, center.y - size );
			dc.DrawLine( center.x + size, center.y - size, center.x + size, center.y + size );
			dc.DrawLine( center.x + size, center.y + size, center.x - size, center.y + size );
			dc.DrawLine( center.x - size, center.y + size, center.x - size, center.y - size );
		}
	}
};

// --------------------------------------------------------------------------
// CEdAdvancedColorPicker custom colors

CEdAdvancedColorPicker::SCustomColor CEdAdvancedColorPicker::s_customColors[NUM_CUSTOM_COLORS];

// --------------------------------------------------------------------------
// CEdAdvancedColorPicker

CEdAdvancedColorPicker::CEdAdvancedColorPicker( wxWindow* parent, Color initialColor, Bool isWindowed /* = false */ )
	: wxFrame( parent, -1, wxEmptyString, CalcAdvancedPickerPosition(), wxSize( ADV_PICKER_WIDTH, ADV_PICKER_HEIGHT ), wxSTAY_ON_TOP | wxFRAME_NO_TASKBAR )
	, m_imageMainPanelLightness( 666.f ) // force initial update
	, m_currCustomColorIndex( 0 )
	, m_panel( NULL )
	, m_isWindowed( isWindowed )
	, m_lastUsedPalettePath( String::EMPTY )
{
	// Init panel

	m_panel = new wxPanel();
	wxXmlResource::Get()->LoadPanel( m_panel, this, wxT("AdvancedColorPicker") );
	
	// Init side value panels

	m_red.Init(			m_panel, TXT("red"),		0, 255,		initialColor.R, 0, 1.0f );
	m_green.Init(		m_panel, TXT("green"),		0, 255,		initialColor.G, 0, 1.0f );
	m_blue.Init(		m_panel, TXT("blue"),		0, 255,		initialColor.B, 0, 1.0f );
	m_hue.Init(			m_panel, TXT("hue"),		0, 1,		m_hsl[0],		0, 1.0f / 1000.f );
	m_saturation.Init(	m_panel, TXT("saturation"),	0, 1,		m_hsl[1],		0, 1.0f / 1000.f );
	m_lightness.Init(	m_panel, TXT("lightness"),	0, 1,		m_hsl[2],		0, 1.0f / 1000.f );

	// Setup sizer
	
	wxBoxSizer *sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_panel );
	SetSizer( sizer );

	// Init helper panels
	
	m_panelMain				= XRCCTRL( *this, "mainPanel",			wxPanel );
	m_panelRed				= XRCCTRL( *this, "redPanel",			wxPanel );
	m_panelGreen			= XRCCTRL( *this, "greenPanel",			wxPanel );
	m_panelBlue				= XRCCTRL( *this, "bluePanel",			wxPanel );
	m_panelHue				= XRCCTRL( *this, "huePanel",			wxPanel );
	m_panelSaturation		= XRCCTRL( *this, "saturationPanel",	wxPanel );
	m_panelLightness		= XRCCTRL( *this, "lightnessPanel",		wxPanel );
	m_panelWhiteness		= XRCCTRL( *this, "whitenessPanel",		wxPanel );
	m_panelColorComparison	= XRCCTRL( *this, "colorComparison",	wxPanel );
	
	// Set color
	
	m_color = Color (0, 0, 0);
	m_rgb[0] = m_rgb[1] = m_rgb[2] = 0;
	m_hsl[0] = m_hsl[1] = m_hsl[2] = 0;
	SetSelectedColor( initialColor );
	UpdateTextControlsValues();
	m_comparisonRefColor.Set3( m_rgb[0], m_rgb[1], m_rgb[2] );
	
	// Mute color comparison panel background erase
	
	m_panelColorComparison->Connect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( CEdAdvancedColorPicker::OnEraseBackground ), NULL, this );

	// Init custom color panels
	
	m_currCustomColorIndex = 0;
#define BUILD_CUSTOM_NAME( id )		#id
#define INIT_CUSTOM_PANEL( index )	m_customColorPanels[ index ] = XRCCTRL( *this, BUILD_CUSTOM_NAME(customColorPanel##index), wxPanel );
	INIT_CUSTOM_PANEL( 0 );
	INIT_CUSTOM_PANEL( 1 );
	INIT_CUSTOM_PANEL( 2 );
	INIT_CUSTOM_PANEL( 3 );
	INIT_CUSTOM_PANEL( 4 );
	INIT_CUSTOM_PANEL( 5 );
	INIT_CUSTOM_PANEL( 6 );
	INIT_CUSTOM_PANEL( 7 );
	INIT_CUSTOM_PANEL( 8 );
	INIT_CUSTOM_PANEL( 9 );	
	INIT_CUSTOM_PANEL( 10 );
	INIT_CUSTOM_PANEL( 11 );
	INIT_CUSTOM_PANEL( 12 );
	INIT_CUSTOM_PANEL( 13 );
	INIT_CUSTOM_PANEL( 14 );
	INIT_CUSTOM_PANEL( 15 );
	INIT_CUSTOM_PANEL( 16 );
	INIT_CUSTOM_PANEL( 17 );
	INIT_CUSTOM_PANEL( 18 );
	INIT_CUSTOM_PANEL( 19 );	
	INIT_CUSTOM_PANEL( 20 );
	INIT_CUSTOM_PANEL( 21 );
	INIT_CUSTOM_PANEL( 22 );
	INIT_CUSTOM_PANEL( 23 );
	INIT_CUSTOM_PANEL( 24 );
	INIT_CUSTOM_PANEL( 25 );
	INIT_CUSTOM_PANEL( 26 );
	INIT_CUSTOM_PANEL( 27 );
	INIT_CUSTOM_PANEL( 28 );
	INIT_CUSTOM_PANEL( 29 );
	COMPILE_ASSERT( 30 == NUM_CUSTOM_COLORS );
#undef INIT_CUSTOM_PANEL
#undef BUILD_CUSTOM_NAME	

	// Init callbacks

#define CONNECT_COLOR_PANEL( panel, allowErase )	panel->Connect( wxEVT_LEFT_DOWN,	wxMouseEventHandler( CEdAdvancedColorPicker::OnColorSelectionChanged ), NULL, this );	\
													panel->Connect( wxEVT_MOTION,		wxMouseEventHandler( CEdAdvancedColorPicker::OnColorSelectionChanged ), NULL, this );	\
													( allowErase ? (void)0 : panel->Connect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( CEdAdvancedColorPicker::OnEraseBackground ), NULL, this ) );
	CONNECT_COLOR_PANEL( m_panelRed,				false )
	CONNECT_COLOR_PANEL( m_panelGreen,				false )
	CONNECT_COLOR_PANEL( m_panelBlue,				false )
	CONNECT_COLOR_PANEL( m_panelHue,				false )
	CONNECT_COLOR_PANEL( m_panelSaturation,			false )
	CONNECT_COLOR_PANEL( m_panelLightness,			false )
	CONNECT_COLOR_PANEL( m_panelWhiteness,			false )
	CONNECT_COLOR_PANEL( m_panelMain,				false )
	CONNECT_COLOR_PANEL( m_panelColorComparison,	true )
#undef CONNECT_COLOR_PANEL

#define CONNECT_COLOR_VALUE_RGB( value )	value.Connect( wxEVT_SPIN_SLIDER_CHANGED, wxCommandEventHandler( CEdAdvancedColorPicker::OnColorValueChangedRGB ), NULL, this );
#define CONNECT_COLOR_VALUE_HSL( value )	value.Connect( wxEVT_SPIN_SLIDER_CHANGED, wxCommandEventHandler( CEdAdvancedColorPicker::OnColorValueChangedHSL ), NULL, this );
	CONNECT_COLOR_VALUE_RGB( m_red );
	CONNECT_COLOR_VALUE_RGB( m_green );
	CONNECT_COLOR_VALUE_RGB( m_blue );
	CONNECT_COLOR_VALUE_HSL( m_hue );
	CONNECT_COLOR_VALUE_HSL( m_saturation );
	CONNECT_COLOR_VALUE_HSL( m_lightness );
#undef CONNECT_COLOR_VALUE_HSL
#undef CONNECT_COLOR_VALUE_RGB

	for ( Uint32 custom_i=0; custom_i<NUM_CUSTOM_COLORS; ++custom_i )
	{
		wxPanel *panel = m_customColorPanels[custom_i];
		if ( panel )
		{
			panel->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler(CEdAdvancedColorPicker::OnCustomColorSelect), NULL, this );
		}
	}

	// Show the shit

	if ( isWindowed )
	{
		SetWindowStyle( wxMINIMIZE_BOX | wxRESIZE_BORDER | wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX | wxCLIP_CHILDREN | wxSTAY_ON_TOP );
	}

	Layout();
	Refresh();
	Show();
}

CEdAdvancedColorPicker::~CEdAdvancedColorPicker()
{
}

Uint32 CEdAdvancedColorPicker::GetNumCustomColors()
{
	return NUM_CUSTOM_COLORS;
}

Vector CEdAdvancedColorPicker::GetCustomColor( Uint32 customColorIndex )
{
	ASSERT( customColorIndex < NUM_CUSTOM_COLORS );
	return s_customColors[ customColorIndex ].rgb;
}

void CEdAdvancedColorPicker::SetCustomColor( Uint32 customColorIndex, const Vector &color )
{
	ASSERT( customColorIndex < NUM_CUSTOM_COLORS );
	s_customColors[ customColorIndex ].rgb = color;
}

void CEdAdvancedColorPicker::OnActivate( wxActivateEvent& event )
{
	// Close window when deactivated
	if ( !event.GetActive() )
	{
		if ( ! m_isWindowed )
		{
			Close();
		}
	}
}

void CEdAdvancedColorPicker::OnEraseBackground( wxEraseEvent& event )
{
	// Empty
}

void CEdAdvancedColorPicker::OnPaint( wxPaintEvent& event )
{	
	// Lazy images update
	UpdateImages();

	// Clear background
	wxPaintDC dc( this );
	dc.SetBackground( wxBrush( m_panel->GetBackgroundColour() ) );
	dc.Clear();
	
	// RGB bars
	DrawRGBBar( m_panelRed,				Vector (        0, m_rgb[1], m_rgb[2] ), Vector (        1, m_rgb[1], m_rgb[2] ), true, m_rgb[0] );
	DrawRGBBar( m_panelGreen,			Vector ( m_rgb[0],        0, m_rgb[2] ), Vector ( m_rgb[0],        1, m_rgb[2] ), true, m_rgb[1] );
	DrawRGBBar( m_panelBlue,			Vector ( m_rgb[0], m_rgb[1],        0 ), Vector ( m_rgb[0], m_rgb[1],        1 ), true, m_rgb[2] );
	
	// HSL bars
	//DrawHSLBar( m_panelHue,			Vector (        0, m_hsl[1], m_hsl[2] ), Vector (        1, m_hsl[1], m_hsl[2] ), true, m_hsl[0] );
	DrawHSLBar( m_panelHue,				Vector (        0,        1,        1 ), Vector (        1,		   1,		 1 ), true, m_hsl[0] );
	DrawHSLBar( m_panelSaturation,		Vector ( m_hsl[0],        0, m_hsl[2] ), Vector ( m_hsl[0],        1, m_hsl[2] ), true, m_hsl[1] );
	DrawHSLBar( m_panelLightness,		Vector ( m_hsl[0], m_hsl[1],        0 ), Vector ( m_hsl[0], m_hsl[1],        1 ), true, m_hsl[2] );
	

	// TODO: Make whiteness panel, not Lightness
	DrawHSLBar( m_panelWhiteness,		Vector ( m_hsl[0], m_hsl[1],        0 ), Vector ( m_hsl[0], m_hsl[1],        1 ), false, m_hsl[2] );
	
	// Main panel
	DrawMainPanel( m_panelMain,			m_imageMainPanel, m_hsl[0], m_hsl[1], m_hsl[2] );

	// Color comparison panel
	{
		wxColour colorRef		= wxColour ( 255.f * Clamp(m_comparisonRefColor.X, 0.f, 1.f), 255.f * Clamp(m_comparisonRefColor.Y, 0.f, 1.f), 255.f * Clamp(m_comparisonRefColor.Z, 0.f, 1.f) );
		wxColour colorPicked	= wxColour ( m_color.R, m_color.G, m_color.B );
		wxSize halfSize			= m_panelColorComparison->GetSize();
		halfSize.SetWidth( halfSize.GetWidth() / 2 );
		
		wxPaintDC dc( m_panelColorComparison );
		dc.SetPen( wxPen ( colorRef ) );
		dc.SetBrush( wxBrush ( colorRef ) );
		dc.DrawRectangle( wxPoint (0, 0), halfSize );
		dc.SetPen( wxPen ( colorPicked ) );
		dc.SetBrush( wxBrush ( colorPicked ) );
		dc.DrawRectangle( wxPoint (halfSize.GetWidth(), 0), halfSize );
	}
	
	// Custom colors
	for ( Uint32 custom_i=0; custom_i<GetNumCustomColors(); ++custom_i )
	{
		bool	 isSelected	= ((Int32)custom_i == m_currCustomColorIndex);
		Vector   colorVec	= Vector::Max4( Vector::ZEROS, Vector::Min4( Vector::ONES, GetCustomColor( custom_i ) ) );
		wxColour color		= wxColour (255.f*colorVec.X, 255.f*colorVec.Y, 255.f*colorVec.Z, 255.f);
		wxPanel *panel		= m_customColorPanels[custom_i];

		wxColour backColorOrig	= panel->GetBackgroundColour();
		wxColour backColor		= isSelected ? wxColour (0, 0, 0, 255) : backColorOrig;

		Int32 innerRectOffset = 2;
		wxSize innerRectSize = panel->GetSize();
		innerRectSize.DecBy( 2 * innerRectOffset + 4 /*for 'sunken border'*/);

		wxBufferedPaintDC dc( panel );
		dc.SetPen( wxPen ( backColor ) );
		dc.SetBrush( wxBrush ( backColor ) );
		dc.DrawRectangle( wxPoint (0,0), panel->GetSize() );
		dc.SetPen( wxPen ( backColorOrig ) );
		dc.SetBrush( wxBrush ( color ) );
		dc.DrawRectangle( wxPoint (innerRectOffset,innerRectOffset), innerRectSize );
	}

	wxFrame::OnPaint( event );
}

void CEdAdvancedColorPicker::SetColorRGB( const Vector &rgb, bool normalized )
{
	if ( SetSelectedColorRGB( rgb, normalized ) )
	{
		PostSelectedColorUpdate();
	}
}

void CEdAdvancedColorPicker::SetColorHSL( const Vector &hsl )
{
	if ( SetSelectedColorHSL( hsl ) )
	{
		PostSelectedColorUpdate();
	}
}

void CEdAdvancedColorPicker::SetColor( const Color &color )
{
	if ( SetSelectedColor( color ) )
	{
		PostSelectedColorUpdate();
	}
}


void CEdAdvancedColorPicker::SetReferenceColor( const Color &color )
{
	m_comparisonRefColor = color.ToVector();
	Refresh( false );
}


bool CEdAdvancedColorPicker::SetSelectedColorRGB( const Vector &rgb, bool normalized )
{
	Float scale		= (normalized ? 1.f : 1.f/255.f);
	Float new_rgb[]	= 
	{
		Clamp( scale * rgb.X, 0.f, 1.f ),
		Clamp( scale * rgb.Y, 0.f, 1.f ),
		Clamp( scale * rgb.Z, 0.f, 1.f ),
	};

	const bool colorChanged = !(m_rgb[0] == new_rgb[0] && m_rgb[1] == new_rgb[1] && m_rgb[2] == new_rgb[2]);

	if ( colorChanged )
	{	
		// Set rgb
		m_rgb[0] = new_rgb[0];
		m_rgb[1] = new_rgb[1];
		m_rgb[2] = new_rgb[2];

		// Set hsl
		RGBToHSL( m_rgb[0], m_rgb[1], m_rgb[2], m_hsl[0], m_hsl[1], m_hsl[2] );
		
		// Set low precision rgb
		m_color = Color ( (Uint8)(255*m_rgb[0]), (Uint8)(255*m_rgb[1]), (Uint8)(255*m_rgb[2]) );
	}

	return colorChanged;
}

bool CEdAdvancedColorPicker::SetSelectedColorHSL( const Vector &hsl )
{
	Vector rgb;
	HSLToRGB( hsl.X, hsl.Y, hsl.Z, rgb.X, rgb.Y, rgb.Z );
	return SetSelectedColorRGB( rgb, true );
}

bool CEdAdvancedColorPicker::SetSelectedColor( const Color &color )
{
	return SetSelectedColorRGB( color.ToVector(), true );
}

void CEdAdvancedColorPicker::PostSelectedColorUpdate()
{
	UpdateTextControlsValues();
	Refresh( false );

	wxCommandEvent fake( wxEVT_COMMAND_SCROLLBAR_UPDATED );
	ProcessEvent( fake );
}

void CEdAdvancedColorPicker::UpdateTextControlsValues()
{
	m_red.UpdateValue( 255.f * m_rgb[0], NULL, true );
	m_green.UpdateValue( 255.f * m_rgb[1], NULL, true );
	m_blue.UpdateValue( 255.f * m_rgb[2], NULL, true );
	m_hue.UpdateValue( m_hsl[0], NULL, true );
	m_saturation.UpdateValue( m_hsl[1], NULL, true );
	m_lightness.UpdateValue( m_hsl[2], NULL, true );
}

void CEdAdvancedColorPicker::UpdateImages()
{
	Float targetMainPanelLightness = 1.f; //m_hsl[2];
	if ( targetMainPanelLightness != m_imageMainPanelLightness )
	{
		Int32 mainPanelWidth = m_panelMain->GetSize().GetWidth();
		Int32 mainPanelHeight = m_panelMain->GetSize().GetHeight();
		if ( !m_imageMainPanel.IsOk() || m_imageMainPanel.GetWidth() != mainPanelWidth || m_imageMainPanel.GetHeight() != mainPanelHeight )
		{
			if ( m_imageMainPanel.IsOk() )
			{
				m_imageMainPanel.Destroy();
			}

			m_imageMainPanel.Create( mainPanelWidth, mainPanelHeight, false );
		}

		DrawMainPanelImage( m_imageMainPanel, targetMainPanelLightness );
		m_imageMainPanelLightness = targetMainPanelLightness;
	}
}

Bool CEdAdvancedColorPicker::LoadPalette( const String &palettePath )
{
	IFile* file = GFileManager->CreateFileReader( palettePath, FOF_Buffered | FOF_AbsolutePath );
	if ( !file ) return false; // error

	const Uint32 maxColors = GetNumCustomColors();
	for ( Uint32 i = 0; i < maxColors; ++i )
	{
		Vector color( 0, 0, 0 );
		file->Serialize( &color, sizeof(Vector) );
		SetCustomColor( i, color );
	}

	delete file;

	m_lastUsedPalettePath = palettePath;

	return true;
}

void CEdAdvancedColorPicker::OnColorSelectionChanged( wxMouseEvent& event )
{
	if ( !event.m_leftDown )
	{
		return;
	}

	wxPanel *eventPanel = (wxPanel*)event.GetEventObject();
	Float valHorizontal = Clamp( event.GetPosition().x / Max( 1.f, eventPanel->GetSize().GetWidth() - 1.f ), 0.f, 1.f );
	Float valVertical   = 1.f - Clamp( event.GetPosition().y / Max( 1.f, eventPanel->GetSize().GetHeight() - 1.f ), 0.f, 1.f );
	Float valVertical1  = Clamp( event.GetPosition().y / Max( 1.f, eventPanel->GetSize().GetHeight() - 1.f ), 0.f, 1.f );

	Bool updated = false;
	
	if ( m_panelRed == eventPanel )
	{	
		updated = SetSelectedColorRGB( Vector ( valHorizontal, m_rgb[1], m_rgb[2] ), true );
	}
	else if ( m_panelGreen == eventPanel )
	{
		updated = SetSelectedColorRGB( Vector ( m_rgb[0], valHorizontal, m_rgb[2] ), true );
	}
	else if ( m_panelBlue == eventPanel )
	{
		updated = SetSelectedColorRGB( Vector ( m_rgb[0], m_rgb[1], valHorizontal ), true );
	}
	else if ( m_panelHue == eventPanel )
	{	 
		updated = SetSelectedColorHSL( Vector ( valHorizontal, m_hsl[1], m_hsl[2] ) );
	}
	else if ( m_panelSaturation == eventPanel )
	{	
		updated = SetSelectedColorHSL( Vector ( m_hsl[0], valHorizontal, m_hsl[2] ) );
	}
	else if ( m_panelLightness == eventPanel )
	{	
		updated = SetSelectedColorHSL( Vector ( m_hsl[0], m_hsl[1], valHorizontal ) );
	}
	else if ( m_panelWhiteness == eventPanel )
	{
		updated = SetSelectedColorHSL( Vector ( m_hsl[0], m_hsl[1], valVertical1 ) );
	}
	else if ( m_panelMain == eventPanel )
	{	
		updated = SetSelectedColorHSL( Vector ( valHorizontal, valVertical, m_hsl[2] ) );
	}
	else if ( m_panelColorComparison == eventPanel )
	{
		bool refToPicked = event.GetPosition().x < m_panelColorComparison->GetSize().x / 2;
		if ( refToPicked )
		{
			updated = SetSelectedColorRGB( m_comparisonRefColor, true );
		}
		else
		{
			m_comparisonRefColor.Set3( m_rgb[0], m_rgb[1], m_rgb[2] );
			updated = true;
		}
	}

	if ( updated )
	{
		PostSelectedColorUpdate();
	}
}

void CEdAdvancedColorPicker::OnColorValueChangedRGB( wxCommandEvent &event )
{
	const Float scale = 1.f / 255.f;
	Vector rgb ( scale * m_red.GetValue(), scale * m_green.GetValue(), scale * m_blue.GetValue() );
	if ( rgb.X != m_rgb[0] || rgb.Y != m_rgb[1] || rgb.Z != m_rgb[2] )
	{
		if ( SetSelectedColorRGB( rgb, true ) )
		{
			Refresh( false );
			wxCommandEvent fake( wxEVT_COMMAND_SCROLLBAR_UPDATED );
			ProcessEvent( fake );
		}
	}
}

void CEdAdvancedColorPicker::OnColorValueChangedHSL( wxCommandEvent &event )
{
	Vector hsl ( m_hue.GetValue(), m_saturation.GetValue(), m_lightness.GetValue() );
	if ( hsl.X != m_hsl[0] || hsl.Y != m_hsl[1] || hsl.Z != m_hsl[2] )
	{
		if ( SetSelectedColorHSL( hsl ) )
		{
			Refresh( false );
			wxCommandEvent fake( wxEVT_COMMAND_SCROLLBAR_UPDATED );
			ProcessEvent( fake );
		}
	}
}

void CEdAdvancedColorPicker::OnCustomColorSelect( wxMouseEvent& event )
{
	Int32 newCustomColor = -1;
	for ( Uint32 i=0; i<GetNumCustomColors(); ++i )
	{
		if ( event.GetEventObject() == m_customColorPanels[i] )
		{
			newCustomColor = (Int32)i;
			break;
		}
	}

	if ( newCustomColor != m_currCustomColorIndex )
	{
		m_currCustomColorIndex = newCustomColor;
		Refresh( false );
	}
}

void CEdAdvancedColorPicker::OnCustomColorSet( wxCommandEvent& event )
{
	if ( m_currCustomColorIndex >= 0 && m_currCustomColorIndex < (Int32)GetNumCustomColors() )
	{
		Vector color = GetVectorRGB( true );
		SetCustomColor( m_currCustomColorIndex, color );
		Refresh( false );
	}
}

void CEdAdvancedColorPicker::OnCustomColorGet( wxCommandEvent& event )
{
	if ( m_currCustomColorIndex >= 0 && m_currCustomColorIndex < (Int32)GetNumCustomColors() )
	{
		Vector color = GetCustomColor( m_currCustomColorIndex );
		SetColorRGB( color, true );
	}
}

void CEdAdvancedColorPicker::OnLoadPalette( wxCommandEvent& event )
{
	String defaultDir = wxEmptyString; //= GDepot->GetAbsolutePath();
	String wildCard = TXT("All files (*.*)|*.*|Palette files (*.plt)|*.plt");
	wxFileDialog loadFileDialog( this, wxT("Load palette"), defaultDir.AsChar(), wxT( "" ), wildCard.AsChar(), wxFD_OPEN );
	if ( loadFileDialog.ShowModal() == wxID_OK )
	{
		String loadPath = loadFileDialog.GetPath().wc_str();
		
		LoadPalette( loadPath );
	}
}

void CEdAdvancedColorPicker::OnSavePalette( wxCommandEvent& event )
{
	String defaultDir = wxEmptyString; // = GDepot->GetAbsolutePath();
	String wildCard = TXT("All files (*.*)|*.*|Palette files (*.plt)|*.plt");

	wxFileDialog saveFileDialog( this, TXT("Save palette as"), defaultDir.AsChar(), wxT( "" ), wildCard.AsChar(), wxFD_SAVE );
	if ( saveFileDialog.ShowModal() == wxID_OK )
	{
		String savePath = saveFileDialog.GetPath().wc_str();

		IFile* file = GFileManager->CreateFileWriter( savePath, FOF_Buffered | FOF_AbsolutePath );
		if ( !file ) return; // error

		const Uint32 maxColors = GetNumCustomColors();
		for ( Uint32 i = 0; i < maxColors; ++i )
		{
			Vector color = GetCustomColor( i );
			file->Serialize( &color, sizeof(Vector) );
		}

		delete file;

		m_lastUsedPalettePath = savePath;
	}
}
