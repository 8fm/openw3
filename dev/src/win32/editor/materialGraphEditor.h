/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Specialized graph editor for material graphs
class CEdMaterialGraphEditor : public CEdGraphEditor
{
public:
	//! Constructor
	CEdMaterialGraphEditor( wxWindow* parent );

	//! Set material graph
	void SetGraph( CMaterialGraph* graph );

	//! Calculate inner area size for given block
	virtual void CalcBlockInnerArea( CGraphBlock* block, wxSize& innerArea );

	//! Draw inner area for given block, rect is given in client space, clip rect is set
	virtual void DrawBlockInnerArea( CGraphBlock* block, const wxRect& rect );

	//! Fill context menu for Empty Selection
	virtual void InitLinkedDefaultContextMenu( wxMenu& menu );

	void EncapsulateSelectedBlocks( wxCommandEvent& event );

protected:
	virtual wxColor GetCanvasColor() const { return MATERIAL_EDITOR_BACKGROUND; }
	virtual const Char * ClipboardChannelName() const { return TXT("CEdMaterialGraphEditor") ; } 
	Uint32 GetAmountOfBlockWithName( String nameOfBlock );

	void OnAKeyDown( wxKeyEvent& event );
};