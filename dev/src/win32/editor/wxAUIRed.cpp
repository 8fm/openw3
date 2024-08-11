#include "build.h"
#include "wxAUIRed.h"
#include <wx/version.h>
#include "../../common/core/core.h"




namespace
{


	//
	// This class is copied from AuiBook.cpp file (wxWidgets::AUI)
	//
	// The reason to do such a terrible thing was not to violate wxWidgets licence.
	// I've added some functionality to widgets by deriving its classes, but I had to cast to wxTabFrame in one place
	// To do this I've copied the class so that it is identical (in sense of memory) and I've casted to it.
	//
	// Notice, that in the constructor there is a compile time assertion that wxWidgets version didn't change.
	// If the version changed:
	//		Please copy new version of wxTabFrame here
	//		Also please put a new compile time assert, so that it will work in the future as well.
	//
	class wxTabFrame : public wxWindow
	{
	public:

		wxTabFrame()
		{

			// LOOK AT THE COMMENT FOR THE WHOLE CLASS
			COMPILE_ASSERT(((wxMAJOR_VERSION == 2) && (wxMINOR_VERSION == 9) && (wxRELEASE_NUMBER == 4) && (wxSUBRELEASE_NUMBER == 0)));


			m_tabs = NULL;
			m_rect = wxRect(0,0,200,200);
			m_tabCtrlHeight = 20;
		}

		~wxTabFrame()
		{
			wxDELETE(m_tabs);
		}

		void SetTabCtrlHeight(int h)
		{
			m_tabCtrlHeight = h;
		}

	protected:
		void DoSetSize(int x, int y,
			int width, int height,
			int WXUNUSED(sizeFlags = wxSIZE_AUTO))
		{
			m_rect = wxRect(x, y, width, height);
			DoSizing();
		}

		void DoGetClientSize(int* x, int* y) const
		{
			*x = m_rect.width;
			*y = m_rect.height;
		}

	public:
		bool Show( bool WXUNUSED(show = true) ) { return false; }

		void DoSizing()
		{
			if (!m_tabs)
				return;

			if (m_tabs->IsFrozen() || m_tabs->GetParent()->IsFrozen())
				return;

			m_tab_rect = wxRect(m_rect.x, m_rect.y, m_rect.width, m_tabCtrlHeight);
			if (m_tabs->GetFlags() & wxAUI_NB_BOTTOM)
			{
				m_tab_rect = wxRect (m_rect.x, m_rect.y + m_rect.height - m_tabCtrlHeight, m_rect.width, m_tabCtrlHeight);
				m_tabs->SetSize     (m_rect.x, m_rect.y + m_rect.height - m_tabCtrlHeight, m_rect.width, m_tabCtrlHeight);
				m_tabs->SetRect     (wxRect(0, 0, m_rect.width, m_tabCtrlHeight));
			}
			else //TODO: if (GetFlags() & wxAUI_NB_TOP)
			{
				m_tab_rect = wxRect (m_rect.x, m_rect.y, m_rect.width, m_tabCtrlHeight);
				m_tabs->SetSize     (m_rect.x, m_rect.y, m_rect.width, m_tabCtrlHeight);
				m_tabs->SetRect     (wxRect(0, 0,        m_rect.width, m_tabCtrlHeight));
			}
			// TODO: else if (GetFlags() & wxAUI_NB_LEFT){}
			// TODO: else if (GetFlags() & wxAUI_NB_RIGHT){}

			m_tabs->Refresh();
			m_tabs->Update();

			wxAuiNotebookPageArray& pages = m_tabs->GetPages();
			size_t i, page_count = pages.GetCount();

			for (i = 0; i < page_count; ++i)
			{
				int height = m_rect.height - m_tabCtrlHeight;
				if ( height < 0 )
				{
					// avoid passing negative height to wxWindow::SetSize(), this
					// results in assert failures/GTK+ warnings
					height = 0;
				}

				wxAuiNotebookPage& page = pages.Item(i);
				if (m_tabs->GetFlags() & wxAUI_NB_BOTTOM)
				{
					page.window->SetSize(m_rect.x, m_rect.y, m_rect.width, height);
				}
				else //TODO: if (GetFlags() & wxAUI_NB_TOP)
				{
					page.window->SetSize(m_rect.x, m_rect.y + m_tabCtrlHeight,
						m_rect.width, height);
				}
				// TODO: else if (GetFlags() & wxAUI_NB_LEFT){}
				// TODO: else if (GetFlags() & wxAUI_NB_RIGHT){}

				#if wxUSE_MDI
				if (wxDynamicCast(page.window, wxAuiMDIChildFrame))
				{
					wxAuiMDIChildFrame* wnd = (wxAuiMDIChildFrame*)page.window;
					wnd->ApplyMDIChildFrameRect();
				}
				#endif
			}
		}

