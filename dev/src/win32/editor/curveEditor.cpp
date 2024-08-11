/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "curveEditor.h"
#include "../../common/engine/curve.h"

wxDEFINE_EVENT( wxEVT_COMMAND_CURVE_CHANGING, wxCommandEvent );
wxDEFINE_EVENT( wxEVT_COMMAND_CURVE_CHANGED, wxCommandEvent );


BEGIN_EVENT_TABLE( CEdCurveEditor, wxPanel )
	EVT_MENU( XRCID( "showControlPoints" ), CEdCurveEditor::OnShowControlPoints )
END_EVENT_TABLE()

CEdCurveEditor::CEdCurveEditor( wxWindow* parent )
	: m_gradientEditor( NULL )
{
	// Load designed panel from resource
	wxXmlResource::Get()->LoadPanel( this, parent, TEXT("CurveEditorPanel") );

	// Set default states for buttons
	wxToolBar* tb = XRCCTRL( *this, "curveEditorControlToolBar1", wxToolBar );
	tb->ToggleTool( XRCID( "showControlPoints" ), true );

	// Connect time and value controls and set default values
	XRCCTRL( *this, "time", wxTextCtrl )->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdCurveEditor::OnTimeValueChanged ), NULL, this );
	XRCCTRL( *this, "value", wxTextCtrl )->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdCurveEditor::OnTimeValueChanged ), NULL, this );
	XRCCTRL( *this, "absoluteChange", wxCheckBox )->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdCurveEditor::OnAbsoluteChanged ), NULL, this );
	ClearTimeValue();

	m_absoluteMove = XRCCTRL( *this, "absoluteChange", wxCheckBox )->IsChecked();

	// Create Curve Editor Canvas panel
	wxPanel* rp = XRCCTRL( *this, "CurveEditorCanvas", wxPanel );
	wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );		
	m_curveEditorCanvas = new CEdCurveEditorCanvas( rp );
	m_curveEditorCanvas->SetParentCurveEditor( this );
	m_curveEditorCanvas->SetHook( this );
	sizer1->Add( m_curveEditorCanvas, 1, wxEXPAND, 0 );
	rp->SetSizer( sizer1 );		
	rp->Layout();

	GetCanvas()->SetXIsUnlocked( true );
	GetCanvas()->SetYIsUnlocked( true );

	// Update and finalize layout
	Layout();
	Show();
}

CEdCurveEditor::~CEdCurveEditor()
{

}

void CEdCurveEditor::ClearTimeValue()
{
	XRCCTRL( *this, "time", wxTextCtrl )->SetValue( wxEmptyString );
	XRCCTRL( *this, "value", wxTextCtrl )->SetValue( wxEmptyString );
}

void CEdCurveEditor::SetTime( Float time )
{
	XRCCTRL( *this, "time", wxTextCtrl )->SetValue( ToString( time ).AsChar() );
}

void CEdCurveEditor::SetValue( Float value )
{
	XRCCTRL( *this, "value", wxTextCtrl )->SetValue( ToString( value ).AsChar() );
}

void CEdCurveEditor::SetTimeValue( Float time, Float value )
{
	SetTime( time );
	SetValue( value );
}

void CEdCurveEditor::OnAbsoluteChanged( wxCommandEvent& event )
{
	m_absoluteMove = XRCCTRL( *this, "absoluteChange", wxCheckBox )->IsChecked();
	ClearTimeValue();
}

void CEdCurveEditor::OnShowControlPoints( wxCommandEvent& event )
{
	wxToolBar* tb = XRCCTRL( *this, "curveEditorControlToolBar1", wxToolBar );
	Bool enabled = tb->GetToolState( XRCID( "showControlPoints" ) );
	
	GetCanvas()->SetShowControlPoints( enabled );
	GetCanvas()->Repaint();
}

void CEdCurveEditor::OnTimeValueChanged( wxCommandEvent& event )
{
	ASSERT( GetCanvas() );

	String timeString( XRCCTRL( *this, "time", wxTextCtrl )->GetValue().wc_str() );
	String valueString( XRCCTRL( *this, "value", wxTextCtrl )->GetValue().wc_str() );
	Float time, value;

	// Move control points (time or value)
	if ( FromString( timeString, time ) )
	{
		GetCanvas()->MoveSelectedControlPoints( time, 0.0f, true, false, m_absoluteMove );
		GetCanvas()->Repaint();
	}
	if ( FromString( valueString, value ) )
	{
		GetCanvas()->MoveSelectedControlPoints( 0.0f, value, false, true, m_absoluteMove );
		GetCanvas()->Repaint();
	}
}

