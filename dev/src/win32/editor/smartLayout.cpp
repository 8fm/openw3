#include "build.h"
#include "smartLayout.h"

wxDEFINE_EVENT( wxEVT_CLOSE_PANEL, wxCloseEvent );

//////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(wxSmartLayoutPanel, wxPanel)

wxBitmap LoadBitmap( const wxString & fileName )
{
	if (fileName.empty()) return wxNullBitmap;

	wxFSFile *fsfile = wxFileSystem().OpenFile(fileName, wxFS_READ | wxFS_SEEKABLE);
	if (fsfile == NULL)
	{
		WARN_EDITOR( TXT( "XRC resource: Cannot create bitmap from '%s'." ), fileName.wc_str() );
		return wxNullBitmap;
	}
	wxImage img(*(fsfile->GetStream()));
	delete fsfile;

	if (!img.Ok())
	{
		WARN_EDITOR( TXT("XRC resource: Cannot create bitmap from '%s'."), fileName.wc_str());
		return wxNullBitmap;
	}

	return wxBitmap(img);
}

wxSmartLayoutPanel::wxSmartLayoutPanel( wxWindow* parent, const String& caption, Bool donotCloseButHide,
										wxWindow *child, wxMenuBar *menuBar /*=NULL*/,
										Bool cloneFullMenuOnDock /*=false*/ )
	: wxPanel()
	, m_frame(NULL)
	, m_frameMenuBar(NULL)
	, m_dockedMenuBar(NULL)
	, m_notebook(NULL)
	, m_recurencyPreventionOn(false)
	, m_openDocked(false)
	, m_donotCloseButHide(donotCloseButHide)
	, m_cloneFullMenuOnDock(cloneFullMenuOnDock)
	, m_lastMenuEvent(NULL)
{
	m_frame = new wxSmartLayoutFrame( parent, caption.AsChar() );

	m_frame->Bind(wxEVT_CLOSE_WINDOW, &wxSmartLayoutPanel::OnFrameClose, this);
	Bind(wxEVT_CLOSE_WINDOW, &wxSmartLayoutPanel::OnClose, this);

	m_originalLabel = m_frame->GetLabel();

	m_frameMenuBar = new wxMenuBar();
	{
		wxMenu	 *menuView = new wxMenu();
		wxMenuItem *menuDock = new wxMenuItem(menuView, XRCID( "viewDock" ), TXT("Dock\tCtrl-D"), wxEmptyString, wxITEM_CHECK);
		menuView->Append(menuDock);
		m_frameMenuBar->Append( menuView, TXT("&View") );

		Bind( wxEVT_COMMAND_MENU_SELECTED, &wxSmartLayoutPanel::OnDock, this, XRCID( "viewDock" ) );
		m_frame->Bind( wxEVT_COMMAND_MENU_SELECTED, &wxSmartLayoutPanel::OnParentMenu, this );
	}
	m_frame->SetMenuBar(m_frameMenuBar);

	if (menuBar)
	{
		wxMenuUtils::CloneMenuBar(menuBar,		m_dockedMenuBar, !cloneFullMenuOnDock );
		wxMenuUtils::CloneMenuBar(m_frameMenuBar, m_dockedMenuBar, false );
	}
	
	Create(m_frame, wxID_ANY, wxDefaultPosition, wxSize(600, 500));

	if (child)
	{
		// Inner panel sizer
		wxSizer *inner_sizer = inner_sizer = new wxBoxSizer( wxVERTICAL );

		child->Reparent( this );
		child->Show();
		inner_sizer->Add( child, 1, wxEXPAND, 0 );

		SetSizer( inner_sizer );
	}

	wxBoxSizer* frame_sizer = new wxBoxSizer( wxVERTICAL );
	frame_sizer->Add( this, 1, wxEXPAND | wxALL, 0 );
	m_frame->SetSizer(frame_sizer);
	
	Layout();
	m_frame->Layout();
}

