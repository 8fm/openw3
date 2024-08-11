/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/behaviorGraphInstance.h"

#include "behaviorEditorPanel.h"

/// Behavior debugger
class CEdBehaviorDebugger	: public CEdBehaviorEditorSimplePanel
							, public CBehaviorGraphHistoryLogger
{
	DECLARE_EVENT_TABLE()

	enum EHistPage
	{
		HP_All,
		HP_Variables,
		HP_Events,
	};

protected:
	Bool			m_connected;
	Bool			m_listener;
	Bool			m_toRelink;

	Uint32			m_selectedSnapshot;
	EHistPage		m_selectedHistPage;

	wxStaticText*	m_conState;

	wxListBox*		m_histList;
	wxStaticText*	m_histEntityDesc;

	Bool			m_toggledMilti;

	Float			m_debugTimer;
	Float			m_debugTimerDuration;

	Float			m_forcedTimeDelta;
	Bool			m_forcedTimeDeltaSet;
	wxTextCtrl*		m_forcedTimeDeltaText;

public:
	CEdBehaviorDebugger( CEdBehaviorEditor* editor );
	~CEdBehaviorDebugger();

	virtual wxString	GetPanelName() const	{ return wxT("Debugger"); }
	virtual wxString	GetPanelCaption() const { return wxT("Debugger"); }
	virtual wxString	GetInfo() const			{ return wxT("Debugger"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	static wxString		GetPanelNameStatic()	{ return wxT("Debugger"); }

	virtual void OnClose();
	virtual void OnReset();
	virtual void OnDebug( Bool flag );
	virtual void OnTick( Float dt );
	virtual void OnLoadEntity();
	virtual void OnUnloadEntity();

protected:
	void Connect( Bool flag );
	void SetEntityDesc( Bool flag );

	void ClearHistList();
	void UpdateHistList();

	Bool CanEditVariables() const;

	void InternalUpdate( Float dt );

protected:
	void OnHistPrev( wxCommandEvent& event );
	void OnHistNext( wxCommandEvent& event );
	void OnHistLast( wxCommandEvent& event );
	void OnUnsafeMode( wxCommandEvent& event );
	void OnForcedTimeDelta( wxCommandEvent& event );

protected:
	virtual Bool CanBeRelinked() const;
	virtual void RequestToRelink();
	virtual void Relink( CBehaviorGraphInstance* newInstance );

	virtual void OnPreUpdateInstance( Float& dt ) override;
	virtual void OnPostUpdateInstance( Float dt ) override;
	virtual void OnPostSampleInstance( const SBehaviorGraphOutput& pose );
	virtual void OnUnbind();
};

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorsListener : public wxFrame
{
	DECLARE_EVENT_TABLE()

	class CListElemData : public wxClientData
	{
	public:
		CListElemData( CBehaviorGraphInstance* instance ) : m_instance( instance ) {}
		
		THandle< CBehaviorGraphInstance > m_instance;
	};

	struct SDebugLogger
	{
		THandle< CBehaviorGraphInstance >	m_instance;
		CBehaviorGraphSimpleLogger*			m_listener;
		wxGrid*								m_grid;
		Int32									m_page;
	};

	static const Uint32			LIST_DEFAULT_SIZE = 20;

protected:
	wxListBox*					m_entityList;
	wxAuiNotebook*				m_notebook;
	CEdTimer						m_updateTimer;

	TDynArray< SDebugLogger >	m_loggers;

public:
	CEdBehaviorsListener( wxWindow* parent );
	~CEdBehaviorsListener();

protected:
	void ClearAll();

	void AddLogger( CBehaviorGraphInstance* instance, const String& name );
	void ClearLoggers();
	void UpdateLoggers();

	Int32 AddPage( const String& name, wxGrid*& grid );
	void RemovePages();

	void FillList( const TDynArray< CEntity* >& actors );
	Float GetRadius() const;
	Int32 GetUpdateTimeStep() const;
	void SetupGrid( wxGrid* list );

	wxColour GetMsgColor( Uint32 code, Float progress ) const;

protected:
	void OnButtFind( wxCommandEvent& event );
	void OnButtShow( wxCommandEvent& event );
	void OnButtConnect( wxCommandEvent& event );
	void OnUpdateLog( wxTimerEvent& event );
	void OnManualUpdateLog( wxCommandEvent& event );
	void OnEditDuration( wxCommandEvent& event );
	void OnShowSkeleton( wxCommandEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorDebuggerInstanceSelectionDlg : wxDialog
{
	DECLARE_EVENT_TABLE();

	CEntity*	m_entity;

	wxBitmap	m_iconValid;
	wxBitmap	m_iconInvalid;

	CName		m_instanceName;
	String		m_componentName;

public:
	CEdBehaviorDebuggerInstanceSelectionDlg( wxWindow* parent, CEntity* entity );

	Int32 DoModal();

	CBehaviorGraphInstance* GetSelectedInstance() const;

protected:
	CName GetEntityTag() const;
	String GetEntityName() const;
	CName GetSelectedInstanceName() const;
	String GetComponent() const;

protected:
	void OnCancel( wxCommandEvent& event );
	void OnOk( wxCommandEvent& event );
	void OnSearch( wxCommandEvent& event );
	void OnComponentSelected( wxCommandEvent& event );
	void OnInstanceSelected( wxCommandEvent& event );

private:
	void FillDefaultValues();
	void FillComponentList();
	void FillInstanceList();
	void RefreshOkButton();

	void SaveConfig();
	void LoadConfig();
};
