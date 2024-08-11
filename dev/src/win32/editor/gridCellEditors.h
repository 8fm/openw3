#pragma once

class IGridCellOutlineRenderer
{
public:
	static void DrawOutline(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected);
};

//////////////////////////////////////////////////////////////////////////

class CGridCellDefaultRenderer : public wxGridCellStringRenderer
{
protected:
	virtual wxString GetText( wxGrid& grid, int row, int col );
	virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected);
	virtual wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col);
	virtual wxGridCellRenderer* Clone() const { return new CGridCellDefaultRenderer(); }
};

//////////////////////////////////////////////////////////////////////////

class CGridCellEmptyRenderer : public wxGridCellRenderer
{
	virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected);
	virtual wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col) { return wxSize(grid.GetColSize(col), grid.GetRowSize(row)); }
	virtual wxGridCellRenderer* Clone() const { return new CGridCellEmptyRenderer(); }
};

//////////////////////////////////////////////////////////////////////////

class CGridCellSeparatorRenderer : public wxGridCellRenderer
{
	virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected);
	virtual wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col) { return wxSize(15, grid.GetRowSize(row)); }
	virtual wxGridCellRenderer* Clone() const { return new CGridCellSeparatorRenderer(); }
};

//////////////////////////////////////////////////////////////////////////

class CGridCellBoolRenderer : public wxGridCellRenderer
{

public:

	CGridCellBoolRenderer() : wxGridCellRenderer() {}

	virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected);
	virtual wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col);
	virtual wxGridCellRenderer* Clone() const { return new CGridCellBoolRenderer(); }

};

//////////////////////////////////////////////////////////////////////////

class CGridCellFloatRenderer : public wxGridCellFloatRenderer
{
public:
	CGridCellFloatRenderer(Int32 width = -1, Int32 precision = -1) : wxGridCellFloatRenderer(width, precision) {}
	virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected);
	virtual wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col) { return wxGridCellFloatRenderer::GetBestSize(grid, attr, dc, row, col); }
	virtual wxGridCellRenderer* Clone() const { return new CGridCellFloatRenderer(GetWidth(), GetPrecision()); }
};

//////////////////////////////////////////////////////////////////////////

class CGridCellNumberRenderer : public wxGridCellNumberRenderer
{
	virtual void Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected);
	virtual wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col) { return wxGridCellNumberRenderer::GetBestSize(grid, attr, dc, row, col); }
	virtual wxGridCellRenderer* Clone() const { return new CGridCellNumberRenderer(); }
};

//////////////////////////////////////////////////////////////////////////

class CResourceControl : public wxControl
{
public:

    CResourceControl( wxWindow *parent, wxWindowID id, const wxString &extension );

    wxString GetValue() const;
    void SetValue( const wxString &text );

protected:

    virtual void DoSetClientSize( Int32 width, Int32 height );
    virtual void DoSetSize( Int32 x, Int32 y, Int32 width, Int32 height, Int32 sizeFlags = wxSIZE_AUTO );

private:

    wxStaticText *m_text;
    wxStaticBitmap *m_bitmapBrowse;
    wxStaticBitmap *m_bitmapUse;
    wxStaticBitmap *m_bitmapClear;
    const wxString &m_extension;

    // Event handlers
    void OnBrowseLeftDown(wxMouseEvent& event);
    void OnUseLeftDown(wxMouseEvent& event);
    void OnClearLeftDown(wxMouseEvent& event);

};

class CGridCellResourceEditor : public wxGridCellEditor
{

public:

    CGridCellResourceEditor( const wxString &extension );

    virtual void Create( wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler );
    virtual void SetSize( const wxRect& rect );
    virtual void BeginEdit( int row, int col, wxGrid* grid );
	virtual bool EndEdit(int row, int col, const wxGrid *grid,
		const wxString& oldval, wxString *newval);
	virtual void ApplyEdit(int row, int col, wxGrid* grid);
    virtual void Reset();
    virtual wxGridCellEditor *Clone() const { return new CGridCellResourceEditor( m_extension ); }
    virtual wxString GetValue() const;

private:

    const wxString m_extension;
	wxString	   m_newValue;

};

class CGridCellResourceRenderer : public wxGridCellStringRenderer
{

public:

    static const Int32 ICON_SIZE = 15;

    CGridCellResourceRenderer();

    virtual void Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected );
    virtual wxSize GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col );
    virtual wxGridCellRenderer* Clone() const { return new CGridCellResourceRenderer; }

private:

    wxBitmap m_iconUse;
    wxBitmap m_iconBrowse;
    wxBitmap m_iconClear;

};

///////////////////////////////////////////////////////////////////
class CGridCellObjectEditor : public wxGridCellEditor
{
public:
	CGridCellObjectEditor();
	~CGridCellObjectEditor() {}

	virtual void Create( wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler );
	virtual void SetSize( const wxRect& rect );
	virtual void BeginEdit( int row, int col, wxGrid* grid );
	virtual bool EndEdit(int row, int col, const wxGrid *grid,
		const wxString& oldval, wxString *newval);
	virtual void ApplyEdit(int row, int col, wxGrid* grid);

	virtual void Reset();
	virtual wxGridCellEditor *Clone() const { return new CGridCellObjectEditor(); }
	virtual wxString GetValue() const;
};

class CGridCellObjectRenderer : public wxGridCellStringRenderer
{
public:
	CGridCellObjectRenderer();

	virtual void Draw( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, const wxRect& rect, int row, int col, bool isSelected );
	virtual wxSize GetBestSize( wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col );
	virtual wxGridCellRenderer* Clone() const { return new CGridCellResourceRenderer; }
};