wxSmartLayoutPanel::wxSmartLayoutPanel( wxWindow* parent, const String& type, Bool donotCloseButHide )
	: m_frame(NULL)
	, m_frameMenuBar(NULL)
	, m_dockedMenuBar(NULL)
	, m_notebook(NULL)
	, m_recurencyPreventionOn(false)
	, m_openDocked(false)
	, m_donotCloseButHide(donotCloseButHide)
	, m_cloneFullMenuOnDock(false)
	, m_lastMenuEvent(NULL)
{
	m_frame = new wxSmartLayoutFrame();

	m_frame->Bind( wxEVT_CLOSE_WINDOW, &wxSmartLayoutPanel::OnFrameClose, this );
	Bind( wxEVT_CLOSE_WINDOW, &wxSmartLayoutPanel::OnClose, this );
	
	wxXmlResource::Get()->LoadFrame( m_frame, NULL, type.AsChar() );
	m_originalLabel = m_frame->GetLabel();
	m_frame->Layout();

	m_frameMenuBar = m_frame->GetMenuBar();
	if (!m_frameMenuBar)
	{
		m_frameMenuBar = new wxMenuBar();
		wxMenu	 *menuView = new wxMenu();
		wxMenuItem *menuDock = new wxMenuItem(menuView, XRCID( "viewDock" ), TXT("Dock\tCtrl-D"), wxEmptyString, wxITEM_CHECK);
		menuView->Append(menuDock);
		m_frameMenuBar->Append( menuView, TXT("&View") );
		m_frame->SetMenuBar(m_frameMenuBar);
	}
	else
	{
		wxMenu* menuView = NULL;
		wxMenuItem *menuDock = NULL;

		Bool separatorNeeded = false;
		int viewMenu = m_frameMenuBar->FindMenu( TXT("View") );
		if ( viewMenu == wxNOT_FOUND )
		{
			menuView = new wxMenu();
			m_frameMenuBar->Append( menuView, TXT("&View") );
		}
		else
		{
			menuView = m_frameMenuBar->GetMenu( viewMenu );
			separatorNeeded = true;
		}

		int viewDock = menuView->FindItem( TXT("Dock") );
		if ( viewDock == wxNOT_FOUND )
		{
			menuDock = new wxMenuItem(menuView, XRCID( "viewDock" ), TXT("Dock\tCtrl-D"), wxEmptyString, wxITEM_CHECK);
			if ( separatorNeeded )
				menuView->AppendSeparator();
			menuView->Append( menuDock );
		}
		else
		{
			menuDock = menuView->FindItem( viewDock );
		}
	}

	Bind( wxEVT_COMMAND_MENU_SELECTED, &wxSmartLayoutPanel::OnDock, this, XRCID( "viewDock" ) );
	m_frame->Bind(wxEVT_COMMAND_MENU_SELECTED, &wxSmartLayoutPanel::OnParentMenu, this );
	
	wxWindowList windowlist = m_frame->GetChildren();
	
	Create(m_frame, wxID_ANY, wxDefaultPosition, wxSize(600, 500));

	// Inner panel sizer
	wxSizer *inner_sizer = inner_sizer = new wxBoxSizer( wxVERTICAL );

	wxWindowList::iterator wnd_curr = windowlist.begin(),
						   wnd_last = windowlist.end();
	for ( ; wnd_curr != wnd_last; ++wnd_curr )
	{
			(*wnd_curr)->Reparent( this );
				inner_sizer->Add( *wnd_curr, 1, wxEXPAND, 0 );
		}
		SetSizer( inner_sizer );

	// Outer panel sizer
	wxBoxSizer* frame_sizer = new wxBoxSizer( wxVERTICAL );
	frame_sizer->Add( this, 1, wxEXPAND | wxALL, 0 );
	m_frame->SetSizer(frame_sizer);
	
	Layout();
	m_frame->Layout();
}

wxSmartLayoutPanel::~wxSmartLayoutPanel()
{
	if ( m_frame )
	{
		m_frame->Unbind( wxEVT_CLOSE_WINDOW, &wxSmartLayoutPanel::OnFrameClose, this );
		//m_frame->Disconnect(wxID_ANY, wxID_ANY, wxEVT_NULL, NULL, NULL, this);
		m_frame->Destroy();
		m_frame = NULL;
	}
}

void wxSmartLayoutPanel::OnClose( wxCloseEvent& event )
{
	ClosePanel();
	event.Skip();
}

