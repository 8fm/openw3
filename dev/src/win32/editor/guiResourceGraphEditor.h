
/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdGuiResourceEditor;
class CVersionControlIconsPainter;

class CEdGuiResourceGraphEditor : public CEdGraphEditor
{
	DECLARE_EVENT_TABLE()

public:
	CEdGuiResourceGraphEditor( wxWindow* parent, CEdGuiResourceEditor* guiResourceEditor );
	~CEdGuiResourceGraphEditor();

private:
	CEdGuiResourceEditor*				m_guiResourceEditor;
	CVersionControlIconsPainter*		m_vciPainter;

	wxPoint								m_mousePosition;

protected:
	//! CEdGraphEditor interface
	virtual wxColor GetCanvasColor() const;
	virtual void PaintCanvas( Int32 width, Int32 height );
	virtual void CalcTitleIconArea( CGraphBlock* block, wxSize &size );
	virtual void DrawTitleIconArea( CGraphBlock* block, const wxRect& rect );
	virtual void CalcBlockInnerArea( CGraphBlock* block, wxSize& size );
	virtual void DrawBlockInnerArea( CGraphBlock* block, const wxRect& rect );
	virtual void AdjustLinkColors( CGraphSocket* source, CGraphSocket* destination, Color& linkColor, Float& linkWidth );
	virtual void AdjustLinkCaps( CGraphSocket* source, CGraphSocket* destination, Bool& srcCapArrow, Bool& destCapArrow );
	virtual void InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu );
	virtual void InitLinkedSocketContextMenu( CGraphSocket *socket, wxMenu &menu );
	virtual void InitLinkedDefaultContextMenu( wxMenu& menu );
	virtual void AdjustBlockColors( CGraphBlock* block, Color& borderColor, Float& borderWidth );
	virtual void DrawBlockLayout( CGraphBlock* block );
	virtual void AnnotateConnection( const CGraphConnection* con, const wxPoint &src, const wxPoint& srcDir, const wxPoint &dest, const wxPoint &destDir, float width = 1.0f );

	virtual const Char * ClipboardChannelName() const { return TXT("CEdGuiGraphEditor") ; } 
	//! Delete selected blocks
	void DeleteSelectedBlocks(); 

private:
	//! wx Events
	void OnLeftClick( wxMouseEvent& event );

	void OnAddGraphBlock( wxCommandEvent& event );
	void OnRemoveSelectedBlocks( wxCommandEvent& event );
	void OnRefreshPreview( wxCommandEvent& event );

private:
	Bool CanDeleteSelectedBlocks();

	/*RED_INLINE CGuiGraph* GetGraph();*/

public:
	// A tool for listing all editable block classes
	void GetEditableBlockClasses( TDynArray< CClass* >& outClasses );
};