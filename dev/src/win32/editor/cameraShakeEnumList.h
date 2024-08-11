/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Enum list selection
class CEdCameraShakeEnumPropertyEditor : public CListSelection
{
public:
	CEdCameraShakeEnumPropertyEditor( CPropertyItem* item );

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
};