void wxSmartLayoutPanel::OnClosePage( wxAuiNotebookEvent& event )
{
	if ( event.GetEventObject() == m_notebook && m_notebook->GetPage( event.GetSelection() ) == this )
	{
		ClosePanel();
		event.Veto();
	}
	else
	{
		event.Skip();
	}
}

void wxSmartLayoutPanel::OnChangePage( wxAuiNotebookEvent& event )
{
	if ( event.GetEventObject() == m_notebook && m_notebook->GetPage( event.GetSelection() ) == this )
	{
		AttachMenuBar();
	}
	
	event.Skip();
}

void wxSmartLayoutPanel::OnFrameClose( wxCloseEvent& event )
{
	SaveOptionsToConfig();
	ClosePanel();
}

void wxSmartLayoutPanel::ClosePanel()
{
	if (IsDocked())
	{
		Bool openDocked = m_openDocked;
		Undock(true);
		m_openDocked = openDocked;
	}
	else if (m_frame)
	{
		m_frame->Hide();
	}

	wxCommandEvent eventEvent( wxEVT_CLOSE_PANEL );
	eventEvent.SetEventObject(this);
	ProcessEvent( eventEvent );

	if (m_frame && !m_donotCloseButHide)
	{
		m_frame->Destroy();
		m_frame = NULL;
	}
}

void wxSmartLayoutPanel::SaveLayout(const String &identifier)
{
	if (m_frame) m_frame->SaveLayout(identifier);

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	config.Write(identifier + TXT("/Docked") , m_openDocked ? 1 : 0);
	if (m_donotCloseButHide)
		config.Write(identifier + TXT("/Visible"), IsVisible()  ? 1 : 0 );
}

void wxSmartLayoutPanel::LoadLayout( const String &identifier )
{
	if (m_frame) m_frame->LoadLayout(identifier);

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	m_openDocked = config.Read(identifier + TXT("/Docked"), 0) != 0;

	if (m_donotCloseButHide)
	{
		if ( config.Read( identifier + TXT("/Visible"), 0 ) != 0)
			Show();
		else if ( !AlwaysVisible() )
			ClosePanel();
	}
}

void wxSmartLayoutPanel::OnParentMenu( wxCommandEvent& event )
{
	if (m_lastMenuEvent == &event) // prevent infinite recursion, if the event should be processed by the parent
	{
		event.Skip();
		return;
	}

	m_lastMenuEvent = &event;
	this->ProcessEvent(event);
	m_lastMenuEvent = NULL;
}

void wxSmartLayoutPanel::Dock()
{
	if (m_frameMenuBar)
	{
		wxMenuItem *dockItem = m_frameMenuBar->FindItem( XRCID( "viewDock" ) );
		if (dockItem)
			dockItem->Check();
	}
	if (m_dockedMenuBar)
	{
		wxMenuItem *dockItem = m_dockedMenuBar->FindItem( XRCID( "viewDock" ) );
		if (dockItem)
			dockItem->Check();
	}

	m_openDocked = true;

	m_frame->Hide();
	Hide();
	CEdFrame *mainFrame = static_cast<CEdFrame*>( wxTheFrame );
	m_notebook = mainFrame->GetEditorsBar( );
	Reparent(m_notebook);
	Show();
	m_notebook->Layout();
}

void wxSmartLayoutPanel::Undock(Bool andHide /* = false */)
{
	if (m_frameMenuBar)
	{
		wxMenuItem *dockItem = m_frameMenuBar->FindItem( XRCID( "viewDock" ) );
		if (dockItem)
			dockItem->Check(false);
	}

	m_openDocked = false;

	if (m_notebook)
	{
		Hide();
		this->Reparent(m_frame);
		for( Uint32 i=0; i<m_notebook->GetPageCount(); ++i )
			if( m_notebook->GetPage( i ) == this )
			{
				m_notebook->RemovePage( i );
				break;
			}
		m_notebook->Unbind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, &wxSmartLayoutPanel::OnClosePage, this );
		m_notebook->Unbind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, &wxSmartLayoutPanel::OnChangePage, this );
		
		// Show the panel, but do not show the frame
		m_recurencyPreventionOn = true;
		Show();
		m_recurencyPreventionOn = false;

		// Layout and Show/Hide the frame
		m_frame->Layout();
		m_frame->Show( !andHide );
	}
}

