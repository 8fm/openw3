/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Class list selection
class CClassListSelection : public CListSelection
{
protected:
	CClass		*m_baseClass;

public:
	CClassListSelection( CPropertyItem* item, CClass* baseClass );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};