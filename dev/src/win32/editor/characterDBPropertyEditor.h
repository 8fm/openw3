#pragma once

class CEdCharacterDBEditor;

/// Custom property editor for character
class CCharacterDBPropertyEditor : public wxEvtHandler,	public ICustomPropertyEditor
{
protected:
    CEdCharacterDBEditor*	m_characterEditorExt;

	wxBitmap				m_iconCharacterEditor;
    wxBitmap				m_iconDeleteCharacter;

public:
	CCharacterDBPropertyEditor( CPropertyItem* propertyItem );
	~CCharacterDBPropertyEditor();

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayValue ) override;

protected:
	CEdCharacterDBEditor* GetEditorExt();

	void OnShowEditor( wxCommandEvent& event );
	void OnClearValue( wxCommandEvent& event );
	void OnEditorOk( wxCommandEvent &event );
	void OnEditorExit( wxCloseEvent &event );
};