void wxSmartLayoutPanel::OnDock( wxCommandEvent& event )
{
	if (!IsDocked())
		Dock();
	else
		Undock();
}

void wxSmartLayoutPanel::AttachMenuBar()
{
	wxFrame *top = static_cast<wxFrame*>( wxGetTopLevelParent(this) );
	if (top->GetMenuBar() != m_frameMenuBar)
	{
		if (!m_dockedMenuBar)
		{
			wxMenuUtils::CloneMenuBar( m_frameMenuBar, m_dockedMenuBar );
			
			CEdFrame *mainFrame = static_cast<CEdFrame*>( wxTheFrame );
			wxMenuUtils::CloneMenuBar( mainFrame->GetMenuBarToMerge(), m_dockedMenuBar );
		}

		top->SetMenuBar(m_dockedMenuBar);

		// What is this doing?!
		while ( top->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED ) ) {}
		
		top->Bind(wxEVT_COMMAND_MENU_SELECTED, &wxSmartLayoutPanel::OnParentMenu, this );
	}
}

bool wxSmartLayoutPanel::IsVisible()
{
	if (!IsDocked())
		return m_frame->IsVisible();
	return true;
}

bool wxSmartLayoutPanel::Show( bool show /*= true*/ )
{
	Bool res = wxPanel::Show( show );

	if (!IsDocked())
	{
		if (show && m_openDocked)
			Dock();
		else
		if (!m_recurencyPreventionOn)
		{
			Red::System::ScopedFlag< Bool > safeFlag(m_recurencyPreventionOn = true, false);
			m_frame->Show( show );
		}
	}
	else if ( m_notebook && GetParent() == m_notebook )
	{
		if (show && !m_recurencyPreventionOn)
		{
			Red::System::ScopedFlag< Bool > safeFlag(m_recurencyPreventionOn = true, false);

			for( Uint32 i=0; i<m_notebook->GetPageCount(); ++i )
				if( m_notebook->GetPage( i ) == this )
				{
					if ( m_notebook->GetSelection() != i )
						m_notebook->SetSelection( i );
					return res;
				}

			m_notebook->AddPage( this, GetShortTitle(), true, wxBitmap( m_frame->GetIcon() ) );
			m_notebook->Bind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, &wxSmartLayoutPanel::OnClosePage, this );
			m_notebook->Bind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, &wxSmartLayoutPanel::OnChangePage, this );
			AttachMenuBar();
		}
	}
	return res;
}

void wxSmartLayoutPanel::CheckMenuItem( Int32 id, Bool state )
{
	if ( m_frameMenuBar )
	{
		wxMenuItem *item = m_frameMenuBar->FindItem( id );
		if ( item ) item->Check( state );
	}
	if ( m_dockedMenuBar )
	{
		wxMenuItem *item = m_dockedMenuBar->FindItem( id );
		if ( item ) item->Check( state );
	}
}

//////////////////////////////////////////////////////////////////////////////////////

void ISmartLayoutWindow::OnActivate( wxActivateEvent& event )
{
	if( event.GetActive() && wxTheFrame )
		wxTheFrame->PushWindow( m_this );
}

IMPLEMENT_CLASS( wxSmartLayoutFrame, wxFrame );

wxSmartLayoutFrame::~wxSmartLayoutFrame()
{
	// destroy children before the frame, because they may want to save the frame in their constructors
	DestroyChildren();

	if (wxTheFrame)
		wxTheFrame->WindowDestroyed( this );
}

wxSmartLayoutFrame::wxSmartLayoutFrame() : ISmartLayoutWindow( this )
{
	Bind( wxEVT_ACTIVATE, &wxSmartLayoutFrame::OnActivate, this );
	wxTheFrame->WindowCreated( this );
}

wxSmartLayoutFrame::wxSmartLayoutFrame( wxWindow* parent, const String& caption )
	: wxFrame( parent, wxID_ANY, caption.AsChar() ), ISmartLayoutWindow( this )
{
	Bind( wxEVT_ACTIVATE, &wxSmartLayoutFrame::OnActivate, this );
	wxTheFrame->WindowCreated( this );
}

