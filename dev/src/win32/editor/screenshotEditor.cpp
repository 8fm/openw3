/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "screenshotEditor.h"
#include "../../common/engine/freeCamera.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/screenshotSystem.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/renderFrame.h"

const String names[] =
{
	TXT("HD Ready (1280 x 720)"),
	TXT("FullHD (1920 x 1080)"),
	TXT("UHD/4K (3840 x 2160)"),
	// Maybe at the future if we get more memory
	//TXT("16 x FullHD (7680 x 4320)"),
	//TXT("64 x FullHD (15360 x 8640)")
};

String sizes[ ][ 2 ] = 
{ 
	//width,		height
	{ TXT("1280"),	TXT("720")	},
	{ TXT("1920"),	TXT("1080")	},
	{ TXT("3840"),	TXT("2160")	},
	// Maybe at the future if we get more memory
	//{ TXT("7680"),	TXT("4320")	},
	//{ TXT("15360"),	TXT("8640")	},
};

// Let's avoid running out of memory
static const Uint32 MAX_TOTAL_PIXELS = 3840*2160;

wxIMPLEMENT_CLASS( CEdScreenshotEditor, wxFrame );

BEGIN_EVENT_TABLE( CEdScreenshotEditor, wxFrame )
	EVT_BUTTON( XRCID("m_ok"), CEdScreenshotEditor::OnGenerate )
	EVT_COMMAND_SCROLL( XRCID( "m_fovSlider" ), CEdScreenshotEditor::OnFOVChanged )
	EVT_CHOICE( XRCID("m_preset"), CEdScreenshotEditor::OnPresetChanged )
	EVT_RADIOBOX( XRCID("m_compositionRBtns"),CEdScreenshotEditor::OnCompositionChanged )
END_EVENT_TABLE()

CEdScreenshotEditor::CEdScreenshotEditor( wxWindow* parent )
{
	m_compositionLineWidth = 2;
	m_compositionLineColor = Color::GRAY;
	m_phi = 1.618f;

	// Load dialog resource
	VERIFY( wxXmlResource::Get()->LoadFrame( this, parent, wxT("ScreenshotEditor") ) );

	// setup controls
	m_widthTB					= XRCCTRL( *this, "m_widthTB", wxTextCtrl );
	m_heightTB					= XRCCTRL( *this, "m_heightTB", wxTextCtrl );
	m_resolutionPresetChoice	= XRCCTRL( *this, "m_preset", wxChoice );
	m_samplesPerPixelTB			= XRCCTRL( *this, "m_samplesPerPixelTB", wxTextCtrl );
	m_fovSlider					= XRCCTRL( *this, "m_fovSlider", wxSlider );
	m_fovTB						= XRCCTRL( *this, "m_fovTB", wxTextCtrl );

	m_renderFilterList			= XRCCTRL( *this, "m_renderFilterList", wxCheckListBox );
	m_saveFormatChoice			= XRCCTRL( *this, "m_saveFormatChoice", wxChoice );

	m_composition				= XRCCTRL( *this, "m_compositionRBtns", wxRadioBox );
	m_toggleX					= XRCCTRL( *this, "m_toggleX", wxToggleButton );
	m_toggleY					= XRCCTRL( *this, "m_toggleY", wxToggleButton );

	CEnum* renderFlagsEnum = SRTTI::GetInstance().FindEnum( CNAME( EScreenshotRenderFlags ) );
	const TDynArray< CName >& options = renderFlagsEnum->GetOptions();
	Uint32 pos = 0;
	m_renderFilterList->Clear();
	for ( TDynArray< CName >::const_iterator it = options.Begin(); it != options.End(); ++it )
	{
		m_renderFilterList->Insert( it->AsString().AsChar(), pos++ );
	}

	CEnum* saveFormatEnum = SRTTI::GetInstance().FindEnum( CNAME( ESaveFormat ) );
	const TDynArray< CName >& saveFormats = saveFormatEnum->GetOptions();
	pos = 0;
	m_saveFormatChoice->Clear();
	for ( TDynArray< CName >::const_iterator it = saveFormats.Begin(); it != saveFormats.End(); ++it )
	{
		m_saveFormatChoice->Insert( it->AsString().AsChar(), pos++ );
	}

	// fill in default values
	m_widthTB->SetValue( wxT("1920") );
	m_heightTB->SetValue( wxT("1080") );
	
	m_preservedGameFreeCameraFOV	= ( GGame && GGame->IsActive() ) ? GGame->GetFreeCamera().GetFOV() : 70.0f;
	m_preservedEditorCameraFOV		= wxTheFrame->GetWorldEditPanel()->GetCameraFov();

	m_saveFormatChoice->SetSelection( 0 ); // BMP

	ASSERT( m_preservedGameFreeCameraFOV >= 0.0f && m_preservedGameFreeCameraFOV < 180.0f );
	ASSERT( m_preservedEditorCameraFOV >= 0.0f && m_preservedEditorCameraFOV < 180.0f );

	// pin up the most accurate one
	if ( GGame && GGame->IsActive() )
	{
		m_fov = GGame->IsFreeCameraEnabled() ? m_preservedGameFreeCameraFOV : GGame->GetActiveWorld()->GetCameraDirector()->GetFov();
	}
	else
	{
		m_fov = m_preservedEditorCameraFOV;
	}

	m_fovSlider->SetValue( m_fov );
	m_fovTB->SetValue( String::Printf( TXT( "%1.2f" ), m_fov ).AsChar() );
	m_samplesPerPixelTB->SetValue( wxT("16") );

	// fill presets
	FillPresets();
	
	Layout();
}

