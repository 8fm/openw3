/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CLayerGUIDListSelection : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	CEdChoice				*m_ctrlChoice;
	THashMap< CGUID, Int32 >  m_layersIndexes; // indexes in control choice
	TDynArray< CGUID >      m_layersGUIDs;

public:
	CLayerGUIDListSelection( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

	void OnChoiceChanged( wxCommandEvent &event );
};