void ISmartLayoutWindow::SaveLayout( const String &identifier, Bool sizeOnly )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, identifier );

	// save frame position & size
	// get info from native window placement to keep last (not maximized) size
	WINDOWPLACEMENT wp;
	GetWindowPlacement( (HWND) m_this->GetHWND(), &wp );

	Int32 x = wp.rcNormalPosition.left;
	Int32 y = wp.rcNormalPosition.top;
	Int32 w = wp.rcNormalPosition.right - x;
	Int32 h = wp.rcNormalPosition.bottom - y;

	// As GetWindowPlacement gets the workspace (not screen) coordinates, we have to take care of it (see SmartSetSize)
	WorkspaceToScreen( x, y );

	if ( !sizeOnly )
	{
		config.Write( TXT("X"), x );
		config.Write( TXT("Y"), y );
	}

	config.Write( TXT("Width"), w );
	config.Write( TXT("Height"), h );

	config.Write( TXT("Maximized"), (long) m_this->IsMaximized() );
}

void ISmartLayoutWindow::SmartSetSize( Int32 x, Int32 y, Int32 w, Int32 h )
{
	if ( w < 0 || h < 0 )
	{ // wxDefaultSize passed, probably
		wxSize s = m_this->GetDefaultSize();
		w = s.x;
		h = s.y;
	}

	// keep windows onscreen
	Int32 rect_x = ::GetSystemMetrics( SM_XVIRTUALSCREEN );
	Int32 rect_y = ::GetSystemMetrics( SM_YVIRTUALSCREEN );
	Int32 rect_width = ::GetSystemMetrics( SM_CXVIRTUALSCREEN );
	Int32 rect_height = ::GetSystemMetrics( SM_CYVIRTUALSCREEN );

	if ( x + w > rect_x + rect_width ) x = rect_x + rect_width - w;
	if ( x < rect_x ) x = rect_x;
	if ( y + h > rect_y + rect_height ) y = rect_y + rect_height - h;
	if ( y < rect_y ) y = rect_y;

	// As SetWindowPlacement gets the workspace (not screen) coordinates, we have to take care of it
	// (an user probably just wants to use screen coord, as this us what he/she usually gets from wxWidgets stuff)
	ScreenToWorkspace( x, y );

	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	wp.flags = 0;
	wp.showCmd = m_this->IsVisible() ? SW_SHOWNA : SW_HIDE;
	wp.rcNormalPosition.left = x;
	wp.rcNormalPosition.top = y;
	wp.rcNormalPosition.right = x + w;
	wp.rcNormalPosition.bottom = y + h;
	// SetSize( x, y, w, h );

	SetWindowPlacement( (HWND)m_this->GetHWND(), &wp );
}

void ISmartLayoutWindow::LoadLayout( const String &identifier, Bool sizeOnly )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, identifier );

	int defaultWindowWidth = 0;
	int defaultWindowHeight = 0;
	m_this->GetSize( &defaultWindowWidth, &defaultWindowHeight );
	if ( defaultWindowWidth == 0 ) defaultWindowWidth = 1024;
	if ( defaultWindowHeight == 0 ) defaultWindowHeight = 768;

	int defaultWindowPosX = 0;
	int defaultWindowPosY = 0;
	m_this->GetPosition( &defaultWindowPosX, &defaultWindowPosY );

	Int32 x = sizeOnly ? defaultWindowPosX : config.Read( TXT("X"), 50 );
	Int32 y = sizeOnly ? defaultWindowPosY : config.Read( TXT("Y"), 50 );
	Int32 w = config.Read( TXT("Width"), defaultWindowWidth );
	Int32 h = config.Read( TXT("Height"), defaultWindowHeight );

	SmartSetSize( x, y, w, h );
	
	bool maximized = ( config.Read( TXT("Maximized"), 0 ) == 1 );
	
	if ( maximized )
	{
		// Window must be visible during the maximization,
		// otherwise wxWidgets uses default display size for window layout -> wrong layout
		Bool isVisible = m_this->IsVisible();
		if ( ! isVisible )
			m_this->Show( true );

		m_this->Maximize( true );

		if ( ! isVisible )
			m_this->Show( false );
	}
	else
	{
		m_this->Maximize( false );
	}
}