CEdScreenshotEditor::~CEdScreenshotEditor()
{
	if ( wxTheFrame && wxTheFrame->GetWorldEditPanel() )
	{
		wxTheFrame->GetWorldEditPanel()->SetCameraFov( m_preservedEditorCameraFOV );
		wxTheFrame->GetWorldEditPanel()->SetScreenshotEditor( nullptr );
	}

	if ( GGame && GGame->IsActive() )
	{
		GGame->GetActiveWorld()->GetCameraDirector()->OverwriteFOV( 0.f );
		const_cast<CGameFreeCamera&>(GGame->GetFreeCamera()).SetFOV( m_preservedGameFreeCameraFOV );
	}
}

void CEdScreenshotEditor::OnViewportGenerateFragments( IViewport* view, CRenderFrame* frame )
{
	Int32 selectedCompositionInd = m_composition ? m_composition->GetSelection() : -1;
	if ( selectedCompositionInd > 0 )
	{
		wxString selectedComposition = m_composition->GetString( selectedCompositionInd );
		if ( selectedComposition == wxString("None") )
		{
			return;
		}

		Int32 width = view->GetWidth();
		Int32 height = view->GetHeight();

		if ( selectedComposition == wxString("Rule of Thirds") )
		{
			GenerateFragmentsRuleOfThirds( frame, width, height );
		}
		else 
		{
			Int32 localHeight = height, localWidth = width;
			Int32 heightMargin = 0, widthMargin = 0;

			if ( (Float)width / (Float)height <= m_phi )
			{
				localHeight = width / m_phi;
				heightMargin = Abs( height - localHeight) / 2.f;
			}
			else
			{
				localWidth = height * m_phi;
				widthMargin = Abs( width - localWidth) / 2.f;
			}

			// draw rectangle of size localWidth x localHeight
			GenerateFragmentsDrawRectangle( frame, widthMargin, heightMargin, localWidth, localHeight );

			if ( selectedComposition == wxString("Golden Rule") )
			{
				GenerateFragmentsGoldenRule( frame, localWidth, localHeight, widthMargin, heightMargin );
			}
			else if ( selectedComposition == wxString("Fibonacci Spiral") )
			{
				GenerateFragmentsFibonacciSpiral( frame, localWidth, localHeight, widthMargin, heightMargin );
			}
			else if ( selectedComposition == wxString("Dynamic Symmetry") )
			{
				GenerateFragmentsDynamicSymmetry( frame, localWidth, localHeight, widthMargin, heightMargin );
			}
		}
	}
}

