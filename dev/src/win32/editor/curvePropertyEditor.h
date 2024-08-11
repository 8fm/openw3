/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

//////////////////////////////////////////////////////////////////////////

class CCurvePropertyEditor	: public wxEvtHandler
							, public ICustomPropertyEditor

{
	wxBitmap	m_icon;

public:
	CCurvePropertyEditor( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayData ) override;

	virtual Bool SaveValue() override;

protected:
	void OnEditorDialog( wxCommandEvent &event );

	virtual void OnOpenCurveEditor();
};

//////////////////////////////////////////////////////////////////////////

class CBaseCurveDataPropertyEditor : public CCurvePropertyEditor
{
public:
	CBaseCurveDataPropertyEditor( CPropertyItem* propertyItem );

protected:
	virtual void OnOpenCurveEditor();

private:
	ICurveDataOwner* GetOwner();
};

//////////////////////////////////////////////////////////////////////////

class CVoiceCurveDataPropertyEditor : public CCurvePropertyEditor
{
	wxBitmap	m_iconImport;

public:
	CVoiceCurveDataPropertyEditor( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

protected:
	void OnImportCurveData( wxCommandEvent &event );

	virtual void OnOpenCurveEditor();
};

//////////////////////////////////////////////////////////////////////////

class CEdCurvePropertyEditorDialog: public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CEdCurvePropertyEditorDialog( wxWindow* parent, CCurve* curve );
	CEdCurvePropertyEditorDialog( wxWindow* parent, TDynArray< SCurveData* >& curves );
	~CEdCurvePropertyEditorDialog();

	void SetActiveRegion( Float start, Float end );

protected:
	void OnOk( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );

protected:
	CEdCurveEditor* m_curveEditor;
	CCurve*			m_curve;
};
