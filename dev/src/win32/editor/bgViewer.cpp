/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "bgViewer.h"
#include "../../common/engine/asyncAnimTickManager.h"
#include "../../common/engine/animationManager.h"

wxIMPLEMENT_CLASS( CEdBgViewer, wxPanel );

CEdBgViewer::CEdBgViewer( wxWindow* parent, CWorld* world, CEdRenderingPanel* viewport )
	: wxPanel( parent )
	, m_world ( world )
	, m_viewport( viewport )
{
	wxBoxSizer* bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_gauge = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	bSizer1->Add( m_gauge, 0, wxALL|wxEXPAND, 5 );

	m_text1 = new wxStaticText( this, wxID_ANY, wxT("MyLabel1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_text1->Wrap( -1 );
	bSizer1->Add( m_text1, 0, wxALL, 5 );

	m_text2 = new wxStaticText( this, wxID_ANY, wxT("MyLabel2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_text2->Wrap( -1 );
	bSizer1->Add( m_text2, 0, wxALL, 5 );

	m_text3 = new wxStaticText( this, wxID_ANY, wxT("MyLabel3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_text3->Wrap( -1 );
	bSizer1->Add( m_text3, 0, wxALL, 5 );

	m_text4 = new wxStaticText( this, wxID_ANY, wxT("MyLabel4"), wxDefaultPosition, wxDefaultSize, 0 );
	m_text4->Wrap( -1 );
	bSizer1->Add( m_text4, 0, wxALL, 5 );

	m_text5 = new wxStaticText( this, wxID_ANY, wxT("MyLabel5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_text5->Wrap( -1 );
	bSizer1->Add( m_text5, 0, wxALL, 5 );

	m_text6 = new wxStaticText( this, wxID_ANY, wxT("MyLabel5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_text6->Wrap( -1 );
	bSizer1->Add( m_text6, 0, wxALL, 5 );

	m_text7 = new wxStaticText( this, wxID_ANY, wxT("MyLabel5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_text7->Wrap( -1 );
	bSizer1->Add( m_text7, 0, wxALL, 5 );

	m_text8 = new wxStaticText( this, wxID_ANY, wxT("MyLabel5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_text8->Wrap( -1 );
	bSizer1->Add( m_text8, 0, wxALL, 5 );

	m_text9 = new wxStaticText( this, wxID_ANY, wxT("MyLabel5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_text9->Wrap( -1 );
	bSizer1->Add( m_text9, 0, wxALL, 5 );

	this->SetSizer( bSizer1 );
	this->Layout();

	m_detachablePanel.Initialize( this, TXT( "BG" ) );
}

CEdBgViewer::~CEdBgViewer()
{

}

void CEdBgViewer::OnToolOpend()
{
	RefreshWidgets();
}

void CEdBgViewer::OnToolClosed()
{
	
}

void CEdBgViewer::RefreshWidgets()
{
#ifndef NO_DEBUG_PAGES
	CAsyncAnimTickManager::SDebugInfo info;
	GAnimationManager->Debug_GetAsynTickMgr()->Debug_GetInfo( info );

	m_gauge->SetValue( (Int32)( info.m_currTime / info.m_timeBudget * 100.f ) );

	m_text1->SetLabel( wxString::Format( TXT("Budget: %1.3f"), info.m_timeBudget ) );

	if ( info.m_currTime > info.m_timeBudget )
	{
		m_text2->SetLabel( wxString::Format( TXT("Real: %1.3f - OVERBUDGET"), info.m_currTime ) );
	}
	else
	{
		m_text2->SetLabel( wxString::Format( TXT("Real: %1.3f"), info.m_currTime ) );
	}

	if ( info.m_currTimeBudget < 1.f )
	{
		m_text3->SetLabel( wxString::Format( TXT("Curr: %1.3f - OVERBUDGET"), info.m_currTimeBudget ) );
	}
	else
	{
		m_text3->SetLabel( wxString::Format( TXT("Curr: %1.3f"), info.m_currTimeBudget ) );
	}


	if ( info.m_syncTime < 1.f )
	{
		m_text4->SetLabel( wxString::Format( TXT("Sync time: %1.3f (%1.3f)"), info.m_syncTime, info.m_syncTime - info.m_waitingTime ) );
	}
	else
	{
		m_text4->SetLabel( wxString::Format( TXT("Sync time: %1.3f (%1.3f) - OVERBUDGET"), info.m_syncTime, info.m_syncTime - info.m_waitingTime ) );
	}

	m_text5->SetLabel( wxString::Format( TXT("Waiting time: %1.3f"), info.m_waitingTime ) );

	m_text6->SetLabel( wxString::Format( TXT("Sync num: %d"), info.m_syncNum ) );

	m_text7->SetLabel( wxString::Format( TXT("Async num: %d"), info.m_asyncNum ) );

	if ( info.m_asyncRestNum == 0 )
	{
		m_text8->SetLabel( wxString::Format( TXT("Async rest num: %d"), info.m_asyncRestNum ) );
	}
	else
	{
		m_text8->SetLabel( wxString::Format( TXT("Async rest num: %d - OVERBUDGET"), info.m_asyncRestNum ) );
	}

	m_text9->SetLabel( wxString::Format( TXT("Buckets: %d"), info.m_bucketsNum ) );
#endif
}

Bool CEdBgViewer::OnViewportTick( IViewport* view, Float timeDelta )
{
	RefreshWidgets();
	return false;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CEdBgViewerTool );

Bool CEdBgViewerTool::Start(	CEdRenderingPanel* viewport, 
								CWorld* world, 
								wxSizer* panelSizer, 
								wxPanel* panel, 
								const TDynArray< CComponent* >& selection )
{
	// Remember world
	m_world = world;

	// Remember viewport
	m_viewport = viewport;

	// Create tool panel
	m_toolPanel = new CEdBgViewer( panel, m_world, viewport );

	// Create panel for custom window
	panelSizer->Add( m_toolPanel, 1, wxEXPAND, 5 );
	panel->Layout();

	// Only entities
	world->GetSelectionManager()->SetGranularity( CSelectionManager::SG_Entities );

	m_toolPanel->OnToolOpend();

	// Initialized
	return true;
}

void CEdBgViewerTool::End()
{
	if ( m_toolPanel )
	{
		m_toolPanel->OnToolClosed();
	}

	m_toolPanel = NULL;
}

Bool CEdBgViewerTool::OnViewportTick( IViewport* view, Float timeDelta )
{
	return m_toolPanel ? m_toolPanel->OnViewportTick( view, timeDelta ) : false;
}
