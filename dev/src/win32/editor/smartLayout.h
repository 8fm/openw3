#pragma once

wxDECLARE_EVENT( wxEVT_CLOSE_PANEL, wxCloseEvent );

class ISmartLayoutWindow
{
protected:
	wxTopLevelWindow* m_this;

public:
	ISmartLayoutWindow(wxTopLevelWindow* me) : m_this(me) {}
	virtual ~ISmartLayoutWindow() {}

public:
	virtual void SmartSetSize( Int32 x, Int32 y, Int32 w, Int32 h );

protected:
    virtual void LoadLayout( const String &identifier, Bool sizeOnly = false );
    virtual void SaveLayout( const String &identifier, Bool sizeOnly = false );
    void OnActivate( wxActivateEvent& event );
};

class wxSmartLayoutFrame : public wxFrame, public ISavableToConfig, public ISmartLayoutWindow
{
    friend class wxSmartLayoutPanel;
public:
    wxSmartLayoutFrame();
    wxSmartLayoutFrame( wxWindow* parent, const String& caption );
    virtual ~wxSmartLayoutFrame();

protected:
	void OnActivate( wxActivateEvent& event ) { ISmartLayoutWindow::OnActivate( event ); }

protected:
	DECLARE_CLASS( wxSmartLayoutFrame );
};

class wxSmartLayoutPanel : public wxPanel, public ISavableToConfig
{
    wxString            m_originalLabel;
    wxSmartLayoutFrame *m_frame;
    wxMenuBar          *m_frameMenuBar;
    wxMenuBar          *m_dockedMenuBar;
    wxAuiNotebook      *m_notebook;
    Bool                m_recurencyPreventionOn;
    Bool                m_openDocked;
    Bool                m_donotCloseButHide;
    Bool                m_cloneFullMenuOnDock;

    wxCommandEvent     *m_lastMenuEvent;

public:
    wxSmartLayoutPanel( wxWindow* parent, const String& caption, Bool donotCloseButHide,
                        wxWindow *child, wxMenuBar *menuBar = NULL, Bool cloneFullMenuOnDock = false );
    wxSmartLayoutPanel( wxWindow* parent, const String& type, Bool donotCloseButHide );
	~wxSmartLayoutPanel();

protected:
    void AttachMenuBar();

    void OnFrameClose( wxCloseEvent& event );
    void OnParentMenu( wxCommandEvent& event );
    void OnClose( wxCloseEvent& event );
    void OnClosePage( wxAuiNotebookEvent& event );
    void OnChangePage( wxAuiNotebookEvent& event );
    void OnDock( wxCommandEvent& event );

public:
    virtual void LoadLayout( const String &identifier );
    virtual void SaveLayout( const String &identifier );

	virtual Bool AlwaysVisible() { return false;}

    wxSmartLayoutFrame *GetOriginalFrame()   { return m_frame; }
    wxString            GetOriginalLabel()   { return m_originalLabel; }

    void ClosePanel();
    void Dock();
    void Undock(Bool andHide = false);

	virtual wxString GetShortTitle()         { return GetTitle(); }
	wxString GetTitle()                      { return m_frame->GetTitle(); }
	void     SetTitle(const wxString &title) { m_frame->SetTitle( title ); }

    Bool     IsDocked()                      { return GetParent() != m_frame; }

    virtual bool IsVisible();

    virtual bool Show( bool show = true );

    virtual void SetIcon(const wxIcon& icon) { m_frame->SetIcon( icon ); }

    virtual wxMenuBar *GetMenuBar() const    { return m_frameMenuBar; }

    void CheckMenuItem( Int32 id, Bool state );

protected:
    DECLARE_CLASS( wxSmartLayoutPanel );
};

class wxSmartLayoutDialog : public wxDialog, public ISavableToConfig, public ISmartLayoutWindow
{
public:
	wxSmartLayoutDialog();
	virtual ~wxSmartLayoutDialog();

protected:
	void OnActivate( wxActivateEvent& event ) { ISmartLayoutWindow::OnActivate( event ); }

protected:
    DECLARE_CLASS( wxSmartLayoutDialog );
};

class wxSmartLayoutAuiNotebook : public wxAuiNotebook, public ISavableToConfig
{
protected:
    void MovePageToPane(wxWindow *srcPage, wxWindow *dstPage, Int32 dstIndex = -1);

    void Init();
    void OnEndDrag( wxAuiNotebookEvent &event );

public:
    wxSmartLayoutAuiNotebook() { Init(); }

    wxSmartLayoutAuiNotebook(wxWindow* parent,
                             wxWindowID id = wxID_ANY,
                             const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize,
                             long style = wxAUI_NB_DEFAULT_STYLE)
        : wxAuiNotebook(parent, id, pos, size, style)
    { Init(); }

public:
    virtual void SaveOptionsToConfig();
    virtual void LoadOptionsFromConfig();
};
