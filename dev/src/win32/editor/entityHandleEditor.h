/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Custom property editor for EntityHandle
class CEntityHandleEditor : public wxEvtHandler, public ICustomPropertyEditor
{
public:
	CEntityHandleEditor( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	void OnSelect( CEntity* entity );

	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayValue ) override;

	virtual Bool SaveValue() override;

protected:
	void DestroySelector();
	void OnSelectReferenced( wxCommandEvent &event );
	void OnUseSelected( wxCommandEvent &event );
	void OnClearReference( wxCommandEvent &event );

	CEntityHandleSelector* m_selector;
};
