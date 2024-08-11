/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Slider that sets numeric value of property
class CSlider : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	wxSlider		*m_slider;
	Float			m_zero;
	Float			m_scale;

    CPropertyTransaction *m_transaction;

public:
	CSlider( CPropertyItem* item );

	virtual void CloseControls() override;
	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

protected:
	virtual void OnValueChanged( wxCommandEvent &event );
};