/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Enum list selection
class CEnumVariantSelection : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	wxChoice*	m_typeChoice;
	wxChoice*	m_valueChoice;

public:
	CEnumVariantSelection( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

protected:
	virtual void OnChoiceChanged( wxCommandEvent &event );
	virtual void OnValueChanged( wxCommandEvent &event );
};