	protected:
		void DoGetSize(int* x, int* y) const
		{
			if (x)
				*x = m_rect.GetWidth();
			if (y)
				*y = m_rect.GetHeight();
		}

	public:
		void Update()
		{
			// does nothing
		}

		wxRect m_rect;
		wxRect m_tab_rect;
		wxAuiTabCtrl* m_tabs;
		int m_tabCtrlHeight;
	};
}





CEdAuiManager::CEdAuiManager( wxWindow* managedWnd /*= NULL*/, Uint32 flags /*= wxAUI_MGR_DEFAULT */ )
	: wxAuiManager(managedWnd, flags)
{

}




wxString CEdAuiManager::SaveWholeLayout()
{
	wxString toRet = SavePerspective();

	wxAuiPaneInfoArray& allPanes = GetAllPanes();
	const Uint32 paneCount = allPanes.GetCount();

	for (Uint32 i = 0; i < paneCount; ++i)
	{
		wxAuiPaneInfo& pane = allPanes.Item(i);
		if (pane.name == wxT("dummy"))
		{
			continue;
		}

		ASSERT(!pane.name.empty(), TXT("All panes must have a unique name"));

		toRet += wxT("{{");
		toRet += pane.name;
		toRet += wxT("}={");

		ASSERT(pane.window, TXT("Window doesn't exist? Something went wrong..."));

		CEdAuiNotebook* notebook = dynamic_cast<CEdAuiNotebook*>(pane.window);
		ASSERT(notebook, TXT("All panes must be wxAUINotebookRED !!!"));

		toRet += notebook->SavePerspective();
		toRet += wxT("}}");
	}

	// end tag
	toRet += wxT("{{end}}");
	return toRet;
}






Bool CEdAuiManager::LoadWholeLayout( const wxString& wholeLayoutString, Bool update /*= true*/ )
{
	if(wholeLayoutString.empty())
	{
		return false;
	}

	wxString perspectiveString = wholeLayoutString.BeforeFirst(wxT('{'));

	if(perspectiveString.empty())
	{
		return false;
	}


	if(!LoadPerspective(perspectiveString))
	{
		return false;
	}


	wxString leftConfig = wholeLayoutString.AfterFirst(wxT('{'));

	while(!leftConfig.empty())
	{
		ASSERT((leftConfig[0] == wxT('{')), TXT("Parsing failed"));
		wxString tmp = leftConfig.AfterFirst(wxT('{'));
		wxString name = tmp.BeforeFirst(wxT('}'));

		if(name == wxT("end"))
		{
			// end tag
			break;
		}

		#ifdef DEBUG
		{
			wxString dbgTmp = tmp.AfterFirst(wxT('}'));
			ASSERT((dbgTmp[0] == wxT('=')) && (dbgTmp[1] == wxT('{')));
		}
		#endif // DEBUG

		wxString tmp2 = tmp.AfterFirst(wxT('{'));
		wxString layoutString = tmp2.BeforeFirst(wxT('}'));

		if(!name.empty() && !layoutString.empty())
		{
			wxAuiPaneInfo& pane = GetPane(name);

			if(!pane.IsOk())
			{
				// if docking between notebooks is implemented - this has to be changed
				return false;
			}

			ASSERT(pane.window, TXT("Window doesn't exist? Something went wrong..."));

			CEdAuiNotebook* notebook = dynamic_cast<CEdAuiNotebook*>(pane.window);
			ASSERT(notebook, TXT("All panes must be wxAUINotebookRED !!!"));

			notebook->LoadPerspective(layoutString);
			notebook->Layout();
		}

		#ifdef DEBUG
		{
			wxString dbgTmp = tmp2.AfterFirst(wxT('}'));
			ASSERT((dbgTmp[0] == wxT('}')) && (dbgTmp[1] == wxT('{')));
		}
		#endif // DEBUG

		leftConfig = tmp2.AfterFirst(wxT('{'));
	}



	if (update)
	{
		Update();
	}


	return true;
}











