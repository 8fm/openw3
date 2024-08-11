/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorEditorPanel.h"

class CEdBehaviorGraphNodeSearcher : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	struct SBehaviorNodeData : public wxTreeItemData
	{
		CBehaviorGraphNode* m_node;
		SBehaviorNodeData( CBehaviorGraphNode* node ) : m_node( node ) {}
	};
	
	wxTreeCtrl*		m_tree;

	wxTextCtrl*		m_nameFilter;
	wxChoice*		m_classFilter;
	wxChoice*		m_transFilter;
	wxChoice*		m_transVarFilter;
	wxChoice*		m_transEventFilter;
	wxChoice*		m_floatFilter;
	wxChoice*		m_vectorFilter;
	wxTextCtrl*		m_animFilter;

	TDynArray< CBehaviorGraphNode* > m_selectedNodes;

	Float			m_paintTimer;
	Float			m_paintTimerSign;
	static const Float PAINT_TIMER_DURATION;

public:
	CEdBehaviorGraphNodeSearcher( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("Searcher"); }
	virtual wxString	GetPanelCaption() const { return wxT("Node searcher"); }
	virtual wxString	GetInfo() const			{ return wxT("Node and transition searcher"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnInstanceReload();
	virtual void OnPanelClose();
	virtual void OnPrintNodes( CEdGraphEditor* graphCanvas );
	virtual void OnTick( Float dt );

protected:
	void OnSearch( wxCommandEvent& event );
	void OnToggleTransEx( wxCommandEvent& event );
	void OnToggleTransContain( wxCommandEvent& event );
	void OnToggleTreeList( wxCommandEvent& event );
	void OnToggleTreeTree( wxCommandEvent& event );
	void OnTreeSelectionChanged( wxTreeEvent& event );
	void OnTreeItemActivated( wxTreeEvent& event );

protected:
	void Clear();

	void FillTree();
	void FillClassFilter();
	void FillTransFilter();
	void FillTransVarFilter();
	void FillTransEventFilter();
	void FillFloatFilter();
	void FillVectorFilter();

	Bool UseFilterName() const;
	Bool UseFilterClass() const;
	Bool UseFilterTransition() const;
	Bool UseFilterTransType() const;
	Bool UseFilterTransEx() const;
	Bool UseFilterTransContain() const;
	Bool UseFilterTransVar() const;
	Bool UseFilterTransEvent() const;
	Bool UseFilterActive() const;
	Bool UseFilterFloat() const;
	Bool UseFilterVector() const;
	Bool UseFilterAnim() const;

	wxString GetFilterName() const;
	wxString GetFilterClass() const;
	wxString GetFilterTransType() const;
	wxString GetFilterTransVar() const;
	wxString GetFilterTransEvent() const;
	wxString GetFilterFloat() const;
	wxString GetFilterVector() const;
	wxString GetFilterAnim() const;

	Bool MarkNodes() const;
	Bool ZoomNodes() const;

	Bool IsTreeListStyle() const;

	wxString GetNodeName( const CBehaviorGraphNode* node ) const;

	friend class CTreeProvider;
};