//////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS( wxSmartLayoutDialog, wxDialog )

wxSmartLayoutDialog::wxSmartLayoutDialog() : ISmartLayoutWindow(this)
{
	Bind( wxEVT_ACTIVATE, &wxSmartLayoutDialog::OnActivate, this );
	wxTheFrame->WindowCreated( this );
}

wxSmartLayoutDialog::~wxSmartLayoutDialog()
{
	if (wxTheFrame)
		wxTheFrame->WindowDestroyed( this );
}

//////////////////////////////////////////////////////////////////////////////////////

void wxSmartLayoutAuiNotebook::Init()
{
	Bind( wxEVT_COMMAND_AUINOTEBOOK_END_DRAG, &wxSmartLayoutAuiNotebook::OnEndDrag, this );
}

void wxSmartLayoutAuiNotebook::OnEndDrag( wxAuiNotebookEvent &event )
{
	event.Skip();

	if (event.GetSelection() < 0)
		return;

	// Undocking
	wxAuiTabCtrl	* src_tabs = (wxAuiTabCtrl*)event.GetEventObject();
	wxAuiNotebookPage page	 = src_tabs->GetPage( event.GetSelection() );
	if (!page.window->IsKindOf(CLASSINFO(wxSmartLayoutPanel)))
		return;

	wxPoint   mouse_screen_pt = ::wxGetMousePosition();
	wxWindow* tab_ctrl		= ::wxFindWindowAtPoint(mouse_screen_pt);
	while (tab_ctrl)
	{
		if (tab_ctrl == this)
			return;
		tab_ctrl = tab_ctrl->GetParent();
	}	
	
	wxSmartLayoutPanel *panel = static_cast<wxSmartLayoutPanel*>( page.window );
	panel->Undock();
}

void wxSmartLayoutAuiNotebook::SaveOptionsToConfig( )
{
	CConfigurationManager &config = SUserConfigurationManager::GetInstance();

	wxString sPanes;

	wxAuiPaneInfoArray &panes = m_mgr.GetAllPanes();
	for (Uint32 i = 0; i < panes.size(); ++i)
	{
		wxAuiPaneInfo &pane = panes[i];

		if (pane.name == TXT("dummy"))
			continue;

		sPanes += TXT("|pane=") + pane.name;
		
		wxAuiTabCtrl *tabControl = NULL;
		int		   tabCtrlIdx = -1;

		// Find tabControl inside the current pane
		for (Uint32 iPage = 0; iPage < GetPageCount(); ++iPage)
		{
			wxWindow *page = GetPage(iPage);
			wxPoint point = page->GetPosition();
			if (point.x >= pane.rect.x && point.x <= pane.rect.x + pane.rect.width &&
				point.y >= pane.rect.y && point.y <= pane.rect.y + pane.rect.height )
			{
				FindTab(page, &tabControl, &tabCtrlIdx);
				break;
			}
		}

		// Should not happen
		if (!tabControl) continue;

		// Append tabs inside the current pane
		for (Uint32 iPage = 0; iPage < tabControl->GetPageCount(); ++iPage)
		{
			sPanes += TXT(";tab=") + tabControl->GetPage(iPage).caption;
		}
	}
	config.Write(TXT("/NotebookPanes"), String(sPanes.GetData()));

	wxString perspective = m_mgr.SavePerspective();
	config.Write(TXT("/NotebookLayout"), String(perspective.GetData()));
}

