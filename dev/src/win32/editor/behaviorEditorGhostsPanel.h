#pragma once

#include "behaviorEditorPanel.h"
#include "animationPreviewGhost.h"

class CEdBehaviorEditorGhostsPanel : public CEdBehaviorEditorSimplePanel
{
	wxDECLARE_EVENT_TABLE();

public:
	CEdBehaviorEditorGhostsPanel( CEdBehaviorEditor* editor );
	~CEdBehaviorEditorGhostsPanel();

	virtual wxString	GetPanelName() const	{ return wxT( "Ghosts" ); }
	virtual wxString	GetPanelCaption() const { return wxT( "Ghosts" ); }
	virtual wxString	GetInfo() const			{ return wxT( "Ghosts Configuration" ); }

private:
	virtual void OnTick( Float dt );
	void ChangeFrameDelta( Float dt );

private:
	void OnNumberOfInstancesChanged( wxSpinEvent& event );
	void OnTypeSelected( wxCommandEvent& event );

	void OnDisable( wxCommandEvent& event );
	void OnDisplayInPreview( wxCommandEvent& event );
	void OnDisplayInGame( wxCommandEvent& event );

	void OnFrameDeltaChange( wxCommandEvent& event );

private:
	PreviewGhostContainer				m_ghostContainer;

	PreviewGhostContainer::EGhostType	m_type;
	Uint32								m_numberOfInstances;

	CEntity*							m_entity;

	wxSlider*							m_frameDeltaCtrl;
	wxStaticText*						m_frameDeltaDisplay;
};

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorEditorScriptPanel : public CEdBehaviorEditorSimplePanel
{
	wxDECLARE_EVENT_TABLE();

	CComponent*		m_component;

public:
	CEdBehaviorEditorScriptPanel( CEdBehaviorEditor* editor );
	~CEdBehaviorEditorScriptPanel();

	virtual wxString	GetPanelName() const	{ return wxT( "Script" ); }
	virtual wxString	GetPanelCaption() const { return wxT( "Script" ); }
	virtual wxString	GetInfo() const			{ return wxT( "Script" ); }

protected:
	virtual void OnTick( Float dt );
	virtual void OnReset();
	virtual void OnLoadEntity();
	virtual void OnInstanceReload();
	virtual void OnGraphModified();

protected:
	void OnConnect( wxCommandEvent& event );

private:
	void SpawnEntity();
	void DespawnEntity();

	void SendEvent( const String& str, Bool withEnt = false );

	void InternalReload();
};