void CEdCurveEditor::OnSelectionChanged()
{
	ASSERT( GetCanvas() );

	// Update UI if selection changed
	TDynArray< CEdCurveEditorCanvas::ControlPointInfo >& selectedControlPoints = GetCanvas()->GetSelection();
	if ( selectedControlPoints.Size() > 0 )
	{
		Bool timeEqual = true;
		Bool valueEqual = true;
		Float time = selectedControlPoints[0].m_controlPoint->GetTime();
		Float value = selectedControlPoints[0].m_controlPoint->GetValue();

		// Check if value / time are the same for all selected control points
		for( Uint32 i = 1; i < selectedControlPoints.Size(); ++i )
		{
			if ( time != selectedControlPoints[i].m_controlPoint->GetTime() )
			{
				timeEqual = false;
			}
			if ( value != selectedControlPoints[i].m_controlPoint->GetValue() )
			{
				valueEqual = false;
			}
		}

		// If time is the same for all selected control points update UI time control
		if ( timeEqual && m_absoluteMove )
		{
			SetTime( time );
		}

		// If value is the same for all selected control points update UI value control
		if ( valueEqual && m_absoluteMove )
		{
			SetValue( value );
		}
	}
	else
	{
		ClearTimeValue();
	}
}

void CEdCurveEditor::OnControlPointsChanged()
{
	if ( m_gradientEditor )
	{
		m_gradientEditor->SetLinked( false );
		m_gradientEditor->RefreshGradients();
	}

	wxCommandEvent event( wxEVT_COMMAND_CURVE_CHANGING );
	wxPostEvent( this, event );
}


void CEdCurveEditor::OnControlPointsChangedComplete()
{
	wxCommandEvent event( wxEVT_COMMAND_CURVE_CHANGED );
	wxPostEvent( this, event );
}


void CEdCurveEditor::AddCurve( SCurveData* curve, const String& curveName, Bool setZoom /* = true */, const Vector &curveScale /*= Vector(0,1.0,0)*/, CObject* curveContainer /*= NULL*/, const Color& color /*= Color( 255, 255, 255 )*/ )
{
	if ( curve )
	{
		ASSERT( GetCanvas() );
		GetCanvas()->AddCurve( curve, curveName, curveScale, curveContainer, color );
		if ( setZoom ) GetCanvas()->SetZoomedRegionToFit();
		GetCanvas()->Repaint();
	}
}

void CEdCurveEditor::AddCurve( CurveParameter* curve, Bool setZoom /* = true */, const Vector &curveScale /*= Vector(0,1.0,0)*/, const Color& color /*= Color( 255, 255, 255 )*/ )
{
	if ( curve )
	{
		const Color bckgColor = curve->GetColor();
		ASSERT( GetCanvas() );
		GetCanvas()->AddCurveGroup( curve, bckgColor, curveScale, color );
		if ( setZoom ) GetCanvas()->SetZoomedRegionToFit();
		GetCanvas()->Repaint();
	}
}

void CEdCurveEditor::AddCurve( CCurve* curve, const String& curveName, Bool setZoom, const Vector &curveScale, const Color& color ) 
{ 
	AddCurve( &curve->GetCurveData(), curveName, setZoom, curveScale, curve, color ); 
}

Bool CEdCurveEditor::UpdateCurveParam( SCurveData* curve, const Vector &curveScale )
{
	if ( curve )
	{
		ASSERT( GetCanvas() );
		Bool retValue = GetCanvas()->UpdateCurveParam( curve, curveScale );
		GetCanvas()->Repaint();
		return retValue;
	}
	else
	{
		return false;
	}
}

void CEdCurveEditor::RemoveCurve( SCurveData* curve )
{
	if ( curve )
	{
		ASSERT( GetCanvas() );
		GetCanvas()->RemoveCurve( curve );
		GetCanvas()->Repaint();
	}
}

void CEdCurveEditor::RemoveCurveGroup( const CName& curveGroupName )
{
	ASSERT( GetCanvas() );
	GetCanvas()->RemoveCurveGroup( curveGroupName );
	GetCanvas()->Repaint();
}

void CEdCurveEditor::RemoveAllCurveGroups( )
{
	ASSERT( GetCanvas() );
	GetCanvas()->RemoveAllCurveGroups( );
	GetCanvas()->Repaint();
}

Bool CEdCurveEditor::IsCurveGroupAdded( const CName& curveGroupName ) const
{
	ASSERT( GetCanvas() );
	return GetCanvas()->IsCurveGroupAdded( curveGroupName );
}

Bool CEdCurveEditor::IsCurveAdded( SCurveData* curve ) const
{
	ASSERT( GetCanvas() );
	return GetCanvas()->IsCurveAdded( curve );
}

void CEdCurveEditor::SetActiveRegion( Float start, Float end )
{
	GetCanvas()->SetActiveRegion( start, end );
}
