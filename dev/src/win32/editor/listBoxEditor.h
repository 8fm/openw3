#pragma once

class CListBoxEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	wxCheckListBox* m_listBoxCtrl;
	Int32			m_listBoxMinHeight;

public:
	CListBoxEditor( CPropertyItem* item );
	~CListBoxEditor(void);

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	//! Close property item editors
	virtual void CloseControls() override;

	//! Save property value, return true if you process this message
	virtual Bool SaveValue() override { return false; }

	//! Grab property value, return true if you process this message
	virtual Bool GrabValue( String& displayValue ) override { return false; }

protected:
	void OnElementSelection( wxCommandEvent& event );

	virtual wxArrayString GetListElements() = 0;
	virtual void SelectPropertyElements() = 0;

	virtual void SelectElement( wxString element ) = 0;
	virtual void DeselectElement( wxString element ) = 0;
};
