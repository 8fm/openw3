/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdRewardEditor;

/// Custom property editor for job tree
class CEdChooseRewardCustomEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	CEdRewardEditor*	m_chooseDialog;

public:
	CEdChooseRewardCustomEditor( CPropertyItem* propertyItem );
	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool GrabValue( String& displayValue ) override;

	void OnShowEditor( wxCommandEvent& event );
	void OnClearValue( wxCommandEvent& event );

	void OnEditorOk( wxCommandEvent &event );
	void OnEditorCancel( wxCommandEvent &event );
};