CEdAuiNotebook::CEdAuiNotebook(
	wxWindow* parent, wxWindowID id /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/,
	long style /*= wxAUI_NB_DEFAULT_STYLE */
)
	: wxAuiNotebook(parent, id, pos, size, style)
{

}






/// based on proposition for wxWidgets 3.x upgrade, modified
wxString CEdAuiNotebook::SavePerspective()
{

	// Build list of panes/tabs
	wxString tabs;
	wxAuiPaneInfoArray& allPanes = m_mgr.GetAllPanes();
	const Uint32 paneCount = allPanes.GetCount();

	for (Uint32 i = 0; i < paneCount; ++i)
	{
		wxAuiPaneInfo& pane = allPanes.Item(i);
		if (pane.name == wxT("dummy"))
		{
			continue;
		}

		wxTabFrame* tabframe = (wxTabFrame*)pane.window;

		if (!tabs.empty())
		{
			tabs += wxT("|");
		}

		tabs += pane.name;
		tabs += wxT("=");

		// add tab id's
		Uint32 pageCount = tabframe->m_tabs->GetPageCount();
		for (Uint32 p = 0; p < pageCount; ++p)
		{
			wxAuiNotebookPage& page = tabframe->m_tabs->GetPage(p);
			const Int32 pageIdx = m_tabs.GetIdxFromWindow(page.window);

			if(p)
			{
				tabs += wxT(",");
			}

			if((Int32)pageIdx == m_curPage)
			{
				tabs += wxT("*");
			}
			else if((Int32)p == tabframe->m_tabs->GetActivePage())
			{
				tabs += wxT("+");
			}

			ASSERT(pageIdx >= 0);
			tabs += wxString::Format(wxT("%d"), pageIdx);
		}
	}

	tabs += wxT("@");

	// Add frame perspective
	// WARNING - this is not the same m_mgr as the general manager for the whole system
	tabs += m_mgr.SavePerspective();

	return tabs;
}


