#pragma once

// Common functionality for any property editor that simply acts as a display and 
class CEdBasicSelectorDialogPropertyEditor : public ICustomPropertyEditor
{
public:
	CEdBasicSelectorDialogPropertyEditor( CPropertyItem* item );
	~CEdBasicSelectorDialogPropertyEditor();

protected:
	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override { HALT( "OnGrabValue() must be defined for CEdBasicSelectorDialogPropertyEditor %s", m_propertyItem->GetName().AsChar() ); return false; }

	virtual void OnSelectDialog( wxCommandEvent& event ) = 0;

protected:
	wxPanel* m_focusPanel;
	wxTextCtrl* m_textDisplay;
};
