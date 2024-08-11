#pragma once

class CEdDirectorySelectionEditor : public ICustomPropertyEditor
{
public:
	CEdDirectorySelectionEditor( CPropertyItem* propertyItem );
	virtual ~CEdDirectorySelectionEditor();

private:
	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override;

	void OnSelectDirectory( wxCommandEvent& event );

private:
	wxPanel* m_focusPanel;
	wxTextCtrl* m_textDisplay;
};
