/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "gradientEditor.h"
#include "../../common/engine/curve.h"

BEGIN_EVENT_TABLE( CEdGradientEditor, wxFrame )
	EVT_CLOSE( CEdGradientEditor::OnCloseWindow )
END_EVENT_TABLE()


CEdGradientEditor::CEdGradientEditor( wxWindow* parent, Bool hidden )
	: m_gradientColor( NULL )
	, m_gradientAlpha( NULL )
	, m_curveEditor( NULL )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadFrame( this, parent, TEXT("GradientEditor") );

	// Load icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_TOOL") ) );
	SetIcon( iconSmall );

	// Setup gradients controls
	m_gradientColor = XRCCTRL( *this, "m_gradientColor", CEdGradientPicker );
	m_gradientAlpha = XRCCTRL( *this, "m_gradientAlpha", CEdGradientPicker );
	ASSERT( m_gradientColor );
	ASSERT( m_gradientAlpha );
	m_gradientColor->SetDisplayMode( CEdGradientPicker::DM_Color );
	m_gradientAlpha->SetDisplayMode( CEdGradientPicker::DM_Alpha );

	// Connect checkboxes
	XRCCTRL( *this, "gradientLinked", wxCheckBox )->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdGradientEditor::OnLinkedChanged ), NULL, this );
	XRCCTRL( *this, "showTransparency", wxCheckBox )->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdGradientEditor::OnShowTransparencyChanged ), NULL, this );

	// Fake event to setup proper update mode in m_gradientColor, m_gradientAlpha
	wxCommandEvent event;
	OnLinkedChanged( event );

	// Update and finalize layout
	Layout();

	if ( !hidden )
	{
		Show();
	}
}

CEdGradientEditor::~CEdGradientEditor()
{
}

void CEdGradientEditor::SetCurves( CCurve* curveR, CCurve* curveG, CCurve* curveB, CCurve* curveA )
{
	ASSERT( curveR && curveG  && curveB && curveA );

	// Set curves to gradient controls
	m_gradientColor->SetCurves( &curveR->GetCurveData(), &curveG->GetCurveData(), &curveB->GetCurveData(), &curveA->GetCurveData() );
	m_gradientAlpha->SetCurves( &curveR->GetCurveData(), &curveG->GetCurveData(), &curveB->GetCurveData(), &curveA->GetCurveData() );

	// Check if curves are in sync to each other
	Bool match = m_gradientColor->CheckAlphaToColor();
	ASSERT( m_gradientAlpha->CheckAlphaToColor() == match );
	SetLinked( match );
}

void CEdGradientEditor::SetLinked( Bool linked )
{
	XRCCTRL( *this, "gradientLinked", wxCheckBox )->SetValue( linked );

	// Fake event to setup proper update mode in m_gradientColor, m_gradientAlpha
	wxCommandEvent event;
	OnLinkedChanged( event );
}

void CEdGradientEditor::OnLinkedChanged( wxCommandEvent& event )
{
	Bool linked = XRCCTRL( *this, "gradientLinked", wxCheckBox )->IsChecked();
	if ( linked )
	{
		m_gradientAlpha->ForceAlphaToColor();
		m_gradientColor->SetUpdateMode( CEdGradientPicker::UM_Both );
		m_gradientAlpha->SetUpdateMode( CEdGradientPicker::UM_Both );
	}
	else
	{
		m_gradientColor->SetUpdateMode( CEdGradientPicker::UM_Color );
		m_gradientAlpha->SetUpdateMode( CEdGradientPicker::UM_Alpha );
	}

	// Refresh both gradient and curve editor
	RefreshGradients();
}

void CEdGradientEditor::OnShowTransparencyChanged( wxCommandEvent& event )
{
	Bool showTransparency = XRCCTRL( *this, "showTransparency", wxCheckBox )->IsChecked();
	m_gradientColor->SetDisplayMode( showTransparency ? CEdGradientPicker::DM_Both : CEdGradientPicker::DM_Color );
	Refresh();
}

void CEdGradientEditor::OnControlPointsChanged()
{
	// If we have associated Curve Editor, signal it that some curves have changed
	if ( m_curveEditor )
	{
		//m_curveEditor->CanvasRepaint();
		m_curveEditor->Refresh();
		m_curveEditor->Update();
	}

	// Refresh both gradients 
	RefreshGradients();
}

void CEdGradientEditor::OnClearSelection()
{
	// Clear selected control points in both gradients
	m_gradientColor->ClearSelection();
	m_gradientAlpha->ClearSelection();
}

void CEdGradientEditor::OnCloseWindow( wxCloseEvent& event )
{
	// Actually we hide when we are closed
	Hide();
}

void CEdGradientEditor::RefreshGradients()
{
	// When Curve Editor changed some curves it request us to refresh gradient
	m_gradientColor->Refresh();
	m_gradientColor->Update();
	m_gradientAlpha->Refresh();
	m_gradientAlpha->Update();
}
