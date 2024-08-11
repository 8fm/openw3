#pragma  once

#include "../../common/engine/curveEntity.h"

class CMultiCurve2DEditorDialog;

class CMultiCurvePropertyEditor
	: public wxEvtHandler
	, public ICustomPropertyEditor
{
public:
	enum EditMode
	{
		EditMode_Default = 0,
		EditMode_3D,
		EditMode_2D
	};

private:
	wxBitmap			m_icon;
	wxBitmap			m_icon3D;
	wxTextCtrl*			m_ctrlText;
	String				m_selectedPropertyName;
	CMultiCurve2DEditorDialog*	m_2DEditorDialog;
	EditMode			m_editMode;

public:
	CMultiCurvePropertyEditor( CPropertyItem* propertyItem, EditMode editMode = EditMode_Default );

	// ICustomPropertyEditor
	virtual void CreateControls( const wxRect& propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override;

protected:
	void On2DEditing( wxCommandEvent& event );
	void On3DEditing( wxCommandEvent& event );
	void Create3DEditor();
	Bool Delete3DEditor();
	Bool GetParentAnimationSet( CPropertyAnimationSet*& animationSet, CName& propertyName, CName& animationName );
	SMultiCurve* GetCurve();
	CNode* GetCurveParent();

	friend class CMultiCurve2DEditorDialog;
};

class CMultiCurve2DEditorDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CMultiCurve2DEditorDialog( CMultiCurvePropertyEditor* curvePropertyEditor, wxWindow* parent, const String& title, SMultiCurve* curve, Bool show3DEditorOnClose );

	void SetActiveRegion( Float start, Float end );

protected:
	void OnOk( wxCommandEvent& event );

protected:
	CEdCurveEditor* m_curveEditor;
	CMultiCurvePropertyEditor* m_curvePropertyEditor;
	Bool m_show3DEditorOnClose;
};

struct SCurveEaseParam;

class CCurveKeyDialog : public wxDialog, public ICurveSelectionListener, public ICurveChangeListener
{
	DECLARE_EVENT_TABLE();
public:
	static void ShowInstance( Bool show, CEdRenderingPanel* panel );
	static Bool IsShown() { return s_instance != NULL; }
	static void RefreshValues( Bool force = false );

	// Overridden from ICurveChangeListener
	virtual void OnCurveChanged( SMultiCurve* curve ) override;

	// Overridden from ICurveSelectionListener
	virtual void OnCurveSelectionChanged( CCurveEntity* curveEntity, const TDynArray< Uint32 >& selectedControlPointIndices ) override;

protected:
	CCurveKeyDialog();
	void RefreshValues_Internal( Bool force );

	CCurveControlPointEntity* GetCurrentControlPoint();
	Bool UpdateCurrentControlPoint();

	SCurveEaseParam* GetEaseParams();
	void GetTransform( EngineTransform& transform );
	Float GetTime();
	void SetTime( Float time );

	void OnKeyDown( wxKeyEvent& event );
	void OnCloseButtonClick( wxCommandEvent& event );
	void OnTimeTextEnter( wxCommandEvent& event );
	void OnEaseInTextEnter( wxCommandEvent& event );
	void OnEaseOutTextEnter( wxCommandEvent& event );
	void OnTransformTextEnter( wxCommandEvent& event );

	CEdRenderingPanel* m_panel;
	CCurveControlPointEntity* m_currentControlPoint;
	wxTextCtrl* m_timeText;
	wxTextCtrl* m_easeInText;
	wxTextCtrl* m_easeOutText;
	wxTextCtrl* m_posText[3];
	wxTextCtrl* m_rotText[3];
	wxTextCtrl* m_scaleText[3];

	static CCurveKeyDialog* s_instance;
};