void CEdScreenshotEditor::OnGenerate( wxCommandEvent& event )
{
	Int32 width;
	Int32 height;
	Int32 samplesPerPixel;

	Bool isValid = FromString<Int32>( String( m_widthTB->GetValue() ), width );
	if ( !isValid || width < 0 )
	{
		GFeedback->ShowError( TXT( "Invalid width." ) );
		return;
	}
	isValid = FromString<Int32>( String( m_heightTB->GetValue() ), height );
	if ( !isValid || height < 0 )
	{
		GFeedback->ShowError( TXT( "Invalid height." ) );
		return;
	}
	isValid = FromString<Int32>( String( m_samplesPerPixelTB->GetValue() ), samplesPerPixel );
	if ( !isValid || samplesPerPixel < 0 )
	{
		GFeedback->ShowError( TXT( "Invalid number of samples per pixel." ) );
		return;
	}

	if ( width*height > MAX_TOTAL_PIXELS )
	{
		GFeedback->ShowError( TXT( "The image is too big!" ) );
		return;
	}

	Uint32 flags = 0;
	for ( Uint32 flagIndex = 1, j = 0; flagIndex < SRF_Max; flagIndex *= 2, ++j )
	{
		if ( m_renderFilterList->IsChecked( j ) )
		{
			flags |= FLAG( j );
		}
	}

	ESaveFormat saveFormat = (ESaveFormat)m_saveFormatChoice->GetSelection();
	
	SScreenshotParameters parameters;
	parameters.m_width				= width;
	parameters.m_height				= height;
	parameters.m_fov				= m_fov;
	parameters.m_superSamplingSize	= samplesPerPixel;
	parameters.m_saveFormat			= saveFormat;

	Functor4< void, const String&, const void*, Bool, const String& > callback( this, &CEdScreenshotEditor::OnScreenshotTaken );

	SScreenshotSystem::GetInstance().RequestScreenshot( parameters, SCF_SaveToDisk, flags, callback );
}

void CEdScreenshotEditor::OnFOVChanged( wxScrollEvent& event )
{
	m_fov = m_fovSlider->GetValue();
	String s = String::Printf( TXT( "%1.2f" ), m_fov );
	m_fovTB->SetValue( s.AsChar() );
	
	if ( GGame && GGame->IsActive() )
	{
		if ( GGame->IsFreeCameraEnabled() )
		{
			const_cast<CGameFreeCamera&>(GGame->GetFreeCamera()).SetFOV( m_fov );
		}
		else
		{
			GGame->GetActiveWorld()->GetCameraDirector()->OverwriteFOV( m_fov );
		}		
	}
	else
	{
		wxTheFrame->GetWorldEditPanel()->SetCameraFov( m_fov );
	}
}

void CEdScreenshotEditor::FillPresets()
{
	m_resolutionPresetChoice->Clear();
	for ( Uint32 i = 0; i < ARRAY_COUNT( names ); ++i )
	{
		m_resolutionPresetChoice->Insert( names[i].AsChar(), i );
	}
}

void CEdScreenshotEditor::OnPresetChanged( wxCommandEvent& event )
{
	Int32 choice = m_resolutionPresetChoice->GetSelection();
	ASSERT( choice >= 0 && choice < ARRAY_COUNT( sizes ) );

	m_widthTB->SetValue( sizes[choice][0].AsChar() );
	m_heightTB->SetValue( sizes[choice][1].AsChar() );
}

void CEdScreenshotEditor::OnCompositionChanged( wxCommandEvent& event )
{
	if ( event.GetInt() >= 0 )
	{
		Bool btnsEnabled = m_composition->GetString( event.GetInt() ) == wxT("Fibonacci Spiral");
		m_toggleX->Enable( btnsEnabled );
		m_toggleY->Enable( btnsEnabled );
	}
}

void CEdScreenshotEditor::OnScreenshotTaken( const String& path, const void* buffer, Bool status, const String& msg )
{
	// notify user
	if ( status )
	{
		String msg = String::Printf( TXT("Screenshot '%s' taken"), path.AsChar() );
		MessageBox( NULL, msg.AsChar(), TXT("Success"), 0 );
	}
	else
	{
		String msg = String::Printf( TXT("Screenshot '%s' failed"), path.AsChar() );
		MessageBox( NULL, msg.AsChar(), TXT("Error"), 0 );
	}	
}

void CEdScreenshotEditor::GenerateFragmentsRuleOfThirds( CRenderFrame* frame, Uint32 width, Uint32 height )
{
	Int32 xStep = width/ 3, yStep = height / 3;
	Uint32 currX = xStep, currY = yStep;

	for ( Uint32 i = 0; i < 2; ++i, currX += xStep, currY += yStep )
	{
		frame->AddDebugRect( 0, currY - 1, width, m_compositionLineWidth, m_compositionLineColor );
		frame->AddDebugRect( currX - 1, 0, m_compositionLineWidth, height, m_compositionLineColor );
	}
}

