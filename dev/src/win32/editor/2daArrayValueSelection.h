/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#pragma once

// Lame custom editor for 2da values. 
// Components, which are owners of properties using this editor have to implement I2dArrayPropertyOwner.
class C2dArrayValueSelection : public CListSelection
{
protected:
	TDynArray< String > m_values;
	wxArrayString		m_choices;
	CEdChoice		*	m_ctrl;

public:
	C2dArrayValueSelection( CPropertyItem* item );

	virtual void CloseControls() override;
	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

protected:
	virtual void OnChoiceChanged( wxCommandEvent &event );

private:
	Bool DoesFilterMatch( const C2dArray *array, const TDynArray< S2daValueFilter > &filters, Uint32 row, Uint32 colSize );
};
