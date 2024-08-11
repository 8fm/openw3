#include "build.h"
#include "curveEditorWindow.h"

BEGIN_EVENT_TABLE( CEdCurveEditorFrame, wxFrame )
	EVT_BUTTON( XRCID("buttOk"), CEdCurveEditorFrame::OnOk )
END_EVENT_TABLE()

void CEdCurveEditorFrame::Init( wxWindow* parent )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadFrame( this, parent, TXT("CurveEditorWindow") );

	// Create Curve Editor and place it in CurvePanel
	{
		wxPanel* rp = XRCCTRL( *this, "CurvePanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );		
		m_curveEditor = new CEdCurveEditor( rp );
		sizer1->Add( m_curveEditor, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );		
		rp->Layout();
	}

	Layout();
}

CEdCurveEditorFrame::CEdCurveEditorFrame( wxWindow* parent, SCurveData* curve )
{
	Init( parent );

	m_curveEditor->AddCurve( curve, TXT("Curve"));

	Float min, max;

	if ( curve->Size() )
	{
		min = curve->GetMinTime();
		max = curve->GetMaxTime();
	}
	else
	{
		min = max = 0.0f;
	}

	Show();
}

CEdCurveEditorFrame::CEdCurveEditorFrame( wxWindow* parent, TDynArray< SCurveData* >& curves )
{
	Init( parent );
	Float min , max;
	if ( curves.Size() > 0 )
	{
		if ( curves[0]->Size() )
		{
			min = curves[0]->GetMinTime();
			max = curves[0]->GetMaxTime();
		}
		m_curveEditor->AddCurve( curves[0], TXT("X"), true, Vector( 0, 1.0f, 0 ), NULL, Color( 255, 0, 0 ) );
	}
	if ( curves.Size() > 1 )
	{
		m_curveEditor->AddCurve( curves[1], TXT("Y"), true, Vector( 0, 1.0f, 0 ), NULL, Color( 0, 255, 0 ) );
	}
	if ( curves.Size() > 2 )
	{
		m_curveEditor->AddCurve( curves[2], TXT("Z"), true, Vector( 0, 1.0f, 0 ),  NULL, Color( 0, 0, 255 ) );
	}
	if ( curves.Size() > 3 )
	{
		m_curveEditor->AddCurve( curves[3], TXT("W") );
	}
	for ( Uint32 i = 4; i < curves.Size(); ++i )
	{
		m_curveEditor->AddCurve( curves[i], TXT("Curve") );
	}

	for ( Uint32 i = 1; i < curves.Size(); ++i )
	{
		if ( curves[i]->Size() )
		{
			min = Min( min, curves[i]->GetMinTime() );
			max = Max( max, curves[i]->GetMaxTime() );
		}
	}

	SetActiveRegion( min, max );

	Show();
}

CEdCurveEditorFrame::~CEdCurveEditorFrame()
{

}

void CEdCurveEditorFrame::SetActiveRegion( Float start, Float end )
{
	m_curveEditor->SetActiveRegion( start, end );
}

void CEdCurveEditorFrame::OnOk( wxCommandEvent& event )
{
	Close();
}