void CEdScreenshotEditor::GenerateFragmentsGoldenRule( CRenderFrame* frame, Uint32 localWidth, Uint32 localHeight, Uint32 widthMargin, Uint32 heightMargin )
{
	Uint32 x = localHeight;
	Uint32 y = localHeight / m_phi;

	frame->AddDebugRect( widthMargin, localHeight - y + heightMargin, localWidth, m_compositionLineWidth, m_compositionLineColor );
	frame->AddDebugRect( widthMargin, y + heightMargin, localWidth, m_compositionLineWidth, m_compositionLineColor );

	frame->AddDebugRect( localWidth - x + widthMargin, heightMargin, m_compositionLineWidth, localHeight, m_compositionLineColor );
	frame->AddDebugRect( x + widthMargin, heightMargin, m_compositionLineWidth, localHeight, m_compositionLineColor );
}

void CEdScreenshotEditor::GenerateFragmentsFibonacciSpiral( CRenderFrame* frame, Uint32 localWidth, Uint32 localHeight, Uint32 widthMargin, Uint32 heightMargin )
{
	Bool flipX = m_toggleX->GetValue();
	Bool flipY = m_toggleY->GetValue();

	Bool horizontal = false;
	Bool fromLeft = !flipX;
	Bool fromUp = flipY;

	Uint32 x = fromLeft ? localHeight : localWidth - localHeight;
	Uint32 y = fromUp ? 0 : localHeight;

	for ( Uint32 i = 0; i < 8; ++i )
	{
		if ( horizontal )
		{
			frame->AddDebugRect( fromLeft ? x + widthMargin : x - localWidth + widthMargin, y + heightMargin, localWidth, m_compositionLineWidth, m_compositionLineColor );
			localHeight = (Float)localWidth / m_phi;
			Int32 diff = localWidth - localHeight;
			x += fromLeft ? diff : -diff;
		}
		else
		{
			frame->AddDebugRect( x + widthMargin, fromUp ? y + heightMargin: y + heightMargin - localHeight, m_compositionLineWidth, localHeight, m_compositionLineColor );
			localWidth = (Float)localHeight / m_phi;
			Int32 diff = localWidth - localHeight;
			y += fromUp ? -diff : diff;
		}

		horizontal = !horizontal;
		if ( horizontal )
		{
			fromUp = !fromUp;
		}
		else
		{
			fromLeft = !fromLeft;
		}
	}
}

void CEdScreenshotEditor::GenerateFragmentsDynamicSymmetry( CRenderFrame* frame, Uint32 localWidth, Uint32 localHeight, Uint32 widthMargin, Uint32 heightMargin )
{
	Float w = localWidth;
	Float h = localHeight;
	Vector2 offset( widthMargin, heightMargin );

	frame->AddDebugLineOnScreen( Vector2( 0.f, 0.f ) + offset, Vector2( w, h ) + offset, m_compositionLineColor );
	Vector2 p1 = Vector2( w - h * h / w, h );
	frame->AddDebugLineOnScreen( Vector2( w, 0.f ) + offset, p1 + offset, m_compositionLineColor );
	frame->AddDebugLineOnScreen( p1 + offset, Vector2( p1.X, 0 ) + offset, m_compositionLineColor );
	frame->AddDebugLineOnScreen( Vector2( p1.X, 0 ) + offset, Vector2( w, h ) + offset, m_compositionLineColor );

	frame->AddDebugLineOnScreen( Vector2( w, 0.f ) + offset, Vector2( 0.f, h ) + offset, m_compositionLineColor );
	Vector2 p2 = Vector2( h * h / w, h );
	frame->AddDebugLineOnScreen( Vector2( 0.f, 0.f ) + offset, p2 + offset, m_compositionLineColor );
	frame->AddDebugLineOnScreen( p2 + offset, Vector2( p2.X, 0 ) + offset, m_compositionLineColor );
	frame->AddDebugLineOnScreen( Vector2( p2.X, 0 ) + offset, Vector2( 0.f, h ) + offset, m_compositionLineColor );
}

void CEdScreenshotEditor::GenerateFragmentsDrawRectangle( CRenderFrame* frame, Uint32 x, Uint32 y, Uint32 width, Uint32 height )
{
	frame->AddDebugRect( x, y, width, m_compositionLineWidth, m_compositionLineColor );
	frame->AddDebugRect( x, height + y, width, m_compositionLineWidth, m_compositionLineColor );

	frame->AddDebugRect( x, y, m_compositionLineWidth, height, m_compositionLineColor );
	frame->AddDebugRect( width + x, y, m_compositionLineWidth, height, m_compositionLineColor );
}
