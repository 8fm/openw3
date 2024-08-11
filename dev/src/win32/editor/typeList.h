/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Class list selection
class CTypeListSelection : public CListSelection
{
protected:
	CEdChoice		*m_ctrlComboBox;
public:
	CTypeListSelection( CPropertyItem* item );

	virtual void CloseControls() override;
	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;
};