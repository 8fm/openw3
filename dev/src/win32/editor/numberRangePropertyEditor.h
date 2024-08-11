/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class INumberRangePropertyEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	wxSpinCtrl*	m_ctrlSpin;

public:
	INumberRangePropertyEditor( CPropertyItem* propertyItem );
	virtual ~INumberRangePropertyEditor();

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;
	virtual Bool DrawValue( wxDC& dc, const wxRect &valueRect, const wxColour& textColour ) override;
	virtual Bool SaveValue() override;
	virtual Bool GrabValue( String& displayValue ) override;

protected:
	virtual void OnSpinChanged( wxCommandEvent& event );

	virtual void ApplyRange();
};