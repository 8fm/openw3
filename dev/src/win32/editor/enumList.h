/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Enum list selection
class CEnumListSelection : public CListSelection
{
public:
	CEnumListSelection( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};