/// based on proposition for wxWidgets 3.x upgrade, modified
Bool CEdAuiNotebook::LoadPerspective(const wxString& layout)
{
	// Code associated with this vector is created to prevent a bug when info about tab was not save but a tab is created
	// example 1 - someone closed the tab and saved perspective
	// example 2 - someone added new functionality on new tab to the editor
	TDynArray<wxWindow*> windowsLeft;
	wxAuiTabCtrl* firstDestTabs = NULL;

	if(!RemoveAllTabCtrls(windowsLeft))
	{
		return false;
	}

	RemoveEmptyTabFrames();



	Uint32 selPage = 0;

	wxString tabs = layout.BeforeFirst(wxT('@'));

	while (true)
	{
		const wxString tabPart = tabs.BeforeFirst(wxT('|'));

		// if the string is empty, we're done parsing
		if (tabPart.empty())
		{
			break;
		}

		// Get pane name
		const wxString paneName = tabPart.BeforeFirst(wxT('='));


		wxAuiTabCtrl* destTabs = CreateTabFrameForLoad(paneName);

		// Get list of tab id's and move them to pane
		wxString tabList = tabPart.AfterFirst(wxT('='));

		while(true)
		{
			wxString tab = tabList.BeforeFirst(wxT(','));

			if (tab.empty())
			{
				break;
			}

			tabList = tabList.AfterFirst(wxT(','));

			// Check if this page has an 'active' marker
			const wxChar c = tab[0];
			if (c == wxT('+') || c == wxT('*'))
			{
				tab = tab.Mid(1); 
			}

			const Uint32 tabId = wxAtoi(tab.c_str());
			if (tabId >= GetPageCount())
			{
				continue;
			}

			// Move tab to pane
			wxAuiNotebookPage& page = m_tabs.GetPage(tabId);
			const Uint32 newPageId = destTabs->GetPageCount();
			destTabs->InsertPage(page.window, page, newPageId);

			if(!firstDestTabs)
			{
				firstDestTabs = destTabs;
			}

			if (c == wxT('+')) destTabs->SetActivePage(newPageId);
			else if ( c == wxT('*')) selPage = tabId;


			{ // @see windowsLeft

				Bool windowFound = false;

				for(auto it = windowsLeft.Begin(); it != windowsLeft.End(); ++it)
				{
					if((*it) == page.window)
					{
						windowsLeft.Erase(it);
						windowFound = true;
						break;
					}
				}

				ASSERT(windowFound, TXT("Want to add window that was not removed previously."));
			}

		}

		destTabs->DoShowHide();
		tabs = tabs.AfterFirst(wxT('|'));
	}

	// Load the frame perspective
	wxString frames = layout.AfterFirst(wxT('@'));
	m_mgr.LoadPerspective(frames);

	// @see windowsLeft
	if(!windowsLeft.Empty())
	{
		if(!firstDestTabs)
		{
			ASSERT((m_pages.Count() == 0), TXT("If no destTabs created, m_pages should be empty?"));
			firstDestTabs = CreateTabFrameForLoad(wxT("autoTab"));
		}

		for(auto it = windowsLeft.Begin(); it != windowsLeft.End(); ++it)
		{
			Int32 tabId = GetPageIndex((*it));
			ASSERT(tabId >= 0, TXT("Page with this window not found"));
			ASSERT(tabId < (Int32)GetPageCount(), TXT("Page with this window not found?"));

			wxAuiNotebookPage& page = m_tabs.GetPage(tabId);

			ASSERT(page.window == (*it));

			const Uint32 newPageId = firstDestTabs->GetPageCount();
			firstDestTabs->InsertPage(page.window, page, newPageId);

			selPage = tabId;
		}

		m_mgr.Update();
	}


	// Force refresh of selection
	m_curPage = -1;
	SetSelection(selPage);

	return true;
}



Bool CEdAuiNotebook::RemoveAllTabCtrls( TDynArray<wxWindow*>& windowsLeft )
{
	const Uint32 tabCount = m_tabs.GetPageCount();
	for (Uint32 i = 0; i < tabCount; ++i)
	{
		wxWindow* wnd = m_tabs.GetWindowFromIdx(i);


		{ // @see windowsLeft

			#ifdef DEBUG
			{
				for(auto it = windowsLeft.begin(); it != windowsLeft.end(); ++it)
				{
					ASSERT(((*it) != wnd), TXT("Window allready added? It was in two tabs?"));
				}
			}
			#endif

			windowsLeft.PushBack(wnd);
		}

		// find out which onscreen tab ctrl owns this tab
		wxAuiTabCtrl* ctrl;

		Int32 ctrlIdx;
		if (!FindTab(wnd, &ctrl, &ctrlIdx))
		{
			return false;
		}

		// remove the tab from ctrl
		if (!ctrl->RemovePage(wnd))
		{
			return false;
		}
	}

	return true;
}



wxAuiTabCtrl* CEdAuiNotebook::CreateTabFrameForLoad( const wxString& paneName )
{
	wxTabFrame* newTabs = new wxTabFrame;
	newTabs->m_tabs = new wxAuiTabCtrl(this,
		m_tabIdCounter++,
		wxDefaultPosition,
		wxDefaultSize,
		wxNO_BORDER|wxWANTS_CHARS);

	newTabs->m_tabs->SetArtProvider(m_tabs.GetArtProvider()->Clone());
	newTabs->SetTabCtrlHeight(m_tabCtrlHeight);
	newTabs->m_tabs->SetFlags(m_flags);
	wxAuiTabCtrl* destTabs = newTabs->m_tabs;

	// create a pane info structure with the information
	// about where the pane should be added
	wxAuiPaneInfo paneInfo = wxAuiPaneInfo().Name(paneName).Left().CaptionVisible(false);
	m_mgr.AddPane(newTabs, paneInfo);

	return destTabs;
}



