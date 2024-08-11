
#include "build.h"
#include "patToolPanel.h"
#include "../../common/core/depot.h"

BEGIN_EVENT_TABLE( CPatToolPanel, wxPanel )
END_EVENT_TABLE()

CPatToolPanel::CPatToolPanel( wxWindow* parent, const AnsiChar* funcName, void* data, Uint32 dataSize, IPatToolCallback* callback )
	: wxPanel( parent )
	, m_callback( callback )
	, m_working( false )
	, m_tool( nullptr )
{
	Connect( wxEVT_ERASE_BACKGROUND, wxEraseEventHandler( CPatToolPanel::OnClear ), nullptr, this );
	Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CPatToolPanel::OnKeyDown ), nullptr, this );
	Connect( wxEVT_KEY_UP, wxKeyEventHandler( CPatToolPanel::OnKeyUp ), nullptr, this );
	Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CPatToolPanel::OnMouseLeftDown ), nullptr, this );
	Connect( wxEVT_LEFT_UP, wxMouseEventHandler( CPatToolPanel::OnMouseLeftUp ), nullptr, this );
	Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( CPatToolPanel::OnMouseMidDown ), nullptr, this );
	Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( CPatToolPanel::OnMouseMidUp ), nullptr, this );
	Connect( wxEVT_MOTION, wxMouseEventHandler( CPatToolPanel::OnMouseMotion ), nullptr, this );
	Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( CPatToolPanel::OnMouseWheel ), nullptr, this );
	Connect( wxEVT_PAINT, wxPaintEventHandler( CPatToolPanel::OnPaint ), nullptr, this );
	Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CPatToolPanel::OnMouseRightDown ), nullptr, this );
	Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( CPatToolPanel::OnMouseRightUp ), nullptr, this );
	Connect( wxEVT_SIZE, wxSizeEventHandler( CPatToolPanel::OnSize ), nullptr, this );

	String toolPath;
	GDepot->GetAbsolutePath( toolPath );
	toolPath = toolPath.StringBefore( TXT("\\"), true );
	toolPath = toolPath.StringBefore( TXT("\\"), true );
	toolPath += TXT("\\bin\\tools\\red_engine_tools");

	if ( SetDllDirectoryW( toolPath.AsChar() ) )
	{
#if defined( RED_PLATFORM_WIN32 )
		m_handle = LoadLibraryW( TXT("red_engine_tools_32.dll") );
#elif defined( RED_PLATFORM_WIN64 )
		m_handle = LoadLibraryW( TXT("red_engine_tools_64.dll") );
#endif

		if( m_handle )
		{
			typedef pat_tool* (*get_faceed_joysticks)(  );
			get_faceed_joysticks func = (get_faceed_joysticks)GetProcAddress( m_handle, funcName );
			if( func )
			{
				m_tool = func();
				m_tool->attach( (HWND)GetHWND(), (AnsiChar*)data, dataSize );
			}
		}

		SetDllDirectory( nullptr );
	}
}

CPatToolPanel::~CPatToolPanel()
{
	// free library
	if ( m_tool )
	{
		m_tool->release();
	}
	FreeLibrary( m_handle );
}

void CPatToolPanel::SetData( void* data, Uint32 dataSize )
{
	if ( m_tool )
	{
		m_tool->setdata( (AnsiChar*)data, (Int32)dataSize );
	}
}

void CPatToolPanel::OnControlsPreChanged()
{
	m_working = true;
	if ( m_callback )
	{
		m_callback->OnPatToolControlsPreChanged();
	}
}

void CPatToolPanel::OnControlsChanging()
{
	if ( m_working && m_callback )
	{
		m_callback->OnPatToolControlsChanging();
	}
}

void CPatToolPanel::OnControlsPostChanged()
{
	m_working = false;
	if ( m_callback )
	{
		m_callback->OnPatToolControlsPostChanged();
	}
}

void CPatToolPanel::OnClear( wxEraseEvent& event )
{

}

void CPatToolPanel::OnKeyDown( wxKeyEvent& event )
{
	SetFocus();
	if ( m_tool )
	{
		m_tool->key( event.GetKeyCode() );
	}
	Refresh();
}

void CPatToolPanel::OnKeyUp( wxKeyEvent& event )
{
	SetFocus();
	if ( m_tool )
	{
		m_tool->keyup( event.GetKeyCode() );
	}
	Refresh();
}

void CPatToolPanel::OnMouseLeftDown( wxMouseEvent& event )
{
	SetFocus();
	if ( m_tool )
	{
		m_tool->lbuttondown( event.GetX(), event.GetY() );
	}
	Refresh();

	OnControlsPreChanged();
}

void CPatToolPanel::OnMouseLeftUp( wxMouseEvent& event )
{
	SetFocus();
	if ( m_tool )
	{
		m_tool->lbuttonup( event.GetX(), event.GetY() );
	}
	Refresh();

	OnControlsPostChanged();
}

void CPatToolPanel::OnMouseMidDown( wxMouseEvent& event )
{
	SetFocus();
	if ( m_tool )
	{
		m_tool->mbuttondown( event.GetX(), event.GetY() );
	}
	Refresh();
}

void CPatToolPanel::OnMouseMidUp( wxMouseEvent& event )
{
	SetFocus();
	if ( m_tool )
	{
		m_tool->mbuttonup( event.GetX(), event.GetY() );
	}
	Refresh();
}

void CPatToolPanel::OnMouseMotion( wxMouseEvent& event )
{
	if ( m_tool )
	{
		m_tool->move( event.GetX(), event.GetY() );
	}
	Refresh();

	OnControlsChanging();
}

void CPatToolPanel::OnMouseWheel( wxMouseEvent& event )
{
	SetFocus();
	if ( m_tool )
	{
		m_tool->wheel( event.GetWheelRotation() );
	}
	Refresh();
}

void CPatToolPanel::OnPaint( wxPaintEvent& event )
{
	if ( m_tool )
	{
		m_tool->draw();	
	}
	event.Skip();
}

void CPatToolPanel::OnMouseRightDown( wxMouseEvent& event )
{
	SetFocus();
	if ( m_tool )
	{
		m_tool->rbuttondown( event.GetX(), event.GetY() );
	}
	Refresh();
}

void CPatToolPanel::OnMouseRightUp( wxMouseEvent& event )
{
	SetFocus();
	if ( m_tool )
	{
		m_tool->rbuttonup( event.GetX(), event.GetY() );
	}
	Refresh();
}

void CPatToolPanel::OnSize( wxSizeEvent& event )
{
	if ( m_tool )
	{
		m_tool->resize( event.GetSize().GetX(), event.GetSize().GetY() );
	}
	event.Skip();
}
