/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "treeEditor.h"
#include "behTreeEditedItem.h"
#include "classHierarchyMapper.h"

class CEdBehTreeEditor;
class CBehTree;
class IBehTreeNodeDefinition;

/// Graph editor for behavior tree
class CEdBehTreeGraphEditor : public CEdTreeEditor, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

protected:
	CClassHierarchyMapper	m_blockClasses;	//!< Block types that may be spawned
	CClassHierarchyMapper	m_taskClasses;	//!< Task classes
	IBehTreeEditedItem*		m_editedItem;	//!< Edited object
	CEdBehTreeEditor*		m_editor;		//!< Tree editor
	IBehTreeNodeDefinition*	m_contextMenuItem;
	static IBehTreeNodeDefinition*	s_copiedBranch;	//!< Copied branch
	static Int32				s_numEditors;	//!< Number of editors

	static const wxColour	NODE_BREAKPOINT_BG_COLOR;
	static const Int32		OFFSET_X_MANY = 150;
	static const Int32		OFFSET_Y_MANY = 0;
	static const Int32		OFFSET_X_SINGLE = 0;
	static const Int32		OFFSET_Y_SINGLE = 50;

public:
	CEdBehTreeGraphEditor( wxWindow * parent, IHook * hook );
	~CEdBehTreeGraphEditor();

	void SetEditor( CEdBehTreeEditor* editor ) { m_editor = editor; }

	//! Set script
	void SetTree( IBehTreeEditedItem* tree );

	//! Node status changed
	Bool OnNodeResultChanged( const IBehTreeNodeDefinition* node, const Bool active );

	//! Node reporting in
	Bool OnNodeReport( const IBehTreeNodeDefinition* node, Bool active );

	void ForceSetDebuggerColor( const IBehTreeNodeDefinition* node, Uint8 R, Uint8 G, Uint8 B );

	//! Tree state changed
	void OnTreeStateChanged();	

	//! Breakpoint reached
	virtual Bool OnBreakpoint( const IBehTreeNodeDefinition* node );

	//! Delete selection
	virtual void DeleteSelection() override { DeleteSelectedObjects(); }

	//! Force layout update
	virtual void ForceLayoutUpdate() override;

	//! Invoked be4 the tree was saved
	void PreSave();

	//! Invoked after the tree was saved
	void PostSave();

	// IEdEventListener
	void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;

protected:
	//! Open context menu
	virtual void OnOpenContextMenu() override;

	//! Get block friendly name
	virtual String GetBlockName( IScriptable & block ) const override;

	//! Get block comment
	virtual String GetBlockComment( IScriptable & block ) const override;

	//! Can this block have children
	virtual Bool CanHaveChildren( IScriptable & block ) const override;

	//! Is this block locked
	virtual Bool IsLocked( IScriptable & block ) const override;

	//! Paint
	virtual void PaintCanvas( Int32 width, Int32 height ) override;

	//! Fill node layout
	void FillNodeLayout( IBehTreeNodeDefinition * node, LayoutInfo * parentLayout );

	//! Load block position from somewhere
	virtual Bool LoadBlockPosition( IScriptable * block, wxPoint & pos ) override;

	//! Save block position to somewhere
	virtual Bool SaveBlockPosition( IScriptable * block, const wxPoint & pos ) override;

	//! Delete selected objects
	void DeleteSelectedObjects( bool askUser = true );

	//! Draw block layout override
	virtual void DrawBlockLayout( LayoutInfo & layout ) override;

	//! Called when move ended
	virtual void OnMoveEnded() override;

	// General node copy functionality
	void CopyNode( IBehTreeNodeDefinition* node );
	void CopyActiveNode();

protected:
	//! On 'Decorate'
	void OnSetDecorator( wxCommandEvent& event );

	//! On block spawn
	void OnSpawnBlock( wxCommandEvent& event );

	//! Cut node
	void OnCutNode( wxCommandEvent& event );

	//! Copy node
	void OnCopyNode( wxCommandEvent& event );

	//! On copy branch
	void OnCopyBranch( wxEvent& );

	//! On cut branch
	void OnCutBranch( wxEvent& );

	//! On copy branch
	void OnPasteBranch( wxEvent& );

	//! On paste node as decorator
	void OnPasteDecorator( wxCommandEvent& );

	//! On read only
	void OnReadOnly( wxEvent& );

	//! On game active
	void OnGameActive( wxEvent& );

	//! On block spawn
	void OnSetTask( wxCommandEvent& event );

	//! Apply default layout
	void OnApplyDefaultLayout( wxCommandEvent& event );

	//! Get canvas color
	virtual wxColour GetCanvasColor() const override { return BEHTREE_EDITOR_BACKGROUND; }	

	//! Result to color
	static void ResultToColor( const Bool active, wxColour& col );

	//! On toggle breakpoint
	void OnBreakpointToggle( wxCommandEvent& event );

	//! On continue after breakpoint
	void OnBreakpointContinue( wxCommandEvent& event );

	//! Tree structured modified 
	void TreeStructureModified();

	//! Fill task classes
	void FillTaskClasses();

	//! Show must checkout info
	void ShowMustCheckoutInfo();

	//! Deals with keyboard shortcuts
	void OnKeyDown( wxKeyEvent& event );
};
