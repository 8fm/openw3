/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CLocalizedStringEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	wxTextCtrl		*m_stringEditField;
	wxTextCtrl		*m_stringIdField;
	wxPanel			*m_panel;

	Bool			m_allowIdReset;
	Bool			m_unlockStringId;

	CEdStringSelector* m_stringSelectorTool;

public:
	CLocalizedStringEditor( CPropertyItem* item, Bool allowIdReset = false );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool SaveValue() override;

	virtual Bool GrabValue( String& stringValue ) override;
	Bool GrabValue( Uint32& stringId, String& stringValue );

protected:
	void OnStringValueChanged( wxCommandEvent& event );
	void OnStringIdChanged( wxCommandEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnResetId( wxCommandEvent& event );
	void OnSelectStringDialog( wxCommandEvent& event );
	void OnSelectStringDialogOK( wxCommandEvent& event );
	void OnSelectStringDialogCancel( wxEvent& event );
};
