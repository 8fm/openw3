/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Custom property editor for CEntityOnLayerReference
class CEntityOnLayerReferenceEditor : public wxEvtHandler,	public ICustomPropertyEditor
{
public:
	CEntityOnLayerReferenceEditor( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayValue ) override;

	virtual Bool SaveValue() override;

protected:
	wxTextCtrl*	m_ctrlText;

	void OnSelectReferenced( wxCommandEvent &event );
	void OnUseSelected( wxCommandEvent &event );
	void OnClearReference( wxCommandEvent &event );
};
