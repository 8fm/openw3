#pragma once

class CEdInteractiveDialogEditor;
class CIDGraphBlock;

class CEdInteractiveDialogGraphEditor : public CEdGraphEditor
{
	DECLARE_EVENT_TABLE()

protected:
	CEdInteractiveDialogEditor*		m_mainEditor;
	wxPoint							m_mousePosition;
	Bool							m_useMousePositionForNewBlocks;

	const CIDGraphBlock*			m_highlightedBlock;

public:
	static const Vector				BLOCK_POS_ADD;

public:
	CEdInteractiveDialogGraphEditor( wxWindow* parent, CEdInteractiveDialogEditor* mainEditor );
	~CEdInteractiveDialogGraphEditor( void );

	void OnCopySelection	( );
	void OnPaste			( );
	void OnCutSelection		( );
	void OnDeleteSelection	( );

	template < class TBlockType > TBlockType* CreateAndAddDialogBlock();
	void DoDeleteBlock( CGraphBlock* block );
	void HiliteBlock( CIDGraphBlock* block );

	void FillBlockMenu( wxMenu& menu );

protected:
	virtual const Char * ClipboardChannelName() const { return TXT("CEdInteractiveDialogGraphEditor") ; } 
	virtual wxColor GetCanvasColor() const { return DIALOG_EDITOR_BACKGROUND; }
	virtual void PaintCanvas( Int32 width, Int32 height );

	virtual void InitLinkedBlockContextMenu( CGraphBlock *block, wxMenu &menu );
	virtual void InitLinkedSocketContextMenu( CGraphSocket *block, wxMenu &menu );
	virtual void InitLinkedDefaultContextMenu( wxMenu& menu );

	void OnAddInput( wxCommandEvent& event );
	void OnAddOutput( wxCommandEvent& event );
	void OnAddOutputTemrinate( wxCommandEvent& event );
	void OnAddText( wxCommandEvent& event );
	void OnAddFlow( wxCommandEvent& event );
	void OnAddBranch( wxCommandEvent& event );
	void OnAddChoice( wxCommandEvent& event );
	void OnAddFork( wxCommandEvent& event );
	void OnAddCondition( wxCommandEvent& event );
	void OnAddFact( wxCommandEvent& event );
	void OnAddCommunicator( wxCommandEvent& event );
	void OnAddRequestFocus( wxCommandEvent& event );
	void OnAddRequestInteraction( wxCommandEvent& event );
	void OnAddEvents( wxCommandEvent& event );
	void OnAddConnector( wxCommandEvent& event );
	void OnAddCheckpoint( wxCommandEvent& event );

	void OnDeleteBlock( wxCommandEvent& event );
	void PerformDeleteSelection();

	void OnLeftClick( wxMouseEvent& event );
	void OnSetFocus( wxFocusEvent& event );
	void OnKillFocus( wxFocusEvent& event );

	template < class TBlockType > void OnAddBlockFromBlockMenu(); 
	void RepositionBlock( CIDGraphBlock* block, const wxPoint& newPositionInScreenSpace ); 
};
