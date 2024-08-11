/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once


// 
class CSpawnsetPhasesEditor : public CListSelection
{
protected:
	CEdChoice*			m_ctrl;

public:
	CSpawnsetPhasesEditor( CPropertyItem* item );

	virtual void CloseControls() override;
	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

protected:
	virtual void OnChoiceChanged( wxCommandEvent &event );

private:
	Bool GetProperties( THashSet< CName >& properties );
	void RefreshValues( const TDynArray< CName >& outProperties );
};
