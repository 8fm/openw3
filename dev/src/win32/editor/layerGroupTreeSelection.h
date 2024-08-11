/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// Selection of layer groups
class CEdLayerGroupTreeSelection : public wxEvtHandler, public ICustomPropertyEditor
{
public:
	CEdLayerGroupTreeSelection( CPropertyItem* propertyItem );

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

protected:
	void OnShowEditor( wxCommandEvent &event );
	void OnClearLayer( wxCommandEvent &event );

	CLayerGroup* GetBaseGroup();
};
