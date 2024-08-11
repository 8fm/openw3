/**
  * Copyright © 2010 CD Projekt Red. All Rights Reserved.
  */

#pragma once

#include "treeEditor.h"
#include "classHierarchyMapper.h"

class IEdSpawnTreeNode;

/// Graph editor for behavior tree
class CEdSpawnTreeEditor 
	: public CEdTreeEditor
	, public IEdEventListener
{
	DECLARE_EVENT_TABLE();

protected:
	static const Int32		OFFSET_X_MANY = 150;
	static const Int32		OFFSET_Y_MANY = 0;
	static const Int32		OFFSET_X_SINGLE = 0;
	static const Int32		OFFSET_Y_SINGLE = 100;
public:
	CEdSpawnTreeEditor( wxWindow* parent, IHook* hook );
	
	~CEdSpawnTreeEditor();

	void SetTree( ICreatureDefinitionContainer* rootNode );

	Bool IsDebugMode() const;

	CEdSpawntreeNodeProxy* GetRootNodeProxy() const { return m_rootNodeProxy.Get(); }

	virtual Bool IsLocked( IScriptable & block ) const override;

	virtual void ForceLayoutUpdate() override;

	virtual void DeleteSelection() override;
	void OnShowStats( wxCommandEvent& event );
protected:
	// CEdTreeEditor
	virtual wxColour GetCanvasColor() const override;
	virtual void PaintCanvas( Int32 width, Int32 height ) override;
	virtual void DrawBlockLayout( LayoutInfo & layout ) override;	
	virtual void OnOpenContextMenu() override;
 	virtual void OnMoveEnded() override;
	virtual Bool LoadBlockPosition( IScriptable* block, wxPoint & pos ) override;
	virtual Bool SaveBlockPosition( IScriptable * block, const wxPoint & pos ) override;
	virtual String GetBlockName( IScriptable& block ) const override;
	virtual String GetBlockComment( IScriptable & block ) const override;
	virtual String GetBitmapName( IScriptable& block ) const override;
	virtual Bool CanHaveChildren( IScriptable& block ) const override;
	virtual Bool IsHiddenBranch( IScriptable & block ) const override;
	virtual void OnGizmoClicked( LayoutInfo* layout, GizmoLocation location ) override;
	virtual void OnAddByDragging( wxPoint pos, GizmoLocation location ) override;
	
private:
	void DeleteActiveNode( const String& undoStepNameOverride = String::EMPTY );
	void CopyActiveNode();
	void PreStructureModification();
	void PostStructureModification();
	void FillNodeLayout( IEdSpawnTreeNode * node, CEdSpawntreeNodeProxy* parentProxy, LayoutInfo * parentLayout );
	void FillDebugNodeLayout( IEdSpawnTreeNode* node );
	CEdSpawntreeNodeProxy* GetActiveNode() const;
	void FillUpAddMenu( wxMenu& menu );
	void FillUpDecorateMenu( wxMenu& menu );

	Bool IsLockedResource() const;

	void CreateNode( CClass*const blockClass, IEdSpawnTreeNode*const parentNode );
	void InsertNode( IEdSpawnTreeNode* node, IEdSpawnTreeNode* parentNode, const wxPoint* pos = NULL, Bool runCreationCallback = true );

	// IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;

	// events
	void OnSpawnNode( wxCommandEvent& event );
	void AddBaseNode( wxCommandEvent& event );
	void AddInitialiserNode( wxCommandEvent& event );
	void AddSubDefNode( wxCommandEvent& event );
	void Decorate( wxCommandEvent& event );
	void OnRestoreDefaultLayout( wxCommandEvent& event );
	void OnDeleteNode( wxCommandEvent& event );
	void OnCopyNode( wxCommandEvent& event );
	void OnCopyBranch( wxEvent& event );
	void OnCutBranch( wxEvent& event );
	void OnPaste( wxEvent& event );
	void OnDebugOption( wxCommandEvent& event );
	void OnSpecialOption( wxCommandEvent& event );
	void OnShowAll( wxCommandEvent& event );
	void OnDefaultVisibility( wxCommandEvent& event );
	void OnHideBranch( wxCommandEvent& event );
	void OnCheckOut( wxCommandEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnRegenerateIds( wxCommandEvent& event );
	void OnGenerateId( wxCommandEvent& event );
	void GetStats( IEdSpawnTreeNode* node, Uint32& count );

	void ClearNodeProxies();
	CEdSpawntreeNodeProxy* CreateNodeProxy( IEdSpawnTreeNode* obj, CEdSpawntreeNodeProxy* parentProxy );
	Bool FindNodeProxies( CObject* obj, TDynArray< CEdSpawntreeNodeProxy* >& outProxies );
	


	ICreatureDefinitionContainer*							m_rootNode;
	THandle< CEdSpawntreeNodeProxy >						m_rootNodeProxy;
	TDynArray< THandle< CEdSpawntreeNodeProxy > >			m_proxies;
	Uint32													m_usedProxies;
	static IEdSpawnTreeNode*								m_clipboard;

	wxPoint													m_draggedPos;
	Bool													m_dragged;
	Bool													m_showStats;
	String													m_textureData;
	THashMap< IEdSpawnTreeNode*, TPair<Float, Uint32> >		m_nodeStatistics;

	//THashSet< const IEdSpawnTreeNode* >						m_externalNodes;
	//THashMap< const IEdSpawnTreeNode*, IEdSpawnTreeNode* >	m_instanceHolders;
};
