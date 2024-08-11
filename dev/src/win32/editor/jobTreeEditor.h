/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/behaviorGraphAnimationSlotNode.h"

class CEdJobTreePreviewPanel;
class CEdUndoManager;

enum EJobTreeItemType
{
	JTIT_Node,
	JTIT_EnterAction,
	JTIT_ExitAction,
	JTIT_FastExitAction,
	JTIT_TreeLeaf,
	JTIT_Invalid
};

class CJobTreeItem : public wxTreeItemData
{
public:
	CJobTreeItem(): m_node(NULL), m_action( NULL ), m_itemType( JTIT_Invalid ) {}

	CJobTreeNode*		m_node;
	CJobActionBase*		m_action;
	EJobTreeItemType	m_itemType;

	wxTreeItemId m_itemID;
};

class CJobTreeContextMenuItem : public wxObject
{
public:
	CJobTreeItem*	m_item;

	CJobTreeContextMenuItem() : m_item( NULL ) {}
	CJobTreeContextMenuItem( CJobTreeItem* item ) : m_item( item ) {}
};

class CEdJobTreePreview;

/// Job tree editor
class CEdJobTreeEditor : public wxSmartLayoutPanel, public ISlotAnimationListener
{
	DECLARE_EVENT_TABLE()

	friend class CUndoJobEditorAnimationActivated;
	friend class CUndoJobEditorNodeExistance;
	friend class CUndoJobActionExistance;
	friend class CUndoJobEditorNodeMove;

protected:
	//! Filter
	enum EBrowserAnimFilterMode
	{
		BAFM_BeginsWith,
		BAFM_Contain
	};

	EBrowserAnimFilterMode					m_filterMode;
	String									m_filterText;
	Bool									m_allAnimsets;


	CEdPropertiesPage*			m_properties;
	wxTreeListCtrl*				m_treeView;
	wxChoice*					m_speedChoice;
	wxTextCtrl*					m_speedValue;
	wxTreeCtrl*					m_animationTree;

	Bool						m_previewingTree;

	CEdJobTreePreviewPanel*		m_previewPanel;

	CJobTree*					m_jobTree;	//<! Actual edited resource
	CJobTreeNode*				m_rootNode;	//<! Root node of the edited job tree
	SJobTreeExecutionContext*	m_jobContext;
	CJobActionBase*				m_currentAction;
	
	THandle< CEntity >			m_actorEntity;

	CJobTreeItem m_itemToPasteData;

	CObject*	m_selectedObject;

	wxChoice*	m_previewCategoryChoice;

	CEdPropertiesPage*			m_settingsPropertyPanel;

	Bool		m_usePreviewEntityData;

	CEdUndoManager* m_undoManager;

public:
	CEdJobTreeEditor( wxWindow* parent, CJobTree* tree );
	~CEdJobTreeEditor();

	// Load job tree into tree view control
	void LoadTree();

protected:
	// Add node to parent
	wxTreeItemId AppendNode( wxTreeItemId* wxParent, CJobTreeNode* node );

	// Reload all children items of given node
	void FillNodeContents( wxTreeItemId treeItemId );

	void FillTreeContents();

	void UpdateTreeFragment( wxTreeItemId treeItemId );

	void OnMove( Bool up );

public:
	//! Selection has changed
	virtual void OnNodeSelected( wxTreeEvent& event );

	//! add node button clicked
	virtual void OnAddNodeToolItem( wxCommandEvent& event );

	//! remove node button clicked
	virtual void OnRemoveNodeToolItem( wxCommandEvent& event );

	//! OK button clicked
	virtual void OnOK( wxCommandEvent& event );

	//! Move item up button
	virtual void OnMoveUp( wxCommandEvent& event );

	//! Move item down button
	virtual void OnMoveDown( wxCommandEvent& event );

	//! Copy item
	virtual void OnCopyItem( wxCommandEvent& event );

	//! Paste copied item
	virtual void OnPasteItem( wxCommandEvent& event );

	//! So we can spawn the NPC externally (through the world editor preview tool)
	CEdJobTreePreviewPanel* GetPreviewPanel() const { return m_previewPanel; }

	//! On Play clicked
	void OnPlay( wxCommandEvent& event );

	//! On Play world clicked
	void OnPlayWorld( wxCommandEvent& event );

	//! On Stop clicked
	void OnStop( wxCommandEvent& event );

	//! On save menu item clicked
	void OnSave( wxCommandEvent& event );

	//! Movement speed choice box selection changed
	void OnSpeedChoice( wxCommandEvent& event );
	
	//! Custom movement speed text box value changed
	void OnCustomSpeedValue( wxCommandEvent& event );

	void OnSettingsChanged( wxCommandEvent& event );
	void OnItemsChanged( wxCommandEvent& event );

	void OnAddEnterAnimation( wxCommandEvent& event );
	void OnAddLeaveAnimation( wxCommandEvent& event );
	void OnAddFastLeaveAnimation( wxCommandEvent& event );
	void OnRemoveItem( wxCommandEvent& event );

	void OnActionPreviewStarted( const CJobActionBase* action );
	void OnActionPreviewEnded( const CJobActionBase* action );
	void OnPreviewActorChanged();

	void OnUndo( wxCommandEvent& event );
	void OnRedo( wxCommandEvent& event );

protected:
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnTreeItemContextMenu( wxTreeEvent& event );
	void OnOpenPreview( wxCommandEvent& event );
	void OnPreviewDestroyed( wxWindowDestroyEvent& event );

	void OnAnimFilter( wxCommandEvent& event );
	void OnAnimFilterCheck( wxCommandEvent& event );
	void OnAllAnimsetsCheck( wxCommandEvent& event );

	void OnAnimationSelected( wxTreeEvent& event );
	void OnAnimationActivated( wxTreeEvent& event );

	void FillPreviewCategoriesChoice();

	void StoreSelection();
	void RestoreSelection();
	wxTreeItemId FindTreeItemForObject( const CObject* object );
	wxTreeItemId FindTreeItemForObject( wxTreeItemId startItemId, const CObject* object );

	void FillItemPanelData( CJobTreeNode* node );
	Bool FilterAnimName( const CName& animName ) const;
	void FillAnimationTree();

	//! Check and correct category validity
	void ExtractCategories( CJobTreeNode* node, TDynArray<CName>& categories );
	void PropagateCategories( CJobTreeNode* node, const TDynArray<CName>& categories );

	void UpdateNodeIconsAndLabels( wxTreeItemId itemId );

	void UpdateCustomSpeedField();
	void ActivateAnimation( CJobActionBase* action, const CName& animName );
	void UpdateToolbarState();

public:
	virtual void OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, EStatus status );
	virtual String GetListenerName() const { return TXT("CEdJobTreeEditor"); }
};