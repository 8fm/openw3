#pragma once

#include <wx/dnd.h>

class CEdAssetBrowser;
class CEdAnimBrowser;
class CDropTargetHelper;

class CDropTarget
{
public:
	CDropTarget();
	CDropTarget( wxWindow *window );
	virtual ~CDropTarget();

	void SetDropTargetWindow( wxWindow *window );

    wxTextDataObject*              GetDraggedDataObject();
    const TDynArray< CResource* > &GetDraggedResources();

    wxDragResult GetDraggedAction();

	virtual Bool OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources ){ return false; }
	virtual Bool OnDropText( wxCoord x, wxCoord y, String &text ){ return false; }
	virtual void OnEnter(wxCoord x, wxCoord y) {}
	virtual void OnLeave() {}
	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def) { return def; }

private:
	wxWindow* m_window;
};

class CDropTargetHelper : public wxTextDropTarget
{
private:
	CDropTarget*	        m_owner;
    wxDragResult            m_action;

    Bool                    m_resourcesExtracted;
    TDynArray< CResource* > m_resources;

public:
	CDropTargetHelper( CDropTarget *owner );
	~CDropTargetHelper();

    wxTextDataObject*              GetDataObject();
    const TDynArray< CResource* > &GetResources();
    wxDragResult                   GetResult() { return m_action; }

protected:
	bool OnDropText( wxCoord x, wxCoord y, const wxString& data );
	wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
	wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
	void OnLeave();
};

// Drop target for Depot Tree
class CDropTargetTreeDepot : public CDropTarget
{
private:
	CEdAssetBrowser* m_browser;
	wxTreeCtrl*		 m_owner;
	wxTreeItemId	 m_item;

public:
	CDropTargetTreeDepot( wxTreeCtrl *window, CEdAssetBrowser* browser) : CDropTarget(window), m_owner(window), m_browser(browser) {}

	Bool OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources );
	void OnEnter(wxCoord x, wxCoord y);
	void OnLeave();
	void OnDragOver(wxCoord x, wxCoord y);
};

// Drop target for Animation browser Notebook
class CDropTargetAnimBrowserNotebook : public CDropTarget
{
private:
	CEdAnimBrowser*	m_browser;
	wxNotebook*		m_owner;

public:
	CDropTargetAnimBrowserNotebook( wxNotebook *window, CEdAnimBrowser* browser) : CDropTarget(window), m_owner(window), m_browser(browser) {}

	Bool OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources );
	void OnEnter(wxCoord x, wxCoord y);
};