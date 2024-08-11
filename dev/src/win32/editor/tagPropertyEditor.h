/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdTagEditor;
class CTagListProvider;

/// Custom property editor for tag list
class CTagPropertyEditor : public wxEvtHandler,	public ICustomPropertyEditor
{
protected:
    CEdTagEditor*	    m_tagEditorExt;
	wxBitmap			m_iconTagEditor;
    wxBitmap			m_iconDeleteTag;
	wxTextCtrl*			m_ctrlText;

public:
	CTagPropertyEditor( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayValue ) override;

	virtual Bool SaveValue() override;

protected:
	void OnTagsSaved( wxCommandEvent &event );
    void OnTagsCanceledExt( wxCommandEvent &event );
	void OnSpawnTagEditorExt( wxCommandEvent &event );
	void OnDeleteTag( wxCommandEvent &event );
	void OnText( wxCommandEvent& event );
};
