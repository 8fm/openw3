#pragma once

class CStringPropertyEditor : public wxEvtHandler, public ICustomPropertyEditor
{
private:
	wxBitmap    m_icon;
	wxTextCtrl* m_ctrlText;
	String      m_boxTitle;
	String      m_boxMessage;
	Bool        m_isPropertyCName; // if false, than property is String

	// edit text box methods
	void OnEditTextEnter( wxCommandEvent& event );
	void OnEditKeyDown( wxKeyEvent& event );

	// custom editor methods
	void OnSpawnStringEditor( wxCommandEvent &event );

public:
	CStringPropertyEditor( CPropertyItem *propertyItem, const String &boxTitle, const String &boxMessage );

	// override ICustomPropertyEditor methods
	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;
};
