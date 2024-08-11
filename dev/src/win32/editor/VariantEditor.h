#pragma once

class CVariantEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	wxTextCtrl		*m_ctrlEdit;
	CEdChoice		*m_ctrlType;
	wxPanel			*m_panel;

public:
	CVariantEditor( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool SaveValue() override;

	Bool GrabValue( String& displayType, String& displayValue );

protected:
	virtual void OnChoiceChanged( wxCommandEvent &event );
};

////////////////////////////////////////////////////////////////////////////////////
class CVariantEnumEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	CEdChoice		*m_ctrlType;
	CEdChoice		*m_ctrlValue;
	wxPanel			*m_panel;

public:
	CVariantEnumEditor( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool SaveValue() override;

	Bool GrabValue( String& displayType, String& displayValue );

protected:
	void FillEnumValues( Bool setValue, const String& strValue );

	virtual void OnTypeChanged( wxCommandEvent &event );
	virtual void OnValueChanged( wxCommandEvent &event );
};