/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "behTreeEditedItem.h"
#include "behTreeGraphEditor.h"
#include "../../common/game/behTree.h"
#include "../../common/game/behTreeMachineListener.h"
#include "../../common/game/behTreeMachine.h"

class CEdUndoManager;

#ifdef EDITOR_AI_DEBUG

void DebugBehTreeStart( CBehTreeMachine* machine );
void DebugBehTreeStopAll();

class CEdBehTreeDebugInterface : public IBehTreeDebugInterface
{
public:
	CEdBehTreeDebugInterface();
	~CEdBehTreeDebugInterface();

	//! Start debugging tree
	void DebugBehTreeStart( CBehTreeMachine* machine );

	//! Start debugging all trees
	void DebugBehTreeStopAll();
};

#endif	//EDITOR_AI_DEBUG

/// Behavior Tree visual editor
class CEdBehTreeEditor : public wxSmartLayoutPanel, public CEdBehTreeGraphEditor::IHook, public IEdEventListener, public IBehTreeMachineListener
{
	DECLARE_EVENT_TABLE()
	friend class CEdBehTreeDebugInterface;	

protected:
	static THandle< CBehTreeMachine >	s_activeMachine;
	IBehTreeEditedItem*					m_editedItem;
	CEdPropertiesBrowserWithStatusbar*	m_properties;

	CEdBehTreeGraphEditor*				m_graphEditor;
	wxBitmap							m_btIcon;
	THandle< CBehTreeMachine >			m_machine;
	CEdUndoManager*						m_undoManager;

	static TDynArray< CEdBehTreeEditor * > s_editors;

public:
	CEdBehTreeEditor( wxWindow* parent, IBehTreeEditedItem* tree );
	~CEdBehTreeEditor();

	//! Show editor
	void ShowEditor();

	//! Get tree
	CBehTree * GetTemplate() { return m_editedItem->GetRes(); }

	//! Get title
	virtual wxString GetShortTitle();

	//! Is editor in debug mode
	Bool IsInDebugMode() const { return m_machine.Get() != NULL ? true : false; }

	//! Get machine
	CBehTreeMachine* GetMachine() const { return m_machine.Get(); }

	//! Node reporting in
	virtual Bool OnNodeReport( const IBehTreeNodeInstance* node );

	//! Node status changed
	virtual Bool OnNodeResultChanged( const IBehTreeNodeInstance* node, const Bool active );

	virtual void OnForceDebugColorSet( const IBehTreeNodeInstance* node, Uint8 R, Uint8 G, Uint8 B );

	//! On tree state changed
	virtual void OnTreeStateChanged();

	//! Stop listening request
	virtual void OnStopListeningRequest();

	//! Breakpoint reached
	virtual Bool OnBreakpoint( const IBehTreeNodeInstance* node );

	//! Selection has changed
	virtual void OnGraphSelectionChanged();

#ifdef EDITOR_AI_DEBUG
	void DebugBehTreeStart( CBehTreeMachine* machine );
#endif //EDITOR_AI_DEBUG

protected:
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnPropertiesRefresh( wxCommandEvent& event );
	void OnEditUndo( wxCommandEvent& event );
	void OnEditRedo( wxCommandEvent& event );
	void OnEditDelete( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnZoomAll( wxCommandEvent& event );
	void OnActivate( wxActivateEvent& event );

	void DispatchEditorEvent( const CName& name, IEdEventData* data );
};