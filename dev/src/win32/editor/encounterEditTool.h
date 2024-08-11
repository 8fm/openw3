/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdEncounterEditWindow;

class CEdEncounterEditTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdEncounterEditTool, IEditorTool, 0 );

protected:
	CEdEncounterEditWindow*	m_editWindow;

public:
	CEdEncounterEditTool();

	virtual String GetCaption() const;

	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );

	virtual void End();

	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );

	virtual Bool UsableInActiveWorldOnly() const;


};

BEGIN_CLASS_RTTI( CEdEncounterEditTool )
	PARENT_CLASS( IEditorTool )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CEdCascadePropertyEditor;

class CEdEncounterEditWindow : public wxFrame, public ISavableToConfig
{
protected:
	CEdCascadePropertyEditor* m_propertyEditor;

public:
	CEdEncounterEditWindow( wxWindow* parent );
	~CEdEncounterEditWindow();

	void SetEncounter( CEncounter* encounter );

	virtual void SaveSession( CConfigurationManager &config );
	virtual void LoadOptionsFromConfig();
	virtual void RestoreSession( CConfigurationManager &config );
	virtual void SaveOptionsToConfig();

	void OnClose( wxCloseEvent& event );

};