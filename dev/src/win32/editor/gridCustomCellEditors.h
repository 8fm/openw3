#pragma once

#include "gridCellEditors.h"

class CEdTagEditor;
class CGridCellTagEditor;

class CGridCellTagRenderer : public CGridCellDefaultRenderer
{
	virtual wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col)
	{
		wxSize size = CGridCellDefaultRenderer::GetBestSize( grid, attr, dc, row, col );
		size.SetWidth( Max( 150, size.GetWidth() ) );
		return size;
	}
	virtual wxGridCellRenderer* Clone() const { return new CGridCellTagRenderer(); }
};

class CGridCellTagEditor : public wxGridCellEditor
{
	friend class CGridCellTagEditorEventHandler;
public:

	CGridCellTagEditor();
	~CGridCellTagEditor();

	virtual void Create( wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler );
	virtual void Destroy();
	virtual void SetSize( const wxRect& rect );
	virtual void BeginEdit( int row, int col, wxGrid* grid );
	virtual bool EndEdit(int row, int col, const wxGrid *grid,
		const wxString& oldval, wxString *newval);
	virtual void ApplyEdit(int row, int col, wxGrid* grid);
	virtual void Reset();
	virtual wxGridCellEditor *Clone() const { return new CGridCellTagEditor; }
	virtual wxString GetValue() const;

private:

	void OnOpenTagEditorExt( wxMouseEvent& event );
	void OnTagEditorExtOK( wxCommandEvent &event );
	void OnTagEditorExtCancelled( wxCommandEvent &event );

	CEdTagEditor*	    m_tagEditorExt;
	wxStaticBitmap		* m_bitmapUse;

	wxString			m_newValue;
};

//////////////////////////////////////////////////////////////////////////

class CGridStringsArrayEditor : public wxGridCellEditor, public wxEvtHandler
{
private:
	TDynArray< String > m_choices;
	wxListBox*			m_ctrl;
	wxString			m_newValue;

	void OnItemActivated( wxCommandEvent& event );

public:

	CGridStringsArrayEditor( const TDynArray< String >& choices );
	~CGridStringsArrayEditor();

	virtual void Create( wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler );
	virtual void Destroy();
	virtual void SetSize( const wxRect& rect );
	virtual void BeginEdit( int row, int col, wxGrid* grid );
	virtual bool EndEdit(int row, int col, const wxGrid *grid,
		const wxString& oldval, wxString *newval);
	virtual void ApplyEdit(int row, int col, wxGrid* grid);
	virtual void Reset();
	virtual wxGridCellEditor *Clone() const { return new CGridStringsArrayEditor( m_choices ); }
	virtual wxString GetValue() const;
};

//////////////////////////////////////////////////////////////////////////

class CLayerNameControl : public wxControl
{
public:

    CLayerNameControl( wxWindow *parent, wxWindowID id );

    wxString GetValue() const;
    void SetValue( const wxString &text );

protected:

    virtual void DoSetClientSize( Int32 width, Int32 height );
    virtual void DoSetSize( Int32 x, Int32 y, Int32 width, Int32 height, Int32 sizeFlags = wxSIZE_AUTO );

private:

    wxStaticText *m_text;
    wxStaticBitmap *m_bitmapUse;
    wxStaticBitmap *m_bitmapClear;

    // Event handlers
    void OnUseLeftDown( wxMouseEvent& event );
    void OnClearLeftDown( wxMouseEvent& event );

};

class CGridCellLayerNameEditor : public wxGridCellEditor
{

public:

    CGridCellLayerNameEditor();

    virtual void Create( wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler );
    virtual void SetSize( const wxRect& rect );
    virtual void BeginEdit( int row, int col, wxGrid* grid );
	virtual bool EndEdit(int row, int col, const wxGrid *grid,
		const wxString& oldval, wxString *newval);
	virtual void ApplyEdit(int row, int col, wxGrid* grid);
    virtual void Reset();
    virtual wxGridCellEditor *Clone() const { return new CGridCellLayerNameEditor(); }
    virtual wxString GetValue() const;

private:
	wxString	m_newValue;
};

class CGridCellLayerNameRenderer : public wxGridCellStringRenderer
{

public:

    CGridCellLayerNameRenderer();

    virtual void Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected );
    virtual wxSize GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col );
    virtual wxGridCellRenderer* Clone() const { return new CGridCellResourceRenderer; }

private:

    wxBitmap m_iconUse;
    wxBitmap m_iconClear;

};

//////////////////////////////////////////////////////////////////////////

// wxGridCellChoiceEditor drop-in replacement that used CEdChoice instead of wxChoice
class CGridCellChoiceEditor : public wxGridCellEditor
{
protected:
	wxString        m_value;
	wxArrayString   m_choices;
	bool            m_allowOthers;

public:
	CGridCellChoiceEditor(const wxArrayString& choices, bool allowOthers = false);

	virtual wxGridCellEditor *Clone() const { return new CGridCellChoiceEditor( m_choices, m_allowOthers ); }

	CEdChoice* Combo() const { return (CEdChoice*)m_control; }

	virtual void Create(wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler);
	virtual void SetSize( const wxRect& rect );

	virtual void BeginEdit(int row, int col, wxGrid* grid);
	virtual bool EndEdit(int row, int col, const wxGrid* grid, const wxString& WXUNUSED(oldval), wxString *newval);
	virtual void ApplyEdit(int row, int col, wxGrid* grid);

	virtual void Reset();
	virtual wxString GetValue() const;
};
