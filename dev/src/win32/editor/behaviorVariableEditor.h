/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdBehaviorEditor;

#include "behaviorEditorPanel.h"

class CEdBehaviorVariableEditor : public CEdBehaviorEditorSimplePanel
{
	friend class CUndoBehaviourGraphVariableChange;

	DECLARE_EVENT_TABLE()

	enum EVecSliderScale
	{
		VSS_Var,
		VSS_1,
		VSS_10,
		VSS_100
	};

	CObject*								m_variableSelectedObject;
	CEdPropertiesBrowserWithStatusbar*		m_variableProperties;

	THashSet< CName >							m_floatInputs;
	THashMap< CName, CName >					m_eventInputs;
	THashSet< CName >							m_internalFloatInputs;

	THashMap< CName, Float >					m_storedFloatValues;
	THashMap< CName, Vector >					m_storedVectorValues;
	THashMap< CName, Float >					m_storedInternalFloatValues;
	THashMap< CName, Vector >					m_storedInternalVectorValues;

	EVecSliderScale							m_vecSliderScale;
	CEdUndoManager*							m_undoManager;

public:
	CEdBehaviorVariableEditor( CEdBehaviorEditor* editor, wxWindow* parent = NULL );
	~CEdBehaviorVariableEditor();

	virtual wxString	GetPanelName() const	{ return wxT("Variable"); }
	virtual wxString	GetPanelCaption() const { return wxT("Variable panel"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnDebug( Bool flag );
	virtual void OnPreInstanceReload();
	virtual void OnInstanceReload();

	void SetUndoManager( CEdUndoManager* undoManager ) 
	{ 
		m_undoManager = undoManager; 
		m_variableProperties->Get().SetUndoManager( undoManager );
	}

public:
	void RefreshAll();

	void RefreshVariablesList();
	void RefreshVectorVariablesList();
	void RefreshEventsList();
	void RefreshInternalVariablesList();
	void RefreshInternalVectorVariablesList();

private:
	void RefreshVariablesListWorker(Bool forInternalVariables);
	void RefreshVectorVariablesListWorker(Bool forInternalVariables);

public:
	void RefreshControlList();

	void RefreshMotionList();

protected:
	void OnPageChange( wxCommandEvent& event );

	void OnVarListBoxSelChange( wxCommandEvent &event );

	void OnVectorVarListBoxSelChange( wxCommandEvent &event );

	void OnInternalVarListBoxSelChange( wxCommandEvent &event );

	void OnInternalVectorVarListBoxSelChange( wxCommandEvent &event );

	void OnEventsListBoxSelChange( wxCommandEvent &event );
	void OnEventsListRaiseEvent( wxCommandEvent &event );

	void OnPropertiesChanged( wxCommandEvent &event );

	void OnAddVariable( wxCommandEvent &event );
	void OnRemoveVariable( wxCommandEvent &event );	
	void OnCopyVariables( wxCommandEvent &event );
	void OnPasteVariables( wxCommandEvent &event );	
	void OnAddVectorVariable( wxCommandEvent &event );
	void OnRemoveVectorVariable( wxCommandEvent &event );	
	void OnCopyVectorVariables( wxCommandEvent &event );
	void OnPasteVectorVariables( wxCommandEvent &event );	

	void OnAddInternalVariable( wxCommandEvent &event );
	void OnRemoveInternalVariable( wxCommandEvent &event );	
	void OnCopyInternalVariables( wxCommandEvent &event );
	void OnPasteInternalVariables( wxCommandEvent &event );	
	void OnAddInternalVectorVariable( wxCommandEvent &event );
	void OnRemoveInternalVectorVariable( wxCommandEvent &event );	
	void OnCopyInternalVectorVariables( wxCommandEvent &event );
	void OnPasteInternalVectorVariables( wxCommandEvent &event );	

private:
	void OnAddVariableWorker( wxCommandEvent &event, Bool forInternalVariable );
	void OnAddVectorVariableWorker( wxCommandEvent &event, Bool forInternalVariable );
	void OnRemoveVariableWorker( wxCommandEvent &event, Bool forInternalVariable );
	void OnRemoveVectorVariableWorker( wxCommandEvent &event, Bool forInternalVariable );
	void OnCopyVariablesWorker( wxCommandEvent &event, Bool forInternalVariable );
	void OnCopyVectorVariablesWorker( wxCommandEvent &event, Bool forInternalVariable );
	void OnPasteVariablesWorker( wxCommandEvent &event, Bool forInternalVariable );
	void OnPasteVectorVariablesWorker( wxCommandEvent &event, Bool forInternalVariable );

public:
	void OnAddEvent( wxCommandEvent &event );
	void OnRemoveEvent( wxCommandEvent &event );
	void OnCopyEvents( wxCommandEvent &event );
	void OnPasteEvents( wxCommandEvent &event );	

	void OnRaiseEvent( wxCommandEvent &event );
	void OnRaiseForceEvent( wxCommandEvent &event );

	void OnValueSlider( wxScrollEvent &event );
	void OnVectorValueSlider( wxScrollEvent &event );
	void OnInternalValueSlider( wxScrollEvent &event );
	void OnInternalVectorValueSlider( wxScrollEvent &event );

private:
	void OnValueSliderWorker( wxScrollEvent &event, Bool forInternalVariable );
	void OnVectorValueSliderWorker( wxScrollEvent &event, Bool forInternalVariable );

public:
	void OnShowVectorVariableInPreview( wxCommandEvent& event );
	void OnShowInternalVectorVariableInPreview( wxCommandEvent& event );

private:
	void OnShowVectorVariableInPreviewWorker( wxCommandEvent& event, Bool forInternalVariable );

public:
	void OnGridValueChanged( wxCommandEvent& event );

	void OnShowInputs( wxCommandEvent &event );

	void OnSelectInput( wxCommandEvent &event );

	void OnMotionChoice( wxCommandEvent& event );

	void OnMotionSetHeading( wxCommandEvent& event );

	void OnMotionStart( wxCommandEvent& event );

	void OnMotionStop( wxCommandEvent& event );

	void OnVectorSliderScale1( wxCommandEvent& event );
	void OnVectorSliderScale10( wxCommandEvent& event );
	void OnVectorSliderScale100( wxCommandEvent& event );

	void OnInternalVectorSliderScale1( wxCommandEvent& event );
	void OnInternalVectorSliderScale10( wxCommandEvent& event );
	void OnInternalVectorSliderScale100( wxCommandEvent& event );

	void UpdateValueSlider();
	void UpdateInternalValueSlider();

private:
	void UpdateValueSliderWorker( Bool forInternalVariable );

public:
	void UpdateVectorValueSlider();
	void UpdateVectorValueSliderScale();

	void UpdateInternalVectorValueSlider();
	void UpdateInternalVectorValueSliderScale();

private:
	void UpdateVectorValueSliderWorker( Bool forInternalVariable );
	void UpdateVectorValueSliderScaleWorker( Bool forInternalVariable );

public:
	void UpdateControls();

public:
	void ShowSelectedVariable();
	void ShowSelectedVectorVariable();
	void ShowSelectedEvent();
	void ShowSelectedInternalVariable();
	void ShowSelectedInternalVectorVariable();

private:
	void ShowSelectedVariableWorker( Bool forInternalVariable );
	void ShowSelectedVectorVariableWorker( Bool forInternalVariable );

public:
	void SelectVariable(const String& varName);
	void SelectVectorVariable(const String& varName);
	void SelectEvent(const CName& eventName);
	void SelectInternalVariable(const String& varName);
	void SelectInternalVectorVariable(const String& varName);

private:
	void SelectItemWorker(const String& varName, wxListBox* inListBox);

protected:
	static String GetHelperNameForSelectedItem(wxListBox* inListBox);

protected:
	void GetVectorSliderRange( const CName var, Vector& min, Vector& max, Vector& value, Bool forInternalVariable ) const;

	void SetVariable( const CName name, const Vector& value );

	void SetVariable( const CName name, Float value );

	void SetInternalVariable( const CName name, const Vector& value );

	void SetInternalVariable( const CName name, Float value );

	void SetObjectProperty( CObject* obj );

	void UpdateShowVectorInPreviewTools();

	void ShowVectorInPreview();

	void ResetInputChoices();

	void FillInputChoices();

	Bool GetCurrFloatInput( CName& input );
	Bool GetCurrInternalFloatInput( CName& input );
private:
	Bool GetCurrFloatInputWorker( CName& input, Bool forInternalVariable );

	Bool GetCurrEventInput( CName& input );

	void AddToInputVarMap( const CName& input, const CName& var );

	void AddToInputEventMap( const CName& input, const String& e );

	void AddToInputInternalVarMap( const CName& input, const CName& var );

	void EventDuplicationTest();

public:
	static Float SLIDER_VALUE_SCALE;
	static Float MAX_SLIDER_VALUE_RANGE;

	static Float SLIDER_VECTOR_VALUE_SCALE;
	static Float MAX_SLIDER_VECTOR_VALUE_RANGE;
};

