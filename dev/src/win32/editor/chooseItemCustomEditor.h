/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdChooseItemDialog;

/// Custom property editor for job tree
class CEdChooseItemCustomEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	CEdChooseItemDialog*	m_chooseDialog;

public:
	CEdChooseItemCustomEditor( CPropertyItem* propertyItem );
	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool GrabValue( String& displayValue ) override;

	void OnShowEditor( wxCommandEvent& event );
	void OnClearValue( wxCommandEvent& event );
	void OnEditorOk( wxCommandEvent &event );
	void OnEditorCancel( wxCommandEvent &event );
};
