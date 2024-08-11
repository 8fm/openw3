/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */
#include "build.h"
#include "wx/graphics.h"
#include "popupNotification.h"

namespace
{
	const int SIZE_W           = 300;
	const int SIZE_H           = 85;
	const int MARGIN           = 5;
	const int MARGIN_TO_PARENT = 10;
	const int CORNER_RADIUS    = 15;

	const int WAIT_TIME        = 500; // [ms]
	const int FADEOUT_FRAME    = 15;  // [ms]
	const int FADEOUT_TIME     = 300; // [ms]
	const int INITIAL_ALPHA    = 220; // [0-255]

	const int WAIT_TIMER_ID    = 1;
	const int FADEOUT_TIMER_ID = 2;
}


CEdPopupNotification::CEdPopupNotification()
	: wxTopLevelWindow( NULL, wxID_ANY, TXT(""), wxPoint( 0, 0 ), wxSize( SIZE_W, SIZE_H ), wxNO_BORDER|wxSTAY_ON_TOP|wxFRAME_SHAPED|wxFRAME_NO_TASKBAR )
	, m_waitTimer( this, WAIT_TIMER_ID )
	, m_fadeOutTimer( this, FADEOUT_TIMER_ID )
{
	ASSERT( SIsMainThread(), TXT("The window must be created in the main thread") );

	wxGraphicsPath path = wxGraphicsRenderer::GetDefaultRenderer()->CreatePath();
	path.AddRoundedRectangle( 0, 0, SIZE_W, SIZE_H, CORNER_RADIUS );
	SetShape( path );

	m_titleLabel = new wxStaticText( 
			this, wxID_ANY, TXT(""), wxPoint( MARGIN, MARGIN ), wxSize( SIZE_W-2*MARGIN, 35 ), 
			wxALIGN_CENTRE_HORIZONTAL|wxST_NO_AUTORESIZE|wxST_ELLIPSIZE_END
		);
	m_titleLabel->SetFont( wxFont( 20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD ) );

	m_messageLabel = new wxStaticText( 
			this, wxID_ANY, TXT(""), wxPoint( MARGIN, 50 ), wxSize( SIZE_W-2*MARGIN, 35 ), 
			wxALIGN_CENTRE_HORIZONTAL|wxST_NO_AUTORESIZE|wxST_ELLIPSIZE_END
		);
	m_messageLabel->SetFont( wxFont( 15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL ) );

	Connect( wxEVT_TIMER, wxTimerEventHandler( CEdPopupNotification::OnTimer ), NULL, this );
}

wxPoint CEdPopupNotification::CalculatePosition( wxWindow* parent ) const
{
	wxPoint parentPos  = parent->GetScreenPosition();
	wxSize  parentSize = parent->GetSize();
	
	switch ( m_location )
	{
	case CENTER:
	default:
		return parentPos + ( parentSize-wxSize( SIZE_W, SIZE_H ) ) / 2;
	case TOP_LEFT:
		return parentPos + wxSize( MARGIN_TO_PARENT, MARGIN_TO_PARENT );
	case TOP_RIGHT:
		return parentPos + wxSize( parentSize.x - MARGIN_TO_PARENT - SIZE_W, MARGIN_TO_PARENT );
	case BOTTOM_LEFT:
		return parentPos + wxSize( MARGIN_TO_PARENT, parentSize.y - MARGIN_TO_PARENT - SIZE_H );
	case BOTTOM_RIGHT:
		return parentPos + wxSize( parentSize.x - MARGIN_TO_PARENT - SIZE_W, parentSize.y - MARGIN_TO_PARENT - SIZE_H );
	}
}

void CEdPopupNotification::Show( wxWindow* parent, const String& title, const String& message )
{
	ASSERT( SIsMainThread(), TXT("This method must be called from the main thread") );

	SetPosition( CalculatePosition( parent ) );

	m_titleLabel->SetLabel( title.AsChar() );
	m_messageLabel->SetLabel( message.AsChar() );

	m_fadeOutTimer.Stop();
	SetTransparent( INITIAL_ALPHA );

	Show();
	parent->SetFocus();

	m_waitTimer.Start( WAIT_TIME, true );
}

void CEdPopupNotification::OnTimer( wxTimerEvent& evt )
{
	switch ( evt.GetTimer().GetId() )
	{
	case WAIT_TIMER_ID:
		{
			m_fadeOutTimer.Start( FADEOUT_FRAME );
			m_stopWatch.Start();
		}
		break;

	case FADEOUT_TIMER_ID:
		{
			long timeSinceStart = m_stopWatch.Time();
			int alpha = INITIAL_ALPHA - INITIAL_ALPHA * timeSinceStart / FADEOUT_TIME;
			if ( alpha <= 0 )
			{
				m_fadeOutTimer.Stop();
				m_stopWatch.Pause();
				Hide();
			}
			else
			{
				SetTransparent( alpha );
			}
		}
		break;
	}
}

WXLRESULT CEdPopupNotification::MSWWindowProc( WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam )
{
    if ( nMsg == WM_NCHITTEST )
	{
		return HTTRANSPARENT;
    }
	else
	{
	    return wxTopLevelWindow::MSWWindowProc(nMsg, wParam, lParam);
	}
}

void CEdPopupNotification::SetLocation( Location pos )
{
	m_location = pos;
}

CEdPopupNotification::Location CEdPopupNotification::GetLocation() const
{
	return m_location;
}

void CEdPopupNotification::LoadOptionsFromConfig()
{
    CUserConfigurationManager& config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/PopupNotification") );
    m_location = static_cast< Location >( config.Read( TXT("Location"), CENTER ) );
}

void CEdPopupNotification::SaveOptionsToConfig()
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/PopupNotification") );
	config.Write( TXT("Location"), static_cast< Int32 >( m_location ) );
}