void wxSmartLayoutAuiNotebook::LoadOptionsFromConfig( )
{
	CConfigurationManager &config = SUserConfigurationManager::GetInstance();

	String sPanes;
	sPanes = config.Read(TXT("/NotebookPanes"), sPanes);

	if (sPanes.Empty())
		return;

	struct SPane
	{
		String			m_name;
		TDynArray<String> m_tabs;
	};
	TDynArray<SPane> panes;
	
	// Load pane info
	while (1)
	{
		String sLeft;
		String sPane;
		if ( !sPanes.Split(TXT("|pane="), &sLeft, &sPane, true ) )
			break;
		sPanes = sLeft;
		if ( sPane.Empty() )
			continue;

		SPane pane;

		String sTabs;
		sPane.Split(TXT(";tab="), &pane.m_name, &sTabs, false);
		sTabs = TXT(";tab=") + sTabs;

		while (1)
		{
			String sTab;
			if ( !sTabs.Split(TXT(";tab="), &sLeft, &sTab, true) )
				break;
			sTabs = sLeft;
			if ( sTab.Empty() )
				continue;

			pane.m_tabs.PushBack(sTab);
		}

		if (pane.m_tabs.Size() == 0)
			continue;

		panes.PushBack(pane);
	}

	// Restore single-notebook view
	for ( Uint32 i = 1; i < GetPageCount(); ++i )
	{
		wxWindow *win   = GetPage	  ( i );
		wxString  title = GetPageText  ( i );
		wxBitmap  image = GetPageBitmap( i );
		RemovePage( i );
		InsertPage( i, win, title, false, image );
	}

	// Restore notebook layout
	for ( Uint32 iPane = 0; iPane < panes.Size(); ++iPane )
	{
		SPane &pane = panes[iPane];

		// Find page to split to this pane
		Uint32 iPage = 0;
		for (Uint32 i = 0; i < GetPageCount(); ++i)
			if ( GetPageText(i) == pane.m_tabs[0].AsChar() )
			{
				iPage = i;
				break;
			}
		
		// Split the page to the new pane
		if (iPane < panes.Size() - 1)
		{
			Split(iPage, wxBOTTOM);
			m_mgr.GetAllPanes()[ m_mgr.GetAllPanes().size()-1 ].Name(pane.m_name.AsChar());

			// Move other tabs to this pane
			for ( Uint32 iTab = 1; iTab < pane.m_tabs.Size(); ++iTab )
				for ( Uint32 iPageSrc = 0; iPageSrc < GetPageCount(); ++iPageSrc )
					if ( GetPageText(iPageSrc) == pane.m_tabs[iTab].AsChar() )
						MovePageToPane( GetPage(iPageSrc), GetPage(iPage), 0 );
		}
		else
			m_mgr.GetAllPanes()[ 1 ].Name(pane.m_name.AsChar());
		++iPage;
	}

	String perspective;
	perspective = config.Read(TXT("/NotebookLayout"), perspective);
	if ( perspective.GetLength() )
		m_mgr.LoadPerspective( perspective.AsChar() );

	Layout();
}

void wxSmartLayoutAuiNotebook::MovePageToPane(wxWindow *srcPage, wxWindow *dstPage, Int32 dstIndex /*= -1*/)
{
	m_mgr.HideHint();

	wxAuiTabCtrl* src_tabs  = NULL;
	wxAuiTabCtrl* dest_tabs = NULL;
	int		   src_tabIdx = -1;
	int		   dst_tabIdx = -1;
	FindTab(srcPage, &src_tabs,  &src_tabIdx);
	FindTab(dstPage, &dest_tabs, &dst_tabIdx);

	if (!src_tabs || !dest_tabs)
		return;

	// only perform a tab split if it's allowed
	if ((m_flags & wxAUI_NB_TAB_SPLIT) && m_tabs.GetPageCount() >= 2)
	{
		// remove the page from the source tabs
		wxAuiNotebookPage page_info = src_tabs->GetPage( src_tabIdx );
		page_info.active = false;
		src_tabs->RemovePage(page_info.window);
		if (src_tabs->GetPageCount() > 0)
		{
			src_tabs->SetActivePage((size_t)0);
			src_tabs->DoShowHide();
			src_tabs->Refresh();
		}

		// add the page to the destination tabs
		if (dstIndex < 0 || dstIndex > static_cast<Int32>( dest_tabs->GetPageCount() ))
			dstIndex = dest_tabs->GetPageCount();
		dest_tabs->InsertPage(page_info.window, page_info, dstIndex);

		if (src_tabs->GetPageCount() == 0)
			RemoveEmptyTabFrames();
		
		DoSizing();
		dest_tabs->DoShowHide();
		dest_tabs->Refresh();

		// force the set selection function reset the selection
		m_curPage = -1;

		// set the active page to the one we just split off
		SetSelection(m_tabs.GetIdxFromWindow(page_info.window));

		UpdateHintWindowSize();
	}
}
