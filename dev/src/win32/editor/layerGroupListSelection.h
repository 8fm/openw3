/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// Mulit selection of layer groups
class CEdLayerGroupListSelection : public wxEvtHandler, public ICustomPropertyEditor
{
public:
	CEdLayerGroupListSelection( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

protected:
	void OnShowEditor( wxCommandEvent &event );
	void OnClearLayer( wxCommandEvent &event );

	CLayerGroup* GetBaseGroup();
};
