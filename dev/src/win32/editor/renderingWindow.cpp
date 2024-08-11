/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderingWindow.h"

#include "googleanalytics.h"

#include <wx\display.h>
#include "..\..\common\engine\renderer.h"
#include "..\..\common\engine\viewport.h"
#include "..\..\common\engine\renderSettings.h"

BEGIN_EVENT_TABLE( CEdRenderingWindow, wxPanel )
	EVT_ERASE_BACKGROUND( CEdRenderingWindow::OnEraseBackground )
	EVT_PAINT( CEdRenderingWindow::OnPaint )
	EVT_SIZE( CEdRenderingWindow::OnSize )
END_EVENT_TABLE()

CEdRenderingWindow::CEdRenderingWindow( wxWindow* parent )
	: wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxTRANSPARENT_WINDOW | wxCLIP_CHILDREN )
{
	Int32 fullScreenWidth = 0, fullScreenHeight = 0;
	Uint32 displayCount = ::wxDisplay::GetCount();
	for ( Uint32 i = 0; i < displayCount; i++ )
	{
		fullScreenWidth = max( fullScreenWidth, ::wxDisplay(i).GetGeometry().width );
		fullScreenHeight = max( fullScreenHeight, ::wxDisplay(i).GetGeometry().height );
	}
	
	// TODO MAREK MERGE
	/*
	String keyboardLayout = TXT("QWERTY");
	// DO NOT USE GCONFIG
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("Input"), TXT("KeyboardLayout"), keyboardLayout );
	m_viewport = GRender->CreateViewport( (HWND)wxTheFrame->GetHandle(), (HWND) GetHandle(), TXT("EditorViewport"), fullScreenWidth, fullScreenHeight, false, keyboardLayout, SRenderSettingsManager::GetInstance().GetSettings().m_vsync );
	*/

	m_viewport = GRender->CreateViewport( (HWND)wxTheFrame->GetHandle(), (HWND) GetHandle(), TXT("EditorViewport"), fullScreenWidth, fullScreenHeight, VWM_Windowed, Config::cvVSync.Get() );

	ANALYTICS_EVENT_T( "Init", "5. viewport initialized" );
}

CEdRenderingWindow::~CEdRenderingWindow()
{}

void CEdRenderingWindow::OnEraseBackground( wxEraseEvent &event )
{
	// Do not erase background
}

void CEdRenderingWindow::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc( this );
}

void CEdRenderingWindow::OnSize( wxSizeEvent& event )
{
	// Adjust viewport size
	if ( m_viewport )
	{
		// mcinek hack: don't allow to viewport width/height be ever 0
		m_viewport->AdjustSize( Max( 1, event.GetSize().GetWidth() ), Max( 1, event.GetSize().GetHeight() ) );
	}
}
